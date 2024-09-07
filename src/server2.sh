#!/bin/bash
# script para compilacion de aplicacion de server2
# 
# Atte. Guillermo Cherencio
#
gcc -Wall -o ../wserver/server2 server2.c util.c -I ../include -lpthread -ldl

