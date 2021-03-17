/*
 * =====================================================================================
 *
 *       Filename:  list-ext.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  15.01.2021 17:39:24
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>

#include "mdbr/list-ext.h"

// ==============================================================================
//  from ./src/backend/nodes/list.c
// ==============================================================================

#define LIST_HEADER_OVERHEAD                                                   \
	((int)((offsetof(List, initial_elements) - 1) / sizeof(ListCell) + 1))
/*
 * Return a freshly allocated List with room for at least min_size cells.
 *
 * Since empty non-NIL lists are invalid, new_list() sets the initial length
 * to min_size, effectively marking that number of cells as valid; the caller
 * is responsible for filling in their data.
 */
static List *new_list(NodeTag type, int min_size)
{
	List *newlist;
	int max_size;

	Assert(min_size > 0);

	/*
	 * We allocate all the requested cells, and possibly some more, as part of
	 * the same palloc request as the List header.  This is a big win for the
	 * typical case of short fixed-length lists.  It can lose if we allocate a
	 * moderately long list and then it gets extended; we'll be wasting more
	 * initial_elements[] space than if we'd made the header small.  However,
	 * rounding up the request as we do in the normal code path provides some
	 * defense against small extensions.
	 */

#ifndef DEBUG_LIST_MEMORY_USAGE

	/*
	 * Normally, we set up a list with some extra cells, to allow it to grow
	 * without a repalloc.  Prefer cell counts chosen to make the total
	 * allocation a power-of-2, since palloc would round it up to that anyway.
	 * (That stops being true for very large allocations, but very long lists
	 * are infrequent, so it doesn't seem worth special logic for such cases.)
	 *
	 * The minimum allocation is 8 ListCell units, providing either 4 or 5
	 * available ListCells depending on the machine's word width.  Counting
	 * palloc's overhead, this uses the same amount of space as a one-cell
	 * list did in the old implementation, and less space for any longer list.
	 *
	 * We needn't worry about integer overflow; no caller passes min_size
	 * that's more than twice the size of an existing list, so the size limits
	 * within palloc will ensure that we don't overflow here.
	 */
	max_size = pg_nextpower2_32(Max(8, min_size + LIST_HEADER_OVERHEAD));
	max_size -= LIST_HEADER_OVERHEAD;
#else

	/*
	 * For debugging, don't allow any extra space.  This forces any cell
	 * addition to go through enlarge_list() and thus move the existing data.
	 */
	max_size = min_size;
#endif

	newlist = (List *)palloc(offsetof(List, initial_elements) +
				 max_size * sizeof(ListCell));
	newlist->type = type;
	newlist->length = min_size;
	newlist->max_length = max_size;
	newlist->elements = newlist->initial_elements;

	return newlist;
}
/*
 * Check that the specified List is valid (so far as we can tell).
 */
static void check_list_invariants(const List *list)
{
	if (list == NIL)
		return;

	Assert(list->length > 0);
	Assert(list->length <= list->max_length);
	Assert(list->elements != NULL);

	Assert(list->type == T_List || list->type == T_IntList ||
	       list->type == T_OidList);
}

// ==============================================================================

List *list_make5_impl(NodeTag t, ListCell datum1, ListCell datum2,
		      ListCell datum3, ListCell datum4, ListCell datum5)
{
	List *list = new_list(t, 5);

	list->elements[0] = datum1;
	list->elements[1] = datum2;
	list->elements[2] = datum3;
	list->elements[3] = datum4;
	list->elements[4] = datum5;
	check_list_invariants(list);
	return list;
}
