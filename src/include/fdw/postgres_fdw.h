/*-------------------------------------------------------------------------
 *
 * postgres_fdw.h
 *		  Foreign-data wrapper for remote PostgreSQL servers
 *
 * Portions Copyright (c) 2012-2020, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		  contrib/postgres_fdw/postgres_fdw.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef POSTGRES_FDW_H
#define POSTGRES_FDW_H

#include "pg.h"

// ==========================================================================

#include "mdbr/mdbr.h"
#include "fdw/deparse.h"

// ==========================================================================

/*
 * FDW-specific planner information kept in RelOptInfo.fdw_private for a
 * postgres_fdw foreign table.  For a baserel, this struct is created by
 * postgresGetForeignRelSize, although some fields are not filled till later.
 * postgresGetForeignJoinPaths creates it for a joinrel, and
 * postgresGetForeignUpperPaths creates it for an upperrel.
 */

/* in connection.c */
PGconn *GetConnection(UserMapping *user, bool will_prep_stmt);
void ReleaseConnection(PGconn *conn);
unsigned int GetCursorNumber(PGconn *conn);
unsigned int GetPrepStmtNumber(PGconn *conn);
PGresult *pgfdw_get_result(PGconn *conn, const char *query);
PGresult *pgfdw_exec_query(PGconn *conn, const char *query);
void pgfdw_report_error(int elevel, PGresult *res, PGconn *conn, bool clear,
			const char *sql);


#endif /* POSTGRES_FDW_H */
