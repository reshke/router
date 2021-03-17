#/bin/bash

# initialize extention

export TMPDIR=./build/tmp

echo "
    Using postgresql sources at ./build/pgcode, postgresql binaryes from $BINDIR
"

set -ex

#====================================
#   config router & cluster shards 


#------------- MDB ROUTER -----------
export MDBROUTERPORT=6434
export MDBROUTERHST=0.0.0.0
export MDBROUTERDB=router
export MDBROUTEUSR=routing_user

#------------- SHARD 1 ---------------

export PGSHARDPORT1=5433
export PGSHARDDB1=lolkek0
export PGSHARDUSR1=routing_user
export PGSHARDHST1=localhost

#------------- SHARD 2 ---------------

export PGSHARDPORT2=5434
export PGSHARDDB2=lolkek1
export PGSHARDUSR2=routing_user
export PGSHARDHST2=localhost

#             config end
#====================================

#====================================
#            cluster up

cluster_up() {
    # start pg
    mkdir -p $TMPDIR/shard1
    mkdir -p $TMPDIR/shard2
    mkdir -p $TMPDIR/mdb-router
 
    $BINDIR/pg_ctl -D $TMPDIR/mdb-router stop || true
    $BINDIR/pg_ctl -D $TMPDIR/shard1 stop || true
    $BINDIR/pg_ctl -D $TMPDIR/shard2 stop || true

    # should be no need after cluster-down && cleanup
    # ./build/pgbins/bin/pg_ctl -D ./build/tmp/shard1 stop || echo kekw
    $BINDIR/initdb $TMPDIR/mdb-router && $BINDIR/initdb $TMPDIR/shard1 && $BINDIR/initdb $TMPDIR/shard2 || echo lolkek


    # init cluster

    $BINDIR/pg_ctl -D $TMPDIR/mdb-router -l $TMPDIR/mdb-router/pg_log.log -o "-p $MDBROUTERPORT -c max_connections=1000" start
    $BINDIR/pg_ctl -D $TMPDIR/shard1 -l $TMPDIR/shard1/pg_log.log -o "-p $PGSHARDPORT1 -c max_connections=1000" start
    $BINDIR/pg_ctl -D $TMPDIR/shard2 -l $TMPDIR/shard2/pg_log.log -o "-p $PGSHARDPORT2 -c max_connections=1000" start


    # config cluster

    cat > $TMPDIR/mdb-router.sql <<EOH
CREATE DATABASE $MDBROUTERDB;
CREATE USER $MDBROUTEUSR WITH SUPERUSER;
EOH

    cat > $TMPDIR/mdb-router-ext.sql <<EOH
DROP EXTENSION IF EXISTS mdb_router CASCADE;

CREATE EXTENSION mdb_router;
--CREATE SERVER router FOREIGN DATA WRAPPER mdb_router OPTIONS (routing_rule 'route_by_first_key_hash');

-- WE ARE DOING THIS ON MDB ROUTER INSTANCE
-- BUT IN FUTURE METADATA SHOULD BE MANAGED AUTOMATICLY

--CREATE FOREIGN TABLE t1(i int, ch text) SERVER router;
--CREATE FOREIGN TABLE t2(ii int, ch text) SERVER router;
--CREATE FOREIGN TABLE t3(iii int, ch text) SERVER router;

CREATE TABLE t1(i int);
CREATE TABLE t2(ii int);
CREATE TABLE t3(iii int);

--CREATE FOREIGN TABLE t11(i int) SERVER router;

--CREATE USER MAPPING FOR $MDBROUTEUSR SERVER router;
EOH

    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d postgres -f $TMPDIR/mdb-router.sql || true
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -f $TMPDIR/mdb-router-ext.sql || true

    cat > $TMPDIR/shard1.sql <<EOH
DROP DATABASE IF EXISTS $PGSHARDDB1;
CREATE DATABASE $PGSHARDDB1;
CREATE USER $PGSHARDUSR1 WITH SUPERUSER;
EOH

    $BINDIR/psql -h $PGSHARDHST1 -p $PGSHARDPORT1 -d postgres -f $TMPDIR/shard1.sql || true

    cat > $TMPDIR/shard2.sql <<EOH
DROP DATABASE IF EXISTS $PGSHARDDB2;
CREATE DATABASE $PGSHARDDB2;
CREATE USER $PGSHARDUSR2 WITH SUPERUSER;
EOH

    $BINDIR/psql -h $PGSHARDHST2 -p $PGSHARDPORT2 -d postgres -f $TMPDIR/shard2.sql || true
    
}

