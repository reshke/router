create table warehouse1(
    w_id smallint not null,
    w_name varchar(10),
    w_street_1 varchar(20),
    w_street_2 varchar(20),
    w_city varchar(20),
    w_state char(2),
    w_zip char(9),
    w_tax decimal(4,2),
    w_ytd decimal(12,2) 
    primary key w_id
) ;

select create_sharding_key('key-tpcc', 'w_id');
select assign_key_range_2_shard('shard1', 'key-tpcc', 0, 3);
select assign_key_range_2_shard('shard2', 'key-tpcc', 3, 10);


create table district1 (
	d_id smallint not null, 
	d_w_id smallint not null, 
	d_name varchar(10), 
	d_street_1 varchar(20), 
	d_street_2 varchar(20), 
	d_city varchar(20), 
	d_state char(2), 
	d_zip char(9), 
	d_tax decimal(4,2), 
	d_ytd decimal(12,2), 
	d_next_o_id int,
    primary key (d_id, d_w_id)
) ; 

select create_sharding_key('key-tpcc2', 'd_w_id');
select assign_key_range_2_shard('shard1', 'key-tpcc2', 0, 3);
select assign_key_range_2_shard('shard2', 'key-tpcc2', 3, 10);

CREATE  TABLE public.customer1 (
    c_id integer NOT NULL,
    c_d_id smallint NOT NULL,
    c_w_id smallint NOT NULL,
    c_first character varying(16),
    c_middle character(2),
    c_last character varying(16),
    c_street_1 character varying(20),
    c_street_2 character varying(20),
    c_city character varying(20),
    c_state character(2),
    c_zip character(9),
    c_phone character(16),
    c_since timestamp without time zone,
    c_credit character(2),
    c_credit_lim bigint,
    c_discount numeric(4,2),
    c_balance numeric(12,2),
    c_ytd_payment numeric(12,2),
    c_payment_cnt smallint,
    c_delivery_cnt smallint,
    c_data text
) ;

select create_sharding_key('key-tpcc3', 'c_w_id');
select assign_key_range_2_shard('shard1', 'key-tpcc3', 0, 3);
select assign_key_range_2_shard('shard2', 'key-tpcc3', 3, 10);


create  table IF NOT EXISTS history1 (
	h_c_id int,
	h_c_d_id smallint,
	h_c_w_id smallint,
	h_d_id smallint,
	h_w_id smallint,
	h_date timestamp,
	h_amount decimal(6,2),
	h_data varchar(24)
) ;

select create_sharding_key('key-tpcc4', 'h_c_w_id');
select assign_key_range_2_shard('shard1', 'key-tpcc4', 0, 3);
select assign_key_range_2_shard('shard2', 'key-tpcc4', 3, 10);

create  table IF NOT EXISTS orders1 (
	o_id int not null,
	o_d_id smallint not null,
	o_w_id smallint not null,
	o_c_id int,
	o_entry_d timestamp,
	o_carrier_id smallint,
	o_ol_cnt smallint,
	o_all_local smallint,
	PRIMARY KEY(o_w_id, o_d_id, o_id)
) ;

select create_sharding_key('key-tpcc5', 'o_w_id');
select assign_key_range_2_shard('shard1', 'key-tpcc5', 0, 3);
select assign_key_range_2_shard('shard2', 'key-tpcc5', 3, 10);


create  table IF NOT EXISTS new_orders1 (
	no_o_id int not null,
	no_d_id smallint not null,
	no_w_id smallint not null,
	PRIMARY KEY(no_w_id, no_d_id, no_o_id)
) ;

select create_sharding_key('key-tpcc6', 'no_w_id');
select assign_key_range_2_shard('shard1', 'key-tpcc6', 0, 3);
select assign_key_range_2_shard('shard2', 'key-tpcc6', 3, 10);

create  table IF NOT EXISTS order_line1 (
	ol_o_id int not null,
	ol_d_id smallint not null,
	ol_w_id smallint not null,
	ol_number smallint not null,
	ol_i_id int,
	ol_supply_w_id smallint,
	ol_delivery_d timestamp,
	ol_quantity smallint,
	ol_amount decimal(6,2),
	ol_dist_info char(24),
	PRIMARY KEY(ol_w_id, ol_d_id, ol_o_id, ol_number)
) ;

select create_sharding_key('key-tpcc7', 'ol_w_id');
select assign_key_range_2_shard('shard1', 'key-tpcc7', 0, 3);
select assign_key_range_2_shard('shard2', 'key-tpcc7', 3, 10);

create  table IF NOT EXISTS stock1 (
	s_i_id int not null,
	s_w_id smallint not null,
	s_quantity smallint,
	s_dist_01 char(24),
	s_dist_02 char(24),
	s_dist_03 char(24),
	s_dist_04 char(24),
	s_dist_05 char(24),
	s_dist_06 char(24),
	s_dist_07 char(24),
	s_dist_08 char(24),
	s_dist_09 char(24),
	s_dist_10 char(24),
	s_ytd decimal(8,0),
	s_order_cnt smallint,
	s_remote_cnt smallint,
	s_data varchar(50),
	PRIMARY KEY(s_w_id, s_i_id)
) ;

select create_sharding_key('key-tpcc8', 's_w_id');
select assign_key_range_2_shard('shard1', 'key-tpcc8', 0, 3);
select assign_key_range_2_shard('shard2', 'key-tpcc8', 3, 10);

create table IF NOT EXISTS item1 (
      i_id int not null,
      i_im_id int,
      i_name varchar(24),
      i_price decimal(5,2),
      i_data varchar(50),
    PRIMARY KEY(i_id)
) ;

select add_local_table('item1');

--select create_sharding_key('key-tpcc9', 'i_id');
--select assign_key_range_2_shard('shard1', 'key-tpcc9', 0, 15);
--select assign_key_range_2_shard('shard2', 'key-tpcc9', 15, 100);
