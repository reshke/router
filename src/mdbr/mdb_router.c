/*
 * =====================================================================================
 *
 *       Filename:  mdb_router.c
 *
 *    Description:  i
 *
 *        Version:  1.0
 *        Created:  31.12.2020 03:32:35
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "mdbr/mdb_router.h"

static mdbr_meta *meta = NULL;

#define MDBR_META_NAMESPACE "MDBR_META_NAMESPACE"

void mdbr_meta_init()
{
	bool found;
	meta = ShmemInitStruct(MDBR_META_NAMESPACE, sizeof(mdbr_meta), &found);
	if (!found) {
		memset(meta, 0, sizeof(mdbr_meta));
		elog(DEBUG2, "initializing structure in shared memory");
	}

	/* meta info is initalied on first use since standart sql command does not lead 
     * * us to invoke extension (only query that are requesres some data from foreign relations) */
	/************************************************/
	// TODO:// maybe some initial/default shards here
	/************************************************/
}

void mdbr_meta_free(mdbr_meta *m)
{
	//TODO : impl this somehow
}

mdbr_meta *mdbr_meta_get()
{
	if (meta == NULL) {
		elog(ERROR, "meta info uninitialized");
	}

	return meta;
}

/*
 * SQL functions
 */

// ============================================================================

PG_FUNCTION_INFO_V1(mdbr_add_shard);
PG_FUNCTION_INFO_V1(mdbr_reset_meta);

MDB_ROUTER_API Datum mdbr_reset_meta(PG_FUNCTION_ARGS)
{
	if (meta == NULL) {
		PG_RETURN_VOID();
	}
	memset(meta, 0, sizeof(mdbr_meta));

	PG_RETURN_VOID();
}

MDB_ROUTER_API Datum mdbr_add_shard(PG_FUNCTION_ARGS)
{
	mdbr_meta_init();
	int argptr = 0;
	char *name = text_to_cstring(PG_GETARG_TEXT_PP(argptr++));

	/*               connection params                                */
	// -----------------------------------------------------------------
	char *host = text_to_cstring(PG_GETARG_TEXT_PP(argptr++));
	char *port = text_to_cstring(PG_GETARG_TEXT_PP(argptr++));
	char *dbname = text_to_cstring(PG_GETARG_TEXT_PP(argptr++));
	char *usr = text_to_cstring(PG_GETARG_TEXT_PP(argptr++));
	char *passwd = text_to_cstring(PG_GETARG_TEXT_PP(argptr++));
	// -----------------------------------------------------------------

	mdbr_meta *m = mdbr_meta_get();
	mdbr_shard *sh;

	mdbr_retcode_t rc = mdbr_shard_alloc(&sh);

	if (rc != MDBR_RETCODE_OK) {
		/* TODO: set errno properly */
		elog(ERROR, "error initializing new shard %d", rc);
		PG_RETURN_VOID();
	}

	m->shard_oids[m->sz++] = sh->oid_self;

	strcpy(sh->name, name);

#define kw_vl_len                                                              \
	2 /* application name/encoding */ + 4 /* function params */ +          \
		1 /* NULL */
	// -----------------------------------------------------------------

	size_t n = 0;

	/* Use "postgres_fdw" as fallback_application_name. */
	strcpy(sh->keywords[n], "fallback_application_name");
	strcpy(sh->values[n], "postgres_fdw");
	n++;

	strcpy(sh->keywords[n], "client_encoding");
	strcpy(sh->values[n], "utf-8");
	n++;

	// -----------------------------------------------------------------

	strcpy(sh->keywords[n], "port");
	strcpy(sh->values[n], port);
	n++;

	strcpy(sh->keywords[n], "host");
	strcpy(sh->values[n], host);
	n++;

	strcpy(sh->keywords[n], "dbname");
	strcpy(sh->values[n], dbname);
	n++;

	strcpy(sh->keywords[n], "user");
	strcpy(sh->values[n], usr);
	n++;
	
        strcpy(sh->keywords[n], "password");
	strcpy(sh->values[n], passwd);
	n++;

	sh->sz = n;
	// -----------------------------------------------------------------

	elog(DEBUG2,
	     "initialized new shard with params %s %s %s %s %s %s %s %s %s %s %s %s",
	     sh->keywords[0], sh->values[0], sh->keywords[1], sh->values[1],
	     sh->keywords[2], sh->values[2], sh->keywords[3], sh->values[3],
	     sh->keywords[4], sh->values[4], sh->keywords[5], sh->values[5]);

	PG_RETURN_VOID();
}

mdbr_retcode_t mdbr_init()
{
	mdbr_oids_init();
	mdbr_shkeys_init();
	mdbr_meta_init();
}