cluster_config() {

    #create users
    # ----------------------------------- SHMEM allocations etc ------------------------------------------------------------------------------
    #-----------------------------------------------------------------------------------------------------------------------------------------

    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select mdbr_reset_meta()" || true # should be unrouter

    if [ -z $USE_BOUNCER ]; then

        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select mdbr_add_shard('shard1', '$PGSHARDHST1', '$PGSHARDPORT1', '$PGSHARDDB1', '$PGSHARDUSR1');"
        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select mdbr_add_shard('shard2', '$PGSHARDHST2', '$PGSHARDPORT2', '$PGSHARDDB2', '$PGSHARDUSR2');"
    else
        BOUNCER_HOST=localhost
        BOUNCER_PORT=6432

        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select mdbr_add_shard('shard1', '$BOUNCER_HOST', '$BOUNCER_PORT', '$PGSHARDDB1', '$PGSHARDUSR1');"
        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select mdbr_add_shard('shard2', '$BOUNCER_HOST', '$BOUNCER_PORT', '$PGSHARDDB2', '$PGSHARDUSR2');"

    fi


    #-----------------------------------------------------------------------------------------------------------------------------------------
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select create_sharding_key('key1', 'i');"
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select assign_key_range_2_shard('shard1', 'key1', 0, 1000);"
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select assign_key_range_2_shard('shard2', 'key1', 1000, 2000);"
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select show_shkey('key1');"

    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select create_sharding_key('key2', 'ii');"
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select assign_key_range_2_shard('shard1', 'key2', 0, 1000);"
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select assign_key_range_2_shard('shard2', 'key2', 1000, 2000);"
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select show_shkey('key2');"

    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select create_sharding_key('key3', 'iii');"
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select assign_key_range_2_shard('shard1', 'key3', 0, 1000);"
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select assign_key_range_2_shard('shard2', 'key3', 1000, 2000);"
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select show_shkey('key3');"

    #-----------------------------------------------------------------------------------------------------------------------------------------
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select create_sharding_key('key1', 'i');"
    
}

_init_shard_2() {
    #init shards
    # manually create tabels (we need :TODO it usign extension-provided sql function)
    
    colname=''
    for tbl_name in t1 t2 t3 t11 t12 t13
    do 
        colname=$colname'i'
        $BINDIR/psql -h $PGSHARDHST1 -p $PGSHARDPORT1 -d $PGSHARDDB1 -U $PGSHARDUSR1 -c 'create table '$tbl_name'('$colname' int)' || true
        
        $BINDIR/psql -h $PGSHARDHST2 -p $PGSHARDPORT2 -d $PGSHARDDB2 -U $PGSHARDUSR2 -c 'create table '$tbl_name'('$colname' int)' || true
    done

    # TODO: // check ipv6 connections
    #try to do some thing
    #=================================================================================================================================================
    #                                                 I N S E R T                              D A T A  
    #=================================================================================================================================================
    
    set +xe
    for tbl_name in t1 t2 t3 t11 t12 t13
    do
        $BINDIR/psql -h $PGSHARDHST1 -p $PGSHARDPORT1 -d $PGSHARDDB1 -U $PGSHARDUSR1 -c "insert into $tbl_name values(229)"
        $BINDIR/psql -h $PGSHARDHST2 -p $PGSHARDPORT2 -d $PGSHARDDB2 -U $PGSHARDUSR2  -c "insert into $tbl_name values(1337)"
    done

    set -xe
}


_init_shard() {
    #init shards
    # manually create tabels (we need :TODO it usign extension-provided sql function)
    
    colname=''
    for tbl_name in t1 t2 t3
    do 
        colname=$colname'i'
        $BINDIR/psql -h $PGSHARDHST1 -p $PGSHARDPORT1 -d $PGSHARDDB1 -U $PGSHARDUSR1 -c 'create table '$tbl_name'('$colname' int, ch text)' || true
        
        $BINDIR/psql -h $PGSHARDHST2 -p $PGSHARDPORT2 -d $PGSHARDDB2 -U $PGSHARDUSR2 -c 'create table '$tbl_name'('$colname' int, ch text)' || true
    done

    # TODO: // check ipv6 connections
    #try to do some thing
    #=================================================================================================================================================
    #                                                 I N S E R T                              D A T A  
    #=================================================================================================================================================
    
    set +xe
    for tbl_name in t1 t2 t3
    do
        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "insert into $tbl_name values(229, 'qhiqwhduiqwhduqwd')"
        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "insert into $tbl_name values(1337, 'wqjiduf1iqhquiwd')"
    done

    for _ in `seq 1 20` 
    do
        for tbl_name in t1 t2 t3
        do
            key=$(shuf -i1-2000 -n1)
            val=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)
            $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "insert into $tbl_name values($key, '$val')"
        done
    done

    set -xe
}

