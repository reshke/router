/*
 * =====================================================================================
 *
 *       Filename:  oid.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  16.01.2021 20:17:39
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "mdbr/oid.h"

static const char *prefix = "MDBR_OID_NAMESPACE";

typedef struct {
	LWLock lock;
	mdbr_oid_t oid_last;
} mdbr_oid_store_t;

MDBR_INIT_F void mdbr_oids_init()
{
	bool found;
	mdbr_oid_store_t *oid_store =
		ShmemInitStruct(prefix, sizeof(mdbr_oid_store_t), &found);
	if (!found) {
		oid_store->oid_last = 0;
	}
}

mdbr_oid_t get_nxt_unused()
{
	bool found;

	mdbr_oid_store_t *oid_store =
		ShmemInitStruct(prefix, sizeof(mdbr_oid_store_t), &found);

	if (!found) {
		/* not initilised */
		elog(ERROR, "oid storae not initialized ololo");
	}
	mdbr_oid_t ret = -1;
	LWLockAcquire(&oid_store->lock, LW_EXCLUSIVE);
	{
		ret = ++oid_store->oid_last;
	}
	LWLockRelease(&oid_store->lock);

	return ret;
}
