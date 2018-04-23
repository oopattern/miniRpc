#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import random
import MySQLdb

HOST    = '127.0.0.1'
USER    = 'root'
PWD     = '123456'
DBNAME  = 'userinfo'
PORT    = 3306

MAX_RECORD = 10

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

if __name__ == '__main__':
    print 'hello world'
    mdb = CMysql()
    mdb.Connect(HOST, USER, PWD, DBNAME, PORT)
    for i in range(MAX_RECORD):
        sname = 'user' + str(i+1)
        sid = random.randint(0, MAX_RECORD)
        stime = 0
        sql = "insert into student values('%s', %d, %d)" % (sname, sid, stime)
        mdb.Insert(sql)
    print 'Mysql finish...'
