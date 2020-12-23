/*-
 * Copyright (c) 2020 Ruslan Bukin <br@bsdpad.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/fbio.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/rman.h>
#include <sys/resource.h>
#include <machine/bus.h>
#include <vm/vm.h>
#include <vm/vm_extern.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>

#include <dev/extres/clk/clk.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_vblank.h>
#include <drm/gpu_scheduler.h>

#include "panfrost_drv.h"
#include "panfrost_drm.h"
#include "panfrost_device.h"
#include "panfrost_gem.h"
#include "panfrost_regs.h"
#include "panfrost_features.h"
#include "panfrost_issues.h"
#include "panfrost_mmu.h"

#define	ARM_MALI_LPAE_TTBR_ADRMODE_TABLE	(3 << 0)
#define	ARM_MALI_LPAE_TTBR_READ_INNER		(1 << 2)
#define	ARM_MALI_LPAE_TTBR_SHARE_OUTER		(1 << 4)

#define	ARM_LPAE_MAIR_ATTR_SHIFT(n)		((n) << 3)
#define	ARM_LPAE_MAIR_ATTR_MASK			0xff
#define	ARM_LPAE_MAIR_ATTR_DEVICE		0x04
#define	ARM_LPAE_MAIR_ATTR_NC			0x44
#define	ARM_LPAE_MAIR_ATTR_INC_OWBRWA		0xf4
#define	ARM_LPAE_MAIR_ATTR_WBRWA		0xff
#define	ARM_LPAE_MAIR_ATTR_IDX_NC		0
#define	ARM_LPAE_MAIR_ATTR_IDX_CACHE		1
#define	ARM_LPAE_MAIR_ATTR_IDX_DEV		2
#define	ARM_LPAE_MAIR_ATTR_IDX_INC_OCACHE	3

#define	ARM_MALI_LPAE_MEMATTR_IMP_DEF		0x88ULL
#define	ARM_MALI_LPAE_MEMATTR_WRITE_ALLOC	0x8DULL

static int mmu_hw_do_operation_locked(struct panfrost_softc *sc, uint32_t as,
    vm_offset_t va, size_t size, uint32_t op);
static void panfrost_mmu_flush_range(struct panfrost_softc *sc, struct panfrost_mmu *mmu, vm_offset_t va, size_t size);

static const char *
panfrost_mmu_exception_name(uint32_t exc_code)
{

	switch (exc_code) {
	case 0x00: return "NOT_STARTED/IDLE/OK";
	case 0x01: return "DONE";
	case 0x02: return "INTERRUPTED";
	case 0x03: return "STOPPED";
	case 0x04: return "TERMINATED";
	case 0x08: return "ACTIVE";

	case 0xC1: return "TRANSLATION_FAULT_LEVEL1";
	case 0xC2: return "TRANSLATION_FAULT_LEVEL2";
	case 0xC3: return "TRANSLATION_FAULT_LEVEL3";
	case 0xC4: return "TRANSLATION_FAULT_LEVEL4";
	case 0xC8: return "PERMISSION_FAULT";
	case 0xC9 ... 0xCF: return "PERMISSION_FAULT";
	case 0xD1: return "TRANSTAB_BUS_FAULT_LEVEL1";
	case 0xD2: return "TRANSTAB_BUS_FAULT_LEVEL2";
	case 0xD3: return "TRANSTAB_BUS_FAULT_LEVEL3";
	case 0xD4: return "TRANSTAB_BUS_FAULT_LEVEL4";
	case 0xD8: return "ACCESS_FLAG";
	case 0xD9 ... 0xDF: return "ACCESS_FLAG";
	case 0xE0 ... 0xE7: return "ADDRESS_SIZE_FAULT";
	case 0xE8 ... 0xEF: return "MEMORY_ATTRIBUTES_FAULT";
	}

	return "UNKNOWN";
}

static const char *
access_type_name(struct panfrost_softc *sc, uint32_t fault_status)
{

	switch (fault_status & AS_FAULTSTATUS_ACCESS_TYPE_MASK) {
	case AS_FAULTSTATUS_ACCESS_TYPE_ATOMIC:
		if (panfrost_has_hw_feature(sc, HW_FEATURE_AARCH64_MMU))
			return "ATOMIC";
		else
			return "UNKNOWN";
	case AS_FAULTSTATUS_ACCESS_TYPE_READ:
		return "READ";
	case AS_FAULTSTATUS_ACCESS_TYPE_WRITE:
		return "WRITE";
	case AS_FAULTSTATUS_ACCESS_TYPE_EX:
		return "EXECUTE";
	default:
		return NULL;
	}
}

struct panfrost_gem_mapping *
panfrost_mmu_find_mapping(struct panfrost_softc *sc, int as, uint64_t addr)
{
	struct panfrost_gem_mapping *mapping;
	struct panfrost_file *pfile;
	struct panfrost_mmu *mmu;
	struct drm_mm_node *node;
	uint64_t offset;

	mapping = NULL;

	mtx_lock_spin(&sc->as_mtx);

	/* Find mmu first */
	TAILQ_FOREACH(mmu, &sc->mmu_in_use, next) {
		if (mmu->as == as)
			goto found;
	};
	goto out;

