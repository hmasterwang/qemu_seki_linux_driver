/**************************************************************************
 * Copyright (c) 2014 Afa.L Cheng <afa@afa.moe>
 *                    Rosen Center for Advanced Computing, Purdue University
 *
 * This file is dual MIT/GPL licensed.
 *
 * <seki_device_defs.h>
 *
 ***************************************************************************/


#ifndef SEKI_DEVICE_DEFS_H
#define SEKI_DEVICE_DEFS_H

#include <linux/spinlock.h>

// Forward declaration
struct proc_dir_entry;

#define SEKI_DRIVER_NAME        "seki_emu"
#define SEKI_MAX_PCI_DEVICES    4       // I don't believe you can plug 4 more
#define SEKI_VENDOR_ID          0xFA58  // This is an unoccupied vendor id
#define SEKI_DEVICE_ID          0x0961  // Random

#define SEKI_UNUSED(var)        ((void)(var))

typedef struct SekiData {
    unsigned int    used;       // indicating if this struct is used
    unsigned int    slot;

    unsigned long   ctrl_mmio_physical_addr;
    void            *ctrl_mmio_virtual_addr;
    unsigned long   ctrl_mmio_length;

    unsigned long   input_mmio_physical_addr;
    void            *input_mmio_virtual_addr;
    unsigned long   input_mmio_length;

    unsigned long   output_mmio_physical_addr;
    void            *output_mmio_virtual_addr;
    unsigned long   output_mmio_length;

    unsigned char   board_revision;

    unsigned int    device_num;

    struct proc_dir_entry  *proc_entry;

    // Locks are for write only
    spinlock_t      ctrl_mmio_lock;
    spinlock_t      input_mmio_lock;
    spinlock_t      output_mmio_lock;
} SekiData;

extern unsigned int _seki_device_count;
extern SekiData _seki_data_array[SEKI_MAX_PCI_DEVICES];
extern int      _seki_device_number_used_map[SEKI_MAX_PCI_DEVICES];

#endif // SEKI_DEVICE_DEFS_H
