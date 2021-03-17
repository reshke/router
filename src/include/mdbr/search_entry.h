#ifndef MDBR_SEARCH_ENTRY_H
#define MDBR_SEARCH_ENTRY_H
/*
 * =====================================================================================
 *
 *       Filename:  search_entry.h
 *
 * =====================================================================================
 */

#include "mdbr/key.h"
#include "mdbr/list.h"
#include "src/include/pg.h"
#include "mdbr/macro.h"

typedef struct {
	/* private */
	Expr *ev;
	bool evaluated;

	/*         */
	char *colname;
	mdbr_ri_ops op;
	mdbr_key_t val;

	mdbr_list_t l; // this is linked list
} mdbr_restrict_info;

/* search condiions manipulation */
extern mdbr_retcode_t mdbr_restrict_info_init(mdbr_restrict_info *ri);
extern void mdbr_restrict_info_assing_exprval(mdbr_restrict_info *ri, Expr *ev);
extern void mdbr_restrict_info_assing_col(mdbr_restrict_info *ri, char *src);
extern void mdbr_restrict_info_eval(mdbr_restrict_info *ri, Index relid);
/*            */
extern void mdbr_restrict_info_free(mdbr_restrict_info *ri);

typedef struct {
	Oid relid;

	mdbr_list_t ri_list; /* a list of restrict infos  */
} mdbr_search_entry; // this struct should be parsed to qry and passed to f()

extern mdbr_search_entry *mdbr_search_entry_init();
extern mdbr_search_entry *mdbr_se_find(List *l /**/,
				       Index relid
				       /* se selector */,
				       bool *found);
/* HEADER manipulation */
extern void mdbr_se_set_rel(mdbr_search_entry *e, Oid relid);
extern mdbr_retcode_t mdbr_se_append_ri(mdbr_search_entry *se,
					mdbr_restrict_info *ri);
/*  */
extern void mdbr_search_entry_free(mdbr_search_entry *e);

#endif /* MDBR_SEARCH_ENTRY_H */
