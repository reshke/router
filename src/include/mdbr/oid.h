#ifndef MDBR_OID_H
#define MDBR_OID_H
/*
 * =====================================================================================
 *
 *       Filename:  oid.h
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  gcc
 *
 *   Organization:  
 *
 * =====================================================================================
 */

#include "src/include/pg.h"

#include "mdbr/macro.h"

typedef int mdbr_oid_t;
#define MAX_OID_LEN 10

extern mdbr_oid_t get_nxt_unused();
MDBR_INIT_F extern void mdbr_oids_init();

#endif /* MDBR_OID_H */
