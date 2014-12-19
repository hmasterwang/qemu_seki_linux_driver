/**************************************************************************
 * Copyright (c) 2014 Afa.L Cheng <afa@afa.moe>
 *                    Rosen Center for Advanced Computing, Purdue University
 *
 * This file is dual MIT/GPL licensed.
 *
 * <seki_pcie_device.c>
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
#include "seki_chardev.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Afa.L Cheng <afa@afa.moe>");
MODULE_DESCRIPTION("Driver for Seki PCIe FPGA Accelerator");

static const struct pci_device_id seki_dev_idtbl[] = {
     // VID, DID, SVID, SDID, CID, Class Mask, Drvier private data
    {SEKI_VENDOR_ID, SEKI_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID,
        PCI_CLASS_PROCESSOR_CO, 0, 0},
    {}
};

MODULE_DEVICE_TABLE(pci, seki_dev_idtbl);

static unsigned int seki_allocate_device_number(void)
{
    int ret = -1;
    for (int i = 0; i < SEKI_MAX_PCI_DEVICES; ++i) {
        if (!_seki_device_number_used_map[i]) {
            _seki_device_number_used_map[i] = 1;
            ret = i;
            break;
        }
    }
    return ret;
}

static void seki_deallocate_device_number(unsigned int dev_num)
{
    _seki_device_number_used_map[dev_num] = 0;
}

static int seki_probe(struct pci_dev *dev, const struct pci_device_id *did)
{
    int rv;
    unsigned int slot;
    unsigned int dev_num;
    SekiData *device_data = 0;

    SEKI_UNUSED(did);
    slot = PCI_SLOT(dev->devfn);

    pr_debug("Device Found on slot %d\n", slot);

    // Allocate a SekiData element
    dev_num = seki_allocate_device_number();
    ++_seki_device_count;
    pr_debug("Device on slot %d allocated deviced number %d\n", slot, dev_num);

    if (dev_num == -1) {
        pr_err("Driver does not support more than 4 devices\n");   // More than 4 devices?

        return -1;
    }

    device_data = _seki_data_array + dev_num;
    device_data->device_num = dev_num;
    device_data->used = 1;
    device_data->slot = slot;
    device_data->board_revision = dev->revision;
    spin_lock_init(&device_data->ctrl_mmio_lock);
    spin_lock_init(&device_data->input_mmio_lock);
    spin_lock_init(&device_data->output_mmio_lock);

    // PCI enable/init sequence
    rv = pci_enable_device(dev);
    if (rv) {
        pr_err("Failed to enable device on slot %d\n", slot);

        goto err_cleanused;
    }

    pci_set_master(dev);

    if (pci_request_region(dev, 0, SEKI_DRIVER_NAME)) {
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


    rv = seki_procfs_create_file_device(device_data);
    if (rv) {
        pr_err("Failed to create procfs file for device on slot %d\n", slot);
        goto err_disable;
    }

    rv = seki_chardev_create_file_seki(device_data);
    if (rv) {
        pr_err("Failed to create chardev for device on slot %d\n", slot);
        goto err_uninit_procfs;
    }

    return 0;

    // Errors:
err_uninit_procfs:
    seki_procfs_remove_file_device(device_data);

err_disable:
    pci_disable_device(dev);

err_cleanused:
    device_data->used = 0;
    --_seki_device_count;
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

    seki_chardev_remove_file_seki(device_data);

    seki_procfs_remove_file_device(device_data);

    if (device_data->ctrl_mmio_virtual_addr)
        iounmap(device_data->ctrl_mmio_virtual_addr);

    if (device_data->input_mmio_virtual_addr)
        iounmap(device_data->input_mmio_virtual_addr);

    if (device_data->output_mmio_virtual_addr)
        iounmap(device_data->output_mmio_virtual_addr);

    pci_release_region(dev, 0);
    pci_clear_master(dev);
    pci_disable_device(dev);

    // Finalize spinlocks
    memset(&device_data->ctrl_mmio_lock,    0, sizeof(spinlock_t));
    memset(&device_data->input_mmio_lock,   0, sizeof(spinlock_t));
    memset(&device_data->input_mmio_lock,   0, sizeof(spinlock_t));

    // Free _seki_data_array
    device_data->used = 0;
    seki_deallocate_device_number(device_data->device_num);
    --_seki_device_count;

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
    .name       = SEKI_DRIVER_NAME,
    .id_table   = seki_dev_idtbl,
    .probe      = seki_probe,
    .remove     = seki_remove,
    .suspend    = seki_suspend,
    .resume     = seki_resume,
};

static int __init seki_driver_init(void)
{
    int rv;

    rv = seki_init_procfs();
    if (rv) {
        pr_err("Unable to init procfs\n");
        return rv;
    }

    rv = seki_chardev_register_file_ctl();
    if (rv) {
        pr_err("Unable to register chardev sekictl\n");
        goto err_uninit_procfs;
    }

    rv = seki_chardev_register_file_seki();
    if (rv) {
        pr_err("Unable to register chardev seki\n");
        goto err_unregister_chardev_file_ctl;
    }

    rv = pci_register_driver(&pcie_seki_driver);
    if (rv) {
        pr_err("Driver registration failed\n");
        goto err_uninit_procfs;
    }


    pr_debug("Driver loaded");
    return 0;

err_unregister_chardev_file_ctl:
    seki_chardev_unregister_file_ctl();
err_uninit_procfs:
    seki_uninit_procfs();
    return rv;

}

static void __exit seki_driver_exit(void)
{
    pci_unregister_driver(&pcie_seki_driver);

    seki_chardev_unregister_file_seki();

    seki_chardev_unregister_file_ctl();

    seki_uninit_procfs();

    pr_debug("Driver unloaded\n");
    return;
}


unsigned int _seki_device_count = 0;

int      _seki_device_number_used_map[SEKI_MAX_PCI_DEVICES] = {};
SekiData _seki_data_array[SEKI_MAX_PCI_DEVICES] = {};

module_init(seki_driver_init);
module_exit(seki_driver_exit);
