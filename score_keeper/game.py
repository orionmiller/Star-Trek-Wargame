class TeamInfo:
    def __init__(self, name='', id=0, key=''):
        self.name = name
        self.id = id
        self.key = key
        self.score = 0
        
class GameInfo:
    def __init__(self, time_start=0, time_end=1):
        self.TeamA = TeamInfo(name='Red')
        self.TeamB = TeamInfo(name='Blue')
        self.finished = 0
        self.time_start = time_start
        self.time_end = time_end
        self.time_left = time_end - time_start

    def time_left(self):
        return True
