/*
 * =====================================================================================
 *
 *       Filename:  planner.c
 *
 *    Description:  :x
 *
 *
 *        Version:  1.0
 *        Created:  11.02.2021 00:08:48
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "pg.h"
#include "nodes/extensible.h"
#include "executor/nodeCustom.h"
#include "nodes/execnodes.h"
#include "access/xact.h"
#include "mdbr/mdbr.h"

// as per backend/optimizer/plan/planner.c */
//
#define CP_EXACT_TLIST 0x0001 /* Plan must return specified tlist */
#define CP_SMALL_TLIST 0x0002 /* Prefer narrower tlists */
#define CP_LABEL_TLIST 0x0004 /* tlist must contain sortgrouprefs */
#define CP_IGNORE_TLIST 0x0008 /* caller will replace tlist */

/* Expression kind codes for preprocess_expression */
#define EXPRKIND_QUAL 0
#define EXPRKIND_TARGET 1
#define EXPRKIND_RTFUNC 2
#define EXPRKIND_RTFUNC_LATERAL 3
#define EXPRKIND_VALUES 4
#define EXPRKIND_VALUES_LATERAL 5
#define EXPRKIND_LIMIT 6
#define EXPRKIND_APPINFO 7
#define EXPRKIND_PHV 8
#define EXPRKIND_TABLESAMPLE 9
#define EXPRKIND_ARBITER_ELEM 10
#define EXPRKIND_TABLEFUNC 11
#define EXPRKIND_TABLEFUNC_LATERAL 12

static ForeignScan *create_foreignscan_pplan(PlannerInfo *root,
					     ForeignPath *best_path,
					     List *tlist, List *scan_clauses)
{
	ForeignScan *scan_plan;
	RelOptInfo *rel = best_path->path.parent;
	Index scan_relid = rel->relid;
	Oid rel_oid = InvalidOid;
	Plan *outer_plan = NULL;

	Assert(rel->fdwroutine != NULL);

	/* transform the child path if any */
	//	if (best_path->fdw_outerpath)
	//		outer_plan = create_plan_recurse(root, best_path->fdw_outerpath,
	//										 CP_EXACT_TLIST);

	/*
	 * If we're scanning a base relation, fetch its OID.  (Irrelevant if
	 * scanning a join relation.)
	 */
	if (scan_relid > 0) {
		RangeTblEntry *rte;

		Assert(rel->rtekind == RTE_RELATION);
		rte = planner_rt_fetch(scan_relid, root);
		Assert(rte->rtekind == RTE_RELATION);
		rel_oid = rte->relid;
	}

	/*
                 * Sort clauses into best execution order.  We do this first since the FDW
                 * might have more info than we do and wish to adjust the ordering.
                 */
	//	scan_clauses = order_qual_clauses(root, scan_clauses);

	/*
                 * Let the FDW perform its processing on the restriction clauses and
                 * generate the plan node.  Note that the FDW might remove restriction
                 * clauses that it intends to execute remotely, or even add more (if it
                 * has selected some join clauses for remote use but also wants them
                 * rechecked locally).
                 */
	//	scan_plan = rel->fdwroutine->GetForeignPlan(root, rel, rel_oid,
	//												best_path,
	//												tlist, scan_clauses,
	//												outer_plan);

	/* Copy cost data from Path to Plan; no need to make FDW do this */
	//	copy_generic_path_info(&scan_plan->scan.plan, &best_path->path);

	/* Copy foreign server OID; likewise, no need to make FDW do this */
	//	scan_plan->fs_server = rel->serverid;

	/*
                 * Likewise, copy the relids that are represented by this foreign scan. An
                 * upper rel doesn't have relids set, but it covers all the base relations
                 * participating in the underlying scan, so use root's all_baserels.
                 */
	if (rel->reloptkind == RELOPT_UPPER_REL)
		scan_plan->fs_relids = root->all_baserels;
	else
		scan_plan->fs_relids = best_path->path.parent->relids;

	/*
                 * If this is a foreign join, and to make it valid to push down we had to
                 * assume that the current user is the same as some user explicitly named
                 * in the query, mark the finished plan as depending on the current user.
                 */
	if (rel->useridiscurrent)
		root->glob->dependsOnRole = true;

	/*
                 * Replace any outer-relation variables with nestloop params in the qual,
                 * fdw_exprs and fdw_recheck_quals expressions.  We do this last so that
                 * the FDW doesn't have to be involved.  (Note that parts of fdw_exprs or
                 * fdw_recheck_quals could have come from join clauses, so doing this
                 * beforehand on the scan_clauses wouldn't work.)  We assume
                 * fdw_scan_tlist contains no such variables.
                 */
	//	if (best_path->path.param_info)
	//	{
	//		scan_plan->scan.plan.qual = (List *)
	//			replace_nestloop_params(root, (Node *) scan_plan->scan.plan.qual);
	//		scan_plan->fdw_exprs = (List *)
	//			replace_nestloop_params(root, (Node *) scan_plan->fdw_exprs);
	//		scan_plan->fdw_recheck_quals = (List *)
	//			replace_nestloop_params(root,
	//									(Node *) scan_plan->fdw_recheck_quals);
	//	}

	/*
                 * If rel is a base relation, detect whether any system columns are
                 * requested from the rel.  (If rel is a join relation, rel->relid will be
                 * 0, but there can be no Var with relid 0 in the rel's targetlist or the
                 * restriction clauses, so we skip this in that case.  Note that any such
                 * columns in base relations that were joined are assumed to be contained
                 * in fdw_scan_tlist.)	This is a bit of a kluge and might go away
                 * someday, so we intentionally leave it out of the API presented to FDWs.
                 */
	scan_plan->fsSystemCol = false;
	if (scan_relid > 0) {
		Bitmapset *attrs_used = NULL;
		ListCell *lc;
		int i;

		/*
                         * First, examine all the attributes needed for joins or final output.
                         * Note: we must look at rel's targetlist, not the attr_needed data,
                         * because attr_needed isn't computed for inheritance child rels.
                         */
		pull_varattnos((Node *)rel->reltarget->exprs, scan_relid,
			       &attrs_used);

		/* Add all the attributes used by restriction clauses. */
		foreach(lc, rel->baserestrictinfo)
		{
			RestrictInfo *rinfo = (RestrictInfo *)lfirst(lc);

			pull_varattnos((Node *)rinfo->clause, scan_relid,
				       &attrs_used);
		}

		/* Now, are any system columns requested from rel? */
		for (i = FirstLowInvalidHeapAttributeNumber + 1; i < 0; i++) {
			if (bms_is_member(i - FirstLowInvalidHeapAttributeNumber,
					  attrs_used)) {
				scan_plan->fsSystemCol = true;
				break;
			}
		}

		bms_free(attrs_used);
	}

	return scan_plan;
}

