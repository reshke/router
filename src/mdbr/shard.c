/*
 * =====================================================================================
 *
 *       Filename:  shard.c
 *
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "mdbr/shard.h"

#define MDBR_SHARDS_NAMESPACE "MDBR_SHARDS_NAMESPACE"
#define MDBR_SHARD_L_NAMESPACE "MDBR_SHARDS_NAMESPACE"

static mdbr_shards_list *l = NULL;

MDBR_INIT_F void mdbr_shards_init()
{
	bool found;
	l = ShmemInitStruct(MDBR_SHARD_L_NAMESPACE, sizeof(mdbr_shard), &found);

	if (!found) {
		memset(l, 0, sizeof(mdbr_shards_list));
	}

	return;
}

mdbr_retcode_t mdbr_shard_alloc(mdbr_shard **sh)
{
	if (l->sz >= MAX_SHARDS) {
		elog(ERROR, "too many shars, limit %d reached", MAX_SHARDS);
	}
	mdbr_oid_t oid = get_nxt_unused();
	//                prefix              + max_oid_len + NULL
	char prefix[sizeof(MDBR_SHARDS_NAMESPACE) + MAX_OID_LEN];
	sprintf(prefix, "%s-%d", MDBR_SHARDS_NAMESPACE, oid);

	bool found;
	*sh = ShmemInitStruct(prefix, sizeof(mdbr_shard), &found);
	if (!found) {
		(*sh)->oid_self = oid;
		l->sh_oids[l->sz++] = oid;
	} else {
		// TODO: set errno
	}

	return MDBR_RETCODE_OK;
}

void mdbr_shard_free(mdbr_shard *sh)
{
	// TODO: impl somehow
}

mdbr_shard *mdbr_shard_get_byoid(mdbr_oid_t oid)
{
	mdbr_shard *sh;

	char prefix[sizeof(MDBR_SHARDS_NAMESPACE) + MAX_OID_LEN];
	sprintf(prefix, "%s-%d", MDBR_SHARDS_NAMESPACE, oid);

	bool found;
	sh = ShmemInitStruct(prefix, sizeof(mdbr_shard), &found);

	if (!found) {
		//TODO: set errno
		return NULL;
	}

	return sh;
}

mdbr_shard *mdbr_shard_get_byname(char *name)
{
	for (size_t i = 0; i < l->sz; ++i) {
		mdbr_shard *sh = mdbr_shard_get_byoid(l->sh_oids[i]);
		if (strcmp(sh->name, name) == 0) {
			return sh;
		}
	}
	// TODO: set errno
	return NULL;
}

void mdbr_get_shard_connection_dst(mdbr_oid_t oid, PGconn **dst)
{
	mdbr_shard *sh = mdbr_shard_get_byoid(oid);
	if (sh == NULL) {
		elog(ERROR, "failed to get shard with oid %d", oid);
	}

	elog(DEBUG2, "trying to acuire connection to shard (oid %d)", oid);

	char **keywords =
		(char **)malloc((sh->sz /* + NULL */ + 1) * sizeof(char *));
	char **values =
		(char **)malloc((sh->sz /* + NULL */ + 1) * sizeof(char *));

	for (size_t i = 0; i < sh->sz; ++i) {
		keywords[i] =
			malloc((MAXCSTRINGSZ /* + NULL */ + 1) * sizeof(char));
		strcpy(keywords[i], sh->keywords[i]);

		values[i] =
			malloc((MAXCSTRINGSZ /* + NULL */ + 1) * sizeof(char));
		strcpy(values[i], sh->values[i]);
	}

	keywords[sh->sz] = values[sh->sz] = NULL;

	*dst = PQconnectdbParams(keywords, values, false);

	return;
	for (size_t i = 0; i < sh->sz; ++i) {
		pfree(keywords[i]);
		pfree(values[i]);
	}

	pfree(keywords);
	pfree(values);

	return;
	/* */
}

PGconn *mdbr_get_shard_connection(mdbr_oid_t oid)
{
	mdbr_shard *sh = mdbr_shard_get_byoid(oid);
	if (sh == NULL) {
		elog(ERROR, "failed to get shard with oid %d", oid);
	}

	elog(DEBUG2, "trying to acuire connection to shard (oid %d)", oid);

	char **keywords =
		(char **)malloc((sh->sz /* + NULL */ + 1) * sizeof(char *));
	char **values =
		(char **)malloc((sh->sz /* + NULL */ + 1) * sizeof(char *));

	for (size_t i = 0; i < sh->sz; ++i) {
		keywords[i] =
			malloc((MAXCSTRINGSZ /* + NULL */ + 1) * sizeof(char));
		strcpy(keywords[i], sh->keywords[i]);

		values[i] =
			malloc((MAXCSTRINGSZ /* + NULL */ + 1) * sizeof(char));
		strcpy(values[i], sh->values[i]);
	}

	keywords[sh->sz] = values[sh->sz] = NULL;

	PGconn *conn = PQconnectdbParams(keywords, values, false);

	return conn;

	for (size_t i = 0; i < sh->sz; ++i) {
		pfree(keywords[i]);
		pfree(values[i]);
	}

	pfree(keywords);
	pfree(values);

	return conn;
	/* */
}
