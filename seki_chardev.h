/**************************************************************************
 * Copyright (c) 2014 Afa.L Cheng <afa@afa.moe>
 *                    Rosen Center for Advanced Computing, Purdue University
 *
 * This file is dual MIT/GPL licensed.
 *
 * <seki_chardev.h>
 *
 ***************************************************************************/


#ifndef SEKI_CHARDEV_H
#define SEKI_CHARDEV_H

#include <linux/fs.h>

struct SekiData;

int seki_chardev_register_file_ctl(void);
void seki_chardev_unregister_file_ctl(void);
int seki_chardev_register_file_seki(void);
void seki_chardev_unregister_file_seki(void);
int seki_chardev_create_file_seki(SekiData *device_data);
void seki_chardev_remove_file_seki(SekiData *device_data);


#endif // SEKI_CHARDEV_H
