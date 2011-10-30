#!/usr/bin/python2.7
import os
import time

class Log:

    def __init__(self, filename='', print_to_screen=False):
        self.filename = filename
        self.print_to_screen=print_to_screen
        try:
            self.log = os.open(self.filename, os.O_WRONLY|os.O_CREAT)
        except:
            print('Error opening log file: \''+self.filename+'\'')

    def write(self, msg=''):
        log_msg = str(str(time.time())+' : '+msg+'\n')
        self.log.write(log_msg)
        self.log.flush() #may decrease performance
        if self.print_to_screen is True:
            print(log_msg)

    def current_file(self):
        return self.log_filename

    def new_file(self, filename=''):
        #save change of log file to old log file
        try:
            self.log_filename = filename
            self.open()
        except:
            self.write(msg='exception caught')
    
    def close(self):
        os.close(self.log)

    def open(self):
        try:
            self.log = open(self.filename, os.O_WRONLY|os.O_CREAT)
        except:
            print('Error opening log file\n')
 

#make local variables "priv." if possible
#add error checking
#add possibly a way to check current full path
