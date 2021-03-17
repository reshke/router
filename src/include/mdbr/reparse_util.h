/*
 * =====================================================================================
 *    Description:  some utilities to reparse shards
 *        Version:  1.0
 *        Created:  15.01.2021 04:37:10
 *
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef MDBR_REPARSE_U_H
#define MDBR_REPARSE_U_H

extern void reparse_shid_from_opexpr(OpExpr *node, int *shid);
extern void reparse_from_tl_list(List *tl, Oid relid, List *es);

#endif /* MDBR_REPARSE_U_H */
