#!/usr/bin/python2.7
import sys
import os
import math


MAX_RANGE = 3

def distance(teamA_posX, teamA_posY, teamB_posX, teamB_posY):
    dx = (teamA_posX - teamB_posX)
    # print 'dx = '+str(dx)
    dy = (teamA_posY - teamB_posY)
    # print 'dy = '+str(dy)
    # print 'dx * dx = '+str(dx*dx)
    # print 'dy * dy = '+str(dy*dy)
    # print 'dy^2 + dy^2 = '+str(dx*dx + dy*dy)
    return math.sqrt(dx*dx + dy*dy)

def gen_positions(posX, posY, pposX, pposY, size):
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


while True:
    in_range = False
    size = int(raw_input('Map Square Size: '))
    Range = int(raw_input('Max Range: '))
    if size > 0:
        teamA_posX = int(raw_input('Team A -- Pos X: '))
        teamA_posY = int(raw_input('Team A -- Pos Y: '))
        teamB_posX = int(raw_input('Team B -- Pos X: '))
        teamB_posY = int(raw_input('Team B -- Pos Y: '))
        positions = gen_positions(teamA_posX, teamA_posY, teamB_posX, teamB_posY, size)
        while len(positions) > 0:
            posX , posY = positions.pop()
            pposX, pposY = positions.pop()
            d = distance(posX, posY, pposX, pposY)
            print ('distance: '+str(d))
            if d <= Range:
                in_range = True
                
        if in_range:
            print 'Ships are within range'

        else:
            print 'Ships are not in range'

    else:
        print 'Map Square Size needs to be  > 0'
