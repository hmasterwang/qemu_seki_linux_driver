/**************************************************************************
 * Copyright (c) 2014 Afa.L Cheng <afa@afa.moe>
 *                    Rosen Center for Advanced Computing, Purdue University
 *
 * This file is dual MIT/GPL licensed.
 *
 ***************************************************************************/


#ifndef SEKI_PROCFS_H
#define SEKI_PROCFS_H

#define SEKI_PROCFS_NAME "seki"

struct SekiData;

int seki_init_procfs(void);
void seki_uninit_procfs(void);
int seki_procfs_create_file_device(SekiData *seki_data);
void seki_procfs_remove_file_device(SekiData *seki_data);



#endif // SEKI_PROCFS_H
