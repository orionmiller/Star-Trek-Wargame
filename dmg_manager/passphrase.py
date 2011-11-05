#!/usr/bin/python2.7

TEAM_A_PASS = 0
TEAM_B_PASS = 1
LOCAL_PASS = 2

passphrases = [ 'K8$WjmsNZ', '3@xYdTFGB', 'TaF6uZv&h']

def correct_pass(passphrase, id):
    if passphrase == passphrases[id]:
        return True
    return False

def get_pass(id):
    return passphrases[id]
