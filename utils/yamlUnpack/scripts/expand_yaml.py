#!/usr/bin/env python  
from pprint import pprint as pp  
from yaml import load,load_all,dump  
import yaml
import os
import sys

if __name__ == "__main__":
        import getopt
        try:
                optlist, args = getopt.getopt(sys.argv[1:], 'hf:s:',
                        ['help', 'file='])
        except getopt.GetoptError, err:
                print(str(err))
                usage(sys.argv[0])
                sys.exit(2)
        for o,arg in optlist:
                if o in ('-h', '--help'):
                        usage(sys.argv[0])
                        sys.exit()
                elif o in ('-s', '--save'):
			s = arg
                elif o in ('-f', '--file'):
			f = arg
try:
    device = load( open( f ) )
    out = open( s, 'w' )
    if not device:
                print("No device specified")
                usage(sys.argv[0])
                sys.exit(1)

    pp(device['NetIODev'], stream=out) 

except yaml.YAMLError as err:
    print("YAML Error " + str(err)) 

#print(hash['AmcCarrier'])
