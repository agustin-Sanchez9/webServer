#!/bin/bash
# script para compilacion de aplicacion app2
#
# Atte. Guillermo Cherencio
#
gcc -fPIC -Wall -g -c -o bdb.o bdb.c -I ../include -ldb
gcc -fPIC -Wall -g -c -o cgi.o cgi.c -I ../include
gcc -fPIC -Wall -g -c -o util.o util.c -I ../include 
gcc -fPIC -Wall -g -c -o app2.o app2.c -ldb -I ../include
gcc -fPIC -shared -Wl,-soname,libapp2.so -o libapp2.so app2.o util.o cgi.o bdb.o -lc -ldb
mv -f libapp2.so ../lib
rm app2.o
