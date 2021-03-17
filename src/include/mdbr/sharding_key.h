/*
 * =====================================================================================
 *   Descr: shkey stands for sharding key
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef MDBR_SHKEY_H
#define MDBR_SHKEY_H

#include "mdbr/macro.h"
#include "src/include/pg.h"
#include "mdbr/oid.h"
#include "mdbr/shard.h"
#include "mdbr/mdb_router.h"
#include "mdbr/list.h"
#include "mdbr/shard.h"
#include "mdbr/key_range.h"
#include "mdbr/search_entry.h"

typedef char *
	entry_key_t; // reparsed colunm name to match some routing rule / sharding key
typedef Expr *
	entry_val_t; // Some expresion which we would simply pass to underlying shkey to match some key range

//                 Sample of subnode in query plan which we want to get for 'select * where i < 7'
//               Expr * = OpExpr * like   pass this whole structure (or a list of) to routing func
//         v              v
//        Var *         ConstExpr *
//   (parse colname       parse 7 from here
//           from here)
#define COLNAME_MAX_SZ 32

#define MAX_SHKEY_LEN 16
#define MAX_SHARDING_KEY_NUM 1 // only one many colname in sharding key hah;
#define MAX_SHKEY_L_SZ 10 // not so many sharding key hah;
#define MAX_KEY_RANGE_PER_SHKEY                                                \
	5 // this is how much shard can contain one relation

typedef struct {
	size_t colnames_cnt;
#if 0 
    mdbr_search_entry *entryes[FLEXIBLE_ARRAY_MEMBER]; /* :TODO: impl */
#else
	char colnames[MAX_SHARDING_KEY_NUM][COLNAME_MAX_SZ];
#endif
	char name[MAX_SHKEY_LEN];
	mdbr_oid_t oid_self;
	size_t sz;
	mdbr_oid_t krs_oids[MAX_KEY_RANGE_PER_SHKEY];
} mdbr_shkey;

typedef struct {
	size_t sz;
	mdbr_oid_t shkeys[MAX_SHKEY_L_SZ]; //TODO:VLA or something
} mdbr_shkeys_list;

extern mdbr_retcode_t mdbr_shkeys_list_add(mdbr_shkey *sh);

MDBR_INIT_F extern mdbr_retcode_t mdbr_shkeys_init();

extern mdbr_shkey *mdbr_shkey_getbyname(char *name);
extern mdbr_shkey *mdbr_search_entries_match_shkey(List *es);

extern mdbr_oid_t mdbr_shkey_route_shard(mdbr_shkey *shkey,
					 mdbr_search_entry *se);

/*
 * SQL functions
 */

//=============================================================================

extern MDB_ROUTER_API Datum create_sharding_key(PG_FUNCTION_ARGS);
extern MDB_ROUTER_API Datum list_sharding_keys(PG_FUNCTION_ARGS);
extern MDB_ROUTER_API Datum assign_key_range_2_shard(PG_FUNCTION_ARGS);
/* just for fun */
extern MDB_ROUTER_API Datum show_shkey(PG_FUNCTION_ARGS);

#endif /* MDBR_SHKEY_H */
