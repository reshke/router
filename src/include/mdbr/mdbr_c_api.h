
#ifndef MDBR_C_API
#define MDBR_C_API
/*
 * =====================================================================================
 *
 *       Filename:  mdbr_c_api.h
 *
 */

#include "pg.h"
#include "mdbr/search_entry.h"

// the main idea is that we create search entryes list, enlairge it with some conditions
// and pass to this f-n, which kindly returns shard connection (or not)
//
// Given a list of search entries (which is a list of structures, containing info about matchig key ranges, grouped by relid)
// return shared memory id of shard or -1 (UROUTED)
//
extern mdbr_oid_t mdbr_route_by_se(List *el /* a list of mdbr search entries */,
				   bool *success);
extern PGconn mdbr_acuire_shard_conn(mdbr_oid_t *id);

#endif /* MDBR_C_API */