static Plan *create_scan_plan(PlannerInfo *root, Path *best_path, int flags)
{
	RelOptInfo *rel = best_path->parent;
	List *scan_clauses;
	List *gating_clauses;
	List *tlist;
	Plan *plan;

	return plan;
	/*
                 * Extract the relevant restriction clauses from the parent relation. The
                 * executor must apply all these restrictions during the scan, except for
                 * pseudoconstants which we'll take care of below.
                 *
                 * If this is a plain indexscan or index-only scan, we need not consider
                 * restriction clauses that are implied by the index's predicate, so use
                 * indrestrictinfo not baserestrictinfo.  Note that we can't do that for
                 * bitmap indexscans, since there's not necessarily a single index
                 * involved; but it doesn't matter since create_bitmap_scan_plan() will be
                 * able to get rid of such clauses anyway via predicate proof.
                 */
	switch (best_path->pathtype) {
	case T_IndexScan:
	case T_IndexOnlyScan:
		scan_clauses = castNode(IndexPath, best_path)
				       ->indexinfo->indrestrictinfo;
		break;
	default:
		scan_clauses = rel->baserestrictinfo;
		break;
	}

	/*
                 * If this is a parameterized scan, we also need to enforce all the join
                 * clauses available from the outer relation(s).
                 *
                 * For paranoia's sake, don't modify the stored baserestrictinfo list.
                 */
	if (best_path->param_info)
		scan_clauses = list_concat_copy(
			scan_clauses, best_path->param_info->ppi_clauses);

	/*
                 * Detect whether we have any pseudoconstant quals to deal with.  Then, if
                 * we'll need a gating Result node, it will be able to project, so there
                 * are no requirements on the child's tlist.
                 */
	// gating_clauses = get_gating_quals(root, scan_clauses);
	if (gating_clauses)
		flags = 0;

	/*
                 * For table scans, rather than using the relation targetlist (which is
                 * only those Vars actually needed by the query), we prefer to generate a
                 * tlist containing all Vars in order.  This will allow the executor to
                 * optimize away projection of the table tuples, if possible.
                 *
                 * But if the caller is going to ignore our tlist anyway, then don't
                 * bother generating one at all.  We use an exact equality test here, so
                 * that this only applies when CP_IGNORE_TLIST is the only flag set.
                 */
	if (flags == CP_IGNORE_TLIST) {
		tlist = NULL;
	} else if (false) {
	} else {
	}

	switch (best_path->pathtype) {
	case T_ForeignScan:
		plan = (Plan *)create_foreignscan_pplan(
			root, (ForeignPath *)best_path, tlist, scan_clauses);
		break;

	default:
		elog(ERROR, "mdbr: unrecognized node type: %d",
		     (int)best_path->pathtype);
		plan = NULL; /* keep compiler quiet */
		break;
	}

	/*
                 * If there are any pseudoconstant clauses attached to this node, insert a
                 * gating Result node that evaluates the pseudoconstants as one-time
                 * quals.
                 */
	//	if (gating_clauses)
	//		plan = create_gating_plan(root, best_path, plan, gating_clauses);

	return plan;
}

static Plan *create_pplan_recurse(PlannerInfo *root, Path *best_path, int flags)
{
	Plan *plan;

	return plan;

	/* Guard against stack overflow due to overly complex plans */
	check_stack_depth();

	switch (best_path->pathtype) {
	case T_SeqScan:
	case T_SampleScan:
	case T_IndexScan:
	case T_IndexOnlyScan:
	case T_BitmapHeapScan:
	case T_TidScan:
	case T_SubqueryScan:
	case T_FunctionScan:
	case T_TableFuncScan:
	case T_ValuesScan:
	case T_CteScan:
	case T_WorkTableScan:
	case T_NamedTuplestoreScan:
	case T_ForeignScan:
	case T_CustomScan:
		plan = create_scan_plan(root, best_path, flags);
		break;
	case T_HashJoin:
	case T_MergeJoin:
	case T_NestLoop:
	case T_Append:
	case T_MergeAppend:
	case T_Result:
	case T_ProjectSet:
	case T_Material:
	case T_Unique:
	case T_Gather:
	case T_Sort:
	case T_IncrementalSort:
	case T_Group:
	case T_Agg:
	case T_WindowAgg:
	case T_SetOp:
	case T_RecursiveUnion:
	case T_LockRows:
	case T_ModifyTable:
	case T_Limit:
	case T_GatherMerge:
	default:
		elog(ERROR, "mdbr: unrecognized node type: %d",
		     (int)best_path->pathtype);
		plan = NULL; /* keep compiler quiet */
		break;
	}

	return plan;
}

typedef struct mdbr_scan_state {
	CustomScanState csc;

	Query *parse;
	char *qry;

	HeapTuple *tuples;
	int data_iter;
	int numrows;
	int numproceded;
	bool once;
	EState *es;

	int parsed_shard;
} mdbr_scan_state_t;

typedef struct mdbr_global {
	PGconn *conn;
	int parsed_shard;

	bool in_transaction;
	int xactState;
} mdbr_global_t;

static mdbr_global_t *global = NULL;

static inline Oid atrrtype_from_varexpr(Var *v)
{
	return v->vartype;
}

static inline Oid atrrtype_from_constexpr(Const *c)
{
	return c->consttype;
}

static inline Oid oid_from_te(TargetEntry *te)
{
	Oid oid;
	switch (nodeTag(te->expr)) {
	case T_Var: {
		oid = atrrtype_from_varexpr(te->expr);
	} break;
	case T_Const: {
		oid = atrrtype_from_constexpr(te->expr);
	} break;
	default: {
		elog(ERROR, "not a var in target entry");
	} break;
	}

	return oid;
}

static inline int get_tts_attindx_from_var(Var *v, List *rtel,
					   RangeTblEntry **rte)
{
	(*rte) = list_nth(rtel, v->varno - 1);
	return v->varattno - 1;
}

static inline int get_tts_attindx(Expr *e, List *rtel, RangeTblEntry **rte)
{
	switch (nodeTag(e)) {
	case T_Var: {
		return get_tts_attindx_from_var(e, rtel, rte);
	}

	default: {
		elog(ERROR, "failed to get res attype");
	} break;
	}

	return -1;
}

