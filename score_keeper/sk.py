#!/usr/bin/python2.7

###CONNECT WORKS###

import time
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
        self.Log = log.Log(filename='/home/commander/score-keeper.log')
        #self.Log = log.Log(filename='/home/sysmtkor/score-keeper.log')
        self.Game = game.GameInfo(time.time(),time.time()+game_time) #change for official code
        self.DB = database.DataBase(self.Game,self.Log)

    def new_game(self):
        self.DB.new_game()

    def connect(self):
        self.DB.connect()

    def service_down(self, team_info, team_getting_points):
        self.DB.new_action(team_info, action=database.ACTION['service_down'])
        self.set_score(team_getting_points, database.ACTION['service_down'])

    def get_score(self, TeamInfo=None):
        return self.DB.get_score(TeamInfo.key)

    def ship_dmg(self, team_info, team_getting_points):
        self.DB.new_action(team_info, action=database.ACTION['weapon_dmg'])
        self.set_score(team_getting_points, database.ACTION['weapon_dmg'])

    def finished_lvl(self, team_info, team_getting_points):
        self.DB.new_action(team_info, action=database.ACTION['lvl_complete'])
        self.set_score(team_getting_points, database.ACTION['lvl_complete'])

    def update_time_left(self):
        self.DB.update_time_left()

    def new_end_time(self, new_end_time):
        self.Game.time_end = new_end_time
        self.DB.set_end_time()

    def new_start_time(self, new_start_time):
        self.Game.time_start = new_start_time
        self.DB.set_start_time()

    def update_scores(self):
        print 'UPDATING SCORES'
        for x in range(0,len(self.Game.TeamA.services)):
            if self.Game.TeamA.services[x] is SRVC_DOWN:
                self.service_down(self.Game.TeamA, self.Game.TeamB)
            else:
                self.Game.TeamA.services[x] = SRVC_DOWN
        
        while self.Game.TeamA.dmg > 0:
            self.ship_dmg(self.Game.TeamA, self.Game.TeamB)
            self.Game.TeamA.dmg -= 1

        for lvl in self.Game.TeamA.lvls:
            if lvl is game.LVL_COMPLETE:
                self.finished_lvl(self.Game.TeamA, self.Game.TeamB)
                lvl = game.LVL_SCORED

        for x in range(0,len(self.Game.TeamB.services)):
            if self.Game.TeamB.services[x] is SRVC_DOWN:
                self.service_down(self.Game.TeamB, self.Game.TeamA)
            else:
                self.Game.TeamB.services[x] = SRVC_DOWN
                
        while self.Game.TeamB.dmg > 0:
            self.ship_dmg(self.Game.TeamB, self.Game.TeamA)
            self.Game.TeamA.dmg -= 1

        for lvl in self.Game.TeamB.lvls:
            if lvl is game.LVL_COMPLETE:
                self.finished_lvl(self.Game.TeamB, self.Game.TeamA)
                lvl = game.LVL_SCORED

    def set_score(self, team_info, action):
        self.DB.set_score(team_info.key, action[1])
