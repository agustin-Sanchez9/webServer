#
AC_INIT([wserver],[0.1],[grchere@yahoo.com])
#esto provoca error en ./configure no encuentra install-sh etc xq lo genera en /build en vez de directorio raiz
AC_CONFIG_AUX_DIR([build]) 
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/server1.c])
AC_CHECK_HEADERS([sys/wait.h unistd.h sys/socket.h sys/types.h arpa/inet.h sys/un.h])
AC_CHECK_HEADERS([dlfcn.h db.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_SEARCH_LIBS([dlopen],[dl dld],[],[AC_MSG_ERROR([no puedeo encontrar a funcion dlopen() en libreria dl])])
AC_SEARCH_LIBS([pthread_create],[pthread],[],[AC_MSG_ERROR([no puedo encontrar a funcion pthread_create() en liberia pthread])])
AC_SEARCH_LIBS([db_create],[db],[],[AC_MSG_ERROR([no puedo encontrar a funcion db_create() en libreria db])])
AC_OUTPUT
