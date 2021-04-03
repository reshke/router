#ifndef MDBR_LOCAl_TABLE
#define MDBR_LOCAl_TABLE

#define MAX_LTABLES 7
#define MAX_LTABLE_NAME 30

typedef struct {
	size_t sz;
	char ltables[MAX_LTABLES][MAX_LTABLE_NAME]; //TODO:VLA or something
} mdbr_ltables_list;

extern MDB_ROUTER_API Datum add_local_table(PG_FUNCTION_ARGS);
extern bool is_local_table(char *n);
extern void mdbr_ltables_init();

#endif /* MDBR_LOCAl_TABLE */
