#!/usr/bin/python2.7

import database

X = 0
Y = 1

SIZE = 40

TEAM_A_POS_KEY = database.TEAM_A_POS_KEY
TEAM_B_POS_KEY = database.TEAM_B_POS_KEY

class Map:
    def __init__(self, DB=None):
        self.st_map = [['   ']*SIZE for self.i in range(SIZE)]
        self.teamA_pos = (0,9)
        self.teamB_pos = (10,1)
        self.DB = DB
        
    def refresh_screen(self):
        self.clean_marks()
        self.teamA_pos = self.DB.get_pos(TEAM_A_POS_KEY)
        self.teamB_pos = self.DB.get_pos(TEAM_B_POS_KEY)
        self.mark_ships()

    def mark_ships(self):
        if self.teamA_pos ==  self.teamB_pos:
            self.st_map[self.teamA_pos[X]][self.wrap(self.teamA_pos[Y])] = '{0}'
        else:
            self.st_map[self.teamA_pos[X]][self.wrap(self.teamA_pos[Y])] = '<A>'
            self.st_map[self.teamB_pos[X]][self.wrap(self.teamB_pos[Y])] = '[B]'

    def clean_marks(self):
        self.st_map[self.teamA_pos[X]][self.wrap(self.teamA_pos[Y])] = '   '
        self.st_map[self.teamB_pos[X]][self.wrap(self.teamB_pos[Y])] = '   '

    def wrap(self, n):
        return n
        #return ((n - (SIZE-1))*(-1))
    
    def print_map(self):
        cols = '   '
        rvr_range = range(0,40)
        rvr_range.reverse()
        for colm in range(0,40):
            cols += str(colm).zfill(2)+' '
        print cols
        for row in rvr_range:
            line = str(row).zfill(2)+':'
            for colm in range(0,40):
                line += self.st_map[colm][row]
            print line

        print cols
