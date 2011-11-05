#!/usr/bin/python2.7

import database
import signal


height = 40

TEAM_A_POS_KEY = 'team:a-pos'
TEAM_B_POS_KEY = 'team:b-pos'

teamA_pos = [0,0]
teamB_pos = [0,0]

INTERV = 2

X = 0
Y = 1

st_map = []

DB = database.DataBase()
DB.connect()
#print st_map


def main():
    global st_map
    st_map = [[' ']*40 for i in range(40)]
    signal.signal(signal.SIGALRM, update_view)
    signal.alarm(INTERV)
    print_map()

    while True:
        pass

def update_view(signum, frame):
    global st_map
    st_map[teamA_pos[X]][(teamA_pos[Y]-39)*(-1)] = ' '
    st_map[teamB_pos[X]][(teamB_pos[Y]-39)*(-1)] = ' '
    
    teamA_pos_t = DB.get_pos(TEAM_A_POS_KEY)
    
    teamB_pos_t = DB.get_pos(TEAM_B_POS_KEY)
        
    teamA_pos[X] = int(teamA_pos_t[X])
    teamA_pos[Y] = int(teamA_pos_t[Y])
    teamB_pos[X] = int(teamB_pos_t[X])
    teamB_pos[Y] = int(teamB_pos_t[Y])
    
    if teamA_pos[X] == teamB_pos[X] and teamA_pos[Y] == teamB_pos[Y]:
        st_map[teamA_pos[X]][teamA_pos[Y]] = 'O'
    else:
        st_map[teamA_pos[X]][(teamA_pos[Y]-39)*(-1)] = 'A'
        st_map[teamB_pos[X]][(teamB_pos[Y]-39)*(-1)] = 'B'
    print_map()
    signal.alarm(INTERV)

        
def print_map():
    global st_map
    print 'st_map len: '+str(len(st_map))
    for row in range(0,40):
        line = ''
        for colm in range(0,40):
            line += st_map[row][colm]
        print row

if __name__ == '__main__':
    main()
