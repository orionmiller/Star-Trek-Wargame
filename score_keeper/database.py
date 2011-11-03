#!/usr/bin/python2.7

import redis
import time

TEAM_ID_INIT = 0
GAME_ID_INIT = 0
ACTION_ID_INIT = 0
GAME_OVER = 1
TYPE = 0
DELTA_SCORE = 1

KEY_TYPE = 1
KEY = 0
INIT_VALUE = 2

TEAM_NAME_OFF=0
TEAM_SCORE_OFF=1

TIME_LEFT_OFF=1
TIME_START_OFF=2
TIME_END_OFF=3


VALID=True
INVALID=False

#1st is uniqe action id 2nd is score change
ACTION={
    'null':(0,0),
    'engage':(1,0),
    'game_over':(2,0),
    'service_down':(3,10),
    'weapon_dmg':(4,1),
    'lvl_complete':(5,15)}

DB_KEYS=(
    ('game_id', 'string', GAME_ID_INIT),
    ('team_id', 'string', TEAM_ID_INIT),
    ('action_id', 'string', ACTION_ID_INIT),
    ('games', 'list', None),
    ('teams', 'list', None),
    ('actions', 'list', None))

DB_KEYS_FIX=[]

class DataBase:
    def __init__(self, GameInfo=None, Log=None):
        self.r_server = None
        self.Log = Log
        self.Game = GameInfo
        
    def connect(self):
        try:
            self.r_server = redis.Redis('localhost')
            print 'connected to server'
        except redis.RedisError as error:
            #check to see how i can pull more informationf
            print('Error Connecting to the redis server.\n')
            print(error)
            return None

    #check if base ids are strings
    def db_check(self):
        print 'checking database'
        db_pass = True
        for curr_key in DB_KEYS: 
            if self.check_key(key=curr_key[KEY], expected_type=curr_key[KEY_TYPE]) is False:
                db_pass = False
                DB_KEYS_FIX.append(curr_key)
        return db_pass


    def db_fix(self):
        print 'fixing database'
        while len(DB_KEYS_FIX) > 1: #magic number
            curr_key = DB_KEYS_FIX.pop()
            #self.r_server.remove(curr_key[KEY])
            if curr_key[INIT_VALUE] is not None:
                self.r_server.set(curr_key[KEY], curr_key[INIT_VALUE])

    def new_game(self):
        print 'new game'
        if self.r_server is not None:
            self.db_check()
            self.db_fix()
            
            self.r_server.incr('game_id')
            self.Game.id = self.r_server.get('game_id')
            self.Game.key = 'game:'+str(self.Game.id)
            self.r_server.rpush(self.Game.key, self.Game.finished)
            self.r_server.rpush(self.Game.key, self.Game.time_left)
            self.r_server.rpush(self.Game.key, self.Game.time_start)
            self.r_server.rpush(self.Game.key, self.Game.time_end)
            self.r_server.lpush('games', self.Game.key)
            
            self.r_server.incr('team_id')
            self.Game.TeamA.id = self.r_server.get('team_id')
            self.Game.TeamA.key = 'team:' + str(self.Game.TeamA.id) + '-' + self.Game.key
            self.r_server.rpush(self.Game.TeamA.key, self.Game.TeamA.name)
            self.r_server.rpush(self.Game.TeamA.key, self.Game.TeamA.score)
        
            self.r_server.incr('team_id')
            self.Game.TeamB.id = self.r_server.get('team_id')
            self.Game.TeamB.key = 'team:' + str(self.Game.TeamB.id) + '-' + self.Game.key
            self.r_server.rpush(self.Game.TeamB.key, self.Game.TeamB.name)
            self.r_server.rpush(self.Game.TeamB.key, self.Game.TeamB.score)
            
            self.new_action(self.Game.TeamA, ACTION['engage'])
            self.new_action(self.Game.TeamB, ACTION['engage'])
            print 'new game created'
        else:
            print 'error'
            self.Log.write(msg='new_game: r_server was none') #used to have +exc

    def finish_game(self):
        self.r_server.lset(self.Game.key, 0, GAME_OVER) #magic number

    def check_key(self, key='', expected_type=''): #renaming of key types expected and actual
        if self.r_server.exists(key) is True:
            key_type = self.r_server.type(key)
            if key_type is expected_type:
                return True

            self.Log.write(msg='key:\''+key+'\'-expected type:\''+expected_type+
                           '\' actual type:\''+key_type+'\'')
            return False

        self.Log.write(msg='key:\''+key+'\' does not exist in the database.')
        return False
            
    def new_action(self, team_info=None, action=ACTION['null']):
        if team_info is not None:
            self.r_server.incr('action_id')
            action_id = self.r_server.get('action_id')
            action_key = 'action:' + str(action_id) + '-' + self.Game.key
            self.r_server.rpush(action_key, time.time())
            self.r_server.rpush(action_key, action[TYPE])
            self.r_server.rpush(action_key, team_info.id)
            self.r_server.lpush('actions', action_key)
            self.set_score(team_info.key,action[1]) #magic number
        else:
            self.Log.write(msg='Bad call of new_action with no team info.')


    def check_score(self,score):
        if isinstance(score, int) and score >= 0:
            return VALID
        else:
            return INVALID

    def set_score(self, team_key='', points_change=0):
        old_score = self.get_score(team_key)
        new_score = old_score + points_change
        if(self.check_score(new_score)):
            self.r_server.lset(team_key,TEAM_SCORE_OFF,new_score)

    def get_score(self, team_key=''):
        if team_key is not '':
           score_set = self.r_server.lrange(team_key,TEAM_SCORE_OFF,TEAM_SCORE_OFF)
           print 'team_key: \''+team_key+'\''
           return int(score_set[0])

    def set_time_left(self):
        self.r_server.lset(self.Game.key, TIME_LEFT_OFF, self.Game.time_left)

    def get_time_left(self):
        time_left = self.r_server.lrange(self.Game.key, TIME_LEFT_OFF, TIME_LEFT_OFF)
        return float(time_left[0])

    def update_time_left(self):
        time_left=self.get_end_time() - time.time()
        if time_left >= 0:
            self.Game.time_left = time_left
        else:
            self.Game.time_left = 0
        self.set_time_left()

    def set_end_time(self):
        self.r_server.lset(self.Game.key, TIME_END_OFF, self.Game.time_end)

    def get_end_time(self):
        time_end = self.r_server.lrange(self.Game.key, TIME_END_OFF, TIME_END_OFF)
        return float(time_end[0])

    def set_time_start(self, time):
        self.r_server.lrange(self.Game.key, TIME_START_OFF, self.Game.time_start)

    def get_time_start(self):
        time_start = self.r_server.lrange(self.Game.key, TIME_START_OFF, TIME_START_OFF)
        return float(time_start[0])

