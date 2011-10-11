#!/usr/bin/python2.7

###CONNECT WORKS###

import sys
import time
import redis
import log

valid = True
invalid = False

class ScoreKeeper:
    def __init__(self):
        self.Log = log.Log(filename='~/Projects/ctf-star_trek/test/')
        self.Game = GameInfo()
        self.DB = DataBase(Game,Log)

    def check_services():
      #the over arching main function of running through all the services and
    #  checking to see if they still are up and/or fucntional
        check_power()
        return False

    def new_game(self):
        self.DB.new_game()

    def check_power(TeamInfo):
#check to see if port/service is up
#if service is up
#  while(not at end of checklist) #do a functionality check
#    if service is up
#      check particular functionality
#      if functional do nothing
#      else
#        check to see if it is still up
#        modify activity log and modify points change
#else
#check to see how long it has been down if longer than 20s 
#  update activity log & points
        return False
    
    def ping(destination='localhost'):
        return True

