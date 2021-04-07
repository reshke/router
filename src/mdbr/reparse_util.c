/*
 * =====================================================================================
 *
 *       Filename:  reparse_util.c
 *
 *    Description:  :x
 *
 *        Version:  1.0
 *        Created:  13.01.2021 01:09:50
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "mdbr/search_entry.h"
#include "pg.h"

static mdbr_search_entry *get_se_byrelid(List *es, Oid relid, bool *success)
{
	ListCell *lc;
	*success = false;

	foreach(lc, es)
	{
		mdbr_search_entry *se = (mdbr_search_entry *)lfirst(lc);

		if (se->relid == relid) {
			*success = true;
			return se;
		}
	}

	return NULL;
}

static void reparse_ri_from_te(TargetEntry *te, Oid relid, List **es)
{
	mdbr_restrict_info *ri = palloc(sizeof(mdbr_restrict_info));
	mdbr_restrict_info_init(ri);
	/* TODO: set errno here */

	mdbr_restrict_info_assing_exprval(ri, te->expr);
	mdbr_restrict_info_assing_col(ri, te->resname);

	bool success;
	mdbr_search_entry *e;
	e = get_se_byrelid(*es, relid, &success);
	if (success) {
		mdbr_se_append_ri(e, ri);
	} else {
		e = mdbr_search_entry_init();
		mdbr_se_set_rel(e, relid);
		mdbr_se_append_ri(e, ri);

		*es = lappend(*es, e);
	}
}

void reparse_from_tl_list(List *tl, Oid relid, List **es)
{
	ListCell *lc;

	foreach(lc, tl)
	{
		TargetEntry *node = lfirst(lc);
		reparse_ri_from_te(node, relid, es);
		/* TODO: check success */
	}
}

#if 0
void reparse_shid_from_opexpr(OpExpr *node, int *shid)
{
	if (node == NULL) {
		return -1;
	}
	// assert opkind is 'b' !!!
	// reparse right child
	reparse_shid_from_tle_constexpr(llast(node->args), shid);
}
#else
void reparse_shid_from_opexpr(OpExpr *node, int *shid)
{
	/* nothing */
}

static void mdbr_reparse_opr(char *oprname, mdbr_ri_ops *dst)
{
	if (strcmp(oprname, "<") == 0 || strcmp(oprname, "<=") == 0) {
		*dst = less_op;
	} else if (strcmp(oprname, "=") == 0) {
		*dst = eq_op;
	} else if (strcmp(oprname, ">") == 0 || strcmp(oprname, ">=") == 0) {
		*dst = gt_op;
	} else {
		elog(ERROR, "unsupported op type");
	}
}

#if 0
void mdbr_restrict_info_assing_exprval(mdbr_restrict_info *ri, Expr *ev)
{
	ri->ev = ev;
}

void mdbr_restrict_info_assing_col(mdbr_restrict_info *ri, char *colname)
{
	ri->colname = strdup(colname);
}
#endif

static void mdbr_reparse_const(Const *node, mdbr_key_t *dst)
{
	Oid typoutput;
	bool typIsVarlena;
	char *extval;

	getTypeOutputInfo(node->consttype, &typoutput, &typIsVarlena);
	extval = OidOutputFunctionCall(typoutput, node->constvalue);

	switch (node->consttype) {
	case INT2OID:
		/* falltrouth */
	case INT4OID:
		/* falltrouth */
	case INT8OID:
		/* falltrouth */
	case OIDOID:
		/* falltrouth */
	case FLOAT4OID:
		/* falltrouth */
	case FLOAT8OID:
		/* falltrouth */
	case NUMERICOID: {
		elog(DEBUG2, " const is %s", extval);
		/*
				 * No need to quote unless it's a special value such as 'NaN'.
				 * See comments in get_const_expr().
				 */
		if (strspn(extval, "0123456789+-eE.") == strlen(extval)) {
			// place here routef somehow (distribute_function)
			char *e;
			*dst = strtoll(extval, &e, 10);
			elog(DEBUG2, "reparsing_util reparsed value %d", *dst);

		} else {
			elog(DEBUG2,
			     "N O T S U P P O R T E D R O U T I G N F E A U T U R E");
		}
	} break;
	default:
		elog(DEBUG2,
		     "N O T S U P P O R T E D R O U T I G N F E A U T U R E");
		break;
	}
}

