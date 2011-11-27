#!/usr/bin/python2.7

import redis

POS_X_OFF = 0
POS_Y_OFF = 1

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
        self.r_server.lrange(KEY, 0, 1)
        
