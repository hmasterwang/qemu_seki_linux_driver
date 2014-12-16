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

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>

#include "seki_device_defs.h"
#include "seki_procfs.h"

static struct proc_dir_entry *seki_proc_base_dir;

int seki_init_procfs(void)
{
    if (!seki_proc_base_dir)
        seki_proc_base_dir = proc_mkdir(SEKI_PROCFS_NAME, NULL);

    return 0;
}

int seki_uninit_procfs(void)
{
    remove_proc_entry(SEKI_PROCFS_NAME, NULL);
    seki_proc_base_dir = 0;

    return 0;
}

// File Ops
static int seki_procfs_file_show(struct seq_file *f, void *data)
{
    SekiData *device_data;
    SEKI_UNUSED(data);

    device_data = f->private;

    if (!device_data->used)
        return -1;

    seq_printf(f,
               "Seki Board Revision:        0x%02x\n"
               "Seki Control MMIO Physical: 0x%016lx\n"
               "Seki Control MMIO Virtual:  0x%016lx\n"
               "Seki Control MMIO Length:   0x%04lxMB\n"
               "Seki Input MMIO Physical:   0x%016lx\n"
               "Seki Input MMIO Virtual:    0x%016lx\n"
               "Seki Input MMIO Length:     0x%04lxMB\n"
               "Seki Output MMIO Physical:  0x%016lx\n"
               "Seki Output MMIO Virtual:   0x%016lx\n"
               "Seki Output MMIO Length:    0x%04lxMB\n"
               ,
               device_data->board_revision,

               device_data->ctrl_mmio_physical_addr,
               (unsigned long)device_data->ctrl_mmio_virtual_addr,
               device_data->ctrl_mmio_length / 0x100000, // To MB

               device_data->input_mmio_physical_addr,
               (unsigned long)device_data->input_mmio_virtual_addr,
               device_data->input_mmio_length / 0x100000,

               device_data->output_mmio_physical_addr,
               (unsigned long)device_data->output_mmio_virtual_addr,
               device_data->output_mmio_length / 0x100000
               );
    return 0;
}

static int seki_procfs_single_open(struct inode *i, struct file *f)
{
    struct proc_dir_entry* pde = PDE(i);    // ``How to access passed data.''
    return single_open(f, seki_procfs_file_show, pde->data);
}

struct file_operations seki_file_ops = {
    .owner  = THIS_MODULE,
    .open   = seki_procfs_single_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};


int seki_procfs_create_device(SekiData *seki_data)
{
    char entry_filename[10];
    snprintf(entry_filename, 10, "seki_%1d", seki_data->slot);

    seki_data->proc_entry = proc_create_data(entry_filename, 0666,
                                             seki_proc_base_dir,
                                             &seki_file_ops, seki_data);

    if(!seki_data->proc_entry) {
        pr_err("Error creating proc entry");
        return -ENOMEM;
    }
    return 0;
}

int seki_procfs_remove_device(SekiData *seki_data)
{
    char entry_filename[10];
    snprintf(entry_filename, 10, "seki_%1d", seki_data->slot);

    remove_proc_entry(entry_filename, seki_proc_base_dir);

    return 0;
}