static void mdbr_reparse_col(Var *node, List *rtbls, char **dst)
{
#if 1
	int varattno = node->varattno;
	int varno = node->varno; // index of range table entry
	RangeTblEntry *rtbl = list_nth(rtbls, varno - 1);
	Index relid = rtbl->relid;

	/* We ARE NOT support fetching the remote side's CTID and OID. */
	if (varattno == SelfItemPointerAttributeNumber) {
		elog(ERROR, "ctid in qry");
	} else if (varattno < 0) {
		/*
		 * All other system attributes are fetched as 0, except for table OID,
		 * which is fetched as the local table OID.  However, we must be
		 * careful; the table could be beneath an outer join, in which case it
		 * must go to NULL whenever the rest of the row does.
		 */
	} else if (varattno == 0) {
		/* Whole row reference */
	} else {
		char *colname = NULL;
		List *options;
		ListCell *lc;

		/* varno must not be any of OUTER_VAR, INNER_VAR and INDEX_VAR. */
		Assert(!IS_SPECIAL_VARNO(varno));

		/*
		 * If it's a column of a foreign table, and it has the column_name FDW
		 * option, use that value.
		 */
		/* TODO:: pass relid here and support  */
		/*
		 * If it's a column of a regular table or it doesn't have column_name
		 * FDW option, use attribute name.
		 */
		// returns pallocd string
		*dst = get_attname(relid, varattno, false);
	}
#endif
}

#define FLEX_SHKEY_OPARATOR 1

static void mdbr_reparse_op(OpExpr *node, List *rtbls, mdbr_search_entry **sel)
{
	char *opname;

	mdbr_restrict_info *ri = palloc(sizeof(mdbr_restrict_info));
	mdbr_restrict_info_init(ri);
	HeapTuple tuple;
	Form_pg_operator form;

	/* Retrieve information about the operator from system catalog. */
	tuple = SearchSysCache1(OPEROID, ObjectIdGetDatum(node->opno));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for operator %u", node->opno);
	form = (Form_pg_operator)GETSTRUCT(tuple);

#if FLEX_SHKEY_OPARATOR
	/* opname is not a SQL identifier, so we should not quote it. */
	opname = NameStr(form->oprname);
	mdbr_reparse_opr(opname, &ri->op);
	elog(DEBUG2, "reparsing opname %s result %d", opname, ri->op);
	ReleaseSysCache(tuple);
#else
	ri->op = eq_op;
#endif

	// TODO: assert we have here exapltly 2 elems
	/* reparse left & right child into dst */
	Expr *exprl = linitial(node->args);

	switch (nodeTag(exprl)) {
	case T_Var: {
		mdbr_reparse_col((Var *)exprl, rtbls, &ri->colname);
	} break;
	default: {
		/* TODO:HANDLE SOMEHOW */
		//elog(ERROR, "too complex qry");
	} break;
	}

	Expr *exprr = llast(node->args);

	switch (nodeTag(exprr)) {
	case T_Const: {
		mdbr_reparse_const((Const *)exprr, &ri->val);
	} break;
	default: {
		/* TODO:HANDLE SOMEHOW */
		//elog(ERROR, "too complex qry");
	} break;
	}

	if (ri->colname) {
		mdbr_se_append_ri(*sel, ri);
	}
}

static void mdbr_reparse_from_node(Node *node, List *rtblEntries,
				   mdbr_search_entry **sel);

static void mdbr_repasre_boolexpr(BoolExpr *node, List *rtblEntries,
				  mdbr_search_entry **sel)
{
	switch (node->boolop) {
	case AND_EXPR: {
		ListCell *lc;
		foreach(lc, node->args)
		{
			Node *curr_node = lfirst(lc);
			mdbr_reparse_from_node(curr_node, rtblEntries, sel);
		}
	} break;
	default: {
		elog(ERROR, "only boolexpr supported on routing");
	} break;
	}
}

