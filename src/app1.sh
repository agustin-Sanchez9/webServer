#!/bin/bash
# script para compilacion de aplicacion app1
#
# ver explicacion -fPIC  PIC=position independent code
# las librerias compartidas .so (shared objects) son objetos PIC que
# requieren ser compiladas con el switch -fPIC
# http://freesoftwaremagazine.com/articles/building_shared_libraries_once_using_autotools/
#
# Atte. Guillermo Cherencio
gcc -fPIC -Wall -g -c -o bdb.o bdb.c -I ../include -ldb
gcc -fPIC -Wall -g -c -o cgi.o cgi.c -I ../include 
gcc -fPIC -Wall -g -c -o util.o util.c -I ../include 
gcc -fPIC -Wall -g -c -o app1.o app1.c -ldb -I ../include 
gcc -fPIC -shared -Wl,-soname,libapp1.so -o libapp1.so app1.o util.o cgi.o bdb.o -lc -ldb
mv -f libapp1.so ../lib
rm app1.o

