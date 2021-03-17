/*
 * =====================================================================================
 *
 *       Filename:  deparse.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  14.01.2021 15:27:19
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef DEPARSE_H
#define DEPARSE_H

#include "src/include/pg.h"
#include "mdbr/mdbr.h"

typedef struct deparse_expr_cxt {
	PlannerInfo *root; /* global planner state */
	RelOptInfo *foreignrel; /* the foreign relation we are planning for */
	RelOptInfo *scanrel; /* the underlying scan relation. Same as
								 * foreignrel, when that represents a join or
								 * a base relation. */
	StringInfo buf; /* output buffer to append to */
	List **params_list; /* exprs that will become remote Params */
#ifdef MDB_ROUTER
	mdbr_shkey *reparsed_key;
	mdbr_route_cb_t routef;
	int mdbr_parsed_shard;
	Expr *curr_topexpr;
	List *search_entry_list; /* enrties to route query */
#endif

} deparse_expr_cxt;

extern void deparse_expr_cxt_init(deparse_expr_cxt *ctx);

#endif
