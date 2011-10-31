#!/usr/bin/python2.7

###CONNECT WORKS###

import sys
import time
import redis
import log
import game
import database

VALID = True
INVALID = False

SRVC_DOWN = False
SRVC_UP = True

game_time = 3 * 60 * 60


class ScoreKeeper:
    def __init__(self):
        self.Log = log.Log(filename='/home/systmkor/Projects/st_ctf/score-keeper.log')
        self.Game = game.GameInfo(time.time(),time.time()+game_time) #change for official code
        self.DB = database.DataBase(self.Game,self.Log)
        self.DB.connect()

    def new_game(self):
        self.DB.new_game()

    def service_down(self, team_info):
        self.DB.new_action(team_info, action=ACTION[service_down])

    def get_score(self, TeamInfo=None):
        return self.DB.get_score(TeamInfo.key)

    def ship_dmg(self, team_info):
        self.DB.new_action(team_info, action=ACTION[weapon_dmg])

    def update_time_left(self):
        self.DB.update_time_left()

    def new_end_time(self, new_end_time):
        self.Game.time_end = new_end_time
        self.DB.set_end_time()

    def new_start_time(self, new_start_time):
        self.Game.time_start = new_start_time
        self.DB.set_start_time()

    def update_score(self):
        for x in range(0,len(Game.TeamA.services)):
            if self.Game.TeamA.services[x] is SRVC_DOWN:
                service_down(Game.TeamA)
            else:
                self.Game.TeamA.services[x] = SRVC_DOWN
        
        while Game.TeamA.dmg > 0:
            ship_dmg(team_info=Game.TeamA)
            self.Game.TeamA.dmg -= 1

        for x in range(0,len(Game.TeamB.services)):
            if self.Game.TeamB.services[x] is SRVC_DOWN:
                self.service_down(team_info=Game.TeamB)
            else:
                self.Game.TeamB.services[x] = SRVC_DOWN
                
        while Game.TeamB.dmg > 0:
            self.ship_dmg(team_info=Game.TeamB)
            self.Game.TeamA.dmg -= 1
