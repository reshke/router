
MODULE_big = router

SRC_DIR:=src

INCLUDE_DIR:=$(SRC_DIR)/include

MDBR_DIR:=$(SRC_DIR)/mdbr
CONN_DIR:=$(SRC_DIR)/conn
PLNR_DIR := $(SRC_DIR)/planner

OBJS = \
	$(WIN32RES) \
	$(MDBR_DIR)/list-ext.o \
	$(MDBR_DIR)/oid.o \
	$(MDBR_DIR)/reparse_util.o \
	$(MDBR_DIR)/mdb_router.o \
	$(MDBR_DIR)/shard.o \
	$(MDBR_DIR)/key_range.o \
	$(MDBR_DIR)/search_entry.o \
	$(MDBR_DIR)/sharding_key.o \
	$(MDBR_DIR)/mdbr_c_api.o \
	$(MDBR_DIR)/local_table.o \
	$(CONN_DIR)/connection.o \
	$(PLNR_DIR)/planner.o \
	mdbr.o

PGFILEDESC = "router - postgresql extention to route queries betwwen shards"

PG_CPPFLAGS = -I$(libpq_srcdir) -I$(INCLUDE_DIR) -DMDB_ROUTER -fstack-protector -Wno-error
SHLIB_LINK_INTERNAL = $(libpq)

EXTENSION = router
DATA = router--0.0.1.sql

REGRESS = router

ifdef USE_PGXS
PG_CONFIG:= 
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
endif

include Makefile.mdbr
