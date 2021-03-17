/*
 * =====================================================================================
 *
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>

#include "mdbr/search_entry.h"

#include "src/include/pg.h"

static void mdbr_se_reparse_const(Const *node, mdbr_key_t *dst)
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

static void mdbr_reparse_col(Var *node, Index relid, char **dst)
{
#if 0
        int varattno = node->varattno;
        int varno = node->varno;

	/* We support fetching the remote side's CTID and OID. */
	if (varattno == SelfItemPointerAttributeNumber) {
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
#if 0
                options = GetForeignColumnOptions(rte->relid, varattno);
		foreach(lc, options)
		{
			DefElem *def = (DefElem *)lfirst(lc);

			if (strcmp(def->defname, "column_name") == 0) {
				colname = defGetString(def);
				break;
			}
		}
#endif
		/*
		 * If it's a column of a regular table or it doesn't have column_name
		 * FDW option, use attribute name.
		 */
		// returns pallocd string
		*dst = get_attname(relid, varattno, false);
	}
#endif
}

static void mdbr_reparse_opr(char *oprname, mdbr_ri_ops *dst)
{
	if (strcmp(oprname, "<") == 0 || oprname, "<=" == 0) {
		*dst = less_op;
	} else if (strcmp(oprname, "=") == 0) {
		*dst = eq_op;
	} else if (strcmp(oprname, ">") == 0 || strcmp(oprname, ">=") == 0) {
		*dst = gt_op;
	} else {
		elog(ERROR, "unsupported op type");
	}
}

static void eval_op_cond(OpExpr *node, Index relid, mdbr_restrict_info *dst)
{
	char *opname;

	HeapTuple tuple;
	Form_pg_operator form;

	/* Retrieve information about the operator from system catalog. */
	tuple = SearchSysCache1(OPEROID, ObjectIdGetDatum(node->opno));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for operator %u", node->opno);
	form = (Form_pg_operator)GETSTRUCT(tuple);

	/* opname is not a SQL identifier, so we should not quote it. */
	opname = NameStr(form->oprname);
	mdbr_reparse_opr(opname, &dst->op);
	elog(DEBUG2, "reparsing opname %s result %d", opname, dst->op);

	// TODO: assert we have here exapltly 2 elems
	/* reparse left & right child into dst */
	Expr *exprl = linitial(node->args);

	switch (nodeTag(exprl)) {
	case T_Var: {
		mdbr_reparse_col((Var *)exprl, relid, &dst->colname);
	} break;
	default: {
		/* TODO:HANDLE SOMEHOW */
		//elog(ERROR, "too complex qry");
	} break;
	}

	Expr *exprr = llast(node->args);

	switch (nodeTag(exprr)) {
	case T_Const: {
		mdbr_se_reparse_const((Const *)exprr, &dst->val);
	} break;
	default: {
		/* TODO:HANDLE SOMEHOW */
		//elog(ERROR, "too complex qry");
	} break;
	}
}

void mdbr_restrict_info_assing_exprval(mdbr_restrict_info *ri, Expr *ev)
{
	ri->ev = ev;
}

void mdbr_restrict_info_assing_col(mdbr_restrict_info *ri, char *colname)
{
	ri->colname = strdup(colname);
}

static void eval_const(Const *node, mdbr_restrict_info *ri)
{
	mdbr_se_reparse_const(node, &ri->val);
}

static void eval_cond(mdbr_restrict_info *ri, Index relid)
{
	Expr *node = ri->ev;

	if (node == NULL) {
		// TODO: set errno
		return;
	}

	switch (nodeTag(node)) {
		/* most propable path when reparsing select stmt */
	case T_OpExpr:
		eval_op_cond(node, relid, ri);
		break;
	case T_Const:
		eval_const(node, ri);
		break;
	default:
		break;
	}
}

void mdbr_restrict_info_eval(mdbr_restrict_info *ri, Index relid)
{
	if (ri->evaluated) {
		return;
	}
	eval_cond(ri, relid);
	ri->evaluated = true;
}

void mdbr_restrict_info_free(mdbr_restrict_info *ri)
{
	mdbr_list_t *it;
	mdbr_list_foreach(&ri->l, it)
	{
		mdbr_restrict_info *ri =
			mdbr_container_of(it, mdbr_restrict_info, l);
	}
}

mdbr_retcode_t mdbr_restrict_info_init(mdbr_restrict_info *ri)
{
	mdbr_list_init(&ri->l);
	ri->evaluated = false;
	ri->colname = NULL;
	ri->ev = NULL;

	return MDBR_RETCODE_OK;
}

mdbr_search_entry *mdbr_search_entry_init()
{
	mdbr_search_entry *e = palloc(sizeof(mdbr_search_entry));

	mdbr_list_init(&e->ri_list);
	return e;
}

// save to shmem somehow
void mdbr_search_entry_free(mdbr_search_entry *e)
{
	// TODO: impl
	//mdbr_restrict_info_free(e->ri_list);

	pfree(e);
}

mdbr_search_entry *mdbr_se_find(List *l /**/,
				Index relid
				/* se selector */,
				bool *found)
{
	ListCell *lc;

	foreach(lc, l)
	{
		mdbr_search_entry *se = (mdbr_search_entry *)lfirst(lc);

		if (se->relid == relid) {
			*found = true;
			return se;
		}
	}

	*found = false;

	return NULL;
}

void mdbr_se_set_rel(mdbr_search_entry *e, Oid relid)
{
	e->relid = relid;
}

mdbr_retcode_t mdbr_se_append_ri(mdbr_search_entry *se, mdbr_restrict_info *ri)
{
	mdbr_list_append(&se->ri_list, &ri->l);
}
