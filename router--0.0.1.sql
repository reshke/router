/* contrib/mdb_router/mdb_router--0.0.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION mdb_router" to load this file. \quit

CREATE FUNCTION mdbr_add_shard(
        i_name      text,
        i_host      text DEFAULT 'localhost',
        i_port      text DEFAULT '5432',
        i_dbname    text DEFAULT 'sharddb',
        i_usr       text DEFAULT 'root',
        i_passwd    text DEFAULT '12345678'
)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE FUNCTION mdbr_reset_meta () RETURNS void as 'MODULE_PATHNAME' LANGUAGE C STRICT;

CREATE FUNCTION create_sharding_key(
        i_name     text,
        i_cols     TEXT -- (not working for now) ARRAY
) RETURNS void as 'MODULE_PATHNAME' LANGUAGE C STRICT;

CREATE FUNCTION assign_key_range_2_shard(
        i_shard_name  text,
        i_shkey_name  text,
        i_l_bound       integer,
        i_u_bound       integer 
) RETURNS void as 'MODULE_PATHNAME' LANGUAGE C STRICT;

CREATE FUNCTION show_shkey (
        i_name text
) RETURNS void as 'MODULE_PATHNAME' LANGUAGE C STRICT;

CREATE FUNCTION add_local_table (
        i_name text
) RETURNS void as 'MODULE_PATHNAME' LANGUAGE C STRICT;
