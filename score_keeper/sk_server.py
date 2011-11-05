#!/usr/bin/python2.7

import ssl
import socket
import sys
import threading
import sk
from struct import *

from passphrase import *

HOST= '10.13.37.15'#'127.0.0.1'
PORT = 3333 

MAX_BUFF_SIZE = 1024

SCORE_KEEPING_INTERV = 60

semaphore = threading.Semaphore()

TEAM_A_NUM = 0
TEAM_A_ADDR = '10.13.37.55'
TEAM_A_PORT = 33332 #25668

TEAM_B_NUM = 1
TEAM_B_ADDR = '10.13.37.59'
TEAM_B_PORT = 48122

LOCAL_ADDR = '127.0.0.1'
LOCAL_PORT = 37791

TEAM_C_PORT = 22444

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
    t = threading.Timer(SCORE_KEEPING_INTERV, update_scores)
    t.start()
    # signal.signal(signal.SIGALRM, update_scores)
    # signal.alarm(SCORE_KEEPING_INTERV)
    # scheduler = sched.scheduler(time.time, time.sleep)
    # scheduler.enter(SCORE_KEEPING_INTERV, 1, update_scores,())
    ScoreKeeper.connect()
    ScoreKeeper.new_game()
    

    teamA = threading.Thread(target=server_thread, 
              args=(ScoreKeeper,TEAM_A_ADDR,TEAM_A_PORT,remote_req,))
    teamB = threading.Thread(target=server_thread, 
              args=(ScoreKeeper,TEAM_B_ADDR,TEAM_B_PORT,remote_req,))
    local = threading.Thread(target=server_thread, 
              args=(ScoreKeeper,LOCAL_ADDR, LOCAL_PORT, local_req,))

    teamA.start()
    print 'teamA\'s score keeper started'
    teamB.start()
    print 'teamB\'s score keeper started'
    print 'teamC\'s score keeper started'
    local.start()
    print 'local\'s score keeper started'
    teamA.join()
    teamB.join()
    local.join()


# def update_scores(signum, frame):
#     print 'caught signal'
#     if signum is signal.SIGALRM and ScoreKeeper is not None:
#         print 'get sem'
#         semaphore.acquire()
#         ScoreKeeper.update_scores()
#         ScoreKeeper.update_time_left()
#         signal.alarm(SCORE_KEEPING_INTERV)
#         semaphore.release()
#     print 'end of function'

def update_scores():
    print 'get sem'
    semaphore.acquire()
    ScoreKeeper.update_scores()
    ScoreKeeper.update_time_left()
    t = threading.Timer(SCORE_KEEPING_INTERV, update_scores)
    t.start()    
    semaphore.release()
    print 'end of function'


def server_thread(ScoreKeeper=None,expected_addr='',port=0, req_handler=None):
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
            print 'Wrapping'
            ssl_stream = ssl.wrap_socket(conn, server_side=True, certfile="/home/commander/includes/server.crt", keyfile="/home/commander/includes/server.key", ssl_version=ssl.PROTOCOL_TLSv1)
            print 'Wrapped'
            data = ssl_stream.read()
            print 'Data read'
            ssl_stream.shutdown(socket.SHUT_RDWR)
            print 'Stream Shutdown'
            ssl_stream.close()
            print 'Stream Closed'
            conn.close()
            print 'Conn Close'
            if req_handler is not None:
                print 'Calling Req Handler'
                req_handler(data)
        else:
            print 'expected addr: '+expected_addr+' addr: '+addr[0]
            conn.close()
            print 'Conn Close'

def local_req(data):
    print 'Local Req Handler'
    print 'Data Size: '+str(len(data))
    print 'DMG Expected Size: '+str(calcsize(DMG_HDR))
    if calcsize(DMG_HDR) is len(data):
        passphrase, team_num, dmg_bool = unpack(SRVC_HDR, data)
#        print 'Passphrase: '+passphrase
        if team_num is TEAM_A_NUM and correct_pass(passphrase, team_num):
            update_dmg(ScoreKeeper.Game.TeamA, dmg_bool)
        elif team_num is TEAM_B_NUM and correct_pass(passphrase, team_num):            
            update_dmg(ScoreKeeper.Game.TeamB, dmg_bool)
        print 'Expects Pass: '+get_pass(team_num)

    # elif calcsize(LVL_HDR) is len(data):
    #     passphrase, team_num, lvl_num = unpack(LVL_HDR, data)
    #     if team_num is TEAM_A_NUM and correct_pass(passphrase, team_num):        
    #         if team_num is TEAM_A_NUM:
    #             update_lvl(ScoreKeeper.TeamA, lvl_num)
    #     elif team_num is TEAM_B_NUM and correct_pass(passphrase, team_num): 
    #         update_lvl(ScoreKeeper.TeamB, lvl_num)


def remote_req(data):
    print 'Remote Req Handler'
    print 'Data Size: '+str(len(data))
    print 'Expected Size: '+str(calcsize(SRVC_HDR))
    if calcsize(SRVC_HDR) is len(data):
        passphrase, team_num, srvc_num, srvc_state = unpack(SRVC_HDR, data)
        print 'Passphrase: '+passphrase
        if team_num is TEAM_A_NUM and correct_pass(passphrase, team_num):
            update_srvc_list(ScoreKeeper.Game.TeamA, srvc_num, srvc_state)
        elif team_num is TEAM_B_NUM and correct_pass(passphrase, team_num):
            update_srvc_list(ScoreKeeper.Game.TeamB, srvc_num, srvc_state)
           

def update_srvc_list(team_info, srvc_num, srvc_state):
    if team_info is not None and srvc_num >= 0:
        semaphore.acquire()
        if srvc_state == SRVC_UP:
            team_info.services[srvc_num] = True
        elif srvc_state == SRVC_DOWN:
            team_info.services[srvc_num] = False
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


if __name__ == "__main__":
    main()
