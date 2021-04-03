
#ifndef MDB_KEYRANGE_H
#define MDB_KEYRANGE_H

/*
 * =====================================================================================
 *
 *       Filename:  key_range.h
 *
 * =====================================================================================
 */

#include "src/include/pg.h"
#include "mdbr/macro.h"
#include "mdbr/list.h"
#include "mdbr/mdb_router.h"
#include "mdbr/shard.h"
#include "mdbr/oid.h"
#include "mdbr/search_entry.h"

typedef struct {
#define MAX_COLNAME_SZ 32
	/* shared key - related */
	char colname[MAX_COLNAME_SZ];
	/*                       */
	mdbr_key_t lower_bound; // key need to be key_cmp() gt that this bound
	mdbr_key_t upper_bound; // key need to be key_cmp() less that this bound

	/* misc */
	mdbr_oid_t oid_self;
	mdbr_oid_t shard_oid;
} mdbr_key_range;

#define MAX_KEYRANGES 64

typedef struct {
	size_t sz;
	mdbr_oid_t krs_oids[MAX_KEYRANGES]; // TODO: VLA or something
} mdbr_kr_list;

MDBR_INIT_F extern mdbr_retcode_t mdbr_kr_init();
extern mdbr_retcode_t mdbr_kr_alloc(mdbr_key_range **kr);
extern mdbr_kr_list *mdbr_kr_list_init();

#define MDBR_KR_NAMESPACE "MDBR_KR_NAMESPACE"
extern mdbr_key_range *mdbr_key_range_get_from_oid(mdbr_oid_t oid);

// ============================================================================

extern bool mdbr_kr_match_se(mdbr_key_range *kr, mdbr_search_entry *e);
extern mdbr_retcode_t lock_key_range_impl(mdbr_key_range *kr);

extern int assign_key_range_2_shard_impl(mdbr_shard *sh, mdbr_key_range *kr);

/*
 * SQL functions
 */

// ============================================================================

extern MDB_ROUTER_API Datum lock_key_range(PG_FUNCTION_ARGS);

#endif
