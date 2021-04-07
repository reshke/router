/*
 * =====================================================================================
 *
 *       Filename:  key_range.c
 *
 *        Version:  1.0
 *       Revision:  none
 *
 *   Organization:  
 *
 * =====================================================================================
 */

#include "mdbr/key_range.h"

#define MDBR_KEYS_L_NAMESPACE "mdbr_key_range_namespace"

static mdbr_kr_list *shkey_krs_list = NULL;

static HTAB *kr_hashtable = NULL;
#define MDBR_KR_HTAB "MDBR_KR_HTAB"

typedef struct {
	mdbr_oid_t oid;
} mdbr_kr_key;

typedef struct {
	mdbr_kr_key key;
	mdbr_key_range *kr_ptr;
} mdbr_kr_cache_entry;

MDBR_INIT_F mdbr_retcode_t mdbr_kr_init()
{
	HASHCTL ctl;
	memset(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(mdbr_kr_key);
	ctl.entrysize = sizeof(mdbr_kr_cache_entry);

	kr_hashtable =
		hash_create(MDBR_KR_HTAB, MAX_KEYRANGES, &ctl, HASH_ELEM);

	bool found;
	shkey_krs_list = ShmemInitStruct(MDBR_KEYS_L_NAMESPACE,
					 sizeof(mdbr_kr_list), &found);

	return MDBR_RETCODE_OK;
}

mdbr_key_range *mdbr_key_range_get_from_oid(mdbr_oid_t oid)
{
#if 1
	bool h_found;
	mdbr_kr_cache_entry *e;
	mdbr_kr_key key;
	key.oid = oid;
	e = hash_search(kr_hashtable, (void *)&key, HASH_FIND, &h_found);

	if (h_found) {
		elog(DEBUG1, "found in cahce !!!");
		return e->kr_ptr;
	} else {
		elog(DEBUG1, "not found in cahce !!!");
	}
#endif
	//                prefix               + max_oid_len + NULL
	char prefix[sizeof(MDBR_KR_NAMESPACE) + MAX_OID_LEN];
	sprintf(prefix, "%s-%d", MDBR_KR_NAMESPACE, oid);

	bool *found;
	mdbr_key_range *kr =
		ShmemInitStruct(prefix, sizeof(mdbr_key_range), &found);

	if (!found) {
		// TODO: set errno properly
		return NULL;
	}

#if 1
	elog(DEBUG1, "store in cahce !!!");
	e = hash_search(kr_hashtable, (void *)&key, HASH_ENTER, &h_found);
	e->kr_ptr = kr;
#endif

	return kr;
}

static mdbr_retcode_t mdbr_key_range_store(mdbr_key_range **kr, mdbr_oid_t *oid)
{
	*oid = get_nxt_unused();
	//                prefix               + max_oid_len
	char prefix[sizeof(MDBR_KR_NAMESPACE) + MAX_OID_LEN];
	sprintf(prefix, "%s-%d", MDBR_KR_NAMESPACE, *oid);

	bool found;
	*kr = ShmemInitStruct(prefix, sizeof(mdbr_key_range), &found);

	return MDBR_RETCODE_OK;
}

mdbr_retcode_t mdbr_kr_alloc(mdbr_key_range **kr)
{
	if (shkey_krs_list == NULL) {
		/* structure in shared memory was not initialized */
		return MDBR_RETCODE_NOTOK;
	}

	if (shkey_krs_list->sz >= MAX_KEYRANGES) {
		/* too many sharding keys */
		// TODO: set errno properly

		elog(ERROR, "too many key ranges allocated, max of %d",
		     MAX_KEYRANGES);
		return MDBR_RETCODE_NOTOK;
	}

	mdbr_oid_t oid;
	// as everything is stored in shared memory we cannot just store a pointer
	mdbr_retcode_t rc = mdbr_key_range_store(kr, &oid);

	shkey_krs_list->krs_oids[++shkey_krs_list->sz] = oid;
	(*kr)->oid_self = oid;

	if (rc != MDBR_RETCODE_OK) {
		// TODO:handle
	}

	return rc;
}

bool mdbr_kr_match_se(mdbr_key_range *kr, mdbr_search_entry *se)
{
	mdbr_list_t *it;
	mdbr_list_foreach(&se->ri_list, it)
	{
		mdbr_restrict_info *ri_curr =
			mdbr_container_of(it, mdbr_restrict_info, l);

		mdbr_restrict_info_eval(ri_curr, se->relid);

		if (strcmp(ri_curr->colname, kr->colname)) {
			continue;
		}
		if (ri_curr->val < kr->lower_bound ||
		    ri_curr->val >= kr->upper_bound) {
			return false;
		}
		// TODO: fffix
	}
	return true;
}

mdbr_retcode_t lock_key_range_impl(mdbr_key_range *kr);

MDB_ROUTER_API Datum lock_key_range(PG_FUNCTION_ARGS);