printf("%s: mmu found\n", __func__);

found:
	pfile = container_of(mmu, struct panfrost_file, mmu);

	mtx_lock_spin(&pfile->mm_lock);

	offset = addr >> PAGE_SHIFT;
	drm_mm_for_each_node(node, &pfile->mm) {
		if (offset >= node->start &&
		    offset < (node->start + node->size)) {
			mapping = container_of(node,
			    struct panfrost_gem_mapping, mmnode);
			/* TODO: take mapping ref */
			break;
		};
	};

	mtx_unlock_spin(&pfile->mm_lock);

out:
	mtx_unlock_spin(&sc->as_mtx);

	return (mapping);
}

static int
panfrost_mmu_page_fault(struct panfrost_softc *sc, int as, uint64_t addr)
{
	struct panfrost_gem_mapping *bomapping;
	struct panfrost_gem_object *bo;
	vm_offset_t page_offset;
	struct page *pages;
	//vm_object_t mapping;

	dprintf("%s: as %d addr %lx\n", __func__, as, addr);
	bomapping = panfrost_mmu_find_mapping(sc, as, addr);
	dprintf("%s: bomapping %p\n", __func__, bomapping);
	if (!bomapping)
		panic("no bomapping");

	printf("%s 1\n", __func__);
	bo = bomapping->obj;
	printf("%s 2\n", __func__);

	addr &= ~((uint64_t)2*1024*1024 - 1);
	page_offset = addr >> PAGE_SHIFT;
	page_offset -= bomapping->mmnode.start;

	printf("%s 3\n", __func__);
	if (bo->pages == NULL) {
		printf("no pages\n");
		panic("no pages");
	}

#if 0
	printf("%s: dump %lx\n", __func__, addr);
	pmap_gfault(&bomapping->mmu->p, addr);
#endif

	printf("%s 4\n", __func__);
	pages = bo->pages;

	printf("%s 5\n", __func__);
	//mapping = bo->base.filp->f_vnode->v_object;
	//printf("%s 6\n", __func__);
	//dprintf("%s: mapping %p\n", __func__, mapping);
	//struct panfrost_mmu *mmu;
	//mmu = bomapping->mmu;

	return (0);
}

static void
panfrost_mmu_fault(struct panfrost_softc *sc, int as)
{
	uint32_t fault_status;
	uint32_t exception_type;
	uint32_t access_type;
	uint32_t source_id;
	uint64_t addr;

	fault_status = GPU_READ(sc, AS_FAULTSTATUS(as));
	dprintf("%s: fault status %x\n", __func__, fault_status);
	dprintf("%s: AS_TRANSTAB_LO %x AS_TRANSTAB_HI %x\n", __func__,
	    GPU_READ(sc, AS_TRANSTAB_LO(0)), GPU_READ(sc, AS_TRANSTAB_HI(0)));

	addr = GPU_READ(sc, AS_FAULTADDRESS_LO(as));
	addr |= (uint64_t)GPU_READ(sc, AS_FAULTADDRESS_HI(as)) << 32;

	exception_type = fault_status & 0xFF;
	access_type = (fault_status >> 8) & 0x3;
	source_id = (fault_status >> 16);

	if ((exception_type & 0xF8) == 0xC0) {
		dprintf("%s: page fault at %lx\n", __func__, addr);
		panfrost_mmu_page_fault(sc, as, addr);
	} else
		dprintf("%s: %s fault at %lx\n", __func__,
		    panfrost_mmu_exception_name(exception_type), addr);

	dprintf("%s: exception type %x, access type %x (%s), source id %x\n",
	    __func__, exception_type, access_type,
	    access_type_name(sc, fault_status), source_id);

	//mmu_hw_do_operation_locked(sc, 0, addr, 8, AS_COMMAND_FLUSH_PT);
}

