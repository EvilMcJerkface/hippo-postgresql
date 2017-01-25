SET enable_seqscan = OFF;
create table hippo_tbl(id int8, id2 int8, payload text);
insert into hippo_tbl(id, id2, payload) select i, random()*1000000, repeat('a', 100) from generate_series (1,100000) i;
Alter table hippo_tbl alter column id2 set statistics 10000;
Analyze hippo_tbl;
create index hippo_idx on hippo_tbl using hippo(id2) with (density=20);
select count(*) from hippo_tbl where id2>10000 and id2 <10010;
insert into hippo_tbl(id, id2, payload) select i, 10009, repeat('a', 100) from generate_series (1, 1000) i;
select count(*) from hippo_tbl where id2>10000 and id2 <10010;
delete from hippo_tbl where  id2>10000 and id2 <10010;
VACUUM;
select count(*) from hippo_tbl where id2>10000 and id2 <10010;
drop index hippo_idx;
drop table hippo_tbl;




