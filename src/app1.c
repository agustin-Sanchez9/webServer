/*
 * app1.c
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
 * app1 simplemente prueba si es posible utilizar buffer estatico en
 * libreria so para luego usarla como app web
 * 
 * $ gcc -fPIC -Wall -g -c -o bdb.o bdb.c -I . -L . -ldb
 * $ gcc -fPIC -Wall -g -c -o cgi.o cgi.c -I . -L .
 * $ gcc -fPIC -Wall -g -c -o util.o util.c -I . -L .
 * $ gcc -fPIC -Wall -g -c -o app1.o app1.c  -I . -L . -ldb
 * $ gcc -fPIC -shared -Wl,-soname,libapp1.so -o libapp1.so app1.o util.o cgi.o bdb.o -lc -ldb
 * $ mv -f libapp1.so ../lib
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

// se probo buffer estatico y funciona Ok!
static char app1_buffer[256];

typedef struct cliente {
	int id;
	char descr[100];
	double saldo;
} cliente;

// handle global para bd BDB de esta aplicacion
static DB *dbp = NULL;

/*
int main(int argc, char **argv)
{
	// hacer pruebas aqui y ejecutar desde linea de comandos
	return 0;
}
*/

void *app_ini(clave_valor *pserver,clave_valor *papp,long *largo) { // puntero a funcion inicial de aplicacion  void *f_init(long *);
	*largo=256L;
	memset(app1_buffer,0,256);
	char *p = util_buscar_default(papp,"db","appZ.db");
	printf("app_ini(): p=%s\n",p);
	int n=-1,rc = bdb_abrir(&dbp,p,DB_HASH);
	printf("app_ini(): rc = %d en apertura bd dbp=[%p]\n",rc,dbp);
	if ( rc ) {
		printf("app_ini(): Error [%s] en apertura bd [%s]\n",db_strerror(rc),p);
		return app1_buffer;
	}
	// si C0 existe, dejo el valor actual, sino, creo clave C0 con valor cero
	printf("app_ini(): busco clave C0\n");
	rc = bdb_get_iclave(dbp,&n,"C0");
	if ( n == -1 ) {
		n=0;
		rc = bdb_actualizo_reg(dbp,"C0",&n,sizeof(int));
	}
	printf("app_ini(): OK\n");
	return app1_buffer;
}
void app_fin(void *buffer,long *largo) { // puntero a funcion final de aplicaicon void f_fin(void *,long *);
	// no hago nada, es un buffer estatico
	// cierro BDB
	bdb_cerrar(&dbp);
}
void app_get(void *buffer,long *largo,clave_valor *pserver,clave_valor *papp,peticion *pet,respuesta *ret) {
	int bloque = 512,salida_largo=bloque;
	char *salida = (char *) malloc(bloque);
	if ( strcmp(pet->recurso,"/app1.insertar") == 0 ) {
		char sclave[20];
		cliente c;
		c.id=0;
		bdb_get_iclave(dbp,&c.id,"C0");
		c.id++;
		cgi_extraer(c.descr,100,"descr",(char *) pet->cuerpo);
		cgi_dextraer(&c.saldo,"saldo",(char *) pet->cuerpo);
		snprintf(sclave,20,"C%d",c.id);
		bdb_actualizo_reg(dbp,sclave,&c,sizeof(cliente));
		bdb_actualizo_reg(dbp,"C0",&c.id,sizeof(int));
		snprintf(salida,bloque,"id %d descr %s saldo %lf grabado Ok",c.id,c.descr,c.saldo);

	} else if ( strcmp(pet->recurso,"/app1.listar") == 0 ) {
		int i,n,tam=0;
		bdb_get_iclave(dbp,&n,"C0");
		cliente c;
		char sclave[20];
		char srecord[200];
		strcpy(salida,"<style>table, th, td { border: 1px solid black; }</style><table><tr><td>Id</td><td>Descripcion</td><td>Saldo</td></tr>");
		tam+=strlen(salida);
		printf("app_get(): n=%d\n",n);
		for(i=1;i<=n;i++) {
			snprintf(sclave,20,"C%d",i);
			bdb_get_vclave(dbp,&c,sizeof(cliente),sclave);
			snprintf(srecord,200,"<tr><td>%d</td><td>%s</td><td>%lf</td></tr>",c.id,c.descr,c.saldo);
			if ( (salida_largo - tam) < 180 ) {
				// amplio buffer de salida en un 1 bloque mas
				salida_largo+=bloque;
				salida = (char *) realloc(salida,salida_largo);
			}
			tam+=strlen(srecord);
			strcat(salida,srecord);
			printf("app_get(): srecord=[%s]\nsalida=[%s]\n",srecord,salida);
		}
		strcat(salida,"</table>");
	}
	ret->cuerpo = salida;
	ret->largo = strlen(salida)+1;
	util_agregar_content(ret->cabecera,ret->largo);
	util_agregar(ret->cabecera,"Content-Type","text/html");
}
