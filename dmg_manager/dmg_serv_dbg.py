#!/usr/bin/python2.7


import socket
import struct

TEAM_A_NUM = 0
TEAM_A_ADDR = '127.0.0.1' #'10.13.37.55'
TEAM_A_PORT = 33997 #25668

TEAM_B_NUM = 1
TEAM_B_ADDR = '127.0.0.1' #'10.13.37.56'
TEAM_B_PORT = 22456

TEAM_NUM = {'a': TEAM_A_NUM, 'b': TEAM_B_NUM }

#SEND SHARED
PASSPHRASE = '9s'
TEAM_NUM = 'B'
SRVC_NUM = 'B'
DMG_BOOL = 'B'

#SEND
POS = 'B'
ID = 'B'
UP_OR_DOWN = 'B'
SHIELD_UP = 1
SHIELD_DOWN = 0
POS_X = POS
POS_Y = POS
DMG_TRUE = 1
SHIELD_HDR = PASSPHRASE + TEAM_NUM + SRVC_NUM + UP_OR_DOWN
DMG_HDR = PASSPHRASE + TEAM_NUM + SRVC_NUM
NAV_HDR = PASSPHRASE + TEAM_NUM + POS_X + POS_Y + 'x'

ENGINE_SRVC_OFF=0
NAVIGATION_SRVC_OFF=1
POWER_SRVC_OFF=2
WEAPONS_SRVC_OFF=3
SHIELDS_SRVC_OFF=4
COMMS_SRVC_OFF = 5

SHIELD_UP = 1
SHIELD_DOWN = 0

srvc = {
    'engine':0,
    'navigation':1,
    'power':2,
    'weapons':3,
    'shields':4,
    'comms':5}


def main():
    while True:
        while True:
            pkt_type = raw_input('shields, weapons, or navigation?: ')
            
            if pkt_type.lower() == 'shields':
                send_shield()
                break
            elif pkt_type.lower() == 'weapons':
                send_dmg()
                break
            elif pkt_type.lower() == 'navigation':
                send_pos()
                break
            else:
                print 'Invalid Command: \''+pkt_type+'\''
        
def send_shield():
    passphrase = get_passphrase()
    team_num = get_team_num()
    srvc_num = get_srvc_num()
    shield_state = get_shield_state()
    data = struct.pack(SHIELD_HDR, passphrase, team_num, srvc_num, shield_state)
    send_pkt(get_destination(), data)

DMG_HDR = PASSPHRASE + TEAM_NUM + SRVC_NUM
NAV_HDR = PASSPHRASE + TEAM_NUM + POS_X + POS_Y + 'x'

def send_dmg():
    passphrase = get_passphrase()
    team_num = get_team_num()
    srvc_num = get_srvc_num()
    data = struct.pack(DMG_HDR, passphrase, team_num, srvc_num)
    send_pkt(get_destination(), data)

def send_pos():
    passphrase = get_passphrase()
    team_num = get_team_num()
    pos = get_pos()
    data = struct.pack(NAV_HDR, passphrase, team_num, pos[0], pos[1])
    send_pkt(get_destination(), data)    

def get_destination():
    while True:
        destination = raw_input('Port [Team A or B]: ')
        if destination.isalpha():
            if destination.lower() == 'a':
                return (TEAM_A_ADDR, TEAM_A_PORT)
            elif destination.lower() == 'b':
                return (TEAM_B_ADDR, TEAM_B_PORT)
            else:
                print 'Team Destination DNE!!'
        else:
            print 'Must ba alphabetical'

def send_pkt(dst, data):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(dst)
    sock.send(data)
    sock.close()
#    print_pkt(data, 'hex')

def get_pos():
    pos_x = 0
    pos_y = 0
    while True:
        pos_x_str = raw_input('X Pos[0-40]: ')
        if pos_x_str.isdigit():
            pos_x = int(pos_x_str)
            if 0 <= pos_x <= 255:
                break
            else:
                print 'Pos must be between [0-255]'
    while True:
        pos_y_str = raw_input('Y Pos[0-40]: ')
        if pos_y_str.isdigit():
            pos_y = int(pos_y_str)
            if 0 <= pos_y <= 255:
                break
            else:
                print 'Pos must be between [0-255]'                    

    return (pos_x, pos_y)


def get_shield_state():
    while True:
        shield_state_str = raw_input('Enter State [Up:1 / Down:0]: ')
        if shield_state_str.isdigit():
            shield_state = int(shield_state_str)
            if shield_state >= 0 and shield_state <= 255:
                return shield_state
            else:
                print 'Value needs to be within [0-255]'
        else:
            print 'Not a number!!'


def get_passphrase():
    while True:
        passphrase = raw_input('Enter Passphrase [default]: ')
        if len(passphrase) == 0:
            return get_pass()
            break
        elif len(passphrase) != 9:
            print 'Passphrase must be 9 characters!!'
        else:
            return passphrase

def get_pass():
    return '123456789'

def get_srvc_num():
    while True:
        srvc_num_str = raw_input('Which Service? [name or explicit num]: ')
        if srvc_num_str.isdigit():
            srvc_num = int(srvc_num_str)
            if 0 <= srvc_num <= 255:
                return srvc_num
            else:
                print 'Team Num must be in range [0-255]'
        elif srvc_num_str.isalpha():
            correct = True
            try:
                srvc_num = srvc[srvc_num_str.lower()]
            except:
                print 'Uknown service name'
                correct = False
            if correct:
                return srvc_num
        else:
            print 'Incorrect Input'

def get_team_num():
    while True:
        team_num_str = raw_input('Enter Team Num [A:0 / B:1]: ')
        if team_num_str.isdigit():
            team_num = int(team_num_str)
            if 0 <= team_num <= 255:
                return team_num
            else:
                print 'Team Num must be in range [0-255]'
        else:
            print 'Input was not a digit'

def print_pkt(data, type):
    i = 0
    if type == 'hex':
        hex_data = data.encode('hex')
        while i < len(hex_data):
            print  hex_data[i]+hex_data[i+1]+' '
            i += 2


if __name__ == '__main__':
    main()
