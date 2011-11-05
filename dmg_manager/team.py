#!/usr/bin/python2.7

import shields
import navigation

NUM_SRVCS = 6
ENGINE_SRVC_OFF=0
NAVIGATION_SRVC_OFF=1
POWER_SRVC_OFF=2
WEAPONS_SRVC_OFF=3
SHIELDS_SRVC_OFF=4
COMMS_SRVC_OFF = 5

class Team:
    def __init__(self):
        self.Shields = shields.Shields()
        self.Navigation = navigation.Navigation()
        self.dmg = 0

    def shield_up(self, srvc_num):
        self.Shields.shield_up(srvc_num)

    def shield_down(self, srvc_num):
        self.Shields.shield_down(srvc_num)

    def is_shield_up(self, srvc_num):
        return self.Shields.is_shield_up(srvc_num)

    def incr_dmg(self):
        self.dmg += 1

    def curr_pos(self):
        return self.Navigation.cur_pos()

    def move(self, x, y):
        self.Navigation.move(x,y)

    def print_state(self):
        self.print_pos()
        self.print_shields_states()
        print

    def print_pos(self):
        pos = self.curr_pos()
        print 'Position: ('+str(pos[0])+','+str(pos[1])+')'

    def print_shields_states(self):
        for srvc_num in range(0,NUM_SRVCS):
            print self.shield_state_toStr(srvc_num)

    def shield_state_toStr(self, srvc_num):
        string = srvc_str[srvc_num]
        if self.is_shield_up(srvc_num):
            string += ': Up'
        else:
            string += ': Down'
        return string

srvc_str = {
    0:'engine',
    1:'navigation',
    2:'power',
    3:'weapons',
    4:'shields',
    5:'comms'}


        
#get position
