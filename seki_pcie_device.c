/**************************************************************************
 * Copyright (c) 2014 Afa.L Cheng <afa@afa.moe>
 *                    Rosen Center for Advanced Computing, Purdue University
 *
 * This file is dual MIT/GPL licensed.
 *
 ***************************************************************************/

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <linux/slab.h>

#include "seki_device_defs.h"
#include "seki_procfs.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Afa.L Cheng <afa@afa.moe>");
MODULE_DESCRIPTION("Driver for Seki PCIe FPGA Accelerator");

static const struct pci_device_id seki_dev_idtbl[] = {
     // VID, DID, SVID, SDID, CID, Class Mask, Drvier private data
    {SEKI_VENDOR_ID, SEKI_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID,
        PCI_CLASS_PROCESSOR_CO, 0, 0},
    {}
};

// C99 force init to 0
SekiData _seki_data_array[SEKI_MAX_PCI_DEVICES];

MODULE_DEVICE_TABLE(pci, seki_dev_idtbl);


static int seki_probe(struct pci_dev *dev, const struct pci_device_id *did)
{
    int rv;
    unsigned int slot;
    SekiData *device_data = 0;

    SEKI_UNUSED(did);
    slot = PCI_SLOT(dev->devfn);

    pr_debug("Device Found on slot %d\n", slot);


    // Allocate a SekiData element
    for (int i = 0; i < SEKI_MAX_PCI_DEVICES; ++i) {
        if (!_seki_data_array[i].used) {
            device_data = _seki_data_array + i;
            device_data->used = 1;
            device_data->slot = slot;
            break;
        }
    }

    if (!device_data) { // Already 4 devices
        pr_err("Driver does not support more than 4 devices");

        return -1;
    }

    device_data->board_revision = dev->revision;

    // PCI enable/init sequence
    rv = pci_enable_device(dev);
    if (rv) {
        pr_err("Failed to enable device on slot %d\n", slot);

        goto err_cleanused;
    }

    pci_set_master(dev);

    if (pci_request_region(dev, 0, "seki_emu")) {
        pr_err("Failed to request region for device on slot %d\n", slot);

        goto err_disable;
    };

    // Control/Status Mem Region, 1MB
    device_data->ctrl_mmio_physical_addr = pci_resource_start(dev, 0);
    device_data->ctrl_mmio_length  = pci_resource_len(dev, 0);
    device_data->ctrl_mmio_virtual_addr =
            ioremap_nocache(device_data->ctrl_mmio_physical_addr,
                            device_data->ctrl_mmio_length);

    // Input Mem Region, 128MB
    device_data->input_mmio_physical_addr = pci_resource_start(dev, 2);
    device_data->input_mmio_length  = pci_resource_len(dev, 2);
    device_data->input_mmio_virtual_addr =
            ioremap_nocache(device_data->input_mmio_physical_addr,
                            device_data->input_mmio_length);

    // Output Mem Region, 64MB
    device_data->output_mmio_physical_addr = pci_resource_start(dev, 4);
    device_data->output_mmio_length  = pci_resource_len(dev, 4);
    device_data->output_mmio_virtual_addr =
            ioremap_nocache(device_data->output_mmio_physical_addr,
                            device_data->output_mmio_length);

    ++seki_device_count;

    seki_procfs_create_device(device_data);

    /*
    // DEBUG BEGIN
    unsigned char dta[] = {0x44, 0x66, 0xaa, 0xee};
    unsigned char dta2[] = {0x55, 0x88, 0xcc, 0xee};
    unsigned char dta3[] = {0x77, 0x99, 0xff, 0xee};

    memcpy_toio(device_data->ctrl_mmio_virtual_addr, dta, 4);
    memcpy_toio(device_data->input_mmio_virtual_addr, dta2, 4);
    memcpy_toio(device_data->output_mmio_virtual_addr, dta3, 4);
    // DEBUG END
    */

    return 0;

    // Errors:
err_disable:
    pci_disable_device(dev);

err_cleanused:
    device_data->used = 0;

    return rv;
}

static void seki_remove(struct pci_dev *dev) {
    SekiData *device_data = 0;
    unsigned int slot;

    slot = PCI_SLOT(dev->devfn);

    for (int i = 0; i < SEKI_MAX_PCI_DEVICES; ++i) {
        if (_seki_data_array[i].used && _seki_data_array[i].slot == slot) {
            device_data = _seki_data_array + i;
            break;
        }
    }
    if (!device_data)   // What device is it?
        return;

    if (device_data->ctrl_mmio_virtual_addr)
        iounmap(device_data->ctrl_mmio_virtual_addr);

    pci_release_region(dev, 0);
    pci_clear_master(dev);
    pci_disable_device(dev);

    --seki_device_count;

    // Remove procfs stub
    seki_procfs_remove_device(device_data);

    device_data->used = 0;
    pr_debug("Device removed, slot %d\n", slot);
}

static int seki_suspend(struct pci_dev *dev, pm_message_t state)
{
    SEKI_UNUSED(dev);
    SEKI_UNUSED(state);

    pr_debug("Device suspended\n");

    return 0;
}

static int seki_resume(struct pci_dev *dev)
{
    pr_debug("Device resumed\n");

    return 0;
}

static struct pci_driver pcie_seki_driver = {
    .name       = "seki_emu",
    .id_table   = seki_dev_idtbl,
    .probe      = seki_probe,
    .remove     = seki_remove,
    .suspend    = seki_suspend,
    .resume     = seki_resume,
};

static int __init seki_driver_init(void)
{
    int rv;

    seki_init_procfs();

    rv = pci_register_driver(&pcie_seki_driver);
    if (rv) {
        pr_err("Driver registration failed.\n");
        return rv;
    }

    pr_debug("Driver loaded");
    return 0;
}

static void __exit seki_driver_exit(void)
{
    pci_unregister_driver(&pcie_seki_driver);

    seki_uninit_procfs();

    pr_debug("Driver unloaded\n");
    return;
}

unsigned int seki_device_count = 0;

module_init(seki_driver_init);
module_exit(seki_driver_exit);
