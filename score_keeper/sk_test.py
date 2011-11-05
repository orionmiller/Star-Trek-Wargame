#!/usr/bin/python2.7

import socket
import sys
import database
import signal
import struct

MAX_BUFF_SIZE = 1024

SCORE_KEEPING_INTERV = 3 #60

TEAM_A_NUM = 0
TEAM_B_ADDR = '127.0.0.1' #'10.13.37.56'
TEAM_A_PORT = 33332 #25668

TEAM_B_NUM = 1
TEAM_A_ADDR = '127.0.0.1' #'10.13.37.55'
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

SRVC_DOWN = False
SRVC_UP = True

team_a = socket.socket(socket.af_inet, socket.sock_stream)
team_a.connect(('127.0.0.1',TEAM_A_PORT))
team_b = socket.socket(socket.af_inet, socket.sock_stream)
team_b.connect(('127.0.0.1',TEAM_B_PORT))
local = socket.socket(socket.af_inet, socket.sock_stream)
