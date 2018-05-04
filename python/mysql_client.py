#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import time
import random
import MySQLdb

HOST    = '127.0.0.1'
USER    = 'root'
PWD     = '123456'
DBNAME  = 'userinfo'
PORT    = 3306
TBLNAME = 'student'

MAX_RECORD = 10000000  # 10  million
MAX_LOAD   = 100000   # 0.1 million

# SQL Example: index invalid 
SQL_INVALID_INDEX_OR = "select * from `student` where `id`=3 or `name`='user222222';"
SQL_INVALID_INDEX_LIKE = "select * from `student` where `name` like '%user123456';"

# SQL Example: manage table
SQL_TRUNCATE_TABLE = "truncate table `student`;"
SQL_CREATE_TABLE = "create table `student` (main_key int primary key not null auto_increment, id int, name varchar(20), login_time int);"
SQL_DELETE_TABLE = "drop table `student`;"
SQL_DEL_INCREMENT = "alter table `student` modify `main_key` int;"
SQL_DEL_MAINKEY = "alter table `student` drop primary key;"
SQL_SET_DEFAULT = "alter table `student` alter column `main_key` set default 0;"

# SQL Example: manage db
# find out variables of mysql, such as binlog, slow_query, engine and so on
# linux command: mysqld --help --verbose | grep slow
SQL_SHOW_VAR = "show variables like '%engine%';"

# SQL Example: manage record
SQL_INSERT_MANY = "insert into `student` (`main_key`, `id`, `name`, `login_time`) values (%s, %s, %s, %s);"
SQL_UPDATE = "update `student` set `name`='xxxx' where `name`='user1';"
SQL_ORDER_BY = "select * from `student` order by `id` desc limit 10;"
# first group by name, then calc sum of id, finally calc total
SQL_GROUP_BY = "select `name`, sum(`id`) as `sum_id` from `student` group by `name` with rollup;"


class CMysql(object):
    def __init__(self):
        self.conn = None
        self.cur = None

    def Connect(self, host, port, user, passwd, dbname):
        self.conn = MySQLdb.connect(host, port, user, passwd, dbname)
        self.cur = self.conn.cursor(cursorclass = MySQLdb.cursors.DictCursor)
        # print 'connect mysql OK'

    def Insert(self, sql, value=None):
        count = self.cur.execute(sql, value)
        self.conn.commit()        

    def InsertMany(self, sql, values):
        count = self.cur.executemany(sql, values)
        self.conn.commit()


def TestManyInsert():
    print 'Mysql start time: ', time.asctime()

    mdb = CMysql()
    mdb.Connect(HOST, USER, PWD, DBNAME, PORT)
    vals = []
    count = 0
    # sql = "insert into student values('%s', %d, %d)" % (sname, sid, stime)
    # sql should use string unify? 
    # sql = "insert into " + TBLNAME + " values(%s, %s, %s)"
    for i in range(MAX_RECORD):        
        # organize record
        sname = 'user' + str(i+1)
        sid = random.randint(0, MAX_RECORD)
        stime = 0
        vals.append((str(i+1), str(sid), sname, str(stime)))
        
        # dump many record
        count += 1
        if (count >= MAX_LOAD):        
            mdb.InsertMany(SQL_INSERT_MANY, vals)               
            count = 0
            vals = []
    if len(vals) > 0:
        mdb.InsertMany(SQL_INSERT_MANY, vals)               

    print 'Mysql end   time: ', time.asctime()


if __name__ == '__main__':
    print 'hello world'
    TestManyInsert()
    print 'Mysql finish...'
