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

MAX_RECORD = 1000000  # 10  million
MAX_LOAD   = 100000   # 0.1 million

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
    sql = "insert into " + TBLNAME + " values(%s, %s, %s)"
    for i in range(MAX_RECORD):        
        # organize record
        sname = 'user' + str(i+1)
        sid = random.randint(0, MAX_RECORD)
        stime = 0
        vals.append((sname, str(sid), str(stime)))
        
        # dump many record
        count += 1
        if (count >= MAX_LOAD):        
            mdb.InsertMany(sql, vals)               
            count = 0
            vals = []
    if len(vals) > 0:
        mdb.InsertMany(sql, vals)               

    print 'Mysql end   time: ', time.asctime()


if __name__ == '__main__':
    print 'hello world'
    TestManyInsert()
    print 'Mysql finish...'
