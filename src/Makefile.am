AM_CPPFLAGS = -I$(top_builddir)/include
bin_PROGRAMS = ../wserver/server1 ../wserver/server2 ../wserver/prueba_cgi ../cgi-bin/cgi1.cgi ../wserver/appdata

___wserver_server1_SOURCES = server1.c util.c $(top_builddir)/include/util.h
___wserver_server1_CPPFLAGS = -Wall -I$(top_builddir)/include

___wserver_server2_SOURCES = server2.c util.c $(top_builddir)/include/util.h
___wserver_server2_CPPFLAGS = -Wall -I$(top_builddir)/include -lpthread -ldl

___wserver_prueba_cgi_SOURCES = prueba_cgi.c
___wserver_prueba_cgi_CPPFLAGS = -Wall

___cgi_bin_cgi1_cgi_SOURCES = cgi1.c cgi.c $(top_builddir)/include/cgi.h
___cgi_bin_cgi1_cgi_CPPFLAGS = -Wall -I$(top_builddir)/include

___wserver_appdata_SOURCES = appdata.c util.c bdb.c $(top_builddir)/include/util.h $(top_builddir)/include/bdb.h
___wserver_appdata_CPPFLAGS = -Wall -I$(top_builddir)/include -ldb

