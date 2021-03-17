/*
 * =====================================================================================
 *
 *       Filename:  macro.h
 *
 * =====================================================================================
 */

#ifndef MDBR_MACRO_H
#define MDBR_MACRO_H

typedef int mdbr_retcode_t;

static const mdbr_retcode_t MDBR_RETCODE_OK = 0;
static const mdbr_retcode_t MDBR_RETCODE_NOTOK = 1;

#define UNROUTED -1

typedef enum { less_op, eq_op, gt_op } mdbr_ri_ops;

#define MDBR_INIT_F
// mark functions which should be called on module/process load

#define mdbr_container_of(N, T, F)                                             \
	((T *)((char *)(N) - __builtin_offsetof(T, F)))

#endif /* MDBR_MACRO_H */
