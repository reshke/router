#ifndef MBDR_LIST_H
#define MBDR_LIST_H

/*
 *
 */

typedef struct mdbr_list mdbr_list_t;

struct mdbr_list {
	mdbr_list_t *next;
	mdbr_list_t *prev;
};

static inline void mdbr_list_init(mdbr_list_t *list)
{
	list->next = list->prev = list;
}

static inline void mdbr_list_append(mdbr_list_t *list, mdbr_list_t *node)
{
	node->next = list;
	node->prev = list->prev;
	node->prev->next = node;
	node->next->prev = node;
}

static inline void mdbr_list_unlink(mdbr_list_t *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
}

static inline void mdbr_list_push(mdbr_list_t *list, mdbr_list_t *node)
{
	node->next = list->next;
	node->prev = list;
	node->prev->next = node;
	node->next->prev = node;
}

static inline mdbr_list_t *mdbr_list_pop(mdbr_list_t *list)
{
	register mdbr_list_t *pop = list->next;
	mdbr_list_unlink(pop);
	return pop;
}

static inline int mdbr_list_empty(mdbr_list_t *list)
{
	return list->next == list && list->prev == list;
}

#define mdbr_list_foreach(list, iterator)                                      \
	for (iterator = (list)->next; iterator != list;                        \
	     iterator = (iterator)->next)

#define mdbr_list_foreach_safe(list, iterator, safe)                           \
	for (iterator = (list)->next;                                          \
	     iterator != list && (safe = iterator->next); iterator = safe)

#endif /* MBDR_LIST_H */
