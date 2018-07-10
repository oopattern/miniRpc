import os
import sys
import socket
import time
import errno

class CSocket:
    def __init__(self):
        self.m_socket = None
        self.m_is_connected = False

    def Init(self, addr):
        try:
            self.m_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.m_socket.settimeout(0.5)
            self.m_socket.connect(addr)
            self.m_socket.setblocking(0)
            self.m_is_connected = True
        except IOError as err:
            print 'Init socket err'
            self.m_is_connected = False

    def IsConnected(self):
        return self.m_is_connected

    # packet is list
    def Send(self, packet):
        send_len = -1
        send_buf = ''.join(packet)
        try:            
            send_len = self.m_socket.send(send_buf)
        except socket.error as e:
            print e.errno
            print os.strerror(e.errno)
        return send_len

if __name__ == '__main__':
    print('hello sakula')

    addr = ("127.0.0.1",8888)
    data = ['a'] * 128 * 1024
    total = 0.0

    print data

    # send data every one second, from 0.5kb ~ 200kb
    s = CSocket()
    s.Init(addr)
    if s.IsConnected() == True:        
        send_len = 0
        for i in range(1000):
            total += len(data)
            send_len = s.Send(data)
            time.sleep(2)
            print("send data len=%d" % send_len)
    
    print('send data size=%.2f(Kb) finish' % (total / 1024))

    while 1:
        print('sleep for every 2 seconds')
        time.sleep(2)
