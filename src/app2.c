/*
 * app2.c
 * 
 * Copyright 2024 osboxes <osboxes@osboxes>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 * app2 implementa login de usuarios muy simple
 * tabla de usuarios en bd BDB
 * 
 * $ gcc -fPIC -Wall -g -c -o bdb.o bdb.c -I . -L . -ldb
 * $ gcc -fPIC -Wall -g -c -o cgi.o cgi.c -I . -L .
 * $ gcc -fPIC -Wall -g -c -o util.o util.c -I . -L .
 * $ gcc -fPIC -Wall -g -c -o app2.o app2.c  -I . -L . -ldb
 * $ gcc -fPIC -shared -Wl,-soname,libapp2.so -o libapp2.so app2.o util.o cgi.o bdb.o -lc -ldb
 * $ mv -f libapp2.so ../lib
 * $ rm *.o
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <db.h>
#include <bdb.h>
#include <util.h>
#include <cgi.h>

void *app_ini(clave_valor *pserver,clave_valor *papp,long *largo); // puntero a funcion inicial de aplicacion  void *f_init(long *);
void app_fin(void *buffer,long *largo); // puntero a funcion final de aplicaicon void f_fin(void *,long *);
void app_get(void *buffer,long *largo,clave_valor *pserver,clave_valor *papp,peticion *pet,respuesta *ret);

typedef struct usuario { // clave en BDB nombre de usuario
	char _usuario[100];
	char clave[100];
} usuario;

typedef struct sesion { // clave en BDB sesion id
	char _usuario[100];
	long long fh_login; // fecha-hora de login en milisegundos
	long long fh_fin;   // fecha-hora que caduca login fh_fin > fh_login
} sesion;

// handle global para bd BDB de esta aplicacion
static DB *dbp = NULL;
//                                   1"  * 60 = 60" = 1 MINUTO
static const long LOGIN_TIME_OUT = 1000L * 60L;
static const int LARGO_SESION = 63;

/*
int main(int argc, char **argv)
{
	// hacer pruebas aqui y ejecutar desde linea de comandos
	return 0;
}
*/

// esta app no desea compartir/exponer ningun buffer al servidor
void *app_ini(clave_valor *pserver,clave_valor *papp,long *largo) { // puntero a funcion inicial de aplicacion  void *f_init(long *);
	*largo=0L;
	char *p = util_buscar_default(papp,"db","appZ.db");
	printf("app_ini(2): p=%s\n",p);
	int rc = bdb_abrir(&dbp,p,DB_HASH);
	printf("app_ini(2): rc = %d en apertura bd dbp=[%p]\n",rc,dbp);
	if ( rc ) printf("app_ini(2): Error [%s] en apertura bd [%s]\n",db_strerror(rc),p);
	// agrego dos usuarios, luego comento esto
	/*
	usuario u;
	strcpy(u._usuario,"grchere");
	strcpy(u.clave,"grchere");
	bdb_actualizo_reg(dbp,u._usuario,&u,sizeof(usuario));
	strcpy(u._usuario,"jromer");
	strcpy(u.clave,"jromer");
	bdb_actualizo_reg(dbp,u._usuario,&u,sizeof(usuario));
	* */
	return NULL;
}
void app_fin(void *buffer,long *largo) { // puntero a funcion final de aplicaicon void f_fin(void *,long *);
	// no hago nada, no tengo ningun buffer, asi que buffer debe ser NULL y largo 0L
	// cierro BDB
	bdb_cerrar(&dbp);
}
void app_get(void *buffer,long *largo,clave_valor *pserver,clave_valor *papp,peticion *pet,respuesta *ret) {
	int bloque = 200;
	char *salida = (char *) malloc(bloque);
	char sclave[100];
	if ( strcmp(pet->recurso,"/app2.login") == 0 ) {
		cgi_extraer(sclave,100,"usuario",(char *) pet->cuerpo);
		usuario u;
		memset(&u,0,sizeof(usuario));
		bdb_get_vclave(dbp,&u,sizeof(usuario),sclave);
		if ( strcmp(sclave,u._usuario) == 0 ) {
			// usuario existe!
			cgi_extraer(sclave,100,"clave",(char *) pet->cuerpo);
			if ( strcmp(sclave,u.clave) == 0 ) {
				// clave coincide!
				sesion s;
				strcpy(s._usuario,u._usuario);
				s.fh_login = cgi_fecha_en_millis();
				s.fh_fin = s.fh_login + LOGIN_TIME_OUT;
				cgi_sesion(sclave,LARGO_SESION);
				printf("app_get(2): sesion=[%s]\n",sclave);
				// grabo sesion
				bdb_actualizo_reg(dbp,sclave,&s,sizeof(sesion));
				// devuelvo sesion
				strcpy(salida,sclave);
			} else {
				printf("app_get(2): clave no coincide\n");
				strcpy(salida,"0"); // clave no coincide
			}
		} else {
			printf("app_get(2): usuario no existe\n");
			strcpy(salida,"0"); // usuario no existe
		}
	} else if ( strcmp(pet->recurso,"/app2.logoff") == 0 ) {
		//
		cgi_extraer(sclave,100,"session",(char *) pet->cuerpo);
		sesion s;
		memset(&s,0,sizeof(sesion));
		bdb_get_vclave(dbp,&s,sizeof(sesion),sclave);
		if ( strlen(s._usuario) > 0 ) {
			// encontre la sesion, la borro
			int rc = bdb_borrar(dbp,sclave);
			printf("app_get(2): logoff ok rc=%d sesion borrada\n",rc);
			strcpy(salida,"1"); // sesion borrada
		} else {
			printf("app_get(2): logoff error no encontre sesion\n");
			strcpy(salida,"0"); // no encontre sesion!
		}
					
	} else if ( strcmp(pet->recurso,"/app2.listar") == 0 ) {
		//
		cgi_extraer(sclave,100,"session",(char *) pet->cuerpo);
		sesion s;
		memset(&s,0,sizeof(sesion));
		bdb_get_vclave(dbp,&s,sizeof(sesion),sclave);
		if ( strlen(s._usuario) > 0 ) {
			// encontre la sesion, estara vencida?
			if ( s.fh_fin > cgi_fecha_en_millis() ) {
				// sesion valida!, hago consulta
				snprintf(salida,200,"<p>Usuario [%s] fh login [%lld] fh fin [%lld] fh actual [%lld]</p>",
					s._usuario,s.fh_login,s.fh_fin,cgi_fecha_en_millis());
			} else {
				printf("app_get(2) listar: sesion expirada? fh login[%lld] fh fin[%lld] fh actual[%lld]\n",
					s.fh_login,s.fh_fin,cgi_fecha_en_millis());
				strcpy(salida,"su sesion ha expirado!"); // sesion expirada
			}
		} else {
			printf("app_get(2) listar: no encontre sesion!\n");
			strcpy(salida,"0"); // no encontre sesion!
		}
	}
	ret->cuerpo = salida;
	ret->largo = strlen(salida)+1;
	util_agregar_content(ret->cabecera,ret->largo);
	util_agregar(ret->cabecera,"Content-Type","text/html");
}
