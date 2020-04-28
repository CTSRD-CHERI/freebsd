/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Ruslan Bukin <br@bsdpad.com>
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
 * DARPA SSITH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_acpi.h"
#include "opt_platform.h"

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bitstring.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/ktr.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/queue.h>
#include <sys/rman.h>
#include <sys/pcpu.h>
#include <sys/proc.h>
#include <sys/cpuset.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/smp.h>

#include <vm/vm.h>
#include <vm/pmap.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <dev/iommu/smmu_var.h>
#include <dev/pci/pcivar.h>

#include "iommu.h"
#include "iommu_if.h"

static MALLOC_DEFINE(M_IOMMU, "IOMMU", "IOMMU framework");

static struct mtx iommu_mtx;

#define	IOMMU_LOCK(iommu)		mtx_lock(&(iommu)->mtx_lock)
#define	IOMMU_UNLOCK(iommu)		mtx_unlock(&(iommu)->mtx_lock)
#define	IOMMU_ASSERT_LOCKED(iommu)	mtx_assert(&(iommu)->mtx_lock, MA_OWNED)

#define	IOMMU_LIST_LOCK()		mtx_lock(&iommu_mtx)
#define	IOMMU_LIST_UNLOCK()		mtx_unlock(&iommu_mtx)
#define	IOMMU_LIST_ASSERT_LOCKED()	mtx_assert(&iommu_mtx, MA_OWNED)

#define IOMMU_DEBUG
#undef IOMMU_DEBUG

#ifdef IOMMU_DEBUG
#define DPRINTF(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define DPRINTF(fmt, ...)
#endif

static LIST_HEAD(, iommu) iommu_list = LIST_HEAD_INITIALIZER(iommu_list);

int
iommu_domain_add_va_range(struct iommu_domain *domain,
    vm_offset_t va, vm_size_t size)
{
	struct iommu *iommu;
	int error;

	KASSERT(size > 0, ("wrong size"));

	iommu = domain->iommu;

	error = vmem_add(domain->vmem, va, size, M_WAITOK);

	return (error);
}

struct iommu_domain *
iommu_domain_alloc(struct iommu *iommu)
{
	struct iommu_domain *domain;

	domain = IOMMU_DOMAIN_ALLOC(iommu->dev);
	if (domain == NULL)
		return (NULL);

	LIST_INIT(&domain->device_list);
	mtx_init(&domain->mtx_lock, "IOMMU domain", NULL, MTX_DEF);
	domain->iommu = iommu;

	domain->vmem = vmem_create("IOMMU vmem", 0, 0, PAGE_SIZE,
	    PAGE_SIZE, M_FIRSTFIT | M_WAITOK);
	if (domain->vmem == NULL)
		return (NULL);

	IOMMU_LOCK(iommu);
	LIST_INSERT_HEAD(&iommu->domain_list, domain, next);
	IOMMU_UNLOCK(iommu);

	return (domain);
}

int
iommu_domain_free(struct iommu_domain *domain)
{
	struct iommu *iommu;
	vmem_t *vmem;
	int error;

	iommu = domain->iommu;
	vmem = domain->vmem;

	IOMMU_LOCK(iommu);
	LIST_REMOVE(domain, next);
	error = IOMMU_DOMAIN_FREE(iommu->dev, domain);
	if (error) {
		LIST_INSERT_HEAD(&iommu->domain_list, domain, next);
		IOMMU_UNLOCK(iommu);
		return (error);
	}

	IOMMU_UNLOCK(iommu);

	vmem_destroy(vmem);

	return (0);
}

struct iommu_domain *
iommu_get_domain_for_dev(device_t dev)
{
	struct iommu_domain *domain;
	struct iommu_device *device;
	struct iommu *iommu;

	LIST_FOREACH(iommu, &iommu_list, next) {
		LIST_FOREACH(domain, &iommu->domain_list, next) {
			LIST_FOREACH(device, &domain->device_list, next) {
				if (device->dev == dev)
					return (domain);
			}
		}
	}

	return (NULL);
}

/*
 * Attach a consumer device to a domain.
 */
int
iommu_device_attach(struct iommu_domain *domain, device_t dev)
{
	struct iommu_device *device;
	struct iommu *iommu;
	int err;

	iommu = domain->iommu;

	device = malloc(sizeof(*device), M_IOMMU, M_WAITOK | M_ZERO);
	device->rid = pci_get_rid(dev);
	device->dev = dev;
	device->domain = domain;

	err = IOMMU_DEVICE_ATTACH(iommu->dev, domain, device);
	if (err) {
		device_printf(iommu->dev, "Failed to add device\n");
		free(device, M_IOMMU);
		return (err);
	}

	DOMAIN_LOCK(domain);
	LIST_INSERT_HEAD(&domain->device_list, device, next);
	DOMAIN_UNLOCK(domain);

	return (err);
}

/*
 * Detach a consumer device from IOMMU domain.
 */
