#!/usr/bin/python2.7

NUM_SRVCS = 5
ENGINE_SRVC_OFF=0
NAVIGATION_SRVC_OFF=1
POWER_SRVC_OFF=2
WEAPONS_SRVC_OFF=3
SHIELDS_SRVC_OFF=4

SRVC_DOWN = False
SRVC_UP = True

class TeamInfo:
    def __init__(self, name='', id=0, key='',sk_port=0):
        self.name = name
        self.id = id
        self.key = key
        self.score = 0
        self.sk_port = sk_port
        self.services = []
        self.dmg = 0
        for spam in range(0,NUM_SRVCS):
            self.services.append(SRVC_DOWN)
            
        
class GameInfo:
    def __init__(self, time_start=0, time_end=1):
        self.TeamA = TeamInfo(name='Red', sk_port=3934)
        self.TeamB = TeamInfo(name='Blue', sk_port=2658)
        self.finished = 0
        self.time_start = time_start
        self.time_end = time_end
        self.time_left = time_end - time_start

    def time_left(self):
        return True
