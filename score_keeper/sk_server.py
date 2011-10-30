#!/usr/bin/python2.7

import socket
import sys
import threading
import sk

HOST = '127.0.0.1'
PORT = 3333 

MAX_BUFF_SIZE = 1024

semaphore = threading.Semaphore()

def main():
    ScoreKeeper = sk.ScoreKeeper()
    ScoreKeeper.new_game()


def update_scores(ScoreKeeper=None):
    if ScoreKeeper is None:
        print 'Update Score Error'
        sys.exit(1)

def team_thread(expected_addr='',port=0):
    sock = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    except socket.error, msg:
        print msg
        sys.exit(1)
    try:
        if port is 0:
            print '0 is an invalid port number'
            sys.exit(1)
        sock.bind((HOST, port))
        sock.listen(15)
    except socket.error, msg:
        print msg
        sock.close()
        sock = None
        sys.exit(1)

    while True:
        if sock is not None:
            conn, addr = sock.accept()
            print 'Connected by', addr
        semaphore.acquire()
        if expected_addr is addr:
            semaphore.acquire()
            data = conn.recv(MAX_BUFF_SIZE)
            
        semaphore.release()
    conn.close()    
            

if __name__ == "__main__":
    main()


    # for res in socket.getaddrinfo(HOST, PORT, socket.AF_INET,
    #                               socket.SOCK_STREAM, 0, socket.AI_INET):
    #     af, socktype, proto, canonname, sa = res