static inline void get_tts_from_aggref(Aggref *agr, List *rtel,
				       TupleDesc *slot_desc)
{
	ListCell *lc;
	foreach(lc, agr->args)
	{
		TargetEntry *te = lfirst(lc);
		if (te->resno) {
			RangeTblEntry *rte;

			int indx = get_tts_attindx(te->expr, rtel, &rte);

			Relation rel;
			rel = table_open(rte->relid, NoLock);

			(*slot_desc)->attrs[te->resno - 1] =
				*TupleDescAttr(RelationGetDescr(rel), indx);

			table_close(rel, NoLock);
		}
	}
}

static inline void get_tts_from_te_expr(Expr *e, List *rtel,
					TupleDesc *slot_desc)
{
	switch (nodeTag(e)) {
	case T_Aggref: {
		get_tts_from_aggref(e, rtel, slot_desc);
	} break;
	default: {
	} break;
	}
}

static inline void get_tts_from_qry(Query *parse, TupleDesc *slot_desc)
{
	switch (parse->commandType) {
	case CMD_INSERT:
		//*  fallthouhtn */
	case CMD_UPDATE: {
		// TODO: reparse returning list here
		if (parse->returningList == NULL) {
			return;
		}
	} break;
	default: {
	} break;
	}

	ListCell *lc;
	foreach(lc, parse->targetList)
	{
		TargetEntry *te = lfirst(lc);

		if (te->resorigtbl) {
			Relation rel;
			rel = table_open(te->resorigtbl, NoLock);

			(*slot_desc)->attrs[te->resno - 1] = *TupleDescAttr(
				RelationGetDescr(rel), te->resorigcol - 1);

			table_close(rel, NoLock);
		}

		get_tts_from_te_expr(te->expr, parse->rtable, slot_desc);
	}

	foreach(lc, parse->rtable)
	{
		RangeTblEntry *rte = lfirst(lc);
		if (rte->subquery) {
			get_tts_from_qry(rte->subquery, slot_desc);
		}
	}

	return;
}

static inline TupleDesc get_qry_td(mdbr_scan_state_t *node)
{
	return node->csc.ss.ps.ps_ResultTupleDesc;
}

static inline int get_ll_non_junk_len(List *te)
{
	int ret = 0;
	ListCell *lc;

	foreach(lc, te)
	{
		TargetEntry *te = lfirst(lc);
		if (!te->resjunk)
			++ret;
	}

	return ret;
}

static char *xact_begin_sql = "BEGIN";
static char *xact_rollback_sql = "ROLLBACK";
static char *xact_commit_sql = "COMMIT";

static bool exec_remote_sql(PGconn *conn, char *sql)
{
	if (!PQsendQuery(conn, sql)) {
		pgfdw_report_error(ERROR, NULL, conn, false, sql);
	}
	// XXX: ultra hack
	PGresult *volatile res = NULL;
	pgfdw_get_result_dst(conn, sql, &res);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		pgfdw_report_error(ERROR, res, conn, true, sql);
	}

	PQclear(res);

	return true;
}

// todo: switch case on second param to begin different iso level transactions
static bool begin_remote_xact(PGconn *conn, int xactIsoLevel)
{
	return exec_remote_sql(conn, xact_begin_sql);
}

static bool rollback_remote_xact(PGconn *conn)
{
	return exec_remote_sql(conn, xact_rollback_sql);
}

static bool commit_remote_xact(PGconn *conn)
{
	return exec_remote_sql(conn, xact_commit_sql);
}

static void mdbr_xact_callback(XactEvent event, void *arg)
{
	elog(DEBUG2, "!!!!!!!!!hiihi!!!!!!!!!%d", event);
	if (global->conn == NULL) {
		elog(DEBUG2, "no remote shard connection opened so skip event");
		return;
	}

	switch (event) {
	case XACT_EVENT_COMMIT:
	case XACT_EVENT_PARALLEL_COMMIT: {
		if (!global->in_transaction) {
			elog(DEBUG1,
			     "do nothing while abort transaction as no qryes proccessed yet");
			break;
		}
		//
		// failed or not, we would not be in transaction anymore
		global->in_transaction = false;
		global->parsed_shard = UNROUTED;
		PGconn *conn = global->conn;
		global->conn = NULL;

		if (!commit_remote_xact(conn)) {
			elog(ERROR, "commit shard transaction failed");
		}
		PQfinish(conn);
	} break;
	case XACT_EVENT_ABORT:
	case XACT_EVENT_PARALLEL_ABORT: {
		if (!global->in_transaction) {
			Assert(global->conn == NULL);
			elog(DEBUG1,
			     "do nothing while abort transaction as no qryes proccessed yet");
			break;
		}

		global->in_transaction = false;
		global->parsed_shard = UNROUTED;
		PGconn *conn = global->conn;
		global->conn = NULL;

		if (!rollback_remote_xact(conn)) {
			elog(ERROR, "close shard transaction faild");
		}
		PQfinish(conn);
	} break;
	}

	return;
}

void beginmdbrscan(mdbr_scan_state_t *node, EState *estate, int eflags)
{
	node->es = estate;
	MemoryContext oldcontext = CurrentMemoryContext;
	// estate->es_processed = 1;

	/*  
                 *      isTopLevel: passed down from ProcessUtility to determine whether we are
         *      inside a function.
         */
	TupleDesc slot = CreateTemplateTupleDesc(
		get_ll_non_junk_len(node->parse->targetList));

	slot->tdtypmod = slot->tdrefcount = -1;
	slot->constr = NULL;

	memset(slot->attrs, 0, slot->natts * sizeof(FormData_pg_attribute));

	get_tts_from_qry(node->parse, &slot);
	node->csc.ss.ps.ps_ResultTupleDesc = slot;

	node->once = false;

	node->data_iter = node->numrows = 0;
	node->tuples = NULL;
	node->parsed_shard = UNROUTED;

	// here we acsually do main part
	mdbr_search_entry *se = mdbr_search_entry_init();

	mdbr_reparse_query(node->parse, &se);

	bool success;
	mdbr_oid_t shoid = mdbr_route_by_se(list_make1(se), &success);
	if (!success) {
		elog(ERROR, "failed to reparse search conditions");
	} else {
		// reparse_shard(context.kr_entry_list);

		// =============================================================================

		elog(NOTICE, "REPARSED SHARD oid %d", shoid);
	}
	// =============================================================================
	//
	node->parsed_shard = shoid;

	Assert(node->parsed_shard != UNROUTED);
	if (global->parsed_shard != UNROUTED &&
	    node->parsed_shard != global->parsed_shard) {
		elog(ERROR,
		     "acquiring connection to 2 different shard inside one transaction is not yet supported");
	}

	if (global->parsed_shard == UNROUTED) {
		global->parsed_shard = node->parsed_shard;
		mdbr_get_shard_connection_dst(node->parsed_shard,
					      &global->conn);
	}
	if (IsInTransactionBlock(false) && !global->in_transaction) {
		// begin was called, start transaction
		begin_remote_xact(global->conn, XactIsoLevel);
		global->in_transaction = true;
	}

	return;
}