static void mdbr_reparse_from_node(Node *node, List *rtblEntries,
				   mdbr_search_entry **sel)
{
	if (node == NULL) {
		// TODO: set errno
		return;
	}

	switch (nodeTag(node)) {
		/* most propable path when reparsing select stmt */
	case T_OpExpr:
		mdbr_reparse_op(node, rtblEntries, sel);
		break;
	case T_BoolExpr:
		mdbr_repasre_boolexpr(node, rtblEntries, sel);
		break;
	default:
		break;
	}
}

void mdbr_reparse_fromExpr(FromExpr *e, List *rtblEntries,
			   mdbr_search_entry **sel)
{
	mdbr_reparse_from_node(e->quals, rtblEntries, sel);
}

static void mdbr_reparse_funcexpr(FuncExpr *fe, mdbr_search_entry **sel,
				  char *colname)
{
	ListCell *lc;

	foreach(lc, fe->args)
	{
		Expr *expr = lfirst(lc);

		switch (nodeTag(expr)) {
		case T_Const: {
			mdbr_restrict_info *ri =
				palloc(sizeof(mdbr_restrict_info));
			;
			mdbr_restrict_info_init(ri);
			mdbr_restrict_info_assing_col(ri, colname);

			mdbr_reparse_const(expr, &ri->val);

			mdbr_se_append_ri(*sel, ri);
		} break;
		default: {
			// something
		} break;
		}
	}
}

static void mdbr_reparse_te_expr(TargetEntry *te, Expr *expr,
				 mdbr_search_entry **sel)
{
	if (te->resname == NULL) {
		return;
	}
	switch (nodeTag(expr)) {
	case T_Const: {
		mdbr_restrict_info *ri = palloc(sizeof(mdbr_restrict_info));
		mdbr_restrict_info_init(ri);
		mdbr_restrict_info_assing_col(ri, te->resname);
		mdbr_reparse_const(expr, &ri->val);

		mdbr_se_append_ri(*sel, ri);
	} break;
	case T_FuncExpr: {
		mdbr_reparse_funcexpr(expr, sel, te->resname);
	} break;
	default: {
		// failed to parse
		// handle somehow
	} break;
	}
}

void mdbr_reparse_targetEntries(List *targetEntries, mdbr_search_entry **sel)
{
	ListCell *lc;
	foreach(lc, targetEntries)
	{
		TargetEntry *te = lfirst(lc);

		mdbr_reparse_te_expr(te, te->expr, sel);
	}
}

static inline void mdbr_repasre_values_entry(List *values, List *targetList,
					     mdbr_search_entry **sel)
{
	ListCell *lc;
	int i = 0;

	foreach(lc, values)
	{
		Expr *e = lfirst(lc);
		TargetEntry *te = list_nth(targetList, i);

		mdbr_reparse_te_expr(te, e, sel);

		++i;
	}
}

static inline void mdbr_reparse_rtevalues(List *values_lists, List *targetList,
					  mdbr_search_entry **sel)
{
	ListCell *lc;
	foreach(lc, values_lists)
	{
		List *values_list = lfirst(lc);
		mdbr_repasre_values_entry(values_list, targetList, sel);
	}
}

void mdbr_reparse_query(Query *q, mdbr_search_entry **sel)
{
	mdbr_reparse_fromExpr(q->jointree, q->rtable, sel);
	mdbr_reparse_targetEntries(q->targetList, sel);

	ListCell *lc;

	foreach(lc, q->rtable)
	{
		RangeTblEntry *rte = lfirst(lc);
		if (rte->subquery) {
			mdbr_reparse_query(rte->subquery, sel);
		}

		if (rte->rtekind == RTE_VALUES) {
			//Assert(q->targetList)
			mdbr_reparse_rtevalues(rte->values_lists, q->targetList,
					       sel);
		}
	}
}
#endif
