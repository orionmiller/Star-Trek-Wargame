#!/usr/bin/python2.7

import math

X = 0
Y = 1
MAX_RANGE = 3

class Weapons:
    def __init__(self, Map):
        self.Map = Map
        self.range = MAX_RANGE

    def in_range(self,TeamA, TeamB):
        teamA_pos = TeamA.curr_pos()
        teamB_pos = TeamB.curr_pos()
        positions = self.gen_positions(teamA_pos, teamB_pos, self.Map.size)
        while len(positions) > 0:
            posX , posY = positions.pop()
            pposX, pposY = positions.pop()
            d = self.distance(posX, posY, pposX, pposY)
            if d <= self.range:
                return True
        return False


    def distance(self, teamA_posX, teamA_posY, teamB_posX, teamB_posY):
        dx = (teamA_posX - teamB_posX)
        dy = (teamA_posY - teamB_posY)
        return math.sqrt(dx*dx + dy*dy)
    
    def gen_positions(self, posA, posB, size):
        posX = posA[0]
        posY = posA[1]
        pposX = posB[0]
        pposY = posB[1]
        positions = []

        positions.append((posX, posY))
        positions.append((pposX, pposY))

        positions.append((posX, posY))
        positions.append((pposX - (size), pposY - (size)))

        positions.append((posX, posY))
        positions.append((pposX, pposY - (size)))

        positions.append((posX, posY))
        positions.append((pposX - (size), pposY))

        return positions