void
panfrost_mmu_intr(void *arg)
{
	struct panfrost_softc *sc;
	uint32_t status;
	int i;

	sc = arg;

	status = GPU_READ(sc, MMU_INT_RAWSTAT);
	dprintf("%s: status %x\n", __func__, status);

	for (i = 0; status != 0; i++) {
		if (status & (1 << i)) {
			panfrost_mmu_fault(sc, i);
			status &= ~(1 << i);
		}
		if (status & (1 << (16 + i)))
			panic("error");
		GPU_WRITE(sc, MMU_INT_CLEAR, (1 << i) | (1 << (i + 16)));
	}
}

int
panfrost_mmu_pgtable_alloc(struct panfrost_file *pfile)
{
	struct panfrost_mmu *mmu;
	pmap_t p;

	mmu = &pfile->mmu;
	p = &mmu->p;

	pmap_pinit(p);
	PMAP_LOCK_INIT(p);

	mmu->as = -1;

	return (0);
}

static int
wait_ready(struct panfrost_softc *sc, uint32_t as)
{
	uint32_t reg;
	int timeout;

	timeout = 1000;

	do {
		reg = GPU_READ(sc, AS_STATUS(as));
		if ((reg & AS_STATUS_AS_ACTIVE) == 0)
			break;
	} while (timeout--);

	if (timeout <= 0)
		panic("failed to read");

	return (0);
}

static int
write_cmd(struct panfrost_softc *sc, uint32_t as, uint32_t cmd)
{
	int status;

	status = wait_ready(sc, as);
	if (status == 0)
		GPU_WRITE(sc, AS_COMMAND(as), cmd);

	return (status);
}

static void
lock_region(struct panfrost_softc *sc, uint32_t as, vm_offset_t va,
    size_t size)
{
	uint8_t region_width;
	uint64_t region;

	region = va & PAGE_MASK;

	size = round_up(size, PAGE_SIZE);

	region_width = 10 + fls(size >> PAGE_SHIFT);
	if ((size >> PAGE_SHIFT) != (1ul << (region_width - 11)))
		region_width += 1;
	region |= region_width;

	GPU_WRITE(sc, AS_LOCKADDR_LO(as), region & 0xFFFFFFFFUL);
	GPU_WRITE(sc, AS_LOCKADDR_HI(as), (region >> 32) & 0xFFFFFFFFUL);
	write_cmd(sc, as, AS_COMMAND_LOCK);
}

static int
mmu_hw_do_operation_locked(struct panfrost_softc *sc, uint32_t as,
    vm_offset_t va, size_t size, uint32_t op)
{
	int error;

	if (op != AS_COMMAND_UNLOCK)
		lock_region(sc, as, va, size);

	write_cmd(sc, as, op);

	error = wait_ready(sc, as);

	return (0);
}

static int
mmu_hw_do_operation(struct panfrost_softc *sc,
    struct panfrost_mmu *mmu, vm_offset_t va, size_t size, uint32_t op)
{
	int error;

	mtx_lock_spin(&sc->as_mtx);
	error = mmu_hw_do_operation_locked(sc, mmu->as, va, size, op);
	mtx_unlock_spin(&sc->as_mtx);

	return (error);
}

static void
panfrost_mmu_flush_range(struct panfrost_softc *sc, struct panfrost_mmu *mmu,
    vm_offset_t va, size_t size)
{

	if (mmu->as < 0)
		return;

	mmu_hw_do_operation(sc, mmu, va, size, AS_COMMAND_FLUSH_PT);
}

