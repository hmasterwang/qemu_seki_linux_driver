/**************************************************************************
 * Copyright (c) 2014 Afa.L Cheng <afa@afa.moe>
 *                    Rosen Center for Advanced Computing, Purdue University
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ***************************************************************************/

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/printk.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Afa.L Cheng <afa@afa.moe>");
MODULE_DESCRIPTION("Driver for Seki PCIe FPGA Accelerator");

static const struct pci_device_id seki_dev_idtbl[] = {
     // VID, DID, SVID, SDID, CID, Class Mask, Drvier private data
    {0xFA58, 0x0961, PCI_ANY_ID, PCI_ANY_ID, 0x0B40, 0, 0},
    {}
};

static int seki_probe(struct pci_dev *dev, const struct pci_device_id *did)
{
    int rv;

    pr_debug("Device Found");

    rv = pci_enable_device(dev);

    if (rv)
        return rv;

    return 0;
}

static void seki_remove(struct pci_dev *dev) {
    pr_debug("Device removed\n");

}

static int seki_suspend(struct pci_dev *dev, pm_message_t state)
{
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
    pr_debug("Driver unloaded\n");
    return;
}

module_init(seki_driver_init);
module_exit(seki_driver_exit);
