#include "mdbr/macro.h"
#include "src/include/pg.h"
#include "mdbr/mdb_router.h"
#include "mdbr/list.h"
#include "mdbr/local_table.h"

static mdbr_ltables_list *l = NULL;
#define MDBR_LTABLE_NAMESPACE "mdbr local table list namespace"

static void mdbr_ltables_init()
{
	elog(DEBUG2, "initializing ltables in shmem");
	bool found;
	l = ShmemInitStruct(MDBR_LTABLE_NAMESPACE, sizeof(mdbr_ltables_list),
			    &found);

	if (!found) {
		l->sz = 0;
	}
}

bool is_local_table(char *n)
{
	mdbr_ltables_init();

	for (size_t i = 0; i < l->sz; ++i) {
		if (strcmp(n, l->ltables[i]) == 0) {
			return true;
		}
	}

	return false;
}

PG_FUNCTION_INFO_V1(add_local_table);
MDB_ROUTER_API Datum add_local_table(PG_FUNCTION_ARGS)
{
	char *name = text_to_cstring(PG_GETARG_TEXT_PP(0));
	mdbr_ltables_init();

	if (l->sz == MAX_LTABLES) {
		elog(ERROR, "too many ltables");
	}

	strcpy(l->ltables[l->sz++], name);
}
