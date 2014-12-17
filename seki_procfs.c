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
static struct proc_dir_entry *seki_proc_status_file;

// Static func prototypes
static int seki_procfs_create_file_status(void);
static int seki_procfs_remove_file_status(void);

// Init & uninit
int seki_init_procfs(void)
{
    if (!seki_proc_base_dir)
        seki_proc_base_dir = proc_mkdir(SEKI_PROCFS_NAME, NULL);

    return seki_procfs_create_file_status();
}

int seki_uninit_procfs(void)
{
    remove_proc_entry(SEKI_PROCFS_NAME, NULL);
    seki_proc_base_dir = 0;

    return seki_procfs_remove_file_status();
}

// File Ops
// Device files
static int seki_procfs_file_dev_show(struct seq_file *f, void *data)
{
    SekiData *device_data;
    SEKI_UNUSED(data);

    device_data = f->private;

    if (!device_data->used)
        return -1;

    seq_printf(f,
               "Board Revision:                 0x%02x\n"
               "Control MMIO Physical:          0x%016lx\n"
               "Control MMIO Kernel Virtual:    0x%016lx\n"
               "Control MMIO Length:            0x%04lxMB\n"
               "Input MMIO Physical:            0x%016lx\n"
               "Input MMIO Kernel Virtual:      0x%016lx\n"
               "Input MMIO Length:              0x%04lxMB\n"
               "Output MMIO Physical:           0x%016lx\n"
               "Output MMIO Kernel Virtual:     0x%016lx\n"
               "Output MMIO Length:             0x%04lxMB\n"
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

static int seki_procfs_file_dev_single_open(struct inode *i, struct file *f)
{
    struct proc_dir_entry* pde = PDE(i);    // ``How to access passed data.''
    return single_open(f, seki_procfs_file_dev_show, pde->data);
}

struct file_operations seki_file_dev_ops = {
    .owner  = THIS_MODULE,
    .open   = seki_procfs_file_dev_single_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

int seki_procfs_create_file_device(SekiData *seki_data)
{
    char entry_filename[10];
    snprintf(entry_filename, 10, "seki_%1d", seki_data->device_num);

    seki_data->proc_entry = proc_create_data(entry_filename, 0666,
                                             seki_proc_base_dir,
                                             &seki_file_dev_ops, seki_data);

    if (!seki_data->proc_entry) {
        pr_err("Error creating procfs device entry");
        return -ENOMEM;
    }
    return 0;
}

int seki_procfs_remove_file_device(SekiData *seki_data)
{
    char entry_filename[10];
    snprintf(entry_filename, 10, "seki_%1d", seki_data->device_num);

    remove_proc_entry(entry_filename, seki_proc_base_dir);

    return 0;
}

// Procfs status file
static int seki_procfs_file_status_show(struct seq_file *f, void *data)
{
    SEKI_UNUSED(data);

    seq_printf(f,
               "Device Count:       %2d\n"
               "\n"
               ,
               _seki_device_count
               );

    for (int i = 0; i < _seki_device_count; ++i) {
        if (!_seki_data_array[i].used) {
            pr_debug("_seki_data_array corrupted\n");

            continue;
        }

        seq_printf(f,
                   "Device %2d:\n"
                   "    Slot:                  %2d\n"
                   "    Board Revision:      0x%02x\n"
                   "    Control Mem Region: %3luMB\n"
                   "    Input Mem Region:   %3luMB\n"
                   "    Output Mem Region:  %3luMB\n"
                   "\n"
                   ,
                   _seki_data_array[i].device_num,
                   _seki_data_array[i].slot,
                   _seki_data_array[i].board_revision,
                   _seki_data_array[i].ctrl_mmio_length / 0x100000,
                   _seki_data_array[i].input_mmio_length / 0x100000,
                   _seki_data_array[i].output_mmio_length / 0x100000
                   );

    }

    return 0;
}

static int seki_procfs_file_status_single_open(struct inode *i, struct file *f)
{
    SEKI_UNUSED(i);
    return single_open(f, seki_procfs_file_status_show, NULL);
}

struct file_operations seki_file_status_ops = {
    .owner  = THIS_MODULE,
    .open   = seki_procfs_file_status_single_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static int seki_procfs_create_file_status(void)
{
    seki_proc_status_file = proc_create_data("status", 0666, seki_proc_base_dir,
                                             &seki_file_status_ops, NULL);

    if (!seki_proc_status_file) {
        pr_err("Error creating procfs status entry");
        return -ENOMEM;
    }
    return 0;
}

static int seki_procfs_remove_file_status(void)
{
    remove_proc_entry("status", seki_proc_base_dir);
    seki_proc_status_file = 0;
    return 0;
}

