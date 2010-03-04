#!/usr/bin/python

import pywbem

conn = pywbem.SFCBUDSConnection()
try: 
    classes = conn.EnumerateClassNames("root/interop")
    exit(0)
except pywbem.cim_operations.CIMError:
    print "PYWBEM Test Failure: ECN (Is httpSocketPath writable by this user?)"
    exit(1)

