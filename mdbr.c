/*
 * =====================================================================================
 *
 *       Filename:  mdbr.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10.02.2021 23:54:00
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
#include "fdw/postgres_fdw.h"
#include "mdbr/mdb_router.h"
#include "planner/planner.h"

PG_MODULE_MAGIC;

void _PG_init(void)
{
	mdbr_init();
	mdbr_planner_init();
}
