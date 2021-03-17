#ifndef MDB_ROUTER_H
#define MDB_ROUTER_H

/*
 * =====================================================================================
 *
 *   Organization:  YNDX 
 *
 * =====================================================================================
 */

//#include "src/include/c.h"

//#include "mdbr/shard.h"

#include "src/include/pg.h"
#include "src/include/c.h"

#include "mdbr/shard.h"

typedef struct {
	mdbr_oid_t shard_oids[MAX_SHARDS];
	size_t sz;
} mdbr_meta;

// TODO:// make more mdbr

extern void mdbr_meta_init();
extern void mdbr_meta_destroy(mdbr_meta *m);

extern mdbr_meta *mdbr_meta_get();

/*
 * SQL functions
 */
//=============================================================================

#define MDB_ROUTER_API

extern MDB_ROUTER_API Datum mdbr_reset_meta(PG_FUNCTION_ARGS);
extern MDB_ROUTER_API Datum mdbr_add_shard(PG_FUNCTION_ARGS);

mdbr_retcode_t mdbr_init();

#endif
