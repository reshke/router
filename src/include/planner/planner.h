
#ifndef MDBR_PLANNER_H
#define MDBR_PLANNER_H

#include "pg.h"

extern PlannedStmt *shard_query_pushdown_planner(Query *parse,
						 const char *query_string,
						 int cursorOptions,
						 ParamListInfo boundParams);

extern void mdbr_planner_init();

#endif /* MDBR_PLANNER_H */
