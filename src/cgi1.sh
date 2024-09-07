#!/bin/bash
# script para compilacion de aplicacion de cgi1
# 
# Atte. Guillermo Cherencio
#
gcc -Wall -o ../cgi-bin/cgi1.cgi cgi1.c cgi.c -I ../include

