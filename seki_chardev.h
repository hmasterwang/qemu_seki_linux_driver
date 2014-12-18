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

int seki_register_chardev_file_ctl(void);
void seki_unregister_chardev_file_ctl(void);


#endif // SEKI_CHARDEV_H
