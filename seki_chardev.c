/**************************************************************************
 * Copyright (c) 2014 Afa.L Cheng <afa@afa.moe>
 *                    Rosen Center for Advanced Computing, Purdue University
 *
 * This file is dual MIT/GPL licensed.
 *
 * <seki_procfs.c>
 * ProcFS file operations.
 *
 * When changing this file, remember to change sysfs as well
 *
 ***************************************************************************/

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/module.h>

#include "seki_device_defs.h"
#include "seki_chardev.h"

// Variables
static dev_t        _seki_chardev_file_ctl_dev_t;
static struct cdev  _seki_chardev_cdev_sekictrl;


// Ctl file ops
// 1 Device each call
static ssize_t seki_chardev_file_ctl_read(struct file *f,
                                          char __user *user_buf,
                                          size_t len, loff_t *offset)
{
    unsigned int bound;
    unsigned int device_num;
    unsigned int dev_offset;
    char *dev_buf;

    SEKI_UNUSED(f);
    bound       = 0x100000 * _seki_device_count;
    device_num  = *offset / 0x100000;
    dev_offset  = *offset % 0x100000;

    if (*offset >= bound)
        return 0;

    if (dev_offset + len >= 0x100000)
        len = 0x100000 - dev_offset;

    dev_buf = vmalloc(len);
    if (!dev_buf)
        return -ENOMEM;

    memcpy_fromio(dev_buf, _seki_data_array[device_num].ctrl_mmio_virtual_addr
                  + dev_offset, len);

    // Non zero indicating failure
    if (copy_to_user(user_buf, dev_buf, len))
        len = 0;    // Failed. len is return value now.

    vfree(dev_buf);

    return len;
}

static ssize_t seki_chardev_file_ctl_write(struct file *f,
                                           const char __user *buf,
                                           size_t len, loff_t *offset)
{
    return len;
}

// Device file ops
static struct file_operations seki_chardev_file_ctl_fops = {
    .owner  = THIS_MODULE,
    .read   = seki_chardev_file_ctl_read,
    .write  = seki_chardev_file_ctl_write
};

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

