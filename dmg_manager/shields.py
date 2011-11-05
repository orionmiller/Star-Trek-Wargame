#!/usr/bin/python2.7


import time

NUM_SHIELDS = 6
ENGINE_SHIELD=0
NAVIGATION_SHIELD=1
POWER_SHIELD=2
WEAPONS_SHIELD=3
SHIELDS_SHIELD=4
COMMS_SHIELD=5

SHIELD_UP = True
SHIELD_DOWN = False


MAX_RANGE = 3


class Shields:
    def __init__(self):
        self.shield = []
        for eggs in range(0,NUM_SHIELDS):
            self.shield.append(SHIELD_DOWN)

    def is_shield_up(self,srvc_num):
        return self.shield[srvc_num]

    def shield_up(self,srvc_num):
        self.shield[srvc_num] = SHIELD_UP

    def shield_down(self,srvc_num):
        self.shield[srvc_num] = SHIELD_DOWN
    

