/*
 * =====================================================================================
 *
 *       Filename:  list-ext.h
 *
 *
 *        Version:  1.0
 *        Created:  15.01.2021 17:39:28
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef LISTEXT_H
#define LISTEXT_H

#include "src/include/pg.h"

#define list_make5(x1, x2, x3, x4, x5)                                         \
	list_make5_impl(T_List, list_make_ptr_cell(x1),                        \
			list_make_ptr_cell(x2), list_make_ptr_cell(x3),        \
			list_make_ptr_cell(x4), list_make_ptr_cell(x5))

extern List *list_make5_impl(NodeTag t, ListCell datum1, ListCell datum2,
			     ListCell datum3, ListCell datum4, ListCell datum5);

#endif /* LISTEXT_H */
