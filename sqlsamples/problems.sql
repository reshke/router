INSERT INTO new_orders1 (no_o_id, no_d_id, no_w_id) SELECT o_id, o_d_id, o_w_id FROM orders1 WHERE o_id>21 and o_w_id=1;
