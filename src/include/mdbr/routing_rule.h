

#ifndef MDBR_ROUTING_RULE_H
#define MDBR_ROUTING_RULE_H

/*
 * =====================================================================================
 *  TODO: add comment & descr
 * =====================================================================================
 */

#include "postgres.h"

#include "mdbr/sharding_key.h"

typedef int shard_id_t;

typedef shard_id_t (*mdbr_route_cb_t)(mdbr_shkey *shkey);

extern int reparse_routef(char *cb_name, mdbr_route_cb_t *dst_f);

// ======================================================================================

extern shard_id_t reparse_shard(List *entries); // List of mdbr_search_entry *

#endif /* MDBR_ROUTING_RULE_H */
