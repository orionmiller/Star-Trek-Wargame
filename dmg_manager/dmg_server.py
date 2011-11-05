#!/usr/bin/python2.7


#######ADDD!!!!!!######
# down shields every 60 seconds
# passwphrase checkin and generating needs to be correct
# SSL connections

import socket
#import signal
import sys
#import os
import threading

from struct import *
from passphrase import *

import team
#import shields
import st_map
import weapons


HOST='127.0.0.1'
MAX_BUFF_SIZE = 1024

TEAM_A_NUM = 0
TEAM_A_ADDR = '127.0.0.1' #'10.13.37.55'
TEAM_A_PORT = 33997 #25668

TEAM_B_NUM = 1
TEAM_B_ADDR = '127.0.0.1' #'10.13.37.56'
TEAM_B_PORT = 22456

LOCAL_PORT = 37791

#PORT = 43361

#SEND/RECV SHARED
PASSPHRASE = '9s'
TEAM_NUM = 'B'
SRVC_NUM = 'B'
DMG_BOOL = 'B'

#RECV
POS = 'B'
ID = 'B'
UP_OR_DOWN = 'B'
SHIELD_UP = 1
SHIELD_DOWN = 0
POS_X = POS
POS_Y = POS
DMG_TRUE = 1
SHIELD_HDR = PASSPHRASE + TEAM_NUM + SRVC_NUM + UP_OR_DOWN
RCV_DMG_HDR = PASSPHRASE + TEAM_NUM + SRVC_NUM
NAV_HDR = PASSPHRASE + TEAM_NUM + POS_X + POS_Y + 'x'

#SEND
SEND_DMG_HDR = PASSPHRASE + TEAM_NUM + DMG_BOOL

semaphore = threading.Semaphore()

#import passphrase checker

DMG_REPORT_INTERV = 60

teamA = team.Team()
teamB = team.Team()
Map = st_map.Map()
Weapons = weapons.Weapons(Map)

def main():
    teamA_thread = threading.Thread(target=thread_server,
                     args=(teamA, TEAM_A_ADDR, TEAM_A_PORT, req_handler,))
    teamB_thread = threading.Thread(target=thread_server,
                     args=(teamB, TEAM_B_ADDR, TEAM_B_PORT, req_handler,))
    teamA_thread.start()
    teamB_thread.start()
    teamA_thread.join()
    teamB_thread.join()


def move(Team, pos_x, pos_y):
    semaphore.acquire()
    Team.move(pos_x, pos_y)
    semaphore.release()

def shield_state(Team, srvc_num, up_or_down):
    if up_or_down == SHIELD_DOWN:
        semaphore.acquire()
        Team.shield_down(srvc_num)
        semaphore.release()
    elif up_or_down == SHIELD_UP:
        semaphore.acquire()
        Team.shield_up(srvc_num)
        semaphore.release()

def dmg(From, To, srvc_num):
    semaphore.acquire()
    if not To.is_shield_up(srvc_num) and Weapons.in_range(From, To):
        if To is teamA and From is teamB:
            send_dmg('123456789', TEAM_A_NUM)
        elif To is teamB and From is teamA:
            send_dmg('123456789', TEAM_B_NUM)
    semaphore.release()


def req_handler(Team, data):
    if len(data) == calcsize(NAV_HDR):
        passphrase, team_num, pos_x, pos_y = unpack(NAV_HDR, data)
        if Team is teamA and correct_pass(passphrase, TEAM_A_PASS):
            if team_num is TEAM_A_NUM:
                move(teamA, pos_x, pos_y)
            elif team_num is TEAM_B_NUM:
                move(teamB, pos_x, pos_y)
        elif Team is teamB and correct_pass(passphrase, TEAM_B_PASS):
            if team_num is TEAM_A_NUM:
                move(teamA, pos_x, pos_y)
            elif team_num is TEAM_B_NUM:
                move(teamB, pos_x, pos_y)
        
    elif len(data) == calcsize(SHIELD_HDR):
        passphrase, team_num, srvc_num, state = unpack(SHIELD_HDR, data)
        if Team is teamA and correct_pass(passphrase, TEAM_A_PASS):
            if team_num == TEAM_A_NUM:
                shield_state(teamA, srvc_num, state)
            elif team_num == TEAM_B_NUM:
                shield_state(teamB, srvc_num, state)
        elif Team is teamB and correct_pass(passphrase, TEAM_B_PASS):
            if team_num == TEAM_A_NUM:
                shield_state(teamA, srvc_num, state)
            elif team_num == TEAM_B_NUM:
                shield_state(teamB, srvc_num, state)
        
    elif len(data) == calcsize(RCV_DMG_HDR):
        passphrase, team_num, srvc_num = unpack(RCV_DMG_HDR, data)
        if Team is teamA and correct_pass(passphrase, TEAM_A_PASS):
            if team_num == TEAM_A_NUM and Team is teamA:
                dmg(teamA, teamB, srvc_num)
            elif team_num == TEAM_B_NUM and Team is teamB:
                dmg(teamB, teamA, srvc_num)
        elif Team is teamB and correct_pass(passphrase, TEAM_B_PASS):
            if team_num == TEAM_A_NUM and Team is teamA:
                dmg(teamA, teamB, srvc_num)
            elif team_num == TEAM_B_NUM and Team is teamB:
                dmg(teamB, teamA, srvc_num)

    semaphore.acquire()
    print '::Team A - Sate::'
    teamA.print_state()
    print '::Team B - Sate::'
    teamB.print_state()
    semaphore.release()

def thread_server(Team=None, expected_addr='',port=0, req_handler=None):
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
            req_handler(Team, data)
        else:
            print 'recvd addr: \''+addr[0]+'\' expected addr: \''+expected_addr+'\''
            conn.close()



def send_dmg(passphrase, team_num):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('127.0.0.1', LOCAL_PORT))
    data = pack(SEND_DMG_HDR, passphrase, team_num, DMG_TRUE)
    print 'sending damage:: team_num:'+str(team_num)
    sock.send(data)
    sock.close()
    
if __name__ == '__main__':
    main()

#needs to shutdown shields every 60 seconds
