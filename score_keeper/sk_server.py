#!/usr/bin/python2.7

import socket
import sys
import threading
import sk
import time
import database
import signal
from struct import *

from passphrase import *

HOST = '127.0.0.1'
PORT = 3333 

MAX_BUFF_SIZE = 1024

SCORE_KEEPING_INTERV = 0#3 #60

semaphore = threading.Semaphore()

TEAM_A_NUM = 0
TEAM_A_ADDR = '127.0.0.1' #'10.13.37.55'
TEAM_A_PORT = 33332 #25668

TEAM_B_NUM = 1
TEAM_B_ADDR = '127.0.0.1' #'10.13.37.56'
TEAM_B_PORT = 48122


LOCAL_ADDR = '127.0.0.1'
LOCAL_PORT = 37791

PASSPHRASE = '9s'
TEAM_NUM = 'B'
SRVC_NUM = 'B'
UP_OR_DWN = 'B'
DMG_BOOL = 'B'
LVL_NUM = 'B'
SRVC_HDR = PASSPHRASE + TEAM_NUM + SRVC_NUM + UP_OR_DWN
DMG_HDR = PASSPHRASE + TEAM_NUM + DMG_BOOL
COMMS_HDR = PASSPHRASE + TEAM_NUM + LVL_NUM

NUM_SRVCS = 5
ENGINE_SRVC_OFF=0
NAVIGATION_SRVC_OFF=1
POWER_SRVC_OFF=2
WEAPONS_SRVC_OFF=3
SHIELDS_SRVC_OFF=4
#COMMS_SRVC_OFF=5

SRVC_DOWN = False
SRVC_UP = True

ScoreKeeper = sk.ScoreKeeper()

DMG_TRUE = 1

def main():
    signal.signal(signal.SIGALRM, update_scores)
    signal.alarm(SCORE_KEEPING_INTERV)
    ScoreKeeper.connect()
    ScoreKeeper.new_game()

    teamA = threading.Thread(target=server_thread, 
              args=(TEAM_A_ADDR,TEAM_A_PORT,remote_req,))
    teamB = threading.Thread(target=server_thread, 
              args=(TEAM_B_ADDR,TEAM_B_PORT,remote_req,))
    local = threading.Thread(target=server_thread, 
              args=(LOCAL_ADDR, LOCAL_PORT, local_req,))

    teamA.start()
    print 'teamA\'s score keeper started'
    teamB.start()
    print 'teamB\'s score keeper started'
    local.start()
    print 'local\'s score keeper started'
    teamA.join()
    teamB.join()
    local.join()
    

def update_scores(signum, frame):
    print 'caught signal'
    if signum is signal.SIGALRM and ScoreKeeper is not None:
        print 'get sem'
        semaphore.acquire()
        ScoreKeeper.update_scores()
        ScoreKeeper.update_time_left()
        signal.alarm(SCORE_KEEPING_INTERV)
        semaphore.release()
    print 'end of function'


def server_thread(expected_addr='',port=0, req_handler=None):
    sock = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    except socket.error, msg:
        print msg
        sys.exit(1)
    try:
        if port == 0:
            print '0 is an invalid port number'
            sys.exit(1)
        print 'binding:: host: '+HOST+' port: '+str(port)
        sock.bind((HOST, port))
        sock.listen(15)
    except socket.error, msg:
        print 'binding error - port: '+str(port)
        print msg
        sock.close()
        sock = None
        sys.exit(1)

    while True:
        if sock is not None:
            conn, addr = sock.accept()
            print 'Connected by', addr
    
        if expected_addr == addr[0]:
            data = conn.recv(MAX_BUFF_SIZE)
            print 'addr: '+str(addr)+' data: '+str(data)
            conn.close()
            req_handler(data)
        else:
            conn.close()

def local_req(data):
    if calcsize(DMG_HDR) is len(data):
        passphrase, team_num, dmg_bool = unpack(SRVC_HDR, data)
        if team_num is TEAM_A_NUM and correct_pass(passphrase, team_num):
            update_dmg(ScoreKeeper.TeamA, dmg_bool)
        elif team_num is TEAM_B_NUM and correct_pass(passphrase, team_num):            
            update_dmg(ScoreKeeper.TeamB, dmg_bool)

    # elif calcsize(LVL_HDR) is len(data):
    #     passphrase, team_num, lvl_num = unpack(LVL_HDR, data)
    #     if team_num is TEAM_A_NUM and correct_pass(passphrase, team_num):        
    #         if team_num is TEAM_A_NUM:
    #             update_lvl(ScoreKeeper.TeamA, lvl_num)
    #     elif team_num is TEAM_B_NUM and correct_pass(passphrase, team_num): 
    #         update_lvl(ScoreKeeper.TeamB, lvl_num)


def remote_req(data):
    if calcsize(SRVC_HDR) is len(data):
        passphrase, team_num, srvc_num, srvc_state = unpack(SRVC_HDR, data)
        if team_num is TEAM_A_NUM and correct_pass(passphrase, team_num):
            update_srvc_list(ScoreKeeper.TeamA, srvc_num, srvc_state)
        elif team_num is TEAM_B_NUM and correct_pass(passphrase, team_num):
            update_srvc_list(ScoreKeeper.TeamB, srvc_num, srvc_state)
           

def update_srvc_list(team_info, srvc_num, srvc_state):
    if team_info is not None and srvc_num >= 0:
        semaphore.acquire()
        team_info.service[srvc_num] = srvc_state
        semaphore.release()

def update_dmg(team_info, dmg_bool):
    if team_info is not None and dmg_bool is DMG_TRUE:
        semaphore.acquire()
        team_info.dmg += 1
        semaphore.release()

# def update_lvl(team_info, lvl_num):
#     if team_info is not None and lvl_num < len(game.NUM_LVLS):
#         semaphore.acquire()
#         team_info.lvls[lvl_num] = game.LVL_COMPLETE
#         semaphore.release()


    # for res in socket.getaddrinfo(HOST, PORT, socket.AF_INET,
    #                               socket.SOCK_STREAM, 0, socket.AI_INET):
    #     af, socktype, proto, canonname, sa = res
if __name__ == "__main__":
    main()