int
iommu_device_detach(struct iommu_domain *domain, device_t dev)
{
	struct iommu_device *device;
	struct iommu *iommu;
	bool found;
	int err;

	iommu = domain->iommu;

	found = false;

	DOMAIN_LOCK(domain);
	LIST_FOREACH(device, &domain->device_list, next) {
		if (device->dev == dev) {
			found = true;
			break;
		}
	}

	if (!found) {
		DOMAIN_UNLOCK(domain);
		return (ENODEV);
	}

	err = IOMMU_DEVICE_DETACH(iommu->dev, device);
	if (err) {
		device_printf(iommu->dev, "Failed to remove device\n");
		DOMAIN_UNLOCK(domain);
		return (err);
	}

	LIST_REMOVE(device, next);
	DOMAIN_UNLOCK(domain);

	return (0);
}

int
iommu_map_page(struct iommu_domain *domain,
    vm_offset_t va, vm_paddr_t pa, vm_prot_t prot)
{
	struct iommu *iommu;
	int error;

	iommu = domain->iommu;

	error = IOMMU_MAP(iommu->dev, domain, va, pa, PAGE_SIZE, prot);
	if (error)
		return (error);

	return (0);
}

int
iommu_unmap_page(struct iommu_domain *domain, vm_offset_t va)
{
	struct iommu *iommu;
	int error;

	iommu = domain->iommu;

	error = IOMMU_UNMAP(iommu->dev, domain, va, PAGE_SIZE);
	if (error)
		return (error);

	return (0);
}

/*
 * Busdma map/unmap interface.
 */
int
iommu_map(struct iommu_domain *domain, bus_dma_segment_t *segs, int nsegs)
{
	struct iommu *iommu;
	vm_offset_t offset;
	vm_offset_t va;
	vm_paddr_t pa;
	vm_size_t size;
	vm_prot_t prot;
	int error;
	int i;

	iommu = domain->iommu;

	for (i = 0; i < nsegs; i++) {
		pa = segs[i].ds_addr & ~(PAGE_SIZE - 1);
		offset = segs[i].ds_addr & (PAGE_SIZE - 1);
		size = roundup2(offset + segs[i].ds_len, PAGE_SIZE);

		error = vmem_alloc(domain->vmem, size,
		    M_FIRSTFIT | M_NOWAIT, &va);
		if (error) {
			device_printf(iommu->dev, "Could not allocate VA.\n");
			return (error);
		}

		DPRINTF("%s: %jx -> %jx (%jd pages)\n",
		    __func__, va, pa, size / PAGE_SIZE);

		prot = VM_PROT_READ | VM_PROT_WRITE;

		error = IOMMU_MAP(iommu->dev, domain, va, pa, size, prot);
		if (error)
			return (error);
		segs[i].ds_addr = va | offset;
	}

	return (0);
}

void
iommu_unmap(struct iommu_domain *domain, bus_dma_segment_t *segs, int nsegs)
{
	struct iommu *iommu;
	vm_offset_t offset;
	vm_offset_t va;
	vm_size_t size;
	int err;
	int i;

	iommu = domain->iommu;

	for (i = 0; i < nsegs; i++) {
		va = segs[i].ds_addr & ~(PAGE_SIZE - 1);
		offset = segs[i].ds_addr & (PAGE_SIZE - 1);
		size = roundup2(offset + segs[i].ds_len, PAGE_SIZE);

		DPRINTF("%s: %jx (%jd pages)\n",
		    __func__, va, size / PAGE_SIZE);

		err = IOMMU_UNMAP(iommu->dev, domain, va, size);
		if (err) {
			/*
			 * It could be that busdma backend tries to unload
			 * the same address twice due to a bug in a device
			 * driver. We can't add this VA back to vmem twice.
			 */
			device_printf(iommu->dev,
			    "Could not unmap VA %jx\n", va);
			continue;
		}
		vmem_free(domain->vmem, va, size);
	}
}

int
iommu_register(device_t dev, intptr_t xref)
{
	struct iommu *iommu;

	iommu = malloc(sizeof(*iommu), M_IOMMU, M_WAITOK | M_ZERO);
	iommu->dev = dev;
	iommu->xref = xref;

	LIST_INIT(&iommu->domain_list);
	mtx_init(&iommu->mtx_lock, "IOMMU", NULL, MTX_DEF);

	IOMMU_LIST_LOCK();
	LIST_INSERT_HEAD(&iommu_list, iommu, next);
	IOMMU_LIST_UNLOCK();

	return (0);
}

int
iommu_unregister(device_t dev)
{
	struct iommu *iommu;
	bool found;

	found = false;

	IOMMU_LIST_LOCK();
	LIST_FOREACH(iommu, &iommu_list, next) {
		if (iommu->dev == dev) {
			found = true;
			break;
		}
	}

	if (!found) {
		IOMMU_LIST_UNLOCK();
		return (ENOENT);
	}

	if (!LIST_EMPTY(&iommu->domain_list)) {
		IOMMU_LIST_UNLOCK();
		return (EBUSY);
	}

	LIST_REMOVE(iommu, next);
	IOMMU_LIST_UNLOCK();

	free(iommu, M_IOMMU);

	return (0);
}

struct iommu *
iommu_lookup(intptr_t xref, int flags)
{
	struct iommu *iommu;

	LIST_FOREACH(iommu, &iommu_list, next) {
		if (iommu->xref == xref)
			return (iommu);
	}

	return (NULL);
}

static void
iommu_init(void)
{

	mtx_init(&iommu_mtx, "IOMMU", NULL, MTX_DEF);
}

SYSINIT(iommu, SI_SUB_DRIVERS, SI_ORDER_FIRST, iommu_init, NULL);
