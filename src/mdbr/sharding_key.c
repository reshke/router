/*
 * =====================================================================================
 *
 * =====================================================================================
 */
#include <stdlib.h>

#include "mdbr/sharding_key.h"

static mdbr_shkeys_list *l = NULL;
#define MDBR_SHKEY_L_NAMESPACE "mdbr shkey list namespace"

static HTAB *shkey_names_hashtable = NULL;
#define MDBR_SHKEYS_HTAB_NAME "MDBR_SHKEYS_HTAB_NAME"

static HTAB *shkey_oid_hashtable = NULL;
#define MDBR_SHKEYS_HTAB_OIDS "MDBR_SHKEYS_HTAB_OIDS"

typedef struct {
	mdbr_oid_t oid;
} mdbr_shkey_key;

typedef struct {
	mdbr_shkey_key key;
	mdbr_shkey *shkey_ptr;
} mdbr_shkey_cache_entry;

MDBR_INIT_F mdbr_retcode_t mdbr_shkeys_init()
{
	elog(DEBUG2, "initializing shkeys in shmem");
#if 0
        RequestAddinShmemSpace(sizeof(mdbr_shkeys_list));
#endif
	bool found;
	l = ShmemInitStruct(MDBR_SHKEY_L_NAMESPACE, sizeof(mdbr_shkeys_list),
			    &found);

	if (!found) {
		l->sz = 0;
	}
#if 0
        RequestAddinShmemSpace(sizeof(mdbr_shkey_cache_entry) * MAX_SHKEY_L_SZ);
        RequestAddinShmemSpace(sizeof(mdbr_shkey) * MAX_SHKEY_L_SZ);
#endif

#if 1
	HASHCTL ctl;
	memset(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(mdbr_shkey_key);
	ctl.entrysize = sizeof(mdbr_shkey_cache_entry);

	shkey_oid_hashtable = hash_create(MDBR_SHKEYS_HTAB_OIDS, MAX_SHKEY_L_SZ,
					  &ctl, HASH_ELEM);
#endif

#if 0
        HASHCTL ctl;
	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(char) * MAX_SHKEY_LEN; //sizeof(uint32);
	ctl.entrysize = sizeof(shkey_names);

	shkey_names_hashtable =
		ShmemInitHash(MDBR_SHKEYS_HTAB_NAME, MAX_SHKEY_L_SZ,
			      MAX_SHKEY_L_SZ, &ctl, HASH_ELEM | HASH_BLOBS);
#endif

	return MDBR_RETCODE_OK;
}

#define MDBR_SHKEY_NAMESPACE "mdbr shkey namespace"

static mdbr_retcode_t mdbr_shkey_store(mdbr_shkey **shk, mdbr_oid_t *oid)
{
	*oid = get_nxt_unused();
	//             len   prefix + 1           +  max_oid_len
	//             (string literal always includes an implicit null-termination character)
	char prefix[sizeof(MDBR_SHKEY_NAMESPACE) + MAX_OID_LEN];
	sprintf(prefix, "%s-%d", MDBR_SHKEY_NAMESPACE, *oid);

	bool found;
	*shk = ShmemInitStruct(prefix, sizeof(mdbr_shkey), &found);

	(*shk)->oid_self = *oid;
	(*shk)->colnames_cnt = (*shk)->sz = 0;

	return MDBR_RETCODE_OK;
}

mdbr_shkey *mdbr_shkey_getbyoid(mdbr_oid_t oid)
{
#if 1
	bool h_found;
	mdbr_shkey_cache_entry *e;
	mdbr_shkey_key key;
	key.oid = oid;
	e = hash_search(shkey_oid_hashtable, (void *)&key, HASH_FIND, &h_found);

	if (h_found) {
		elog(WARNING, "found in cahce !!!");
		return e->shkey_ptr;
	} else {
		elog(WARNING, "not found in cahce !!!");
	}
#endif

	//                prefix                  + max_oid_len + NULL
	char prefix[sizeof(MDBR_SHKEY_NAMESPACE) + MAX_OID_LEN];
	sprintf(prefix, "%s-%d", MDBR_SHKEY_NAMESPACE, oid);

	bool found;
	mdbr_shkey *shk;
	shk = ShmemInitStruct(prefix, sizeof(mdbr_shkey), &found);

	if (!found) {
		// TODO: set errno properly
		elog(ERROR, "sharding with oid %d not found", oid);
		return NULL;
	}
#if 1
	elog(WARNING, "store in cahce !!!");
	e = hash_search(shkey_oid_hashtable, (void *)&key, HASH_ENTER,
			&h_found);
	e->shkey_ptr = shk;
#endif
	return shk;
}

mdbr_shkey *mdbr_shkey_getbyname(char *name)
{
	for (size_t i = 0; i < l->sz; ++i) {
		mdbr_shkey *shkey = mdbr_shkey_getbyoid(l->shkeys[i]);
		if (strcmp(shkey->name, name) == 0) {
			return shkey;
		}
	}
	// TODO: set errno properly and handle somehow
	//
	elog(ERROR, "failed to get desired shkey by name %s", name);
	return NULL;
}

static mdbr_shkey *llist_search(mdbr_shkeys_list *l, char *name, bool *found)
{
	mdbr_shkey *ret = NULL;
	*found = false;

	if (l == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < l->sz; ++i) {
		mdbr_shkey *curr = mdbr_shkey_getbyoid(l->shkeys[i]);
		if (strcmp(curr->name, name) == 0) {
			*found = true;
			ret = curr;
			return ret;
		}
	}

	return ret;
}

static bool shkey_match_ri(mdbr_shkey *shk, mdbr_search_entry *e)
{
	size_t cnt_matches = 0;

	for (size_t i = 0; i < shk->colnames_cnt; ++i) {
		mdbr_list_t *it;

		mdbr_list_foreach(&e->ri_list, it)
		{
			mdbr_restrict_info *ri =
				mdbr_container_of(it, mdbr_restrict_info, l);
			//	mdbr_restrict_info_eval(ri, e->relid);

			if (strcmp(shk->colnames[i], ri->colname) == 0) {
				// one more colname match
				// (TODO: make this NOT o(n^2), sort colnames or something)
				if (++cnt_matches == shk->colnames_cnt) {
					return true;
				}
			}
		}
	}

	return false;
}

// TODO: pass relname and cache function calls relname -> shkey
// the diff between the above 2 functiin is that second one deals with list
static mdbr_shkey *_mdbr_find_matching_shkey(mdbr_search_entry *e)
{
	for (size_t i = 0; i < l->sz; ++i) {
		mdbr_shkey *shk = mdbr_shkey_getbyoid(l->shkeys[i]);
		if (shkey_match_ri(shk, e)) {
			// it is ok to return this ptr because it is ptr to shmem
			return shk;
		}
	}

	return NULL;
}

mdbr_shkey *mdbr_find_matching_shkey(mdbr_search_entry *se, bool *success)
{
	if (l->sz == 0) {
		// TODO: set errno properly
		elog(ERROR, "no sharding keys configured");
	}

	mdbr_shkey *shk = _mdbr_find_matching_shkey(se);
	if (shk != NULL) {
		*success = true;
		elog(DEBUG2, "sharding key match found, name %s", shk->name);
		return shk;
	}
	*success = false;
	elog(ERROR, "shared keys match not found");
	return NULL;
}

mdbr_oid_t mdbr_shkey_route_shard(mdbr_shkey *shkey, mdbr_search_entry *se)
{
	mdbr_shard_id_t shard_oids = UNROUTED; // unrouted
	for (size_t i = 0; i < shkey->sz; ++i) {
		mdbr_key_range *kr =
			mdbr_key_range_get_from_oid(shkey->krs_oids[i]);
		if (mdbr_kr_match_se(kr, se)) {
			if (shard_oids == UNROUTED) {
				shard_oids = kr->shard_oid;
			} else {
				return UNROUTED;
			}
		}
	}

	return shard_oids;
}

mdbr_oid_t mdbr_route_by_se(List *sel /* a list of mdbr search entries */,
			    bool *success)
{
	ListCell *lc;
	*success = false;
	mdbr_oid_t ret = UNROUTED;

	foreach(lc, sel)
	{
		mdbr_search_entry *se = (mdbr_search_entry *)lfirst(lc);
		bool success;
		mdbr_shkey *shkey = mdbr_find_matching_shkey(se, &success);

		if (!success) {
			elog(ERROR,
			     "query could not be proved execute on single shard");
			return UNROUTED;
		}

		mdbr_oid_t curr = mdbr_shkey_route_shard(shkey, se);

		elog(DEBUG2, "processed shoid %d", curr);

		if (ret == UNROUTED) {
			ret = curr;
		} else if (curr != UNROUTED && curr != ret) {
			elog(ERROR,
			     "query could not be proved execute on single shard");
			return UNROUTED;
		}
	}

	if (ret != UNROUTED) {
		*success = true;
	}

	return ret;
}

mdbr_retcode_t mdbr_shkeys_list_add(mdbr_shkey *sh)
{
	if (l->sz >= MAX_SHKEY_L_SZ) {
		elog(ERROR, "too many sharing keys");
	}
	l->shkeys[l->sz++] = sh->oid_self;
	elog(DEBUG2, "inserted new shkey, having a total of %d", l->sz);
}

/*
 * SQL functions
 */

// ============================================================================

PG_FUNCTION_INFO_V1(create_sharding_key);
// TODO:: map shkey name into oid and raise error on shkey creation when shkey name in already usd
MDB_ROUTER_API Datum create_sharding_key(PG_FUNCTION_ARGS)
{
	// as this func is not called in the fdw context
	// -----------------------------------------------------------------
	mdbr_init();

	char *name = text_to_cstring(PG_GETARG_TEXT_PP(0));
#ifdef SHKEY_USE_ARRAY_TYPE
	// not working for now
	ArrayType *shkey_args = PG_GETARG_ARRAYTYPE_P(1);
#else
	char *colname = text_to_cstring(PG_GETARG_TEXT_PP(1));
#endif
	// -----------------------------------------------------------------

	if (strlen(name) + 1 > MAX_SHKEY_LEN) {
		// TODO:: set errno properly
		elog(ERROR, "shkey name too long");
	}

	bool found;
	//uint32 namehash;
	//namehash = string_hash(name, strlen(name));
#if 0 
        shkey_names *oid_htbl;

	// elog(DEBUG2, "shkey name hash %d", namehash);

	oid_htbl = hash_search(shkey_names_hashtable, (void *)&name, HASH_FIND,
			       &found);
#endif

	if (found) {
		elog(DEBUG2, "shared keys with name %s already exists", name);
	}

	mdbr_shkey *key;
	mdbr_oid_t oid;
	mdbr_shkey_store(&key, &oid);

	strcpy(key->name, name);

#ifdef SHKEY_USE_ARRAY_TYPE

	elog(DEBUG2, "shmem oid of new shkey %d\n", oid);

	int ndims = ARR_NDIM(shkey_args);
	int *dims = ARR_DIMS(shkey_args);
	Oid element_type = ARR_ELEMTYPE(shkey_args);
	array_iter it;

	int nitems = ArrayGetNItems(ndims, dims);

	if (nitems > MAX_SHARDING_KEY_NUM) {
		elog(ERROR, "max number of colname (%d) exeeded",
		     MAX_SHARDING_KEY_NUM);
	}

	array_iter_setup(&it, shkey_args);
	ArrayMetaState *my_extra;

	my_extra = MemoryContextAlloc(fcinfo->flinfo->fn_mcxt,
				      sizeof(ArrayMetaState));

	/* Get info about element type, including its send proc */
	get_type_io_data(element_type, IOFunc_send, &my_extra->typlen,
			 &my_extra->typbyval, &my_extra->typalign,
			 &my_extra->typdelim, &my_extra->typioparam,
			 &my_extra->typiofunc);

	for (size_t i = 0; i < nitems; ++i) {
		Datum elt;
		bool isnull;
		elt = array_iter_next(&it, &isnull, i, my_extra->typlen,
				      my_extra->typbyval, my_extra->typalign);

		char *colname = DatumGetCString(elt);

		if (strlen(colname) + 1 >= COLNAME_MAX_SZ) {
			elog("colunm %s name too long (max of %d)", colname,
			     COLNAME_MAX_SZ);
		}

		strcpy(key->colnames[key->sz++], colname);
	}
#else
	if (strlen(colname) + 1 >= COLNAME_MAX_SZ) {
		elog("colunm %s name too long (max of %d)", colname,
		     COLNAME_MAX_SZ);
	}
	strcpy(key->colnames[key->colnames_cnt++], colname);
	elog(DEBUG2, "initialized key with colname %s oid %d", key->colnames[0],
	     key->oid_self);
#endif
#if 0
	/* key created succesfully */
	oid_htbl = hash_search(shkey_names_hashtable, (void *)&name, HASH_ENTER,
			       &found);
	oid_htbl->oid = oid;
#endif

	mdbr_shkeys_list_add(key);

	PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(show_shkey);

MDB_ROUTER_API Datum show_shkey(PG_FUNCTION_ARGS)
{
	mdbr_init();

	char *shkey_name = text_to_cstring(PG_GETARG_TEXT_PP(0));

	bool found;

	mdbr_shkey *shk = llist_search(l, shkey_name, &found);

	if (!found) {
		elog(ERROR, "shkey wtih name %s not found", shkey_name);
	}

	elog(DEBUG2, "FOUND  OK , KR NUM %d", shk->sz);
	PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(list_sharding_keys);

#define RET_ROWS 3
MDB_ROUTER_API Datum list_sharding_keys(PG_FUNCTION_ARGS);

int assign_key_range_2_shard_impl(mdbr_shard *sh, mdbr_key_range *kr)
{
}

PG_FUNCTION_INFO_V1(assign_key_range_2_shard);
MDB_ROUTER_API Datum assign_key_range_2_shard(PG_FUNCTION_ARGS)
{
	mdbr_init();

	/**/
	char *shard_name = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char *shkey_name = text_to_cstring(PG_GETARG_TEXT_PP(1));
#ifdef SHKEY_USE_ARRAY_TYPE
	/* TODO: impl */
#else
	mdbr_key_t l_bound = PG_GETARG_INT64(2);
	mdbr_key_t u_bound = PG_GETARG_INT64(3);
#endif
	bool found = false;
#if 0
	shkey_names *oid_htbl = hash_search(
		shkey_names_hashtable, (void *)&shkey_name, HASH_FIND, &found);
#endif

	mdbr_shkey *shkey;
	if (!found) {
		elog(DEBUG2, "shared key not found in HTAB name %s",
		     shkey_name);
		shkey = llist_search(l, shkey_name, &found);

		if (!found) {
			elog(ERROR, "shared key not found in llsit name %s",
			     shkey_name);
		}
	}

	if (found) {
		elog(DEBUG2, "shkey found ok");
	}

	if (shkey->sz >= MAX_KEY_RANGE_PER_SHKEY) {
		elog(ERROR, "too many key ranges per shkey");
	}

	mdbr_shard *sh = mdbr_shard_get_byname(shard_name);

	if (sh == NULL) {
		elog(ERROR, "shard with name %s not found", shard_name);
	}

	mdbr_key_range *kr;
	mdbr_kr_alloc(&kr);

	strcpy(kr->colname, shkey->colnames[0]);

	// TODO: make better
	kr->lower_bound = l_bound;
	kr->upper_bound = u_bound;

	kr->shard_oid = sh->oid_self;

	shkey->krs_oids[shkey->sz++] = kr->oid_self;

	elog(DEBUG2, "new key range colname %s shoid %d oid self %d",
	     kr->colname, kr->shard_oid, kr->oid_self);

#ifdef SHKEY_USE_ARRAY_TYPE
	/* something */
#else
#endif
}
