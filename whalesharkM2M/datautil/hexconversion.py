import binascii


def str2hex(arg_str):
    """
    Return Converted hex values from String
    """
    fun_id = arg_str.encode('ASCII')
    hex_fun_id = binascii.hexlify(fun_id)#return byte type
    hex_fun_id = hex_fun_id.decode('ASCII')
    return hex_fun_id

def hex2str(arg_hex):
    """
    Return Converted ASCII String from hex values
    """
    #Hex to String
    fun_id_byte = bytes.fromhex(arg_hex)
    fun_id_str = fun_id_byte.decode('ASCII')
    return fun_id_str

def int2hex(arg_int):
    """
    Return integer values from hex values
    """
    return hex(arg_int)

def hex2int(arg_hex):
    """
    Return hex value from integer values
    """
    return int(arg_hex, 16)