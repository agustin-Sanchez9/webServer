#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <db.h>

/****************FUNCIONES PARA BDB Berkeley DB*********************/
// funciones BDB
int bdb_actualizo(DB *dbp,char *clave,char *valor);
int bdb_actualizo_reg(DB *dbp,char *clave,void *valor,int largo);
int bdb_get_iclave(DB *dbp,int *dest,char *clave);
int bdb_get_dclave(DB *dbp,double *dest,char *clave);
int bdb_get_fclave(DB *dbp,float *dest,char *clave);
int bdb_get_lclave(DB *dbp,long *dest,char *clave);
int bdb_get_llclave(DB *dbp,long long *dest,char *clave);

int bdb_get_vclave(DB *dbp,void *dest,int largo,char *clave);

int bdb_borrar(DB *dbp,char *clave);
int bdb_vborrar(DB *dbp,void *clave,int largo);

int bdb_abrir(DB **dbp,const char *db,DBTYPE tipo); // si no existe db, la crea; tipo de bd DBTYPE DB_HASH,DB_BTREE,DB_RECNO ...
int bdb_cerrar(DB **dbp); // cierra bd
