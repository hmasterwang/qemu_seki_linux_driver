/**************************************************************************
 * Copyright (c) 2014 Afa.L Cheng <afa@afa.moe>
 *                    Rosen Center for Advanced Computing, Purdue University
 *
 * This file is dual MIT/GPL licensed.
 *
 * <seki_chardev.c>
 *
 * Note that sekictrl and wseki[0-3] are mmap only char device
 *
 ***************************************************************************/

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mm.h>

#include "seki_device_defs.h"
#include "seki_chardev.h"

// Variables
static dev_t        _seki_chardev_file_ctl_dev_t;
static struct cdev  _seki_chardev_cdev_sekictrl;

// FIXME: What will happen if a device is unplugged
//          when already mmaped?
// Ctl file ops
static int
seki_chardev_file_ctl_mmap(struct file *filp,
                           struct vm_area_struct *vma)
{
    // the unit of vm_pgoff is actually page frame
    // if mmap length is less than a page frame,
    // it will be automatically extended to 1 pf

    // Only 1 device can be mapped one time
    unsigned long dev_num = vma->vm_pgoff / 0x100;  // 100 pf per Ctrl Mem
    unsigned long len = vma->vm_end - vma->vm_start; // in bytes

    if (dev_num >= _seki_device_count) {
        pr_err("mmap offset off range");

        return -EINVAL;
    }

    if (len > 0x100000) {
        pr_err("mmap length too large");

        return -EINVAL;
    }

    if (!_seki_data_array[dev_num].used) {
        pr_err("mmap invalid device struct. This should not happed");

        return -EAGAIN;
    }

    vma->vm_flags |= VM_LOCKED;

    if (io_remap_pfn_range(vma, vma->vm_start,
                           (_seki_data_array[dev_num].ctrl_mmio_physical_addr
                           >> PAGE_SHIFT) + vma->vm_pgoff,
                           len,
                           vma->vm_page_prot)) {
        return -EAGAIN;
    }
    return 0;

}

static struct file_operations seki_chardev_file_ctl_fops = {
    .owner  = THIS_MODULE,
    .open   = nonseekable_open,
    .llseek = no_llseek,
    .mmap   = seki_chardev_file_ctl_mmap
};

// Device file ops


// Register & unregister
int seki_register_chardev_file_ctl(void)
{
    int rv;
    rv = alloc_chrdev_region(&_seki_chardev_file_ctl_dev_t, 0, 1, "sekictrl");
    if (rv < 0) {
        pr_err("Unable to allocate sekictrl device region\n");

        return rv;
    }


    cdev_init(&_seki_chardev_cdev_sekictrl, &seki_chardev_file_ctl_fops);
    rv = cdev_add(&_seki_chardev_cdev_sekictrl, _seki_chardev_file_ctl_dev_t, 1);
    if (rv < 0) {
        pr_err("Unable to add sekictrl char device\n");

        return rv;
    }


    return 0;
}

void seki_unregister_chardev_file_ctl(void)
{
    cdev_del(&_seki_chardev_cdev_sekictrl);

    unregister_chrdev_region(_seki_chardev_file_ctl_dev_t, 1);
}

