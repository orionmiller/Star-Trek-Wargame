#!/usr/bin/python2.7

class Weapons:
    def __init__(self, Map):
        self.Map = Map
        self.range = MAX_RANGE

    def in_range(TeamA, TeamB):
        positions = self.gen_positions(TeamA.x, teamA_posY, teamB_posX, teamB_posY, Map.size)
        while len(positions) > 0:
            posX , posY = positions.pop()
            pposX, pposY = positions.pop()
            d = distance(posX, posY, pposX, pposY)
            if d <= Range:
                return True


    def distance(teamA_posX, teamA_posY, teamB_posX, teamB_posY):
        dx = (teamA_posX - teamB_posX)
        dy = (teamA_posY - teamB_posY)
        return math.sqrt(dx*dx + dy*dy)
    
    def gen_positions(posA, posB, size):
        posX = posA[0]
        posY = posA[1]
        pposX = posB[0]
        pposY = posB[0]
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
