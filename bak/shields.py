#!/usr/bin/python2.7


import time

NUM_SHIELDS = 5
ENGINE_SHIELD=0
NAVIGATION_SHIELD=1
POWER_SHIELD=2
WEAPONS_SHIELD=3
SHIELDS_SHIELD=4

SHIELD_UP = True
SHIELD_DOWN = False



MAX_RANGE = 3


class Shields:
    def __init__(self):
        self.shield = []
        for eggs in range(0,NUM_SHIELDS):
            shield.append(SHIELD_DOWN)

    def shield_up(srvc_num):
        return shield[srvc_num]
        


    