#if 1

static bool expr_has_synthretic(Expr *e)
{
	switch (nodeTag(e)) {
	case T_Var: {
		return ((Var *)e)->varnosyn;
	} break;
	default: {
		return false;
	}
	}
}

static bool aggref_has_synthetic_indx(Aggref *aggr)
{
	ListCell *lc;
	foreach(lc, aggr->args)
	{
		TargetEntry *te = lfirst(lc);
		if (expr_has_synthretic(te->expr)) {
			return true;
		}
	}
	return false;
}

static bool has_synthetic_indx(Expr *e)
{
	switch (nodeTag(e)) {
	case T_Aggref: {
		return aggref_has_synthetic_indx(e);
	} break;
	default: {
		return false;
	}
	}
}

static HeapTuple make_tuple_from_result_row(PGresult *res, int row,
					    TupleDesc tupDesc,
					    List *retrieved_attrs,
					    MemoryContext temp_context)
{
	AttInMetadata *attinmeta = TupleDescGetAttInMetadata(tupDesc);
	HeapTuple tuple;
	Datum *values;
	bool *nulls;
	MemoryContext oldcontext;
	ListCell *lc;
	int j;

	Assert(row < PQntuples(res));

	/*
                 * Do the following work in a temp context that we reset after each tuple.
                 * This cleans up not only the data we have direct access to, but any
                 * cruft the I/O functions might leak.
                 */
	oldcontext = MemoryContextSwitchTo(temp_context);

	values = (Datum *)palloc0(tupDesc->natts * sizeof(Datum));
	nulls = (bool *)palloc(tupDesc->natts * sizeof(bool));
	/* Initialize to nulls for any columns not present in result */
	memset(nulls, true, tupDesc->natts * sizeof(bool));

	/*
                 * Set up and install callback to report where conversion error occurs.
                 */
	/*
                 * i indexes columns in the relation, j indexes columns in the PGresult.
                 */
	j = 0;
	foreach(lc, retrieved_attrs)
	{
		TargetEntry *te = lfirst(lc);
		int i = te->resorigcol;
		char *valstr;

		/* fetch next column's textual value */
		if (PQgetisnull(res, row, j))
			valstr = NULL;
		else
			valstr = PQgetvalue(res, row, j);

		/*
		 * convert value to internal representation
		 *
		 * Note: we ignore system columns other than ctid and oid in result
		 */
		if (i > 0 || has_synthetic_indx(te->expr)) {
			/* ordinary column */
			Assert(j <= tupDesc->natts);

			nulls[j] = (valstr == NULL);
			/* Apply the input function even to nulls, to support domains */
			values[j] = InputFunctionCall(&attinmeta->attinfuncs[j],
						      valstr,
						      attinmeta->attioparams[j],
						      attinmeta->atttypmods[j]);
		}

		j++;
	}

	/*
	 * Check we got the expected number of columns.  Note: j == 0 and
	 * PQnfields == 1 is expected, since deparse emits a NULL if no columns.
	 */
	if (j > 0 && j != PQnfields(res) && false) { // FIXME
		// SELECT c_id
		//		 	    FROM customer1
		//			   WHERE c_w_id = 1 AND c_d_id= 9
		//                           AND c_last='BAROUGHTESE' ORDER BY c_first
		//
		elog(ERROR,
		     "remote query result does not match the foreign table");
	}
	/*
	 * Build the result tuple in caller's memory context.
	 */
	MemoryContextSwitchTo(oldcontext);

	tuple = heap_form_tuple(tupDesc, values, nulls);

	/*
	 * Stomp on the xmin, xmax, and cmin fields from the tuple created by
	 * heap_form_tuple.  heap_form_tuple actually creates the tuple with
	 * DatumTupleFields, not HeapTupleFields, but the executor expects
	 * HeapTupleFields and will happily extract system columns on that
	 * assumption.  If we don't do this then, for example, the tuple length
	 * ends up in the xmin field, which isn't what we want.
	 */
	HeapTupleHeaderSetXmax(tuple->t_data, InvalidTransactionId);
	HeapTupleHeaderSetXmin(tuple->t_data, InvalidTransactionId);
	HeapTupleHeaderSetCmin(tuple->t_data, InvalidTransactionId);

	/* Clean up */
	MemoryContextReset(temp_context);

	return tuple;
}