_test() {

    # ===============================================================================================================================================
    #                                              T E S T                     R O U T I N G
    # ===============================================================================================================================================
    for tbl_name in t1 t2 t3
    do
        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from $tbl_name" || true
        # should fail as tables are separated bt shards 1 & 2
    done

    for sharding_key in 229 999 1000 1337
    do
        colname=''
        # TODO: assert somehow that qryes are routed to different shards
        for tbl_name in t1 t2 t3
        do
            colname=$colname'i'
            $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from $tbl_name WHERE $colname = $sharding_key"    
            # should be ok
        done
    done 

    for sharding_key in 229 999 1000 1337
    do
        colname=''
        # TODO: assert somehow that qryes are routed to different shards
        for tbl_name in t1 t2 t3
        do
            colname=$colname'i'
            $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from $tbl_name WHERE $colname = $sharding_key"    
            # should be ok
        done
    done 

    #=================================================================================================================================================
    #                                                                 J O I N S 
    #=================================================================================================================================================


    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from t1, t2 WHERE i < 999 and ii < 999 limit 10"    
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from t1, t2, t3 WHERE i < 999 and ii < 999 and iii < 999 limit 10"    
    
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from t1, t2 WHERE i > 1000 and ii > 1000 limit 10"    
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from t1, t2, t3 WHERE i > 1000 and ii > 1000 and iii > 1000 limit 10"    

    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from t1, t2 WHERE i = 12 and ii = 200"    
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from t1, t2, t3 WHERE i = 12 and ii = 200 and iii < 888"    
    
    #=================================================================================================================================================
    #                                                                 U P D A T E S
    #=================================================================================================================================================
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "insert into t1 values(111, 'lolkek cheburek')"    
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from t1 where i = 111" 
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "update t1 set ch = 'ololololoo' where i < 200" 
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select * from t1 where i = 111" # should change 

    #=================================================================================================================================================
    #                                                                 D E L E T E S
    #=================================================================================================================================================
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "delete from t1 where i > 1000" 
    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select count(1) = 0 as test from t1 where i > 1000" 

}

_tpcc_config() {

    #=================================================================================================================================================
    #                                                                   S H A R D S
    #=================================================================================================================================================

    if [ -z $ROUTER_ONLY ]; then
        $BINDIR/psql -h $PGSHARDHST1 -p $PGSHARDPORT1 -d $PGSHARDDB1 -U $PGSHARDUSR1 -f ./tpcc-sql/prepare-shard.sql    
        $BINDIR/psql -h $PGSHARDHST2 -p $PGSHARDPORT2 -d $PGSHARDDB2 -U $PGSHARDUSR2 -f ./tpcc-sql/prepare-shard.sql
    fi

    #=================================================================================================================================================
    #                                                                   R O U T E R
    #=================================================================================================================================================


    $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select mdbr_reset_meta()" || true # should be unrouter

    if [ -z $USE_BOUNCER ]; then

        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select mdbr_add_shard('shard1', '$PGSHARDHST1', '$PGSHARDPORT1', '$PGSHARDDB1', '$PGSHARDUSR1');"
        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select mdbr_add_shard('shard2', '$PGSHARDHST2', '$PGSHARDPORT2', '$PGSHARDDB2', '$PGSHARDUSR2');"
        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -f ./tpcc-sql/router.sql
    else
        BOUNCER_HOST=localhost
        BOUNCER_PORT=6432

        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select mdbr_add_shard('shard1', '$BOUNCER_HOST', '$BOUNCER_PORT', '$PGSHARDDB1', '$PGSHARDUSR1');"
        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -c "select mdbr_add_shard('shard2', '$BOUNCER_HOST', '$BOUNCER_PORT', '$PGSHARDDB2', '$PGSHARDUSR2');"

        $BINDIR/psql -h $MDBROUTERHST -p $MDBROUTERPORT -d $MDBROUTERDB -U $MDBROUTEUSR -f ./tpcc-sql/router.sql
    fi

    sed -i "s|#session_preload_libraries = ''|session_preload_libraries = '/home/reshke/code/router/router.so'|g" $TMPDIR/mdb-router/postgresql.conf
    $BINDIR/pg_ctl -D $TMPDIR/mdb-router -l $TMPDIR/mdb-router/pg_log.log -o "-p $MDBROUTERPORT -c max_connections=1000" reload
    $BINDIR/pg_ctl -D $TMPDIR/shard1 -l $TMPDIR/shard1/pg_log.log -o "-p $PGSHARDPORT1 -c max_connections=1000" reload
    $BINDIR/pg_ctl -D $TMPDIR/shard2 -l $TMPDIR/shard2/pg_log.log -o "-p $PGSHARDPORT2 -c max_connections=1000" reload
}

_tpcc() {
    echo lol
}


while getopts "s:t:" arg; do
  case $arg in
    s)
      skip=$OPTARG
      ;;
    t)
      tpcc=$OPTARG
      ;;
  esac
done

if [ ! -z $g ]; then
    _tpcc_config
    exit 0
fi

if [ ! -z $skip ]; then 
    cluster_up
    cluster_config
    _init_shard_2
elif [ ! -z $tpcc ]; then
    cluster_up
    _tpcc_config
    _tpcc
else
    cluster_up
    cluster_config
    _test
fi

