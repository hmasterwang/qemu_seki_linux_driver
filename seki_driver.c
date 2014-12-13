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

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Afa.L Cheng <afa@afa.moe>");
MODULE_DESCRIPTION("PCI driver for the tutorial device");

static const struct pci_device_id pcidevid[] = {
    /*
     * Vendor ID, Device ID, Subvendor ID, Subdevice ID,
     * Class ID, Class Mask, Drvier private data
     */
    { 0xFA58, 0x0961, PCI_ANY_ID, PCI_ANY_ID, 0x0B40, 0, 0 },
    { }
};

static int seki_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    pr_debug("Device Found: Vendor %x, Device %x, Class %x\n",
            pdev->vendor, pdev->device, pdev->class);
    return 0;
}

/* called when the PCI core realizes that one of the pci_dev's handled by
 * this driver are removed (for whatever reason) from the system
 */
static void seki_remove(struct pci_dev *pdev) {
    pr_debug("Unloaded\n");
}

static struct pci_driver pcie_seki_driver = {
    .name = "Seki Accelerator Driver",
    .id_table = pcidevid,
    .probe = seki_probe,
    .remove = seki_remove,
};

static int __init seki_driver_init(void)
{
    int rv;
    pr_debug("Registering driver...\n");

    rv = pci_register_driver(&pcie_seki_driver);
    if (rv) {
        pr_err("Driver registration failed.\n");
        return rv;
    }

    return 0;
}

static void __exit seki_driver_exit(void)
{
    pr_debug("Driver unloaded\n");
    return;
}

/* these lines show what should be done on insmod/rmmod */
module_init(seki_driver_init);
module_exit(seki_driver_exit);