static void fetch_scan_tuples_buff(mdbr_scan_state_t *node)
{
	Query *parse = node->parse;
	RangeTblEntry *rte =
		(RangeTblEntry *)parse->rtable->elements[0].ptr_value;
	PGresult *volatile res = NULL;
	MemoryContext oldcontext;

	/*
	 * We'll store the tuples in the batch_cxt.  First, flush the previous
	 * batch.
	 */
	PGconn *conn = global->conn;
	char *qry = node->qry;
	//MemoryContextReset(fsstate->batch_cxt);
	//oldcontext = MemoryContextSwitchTo(fsstate->batch_cxt);

	/* PGresult must be released before leaving this function. */
	PG_TRY();
	{
		int numrows;
		int i;

		if (!PQsendQuery(conn, qry)) {
			pgfdw_report_error(ERROR, NULL, conn, false, qry);
		}

		pgfdw_get_result_dst(conn, qry, &res);

		/* On error, report the original query, not the FETCH. */
		switch (PQresultStatus(res)) {
		case PGRES_TUPLES_OK: {
			//		Assert(parse->commandType == CMD_SELECT); * or has returning*
		} break;
		case PGRES_COMMAND_OK: {
			//		Assert(parse->commandType == CMD_INSERT ||
			//		       parse->commandType == CMD_UPDATE ||
			//		       parse->commandType == CMD_DELETE);* or has returning*
		} break;
		default: {
			pgfdw_report_error(ERROR, res, conn, false, qry);
		} break;
		}

		/* Convert the data into HeapTuples */
		numrows = PQntuples(res);
		node->numrows = numrows;

		if (numrows) {
			node->tuples = (HeapTuple *)palloc0(numrows *
							    sizeof(HeapTuple));

			MemoryContext temp_cxt = AllocSetContextCreate(
				CurrentMemoryContext,
				"postgres_fdw temporary data",
				ALLOCSET_SMALL_SIZES);

			List *retrieved_attrs = parse->targetList;

			for (i = 0; i < numrows; i++) {
				Assert(IsA(node->csc.ss.ps.plan, CustomScan));

				node->tuples[i] = make_tuple_from_result_row(
					res, i, get_qry_td(node),
					retrieved_attrs, temp_cxt);
			}

		} else {
			node->tuples = NULL;
		}
		/* Must be EOF if we didn't get as many tuples as we asked for. */
	}
	PG_FINALLY();
	{
		if (res)
			PQclear(res);
	}
	PG_END_TRY();

	/* close the connection to the database and cleanup */
	if (!global->in_transaction) {
		PQfinish(conn);
	}
	//	MemoryContextSwitchTo(oldcontext);

	return;
}
#endif

TupleTableSlot *mdbr_exec(mdbr_scan_state_t *node)
{
#if 1
	List *rtel = node->parse->rtable;

	TupleDesc tupDesc = get_qry_td(node);

	if (!node->once) {
		node->once = true;

		// here we connect to shard
		fetch_scan_tuples_buff(node);
		// TODO XXX FIXME
		switch (node->parse->commandType) {
		case CMD_INSERT:
		case CMD_UPDATE:
		case CMD_DELETE: {
			node->es->es_processed = 1;
		} break;
		}
	}

	if (node->data_iter == node->numrows) {
		return NULL;
	}

	TupleTableSlot *slot;
	slot = MakeSingleTupleTableSlot(tupDesc, &TTSOpsHeapTuple);
	//      slot->tts_tupleDescriptor = tupDesc;

	ExecStoreHeapTuple(node->tuples[node->data_iter++], slot, true);

	return slot;
#else
	return NULL;
#endif
}

void mdbr_end_scan(CustomScanState *node)
{
}

static CustomExecMethods mdbr_mt = {
	.BeginCustomScan = beginmdbrscan,
	.ExecCustomScan = mdbr_exec,
	.EndCustomScan = mdbr_end_scan,
};

static Node *_create_csc(CustomScan *cscan)
{
	mdbr_scan_state_t *ret;

	ret = palloc0(sizeof(mdbr_scan_state_t));

	ret->csc.ss.ps.type = T_CustomScanState;
	ret->parse = list_nth(cscan->custom_private, 0);
	ret->qry = list_nth(cscan->custom_private, 1);
	ret->csc.methods = &mdbr_mt;

	return ret;
}

CustomScanMethods *bind_meethods()
{
	CustomScanMethods *ret = palloc0(sizeof(CustomScanMethods));

	ret->CreateCustomScanState = _create_csc;
}

typedef struct mdbr_private {
	Query *parse;
	char *qry;
} mdbr_private_t;

Plan *create_pplan(PlannerInfo *root, Path *best_path, Query *parse, char *qry)
{
	CustomScan *cs;

	cs = palloc0(sizeof(CustomScan));

	// cs->scan.plan.plan_params = NIL;
	// hack
	cs->scan.plan.type = T_CustomScan;
	cs->scan.scanrelid = 0;
	cs->custom_private = list_make2(parse, strdup(qry));
	cs->methods = bind_meethods();

	// end hack
	//
	return cs;
}