int
panfrost_mmu_enable(struct panfrost_softc *sc, struct panfrost_mmu *mmu)
{
	vm_paddr_t paddr;
	uint64_t memattr;
	pmap_t p;
	int as;

	as = mmu->as;
	p = &mmu->p;

	paddr = p->pm_l0_paddr;
printf("%s: l0 paddr %lx, mmu as %d\n", __func__, paddr, as);
	paddr |= ARM_MALI_LPAE_TTBR_READ_INNER;
	paddr |= ARM_MALI_LPAE_TTBR_ADRMODE_TABLE;

	memattr = (ARM_MALI_LPAE_MEMATTR_IMP_DEF
	     << ARM_LPAE_MAIR_ATTR_SHIFT(ARM_LPAE_MAIR_ATTR_IDX_NC)) |
	    (ARM_MALI_LPAE_MEMATTR_WRITE_ALLOC
	     << ARM_LPAE_MAIR_ATTR_SHIFT(ARM_LPAE_MAIR_ATTR_IDX_CACHE)) |
	    (ARM_MALI_LPAE_MEMATTR_IMP_DEF
	     << ARM_LPAE_MAIR_ATTR_SHIFT(ARM_LPAE_MAIR_ATTR_IDX_DEV));

	mmu_hw_do_operation_locked(sc, as, 0, ~0UL, AS_COMMAND_FLUSH_MEM);

	wmb();

	GPU_WRITE(sc, AS_TRANSTAB_LO(as), paddr & 0xffffffffUL);
	GPU_WRITE(sc, AS_TRANSTAB_HI(as), paddr >> 32);

	GPU_WRITE(sc, AS_MEMATTR_LO(as), memattr & 0xffffffffUL);
	GPU_WRITE(sc, AS_MEMATTR_HI(as), memattr >> 32);

	write_cmd(sc, as, AS_COMMAND_UPDATE);

	return (0);
}

uint32_t
panfrost_mmu_as_get(struct panfrost_softc *sc, struct panfrost_mmu *mmu)
{
	int as;

	if (mmu->as >= 0)
		return (mmu->as);

	mtx_lock_spin(&sc->as_mtx);
	as = ffz(sc->as_alloc_set);
	sc->as_alloc_set |= (1 << as);
	mtx_unlock_spin(&sc->as_mtx);

	mmu->as = as;

printf("%s: new as %d\n", __func__, as);

	mtx_lock_spin(&sc->as_mtx);
	TAILQ_INSERT_TAIL(&sc->mmu_in_use, mmu, next);
	mtx_unlock_spin(&sc->as_mtx);

	panfrost_mmu_enable(sc, mmu);

	return (as);
}

int
panfrost_mmu_map(struct panfrost_softc *sc,
    struct panfrost_gem_mapping *mapping)
{
	struct panfrost_gem_object *bo;
	struct panfrost_mmu *mmu;
	vm_prot_t prot;
	vm_offset_t va;
	vm_paddr_t pa;
	vm_page_t m;
	int error;
	vm_offset_t sva;
	int i;

	bo = mapping->obj;
	mmu = mapping->mmu;

	error = panfrost_gem_get_pages(bo);
	if (error != 0)
		panic("could not get pages");

	m = bo->pages;

	va = mapping->mmnode.start << PAGE_SHIFT;
	sva = va;
	prot = VM_PROT_READ | VM_PROT_WRITE;
	if (bo->noexec == 0)
		prot |= VM_PROT_EXECUTE;

	printf("%s: bo %p mmu %p as %d mapping %lx -> %lx, %d pages\n",
	    __func__, bo, mmu, mmu->as, sva, VM_PAGE_TO_PHYS(m), bo->npages);

	/* map pages */
	for (i = 0; i < bo->npages; i++, m++) {
		pa = VM_PAGE_TO_PHYS(m);
		dprintf("%s: mapping %lx -> %lx\n", __func__, va, pa);
		error = pmap_genter(&mmu->p, va, pa, prot, 0);

		va += PAGE_SIZE;

#if 0
		int j;
		vm_offset_t kva, *kvva;
		kva = kva_alloc(PAGE_SIZE);
		kvva = (vm_offset_t *)kva;
		pmap_kenter(kva, PAGE_SIZE, pa, VM_MEMATTR_UNCACHEABLE);
		printf("words ");
		for (j = 0; j < 3; j++) {
			printf("%lx ", kvva[j]);
		}
		printf("\n");
#endif
	}

	mapping->active = true;

	wmb();

	panfrost_mmu_flush_range(sc, mmu, sva, va - sva);

	return (0);
}

int
panfrost_mmu_init(struct panfrost_softc *sc)
{

	/* Enable interrupts. */
	GPU_WRITE(sc, MMU_INT_CLEAR, ~0);
	GPU_WRITE(sc, MMU_INT_MASK, ~0);

	return (0);
}
