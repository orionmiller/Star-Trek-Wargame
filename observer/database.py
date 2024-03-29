#!/usr/bin/python2.7

import redis

POS_X_OFF = 0
POS_Y_OFF = 1

TEAM_A_POS_KEY = 'team:a-pos'
TEAM_B_POS_KEY = 'team:b-pos'

TEAM_SCORE_OFF=1


GAME_KEY = 'game:1'
TEAM_A_KEY = 'team:a-'+GAME_KEY
TEAM_B_KEY = 'team:b-'+GAME_KEY

class DataBase:
    def __init__(self):
        self.r_server = None
        
    def connect(self):
        try:
            self.r_server = redis.Redis('localhost')
            print 'connected to server'
        except redis.RedisError as error:
            #check to see how i can pull more informationf
            print('Error Connecting to the redis server.\n')
            print(error)
            return None

    def init_pos(self, KEY):
        self.r_server.delete(KEY)
        self.r_server.rpush(KEY, 0)
        self.r_server.rpush(KEY, 0)

    def set_pos(self, KEY, pos_x, pos_y):
        self.r_server.lset(KEY, POS_X_OFF, pos_x)
        self.r_server.lset(KEY, POS_Y_OFF, pos_y)

    def get_pos(self, KEY):
        pos_temp = self.r_server.lrange(KEY, 0, 1)
        x = int(pos_temp[0])
        y = int(pos_temp[1])
        return (x,y)

    def get_score(self, team_key=''):
        if team_key != '':
           score_set = self.r_server.lrange(team_key,TEAM_SCORE_OFF,TEAM_SCORE_OFF)
           return int(score_set[0])
        
    