PlannerInfo *subqry_planner(PlannerGlobal *glob, Query *parse,
			    PlannerInfo *parent_root, bool hasRecursion,
			    double tuple_fraction)
{
	PlannerInfo *root;
	List *newWithCheckOptions;
	List *newHaving;
	bool hasOuterJoins;
	bool hasResultRTEs;
	RelOptInfo *final_rel;
	ListCell *l;

	/* Create a PlannerInfo data structure for this subquery */
	root = makeNode(PlannerInfo);
	root->parse = parse;
	root->glob = glob;
	root->query_level = parent_root ? parent_root->query_level + 1 : 1;
	root->parent_root = parent_root;
	root->plan_params = NIL;
	root->outer_params = NULL;
	root->planner_cxt = CurrentMemoryContext;
	root->init_plans = NIL;
	root->cte_plan_ids = NIL;
	root->multiexpr_params = NIL;
	root->eq_classes = NIL;
	root->ec_merging_done = false;
	root->append_rel_list = NIL;
	root->rowMarks = NIL;
	memset(root->upper_rels, 0, sizeof(root->upper_rels));
	memset(root->upper_targets, 0, sizeof(root->upper_targets));
	root->processed_tlist = NIL;
	root->grouping_map = NULL;
	root->minmax_aggs = NIL;
	root->qual_security_level = 0;
	root->inhTargetKind = INHKIND_NONE;
	root->hasPseudoConstantQuals = false;
#if PG_VERSION_NUM >= 140000
	root->hasAlternativeSubPlans = false;
#endif
	root->hasRecursion = hasRecursion;
	if (hasRecursion)
		root->wt_param_id = assign_special_exec_param(root);
	else
		root->wt_param_id = -1;
	root->non_recursive_path = NULL;
	root->partColsUpdated = false;

	/*
	 * If there is a WITH list, process each WITH query and either convert it
	 * to RTE_SUBQUERY RTE(s) or build an initplan SubPlan structure for it.
	 */
	if (parse->cteList)
		SS_process_ctes(root);

	/*
	 * If the FROM clause is empty, replace it with a dummy RTE_RESULT RTE, so
	 * that we don't need so many special cases to deal with that situation.
	 */
	replace_empty_jointree(parse);

	/*
	 * Look for ANY and EXISTS SubLinks in WHERE and JOIN/ON clauses, and try
	 * to transform them into joins.  Note that this step does not descend
	 * into subqueries; if we pull up any subqueries below, their SubLinks are
	 * processed just before pulling them up.
	 */

	/*
	 * Scan the rangetable for function RTEs, do const-simplification on them,
	 * and then inline them if possible (producing subqueries that might get
	 * pulled up next).  Recursion issues here are handled in the same way as
	 * for SubLinks.
	 */

	/*
	 * Check to see if any subqueries in the jointree can be merged into this
	 * query.
	 */
	//pull_up_subqueries(root);

	/*
	 * If this is a simple UNION ALL query, flatten it into an appendrel. We
	 * do this now because it requires applying pull_up_subqueries to the leaf
	 * queries of the UNION ALL, which weren't touched above because they
	 * weren't referenced by the jointree (they will be after we do this).
	 */
	if (parse->setOperations)
		flatten_simple_union_all(root);

	/*
	 * Survey the rangetable to see what kinds of entries are present.  We can
	 * skip some later processing if relevant SQL features are not used; for
	 * example if there are no JOIN RTEs we can avoid the expense of doing
	 * flatten_join_alias_vars().  This must be done after we have finished
	 * adding rangetable entries, of course.  (Note: actually, processing of
	 * inherited or partitioned rels can cause RTEs for their child tables to
	 * get added later; but those must all be RTE_RELATION entries, so they
	 * don't invalidate the conclusions drawn here.)
	 */
	root->hasJoinRTEs = false;
	root->hasLateralRTEs = false;
	hasOuterJoins = false;
	hasResultRTEs = false;
	foreach(l, parse->rtable)
	{
		RangeTblEntry *rte = lfirst_node(RangeTblEntry, l);

		switch (rte->rtekind) {
		case RTE_RELATION:
			if (rte->inh) {
				/*
					 * Check to see if the relation actually has any children;
					 * if not, clear the inh flag so we can treat it as a
					 * plain base relation.
					 *
					 * Note: this could give a false-positive result, if the
					 * rel once had children but no longer does.  We used to
					 * be able to clear rte->inh later on when we discovered
					 * that, but no more; we have to handle such cases as
					 * full-fledged inheritance.
					 */
				rte->inh = has_subclass(rte->relid);
			}
			break;
		case RTE_JOIN:
			root->hasJoinRTEs = true;
			if (IS_OUTER_JOIN(rte->jointype))
				hasOuterJoins = true;
			break;
		case RTE_RESULT:
			hasResultRTEs = true;
			break;
		default:
			/* No work here for other RTE types */
			break;
		}

		if (rte->lateral)
			root->hasLateralRTEs = true;

		/*
		 * We can also determine the maximum security level required for any
		 * securityQuals now.  Addition of inheritance-child RTEs won't affect
		 * this, because child tables don't have their own securityQuals; see
		 * expand_single_inheritance_child().
		 */
	}

	/*
	 * Preprocess RowMark information.  We need to do this after subquery
	 * pullup, so that all base relations are present.
	 */

	// preprocess_rowmarks(root);

	/*
	 * Set hasHavingQual to remember if HAVING clause is present.  Needed
	 * because preprocess_expression will reduce a constant-true condition to
	 * an empty qual list ... but "HAVING TRUE" is not a semantic no-op.
	 */

	/*
	 * Do expression preprocessing on targetlist and quals, as well as other
	 * random expressions in the querytree.  Note that we do not need to
	 * handle sort/group expressions explicitly, because they are actually
	 * part of the targetlist.
	 */

	/* Constant-folding might have removed all set-returning functions */

	// preprocess_qual_conditions(root, (Node *)parse->jointree);

	foreach(l, parse->windowClause)
	{
		WindowClause *wc = lfirst_node(WindowClause, l);

		/* partitionClause/orderClause are sort/group expressions */
	}

	if (parse->onConflict) {
		/* exclRelTlist contains only Vars, so no preprocessing needed */
	}

	/* Also need to preprocess expressions within RTEs */
	foreach(l, parse->rtable)
	{
		RangeTblEntry *rte = lfirst_node(RangeTblEntry, l);
		int kind;
		ListCell *lcsq;

		if (rte->rtekind == RTE_RELATION) {
		} else if (rte->rtekind == RTE_SUBQUERY) {
			/*
			 * We don't want to do all preprocessing yet on the subquery's
			 * expressions, since that will happen when we plan it.  But if it
			 * contains any join aliases of our level, those have to get
			 * expanded now, because planning of the subquery won't do it.
			 * That's only possible if the subquery is LATERAL.
			 */
		} else if (rte->rtekind == RTE_FUNCTION) {
			/* Preprocess the function expression(s) fully */
		} else if (rte->rtekind == RTE_TABLEFUNC) {
			/* Preprocess the function expression(s) fully */
		} else if (rte->rtekind == RTE_VALUES) {
			/* Preprocess the values lists fully */
		}

		/*
		 * Process each element of the securityQuals list as if it were a
		 * separate qual expression (as indeed it is).  We need to do it this
		 * way to get proper canonicalization of AND/OR structure.  Note that
		 * this converts each element into an implicit-AND sublist.
		 */
		//	foreach(lcsq, rte->securityQuals)
		//	{
		//		lfirst(lcsq) = preprocess_expression(
		//			root, (Node *)lfirst(lcsq), EXPRKIND_QUAL);
		//	}
	}

	/*
	 * Now that we are done preprocessing expressions, and in particular done
	 * flattening join alias variables, get rid of the joinaliasvars lists.
	 * They no longer match what expressions in the rest of the tree look
	 * like, because we have not preprocessed expressions in those lists (and
	 * do not want to; for example, expanding a SubLink there would result in
	 * a useless unreferenced subplan).  Leaving them in place simply creates
	 * a hazard for later scans of the tree.  We could try to prevent that by
	 * using QTW_IGNORE_JOINALIASES in every tree scan done after this point,
	 * but that doesn't sound very reliable.
	 */
	//	if (root->hasJoinRTEs) {
	//		foreach(l, parse->rtable)
	//		{
	//			RangeTblEntry *rte = lfirst_node(RangeTblEntry, l);
	//
	//			rte->joinaliasvars = NIL;
	//		}
	//	}

	/*
	 * In some cases we may want to transfer a HAVING clause into WHERE. We
	 * cannot do so if the HAVING clause contains aggregates (obviously) or
	 * volatile functions (since a HAVING clause is supposed to be executed
	 * only once per group).  We also can't do this if there are any nonempty
	 * grouping sets; moving such a clause into WHERE would potentially change
	 * the results, if any referenced column isn't present in all the grouping
	 * sets.  (If there are only empty grouping sets, then the HAVING clause
	 * must be degenerate as discussed below.)
	 *
	 * Also, it may be that the clause is so expensive to execute that we're
	 * better off doing it only once per group, despite the loss of
	 * selectivity.  This is hard to estimate short of doing the entire
	 * planning process twice, so we use a heuristic: clauses containing
	 * subplans are left in HAVING.  Otherwise, we move or copy the HAVING
	 * clause into WHERE, in hopes of eliminating tuples before aggregation
	 * instead of after.
	 *
	 * If the query has explicit grouping then we can simply move such a
	 * clause into WHERE; any group that fails the clause will not be in the
	 * output because none of its tuples will reach the grouping or
	 * aggregation stage.  Otherwise we must have a degenerate (variable-free)
	 * HAVING clause, which we put in WHERE so that query_planner() can use it
	 * in a gating Result node, but also keep in HAVING to ensure that we
	 * don't emit a bogus aggregated row. (This could be done better, but it
	 * seems not worth optimizing.)
	 *
	 * Note that both havingQual and parse->jointree->quals are in
	 * implicitly-ANDed-list form at this point, even though they are declared
	 * as Node *.
	 */
	//newHaving = NIL;
	//	foreach(l, (List *)parse->havingQual)
	//	{
	//		Node *havingclause = (Node *)lfirst(l);
	//	}
	//	parse->havingQual = (Node *)newHaving;

	/* Remove any redundant GROUP BY columns */
	// remove_useless_groupby_columns(root);

	/*
	 * If we have any outer joins, try to reduce them to plain inner joins.
	 * This step is most easily done after we've done expression
	 * preprocessing.
	 */

	/*
	 * If we have any RTE_RESULT relations, see if they can be deleted from
	 * the jointree.  This step is most effectively done after we've done
	 * expression preprocessing and outer join reduction.
	 */

	/*
	 * Do the main planning.  If we have an inherited target relation, that
	 * needs special processing, else go straight to grouping_planner.
	 */

	/*
	 * Capture the set of outer-level param IDs we have access to, for use in
	 * extParam/allParam calculations later.
	 */
	SS_identify_outer_params(root);

	/*
	 * If any initPlans were created in this query level, adjust the surviving
	 * Paths' costs and parallel-safety flags to account for them.  The
	 * initPlans won't actually get attached to the plan tree till
	 * create_pplan() runs, but we must include their effects now.
	 */
	final_rel = fetch_upper_rel(root, UPPERREL_FINAL, NULL);
	SS_charge_for_initplans(root, final_rel);

	/*
	 * Make sure we've identified the cheapest Path for the final rel.  (By
	 * doing this here not in grouping_planner, we include initPlan costs in
	 * the decision, though it's unlikely that will change anything.)
	 */
	//	set_cheapest(final_rel);

	return root;
}

