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



class ScoreKeeper:
    def __init__(self):
        self.Log = log.Log(filename='/home/systmkor/Projects/st_ctf/score-keeper.log')
        self.Game = game.GameInfo()
        self.DB = database.DataBase(self.Game,self.Log)


    def new_game(self):
        self.DB.new_game()

    def service_down(self, team_info):
        DB.new_action(team_info, action=ACTION[service_down])


    def ship_dmg(self, team_info):
        DB.new_action(team_info, action=ACTION[weapon_dmg])

    def update_score(self):
        for x in range(0,len(Game.TeamA.services)):
            if Game.TeamA.services[x] is SRVC_DOWN:
                service_down(Game.TeamA)
            else:
                Game.TeamA.services[x] = SRVC_DOWN
        
        while Game.TeamA.dmg > 0:
            ship_dmg(team_info=Game.TeamA)
            Game.TeamA.dmg -= 1

        for x in range(0,len(Game.TeamB.services)):
            if Game.TeamB.services[x] is SRVC_DOWN:
                service_down(team_info=Game.TeamB)
            else:
                Game.TeamB.services[x] = SRVC_DOWN
                
        while Game.TeamB.dmg > 0:
            ship_dmg(team_info=Game.TeamB)
            Game.TeamA.dmg -= 1
