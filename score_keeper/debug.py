
def check_type(self, Log=None, obj=None, type):
    try:
        if isinstance(obj, type) is True:
            return True
    except:
        Log.write(msg='error with type checking')
    return False