PlannedStmt *pplanner(Query *parse, const char *query_string, int cursorOptions,
		      ParamListInfo boundParams)
{
	PlannedStmt *result;
	PlannerGlobal *glob;
	double tuple_fraction;
	PlannerInfo *root;
	RelOptInfo *final_rel;
	Path *best_path;
	Plan *top_plan;
	ListCell *lp, *lr;

	/*
	 * Set up global state for this planner invocation.  This data is needed
	 * across all levels of sub-Query that might exist in the given command,
	 * so we keep it in a separate struct that's linked to by each per-Query
	 * PlannerInfo.
	 */
	glob = makeNode(PlannerGlobal);

	glob->boundParams = boundParams;
	glob->subplans = NIL;
	glob->subroots = NIL;
	glob->rewindPlanIDs = NULL;
	glob->finalrtable = NIL;
	glob->finalrowmarks = NIL;
	glob->resultRelations = NIL;
	glob->appendRelations = NIL;
	glob->relationOids = NIL;
	glob->invalItems = NIL;
	glob->paramExecTypes = NIL;
	glob->lastPHId = 0;
	glob->lastRowMarkId = 0;
	glob->lastPlanNodeId = 0;
	glob->transientPlan = false;
	glob->dependsOnRole = false;

	/*
	 * Assess whether it's feasible to use parallel mode for this query. We
	 * can't do this in a standalone backend, or if the command will try to
	 * modify any data, or if this is a cursor operation, or if GUCs are set
	 * to values that don't permit parallelism, or if parallel-unsafe
	 * functions are present in the query tree 
         * or true
	 *
	 * (Note that we do allow CREATE TABLE AS, SELECT INTO, and CREATE
	 * MATERIALIZED VIEW to use parallel plans, but as of now, only the leader
	 * backend writes into a completely new table.  In the future, we can
	 * extend it to allow workers to write into the table.  However, to allow
	 * parallel updates and deletes, we have to solve other problems,
	 * especially around combo CIDs.)
	 *
	 * For now, we don't try to use parallel mode if we're running inside a
	 * parallel worker.  We might eventually be able to relax this
	 * restriction, but for now it seems best not to have parallel workers
	 * trying to create their own parallel workers.
	 */
	if ((cursorOptions & CURSOR_OPT_PARALLEL_OK) != 0 &&
	    IsUnderPostmaster && parse->commandType == CMD_SELECT &&
	    !parse->hasModifyingCTE && max_parallel_workers_per_gather > 0 &&
	    !IsParallelWorker() && false) {
		/* all the cheap tests pass, so scan the query tree */
	} else {
		/* skip the query tree scan, just assume it's unsafe */
		glob->maxParallelHazard = PROPARALLEL_UNSAFE;
		glob->parallelModeOK = false;
	}

	/*
	 * glob->parallelModeNeeded is normally set to false here and changed to
	 * true during plan creation if a Gather or Gather Merge plan is actually
	 * created (cf. create_gather_plan, create_gather_merge_plan).
	 *
	 * However, if force_parallel_mode = on or force_parallel_mode = regress,
	 * then we impose parallel mode whenever it's safe to do so, even if the
	 * final plan doesn't use parallelism.  It's not safe to do so if the
	 * query contains anything parallel-unsafe; parallelModeOK will be false
	 * in that case.  Note that parallelModeOK can't change after this point.
	 * Otherwise, everything in the query is either parallel-safe or
	 * parallel-restricted, and in either case it should be OK to impose
	 * parallel-mode restrictions.  If that ends up breaking something, then
	 * either some function the user included in the query is incorrectly
	 * labeled as parallel-safe or parallel-restricted when in reality it's
	 * parallel-unsafe, or else the query planner itself has a bug.
	 */
	glob->parallelModeNeeded = false;

	/* Determine what fraction of the plan is likely to be scanned */
	if (cursorOptions & CURSOR_OPT_FAST_PLAN) {
	} else {
		/* Default assumption is we need all the tuples 
                 * yes we need them all */
		tuple_fraction = 0.0;
	}

	/* primary planning entry point (may recurse for subqueries) */
	root = subqry_planner(glob, parse, NULL, false, tuple_fraction);

	/* Select best Path and turn it into a Plan */
	final_rel = fetch_upper_rel(root, UPPERREL_FINAL, NULL);
	best_path = get_cheapest_fractional_path(final_rel, tuple_fraction);

	top_plan = create_pplan(root, best_path, parse, query_string);

	/*
	 * If creating a plan for a scrollable cursor, make sure it can run
	 * backwards on demand.  Add a Material node at the top at need.
	 */
	if (cursorOptions & CURSOR_OPT_SCROLL) {
		if (!ExecSupportsBackwardScan(top_plan))
			top_plan = materialize_finished_plan(top_plan);
	}

	/*
	 * Optionally add a Gather node for testing purposes, provided this is
	 * actually a safe thing to do.
	 */
	if (force_parallel_mode != FORCE_PARALLEL_OFF &&
	    top_plan->parallel_safe) {
		/* never */
	}

	/*
	 * If any Params were generated, run through the plan tree and compute
	 * each plan node's extParam/allParam sets.  Ideally we'd merge this into
	 * set_plan_references' tree traversal, but for now it has to be separate
	 * because we need to visit subplans before not after main plan.
	 */
	if (glob->paramExecTypes != NIL) {
		Assert(list_length(glob->subplans) ==
		       list_length(glob->subroots));
		forboth(lp, glob->subplans, lr, glob->subroots)
		{
			Plan *subplan = (Plan *)lfirst(lp);
			PlannerInfo *subroot = lfirst_node(PlannerInfo, lr);

			SS_finalize_plan(subroot, subplan);
		}
		SS_finalize_plan(root, top_plan);
	}

	/* final cleanup of the plan */
	Assert(glob->finalrtable == NIL);
	Assert(glob->finalrowmarks == NIL);
	Assert(glob->resultRelations == NIL);
	Assert(glob->appendRelations == NIL);
	//top_plan = set_plan_references(root, top_plan);
	/* ... and the subplans (both regular subplans and initplans) */
	Assert(list_length(glob->subplans) == list_length(glob->subroots));
	forboth(lp, glob->subplans, lr, glob->subroots)
	{
		Plan *subplan = (Plan *)lfirst(lp);
		PlannerInfo *subroot = lfirst_node(PlannerInfo, lr);

		lfirst(lp) = set_plan_references(subroot, subplan);
	}

	/* build the PlannedStmt result */
	result = makeNode(PlannedStmt);

	result->commandType = parse->commandType;
	result->queryId = parse->queryId;
	result->hasReturning = (parse->returningList != NIL);
	result->hasModifyingCTE = parse->hasModifyingCTE;
	result->canSetTag = parse->canSetTag;
	result->transientPlan = glob->transientPlan;
	result->dependsOnRole = glob->dependsOnRole;
	result->parallelModeNeeded = glob->parallelModeNeeded;
	result->planTree = top_plan;
	result->rtable = glob->finalrtable;
	result->resultRelations = glob->resultRelations;
	result->appendRelations = glob->appendRelations;
	result->subplans = glob->subplans;
	result->rewindPlanIDs = glob->rewindPlanIDs;
	result->rowMarks = glob->finalrowmarks;
	result->relationOids = glob->relationOids;
	result->invalItems = glob->invalItems;
	result->paramExecTypes = glob->paramExecTypes;
	/* utilityStmt should be null, but we might as well copy it */
	result->utilityStmt = parse->utilityStmt;
	result->stmt_location = parse->stmt_location;
	result->stmt_len = parse->stmt_len;

	// result->jitFlags = PGJIT_NONE;
	{
		/* TODO: handle JIT */
	}

	if (glob->partition_directory != NULL)
		DestroyPartitionDirectory(glob->partition_directory);

	return result;
}

