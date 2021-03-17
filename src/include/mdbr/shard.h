#ifndef MDBR_SHARD_H
#define MDBR_SHARD_H

/*
 * =====================================================================================
 *
 *       Filename:  shard.h
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "mdbr/macro.h"
#include "mdbr/oid.h"

typedef int mdbr_shard_id_t;

#define MAXCSTRINGSZ 40
#define MAX_SHARD_CONNPARAMS 12

// TODO:// VLA
#define MAX_SHARDS 4

// TODO: dynamic shmem access/allocation
typedef struct {
	size_t sz;
	/* connection params  */
	char keywords[MAX_SHARD_CONNPARAMS][MAXCSTRINGSZ];
	char values[MAX_SHARD_CONNPARAMS][MAXCSTRINGSZ];
	/*                    */
	mdbr_oid_t oid_self;
	char name[MAXCSTRINGSZ];
} mdbr_shard;

typedef struct {
	mdbr_oid_t sh_oids[MAX_SHARDS]; // TODO: VLA or something
	size_t sz;
} mdbr_shards_list;

MDBR_INIT_F extern void mdbr_shards_init();
extern void mdbr_shard_destroy(mdbr_shard *sh);

extern mdbr_retcode_t mdbr_shard_alloc(mdbr_shard **sh);
extern mdbr_shard_id_t mdbr_shoid_2_shard_id(mdbr_oid_t oid);

extern mdbr_shard *mdbr_shard_get_byoid(mdbr_oid_t oid);
extern mdbr_shard *mdbr_shard_get_byname(char *name);

extern PGconn *mdbr_get_shard_connection(mdbr_oid_t oid);

#endif /* MDBR_SHARD_H */
