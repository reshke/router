# router
Request router for OLTP PostgreSQL sharding


### structure of directry build


     build -|
            | --- tmp  -| # temp directory for shards/router data store
                        |
                        | - shard1 -pg shard1 data folder(not pathed or something)
                        |
                        | - shard2 -pg shard2 ---/*/----
                        |
                        | - mdb_router router database with mdb_router extension

### how to start
    make PG_CONFIG=/home/reshke/bins/pgsql/bin/pg_config clean prepare compile-extension cluster-up


### Current problems:
    bad memory management
    no metadata consistency control
    shared memory structures invalidates after restart
    we need to use HTAB to avoid linear search in list

#   TPCC BENCH SQL CAN BE FOUND IN ./cool-sql folder
execute ddl on each shard and then load some data using router

### WHAT DO WE NEED FOR TPCC
    DDL routing?
   
### RUN TPCC
    make PG_CONFIG=/home/reshke/bins/pgsql/bin/pg_config CONFIG_OPTS='-t=yes' clean compile-extension cluster-up 
    ./tpcc.lua --pgsql-user=routing_user --pgsql-db=lolkek1 --time=360 --threads=12 --report-interval=1 --tables=1 --scale=5 --use_fk=0 --trx_level=RC --db-driver=pgsql --pgsql-port=6432 --pgsql-host=localhost prepare