static bool qry_local_tables(Query *parse)
{
	ListCell *lc;

	foreach(lc, parse->rtable)
	{
		RangeTblEntry *rte = lfirst(lc);

		if (!rte->relid)
			continue;

		Relation rel = table_open(rte->relid, NoLock);
		char *relname = RelationGetRelationName(rel);
		table_close(rel, NoLock);
		if (!is_local_table(relname)) {
			return false;
		}
	}

	return true;
}

static bool qry_local_executable(Query *parse)
{
	if (qry_local_tables(parse)) {
		return true;
	}

	// admin f call
	if (parse->jointree->quals != NULL ||
	    parse->jointree->fromlist != NULL ||
	    parse->commandType != CMD_SELECT) {
		return false;
	}

	ListCell *lc;

	foreach(lc, parse->targetList)
	{
		TargetEntry *te = lfirst(lc);
		switch (nodeTag(te->expr)) {
		case T_FuncExpr:
			break;
		default: {
			return false;
		}
		}
	}

	return true;
}

static void eject_not_matter(Query *parse)
{
}

static PlannedStmt *shard_query_pushdown_planner(Query *parse,
						 const char *query_string,
						 int cursorOptions,
						 ParamListInfo boundParams)
{
	//  very dirty hack for admin mgmnt quieries thashould be executed locally
	//  like select mdbr_create_shard()
	if (qry_local_executable(parse)) {
		PlannedStmt *local_res = standard_planner(
			parse, query_string, cursorOptions, boundParams);
		return local_res;
	}

	// ====================================================
	// XXX : dirty hack, TODO FIXME
	eject_not_matter(parse);
	// ====================================================
	PlannedStmt *result =
		pplanner(parse, query_string, cursorOptions, boundParams);

	switch (result->commandType) {
	case CMD_INSERT:
	case CMD_UPDATE:
	case CMD_DELETE: {
		result->planTree->plan_rows = 1;
	} break;
	}

	return result;
}

void mdbrExecutorRun(QueryDesc *queryDesc, ScanDirection direction,
		     uint64 count, bool execute_once);

void mdbr_planner_init()
{
	ExecutorRun_hook = ExecutorRun_hook;
	planner_hook = shard_query_pushdown_planner;

	global = malloc(sizeof(mdbr_global_t));

	global->in_transaction = false;
	global->conn = NULL;
	global->parsed_shard = UNROUTED;

	RegisterXactCallback(mdbr_xact_callback, &global);
}
