#!/bin/python

def set_args_64(i):
    return """
####################
## SET ARGUMENT """ + str(i) + """ ##
####################

set_arg""" + str(i) + """:
    srli    t1, t0, FFI_FLAG_BITS_X""" + str(i) + """ # Shift to get the bits for this argument
    andi    t1, t1, 3               # Mask out the bits for this argument
    
    # when its zero, it means its just a word-sized int/ptr
    bne     t1, zero, set_arg""" + str(i) + """_float 
    REG_L   a""" + str(i) + """, FFI_SIZEOF_ARG_X""" + str(i) + """(sp) # load argument
    j       set_arg""" + str(i+1) + """
    
set_arg""" + str(i) + """_float:
    addi    t1, t1, -1
    
    # when its zero, it means its just a word-sized float
    bne     t1, zero, set_arg""" + str(i) + """_double
    flw     fa""" + str(i) + """, FFI_SIZEOF_ARG_X""" + str(i) + """(sp) # load argument
    j       set_arg""" + str(i+1) + """
    
set_arg""" + str(i) + """_double:
    # otherwise it must be a double we're dealing with
    fld     fa""" + str(i) + """, FFI_SIZEOF_ARG_X""" + str(i) + """(sp)
"""

def gen64_setargs_string():
    setargs = ""
    for i in range(8):
        setargs += set_args_64(i)
    return setargs

print gen64_setargs_string()
