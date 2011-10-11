#notes
# db_check and db_fix
#    code functional for what it did before but is hocky and needs retouching


TEAM_ID_INIT = 0
GAME_ID_INIT = 0
ACTION_ID_INIT = 0
GAME_OVER = 1
TYPE = 0
DELTA_SCORE = 1

KEY_TYPE = 1
KEY = 0
INIT_VALUE = 2

ACTION={
    'null':(0,0),
    'engage':(1,0),
    'spam':(2,8),
    'eggs':(3,4)}

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
        self.r_server = redis.Redis('localhost')
        Self.Log = Log
        
    def connect(self):
        try:
            self. r_server = redis.Redis("localhost")
        except redis.RedisError as error:
            #check to see how i can pull more informationf
            print('Error Connecting to the redis server.\n')
            print(error)
            return None

    #check if base ids are strings
    def db_check(self):
        db_pass = True
        for curr_key in DB_KEYS: #x_key is shit name
            if check_key(key=curr_key[KEY], key_type=curr_key[KEY_TYPE]) is False: #magic numbers
                db_pass = False
                db_key_state.push(curr_key)
                #append key to fix !!!!NEED TO KNOW WHAT TO FIX I
        return db_pass


    def db_fix(self):
        while db_keys_fix.count() > 1: #magic number
            curr_key = db_keys_fix.pop()
            self.r_server.remove(curr_key[KEY])
            if curr_key[INIT_VALUE] is not None:
                self.r_server.set(key[KEY], key[INIT_VALUE])

    def new_game(self):
        try:
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
            self.r_server.rpush(self.Game.TeamA.key, self.Game.TeamB.name)
            self.r_server.rpush(self.Game.TeamA.key, self.Game.TeamB.score)
            
            self.new_action(self.Game.TeamA, action_type['engage'])
            self.new_action(self.Game.TeamB, action_type['engage'])
        except:
            self.Log.write('error while generating new game' + exc)

    def finish_game(self):
        self.r_server.lset(self.Game.key, 0, GAME_OVER) #magic number

    def check_key(self, key='', key_type=''): #renaming of key types expected and actual
        if self.r_server.exists(key) is True:
            if (type=self.r_server.type(key)) is key_type:
                return True

            self.Log.write('key:\''+key'\'-expected type:\''+key_type+
                           '\' actual type:\''+type+'\'')
            return False

        self.Log.write('key:\''+key+'\' does not exist in the database.')
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
        else:
            self.Log.write('Bad call of new_action with no team info.')


    def check_score(score):
        if isinstance(score, int) and score >= 0:
            return valid
        else:
            return invalid

    def set_score(self, points_change):
    #old_score access database's score
        new_score = old_score + points_change
    #modify score
        if(check_score(new_score)):
        #database score update
            return valid
        else:    
            return invalid

    def get_score(self, TeamInfo=None):
        if TeamInfo is not None:
            
    def update_time_left(self):
        return True

    def update_time_end(self, time_end=1):
        return False

    def update_time_start(self, time_start=0):
        return False


# NEW GAME PSEUDO CODE
# ::Game::
# add to the front of game queue new game
# pull from auto-incrment game-id / fill in game-id
# pull from web new game wizard start-time / fill in start time
# pull from web new game wizard end-time / fill in end time
# calculate time-left / fill in time left
# set finished to false

# ::Red Team::
# pull from current game-id fill in game-id
# pull from auto-incrment team-id fill in team-id
# pull from web new game wizard red team name fill in name
# fill in score to 0

# ::Blue Team::
# pull from current game-id fill in game-id
# pull from auto-incrment team-id fill in team-id
# pull from web new game wizard blue team name fill in name
# fill in score to 0

# ::Red Team 1st Activity::
# pull from pythons utc/epoch fill in time stamp
# fill in action id with genesis/engage id
# fill in red-teams id
# fill in blue-teams change-in-score 0

# ::Blue Team 1st Activity::
# pull from pythons utc/epoch fill in time stamp
# fill in action id with genesis/engage id
# fill in red-teams id
# fill in blue-teams change-in-score 0


# possibly put in try...exception block to handle errors
# check to see if base keys exist (team-id, game-id, games, teams, activity)
# if not create them
# check to see if time has passed
# check to see if finished
# make sure team-id is not bellow other team-ids
# make sure gamke-id is not bellow other game-ids
# make sure the scores 'balance'


# if self.r_server.exists('team_id') is False:
#     self.r_server.set('team_id', TEAM_ID_INIT)
# else:
#     self.r_server.type('team_id') is not 'string'
#     print('team_id exists but is the wrong type')#needs to be a properly logged comment
# if self.r_server.exists("game_id") is False:
#     self.r_server.set("game_id", GAME_ID_INIT)
# else:
#     self.r_server.type('action_id') is not 'string'
#     print('team_id exists but is the wrong type')#needs to be a properly logged comment
# if self.r_server.exists("action_id") is False:
#     if self.r_server.type('games') != 'list':
#     self.r_server.set("action_id", ACTION_ID_INIT)
# else:
#     self.r_server.type('action_id') is not 'string'
#     print('team_id exists but is the wrong type')#needs to be a properly logged comment
# if self.r_server.exists('games') is True:
#     if self.r_server.type('games') != 'list':
#         print('Error w/ Db - games list is off')#needs to be a properly logged comment
# if self.r_server.exists('actions') is True:
#     if self.r_server.type('games') != 'list':
#         print('Error w/ Db - actions list is off')#needs to be a properly logged comment
# if self.r_server.exists('teams') is True:
#     if self.r_server.type('games') != 'list':
#         print('Error w/ Db - teams list is off')#needs to be a properly logged comment
