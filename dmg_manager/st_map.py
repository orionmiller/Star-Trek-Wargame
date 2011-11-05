#!/usr/bin/python2.7

DEFUALT_SIZE = 40

class Map:
    def __init__(self, size=DEFUALT_SIZE):
        self.size = size
        self.x_max = size - 1
        self.y_max = size - 1

