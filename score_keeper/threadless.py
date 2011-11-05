#!/usr/bin/python2.7

import threading
import time
import sys
import os
import signal
import socket


HOST = '127.0.0.1'
semaphore = threading.Semaphore()


def main():
    signal.signal(signal.SIGALRM, update_score)
    teamA = threading.Thread(target=spam, args=(4444,))
    teamB = threading.Thread(target=spam, args=(5555,))
    score_keeping = threading.Timer(1.0, update_score)
    
    print 'Thread test'
    
    teamA.start()
    teamB.start()
    signal.alarm(1)
    while True:
        pass
#        score_keeping.start()
#        score_keeping.join()
        
def update_score(signum, frame):
    print 'signal handler'
    semaphore.acquire()
    print 'Updating score'
    signal.alarm(1)
    semaphore.release()


def spam(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((HOST, port))
    sock.listen(15)
    while True:
        conn, addr = sock.accept()
        data = conn.recv(1024)
        semaphore.acquire()
        print str(port)+'proc data: \''+data+'\''
        semaphore.release()


if __name__ == '__main__':
    main()
