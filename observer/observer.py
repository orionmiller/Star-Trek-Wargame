#!/usr/bin/python2.7

import database
import signal
import st_map

INTERV = 30 #Seconds

DB = database.DataBase()
DB.connect()

Map = st_map.Map(DB)

def main():
    signal.signal(signal.SIGALRM, refresh)
    signal.alarm(INTERV)

    while True:
        pass

def score_board():
    score_board_str = 'TEAM A - '+str(DB.get_score(database.TEAM_A_KEY))+' '
    score_board_str += ':: TEAM B - '+str(DB.get_score(database.TEAM_B_KEY))
    print score_board_str
        
def refresh(signum, frame):
    if signum == signal.SIGALRM:
        score_board()
        Map.refresh_screen()
        Map.print_map()
        signal.alarm(INTERV)
    

if __name__ == '__main__':
    main()
