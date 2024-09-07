#!/bin/bash
# script para compilacion de aplicacion app3
#
# Atte. Guillermo Cherencio
#
gcc -fPIC -Wall -g -c -o cgi.o cgi.c -I ../include
gcc -fPIC -Wall -g -c -o util.o util.c -I ../include
gcc -fPIC -Wall -g -c -o app3.o app3.c -I ../include
gcc -fPIC -shared -Wl,-soname,libapp3.so -o libapp3.so app3.o util.o cgi.o -lc
mv -f libapp3.so ../lib
rm app3.o
