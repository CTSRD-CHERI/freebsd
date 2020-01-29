/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Andrew Turner
 * Copyright (c) 2019 Ruslan Bukin <br@bsdpad.com>
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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/rman.h>

#include <vm/vm.h>
#include <vm/vm_extern.h>
#include <vm/vm_page.h>

#include <contrib/dev/acpica/include/acpi.h>
#include <contrib/dev/acpica/include/accommon.h>

#include <dev/acpica/acpivar.h>
#include <dev/acpica/acpi_pcibvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcib_private.h>
#include <dev/pci/pci_host_generic.h>
#include <dev/pci/pci_host_generic_acpi.h>

#include "pcib_if.h"

#define	AP_NS_SHARED_MEM_BASE	0x06000000
#define	N1SDP_MAX_SEGMENTS	2 /* Two PCIe root complex devices. */
#define	BDF_TABLE_SIZE		(16 * 1024)
#define	PCI_CFG_SPACE_SIZE	0x1000

static vm_offset_t rc_remapped_addr[N1SDP_MAX_SEGMENTS];
static struct pcie_discovery_data {
	uint32_t rc_base_addr;
	uint32_t nr_bdfs;
	uint32_t valid_bdfs[0];
} *pcie_discovery_data[N1SDP_MAX_SEGMENTS];

static int
n1sdp_init(struct generic_pcie_acpi_softc *sc)
{
	struct pcie_discovery_data *shared_data;
	vm_offset_t vaddr_rc;
	vm_offset_t vaddr;
	vm_paddr_t paddr_rc;
	vm_paddr_t paddr;
	int table_count;
	int bdfs_size;
	int i;

	paddr = AP_NS_SHARED_MEM_BASE + sc->segment * BDF_TABLE_SIZE;
	vaddr = kva_alloc((vm_size_t)BDF_TABLE_SIZE);
	if (vaddr == 0) {
		printf("%s: Can't allocate KVA memory.", __func__);
		return (ENXIO);
	}
	pmap_kenter(vaddr, (vm_size_t)BDF_TABLE_SIZE, paddr,
	    VM_MEMATTR_UNCACHEABLE);

	shared_data = (struct pcie_discovery_data *)vaddr;
	bdfs_size = sizeof(struct pcie_discovery_data) +
	    sizeof(uint32_t) * shared_data->nr_bdfs;
	pcie_discovery_data[sc->segment] =
	    malloc(bdfs_size, M_DEVBUF, M_WAITOK | M_ZERO);
	memcpy(pcie_discovery_data[sc->segment], shared_data, bdfs_size);

	paddr_rc = (vm_offset_t)shared_data->rc_base_addr;
	vaddr_rc = kva_alloc((vm_size_t)PCI_CFG_SPACE_SIZE);
	if (vaddr == 0) {
		printf("%s: Can't allocate KVA memory.", __func__);
		return (ENXIO);
	}
	pmap_kenter_device(vaddr_rc, (vm_size_t)PCI_CFG_SPACE_SIZE, paddr_rc);

	rc_remapped_addr[sc->segment] = vaddr_rc;

	if (bootverbose) {
		table_count = pcie_discovery_data[sc->segment]->nr_bdfs;
		for (i = 0; i < table_count; i++)
			printf("valid bdf %x\n",
			    pcie_discovery_data[sc->segment]->valid_bdfs[i]);
	}

	pmap_kremove(vaddr);
	kva_free(vaddr, (vm_size_t)BDF_TABLE_SIZE);

	return (0);
}

static int
n1sdp_check_bdf(struct generic_pcie_acpi_softc *sc,
    u_int bus, u_int slot, u_int func)
{
	int table_count;
	int bdf;
	int i;

	bdf = PCIE_ADDR_OFFSET(bus, slot, func, 0);
	if (bdf == 0)
		return (1);

	table_count = pcie_discovery_data[sc->segment]->nr_bdfs;

	for (i = 0; i < table_count; i++)
		if (bdf == pcie_discovery_data[sc->segment]->valid_bdfs[i])
			return (1);

	return (0);
}

static int
n1sdp_pcie_acpi_probe(device_t dev)
{
	ACPI_DEVICE_INFO *devinfo;
	ACPI_TABLE_HEADER *hdr;
	ACPI_STATUS status;
	ACPI_HANDLE h;
	int root;

	if (acpi_disabled("pcib") || (h = acpi_get_handle(dev)) == NULL ||
	    ACPI_FAILURE(AcpiGetObjectInfo(h, &devinfo)))
		return (ENXIO);

	root = (devinfo->Flags & ACPI_PCI_ROOT_BRIDGE) != 0;
	AcpiOsFree(devinfo);
	if (!root)
		return (ENXIO);

	/* TODO: Move this to an ACPI quirk? */
	status = AcpiGetTable(ACPI_SIG_MCFG, 1, &hdr);
	if (ACPI_FAILURE(status))
		return (ENXIO);

	if (memcmp(hdr->OemId, "ARMLTD", ACPI_OEM_ID_SIZE) != 0 ||
	    memcmp(hdr->OemTableId, "ARMN1SDP", ACPI_OEM_TABLE_ID_SIZE) != 0 ||
	    hdr->OemRevision != 0x20181101)
		return (ENXIO);

	device_set_desc(dev, "ARM N1SDP PCI host controller");
	return (BUS_PROBE_DEFAULT);
}

