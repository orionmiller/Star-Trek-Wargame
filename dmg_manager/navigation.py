#!/usr/bin/python2.7

class Navigation:
    def __init__(self):
        self.pos = {'x':0 ,'y':0}

    def cur_pos(self):
        return (self.pos['x'], self.pos['y'])

    def move(self,x,y):
        self.pos['x'] = x
        self.pos['y'] = y