static int
n1sdp_pcie_acpi_attach(device_t dev)
{
	struct generic_pcie_acpi_softc *sc;
	ACPI_HANDLE handle;
	ACPI_STATUS status;
	int err;

	sc = device_get_softc(dev);

	handle = acpi_get_handle(dev);

	/* Get PCI Segment (domain) needed for IOMMU space remap. */
	status = acpi_GetInteger(handle, "_SEG", &sc->segment);
	if (ACPI_FAILURE(status)) {
		device_printf(dev, "No _SEG for PCI Bus\n");
		return (ENXIO);
	}

	if (sc->segment >= N1SDP_MAX_SEGMENTS) {
		device_printf(dev, "Unknown PCI Bus segment (domain) %d\n",
		    sc->segment);
		return (ENXIO);
	}

	err = n1sdp_init(sc);
	if (err)
		return (err);

	err = pci_host_generic_acpi_attach(dev);

	return (err);
}

static uint32_t
n1sdp_pcie_read_config(device_t dev, u_int bus, u_int slot,
    u_int func, u_int reg, int bytes)
{
	struct generic_pcie_acpi_softc *sc_acpi;
	struct generic_pcie_core_softc *sc;
	uint64_t offset;
	uint32_t data;

	sc_acpi = device_get_softc(dev);
	sc = &sc_acpi->base;

	if ((bus < sc->bus_start) || (bus > sc->bus_end))
		return (~0U);
	if ((slot > PCI_SLOTMAX) || (func > PCI_FUNCMAX) ||
	    (reg > PCIE_REGMAX))
		return (~0U);

	if (n1sdp_check_bdf(sc_acpi, bus, slot, func) == 0)
		return (~0U);

	offset = PCIE_ADDR_OFFSET(bus - sc->bus_start, slot, func, reg);

	if (bus == 0 && slot == 0 && func == 0)
		data = *(uint32_t *)(rc_remapped_addr[sc_acpi->segment] +
		    (offset & ~3));
	else
		data = bus_space_read_4(sc->bst, sc->bsh, offset & ~3);

	switch (bytes) {
	case 1:
		data >>= (offset & 3) * 8;
		data &= 0xff;
		break;
	case 2:
		data >>= (offset & 3) * 8;
		data = le16toh(data);
		break;
	case 4:
		data = le32toh(data);
		break;
	default:
		return (~0U);
	}

	return (data);
}

static void
n1sdp_pcie_write_config(device_t dev, u_int bus, u_int slot,
    u_int func, u_int reg, uint32_t val, int bytes)
{
	struct generic_pcie_acpi_softc *sc_acpi;
	struct generic_pcie_core_softc *sc;
	bus_space_handle_t h;
	bus_space_tag_t t;
	uint64_t offset;
	uint32_t data;

	sc_acpi = device_get_softc(dev);
	sc = &sc_acpi->base;

	if ((bus < sc->bus_start) || (bus > sc->bus_end))
		return;
	if ((slot > PCI_SLOTMAX) || (func > PCI_FUNCMAX) ||
	    (reg > PCIE_REGMAX))
		return;

	if (n1sdp_check_bdf(sc_acpi, bus, slot, func) == 0)
		return;

	offset = PCIE_ADDR_OFFSET(bus - sc->bus_start, slot, func, reg);

	t = sc->bst;
	h = sc->bsh;

	if (bus == 0 && slot == 0 && func == 0)
		data = *(uint32_t *)(rc_remapped_addr[sc_acpi->segment] +
		    (offset & ~3));
	else
		data = bus_space_read_4(t, h, offset & ~3);

	/*
	 * TODO: This is probably wrong on big-endian, however as arm64 is
	 * little endian it should be fine.
	 */
	switch (bytes) {
	case 1:
		data &= ~(0xff << ((offset & 3) * 8));
		data |= (val & 0xff) << ((offset & 3) * 8);
		break;
	case 2:
		data &= ~(0xffff << ((offset & 3) * 8));
		data |= (val & 0xffff) << ((offset & 3) * 8);
		break;
	case 4:
		data = val;
		break;
	default:
		return;
	}

	if (bus == 0 && slot == 0 && func == 0)
		*(uint32_t *)(rc_remapped_addr[sc_acpi->segment] +
		    (offset & ~3)) = data;
	else
		bus_space_write_4(t, h, offset & ~3, htole32(data));
}

static int
n1sdp_pcie_alloc_msi(device_t pci, device_t child, int count, int maxcount,
    int *irqs)
{

	/*
	 * No MSI/MSI-X support for n1sdp due to an unknown bug:
	 * ARM ITS driver setups MSI/MSIx but interrupts are not coming.
	 * Further investigation is required.
	 */

	return (ENXIO);
}

static int
n1sdp_pcie_alloc_msix(device_t pci, device_t child, int *irq)
{

	/* No MSI/MSI-X support. */

	return (ENXIO);
}

static device_method_t n1sdp_pcie_acpi_methods[] = {
	DEVMETHOD(device_probe,		n1sdp_pcie_acpi_probe),
	DEVMETHOD(device_attach,	n1sdp_pcie_acpi_attach),

	/* pcib interface */
	DEVMETHOD(pcib_read_config,	n1sdp_pcie_read_config),
	DEVMETHOD(pcib_write_config,	n1sdp_pcie_write_config),
	DEVMETHOD(pcib_alloc_msi,	n1sdp_pcie_alloc_msi),
	DEVMETHOD(pcib_alloc_msix,	n1sdp_pcie_alloc_msix),

	DEVMETHOD_END
};

DEFINE_CLASS_1(pcib, n1sdp_pcie_acpi_driver, n1sdp_pcie_acpi_methods,
    sizeof(struct generic_pcie_acpi_softc), generic_pcie_acpi_driver);

static devclass_t n1sdp_pcie_acpi_devclass;

DRIVER_MODULE(n1sdp_pcib, acpi, n1sdp_pcie_acpi_driver,
    n1sdp_pcie_acpi_devclass, 0, 0);
