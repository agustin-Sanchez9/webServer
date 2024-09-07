/*
 * appdata.c
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
 * Programa que crea datos requeridos por las aplicaciones de prueba
 * crea/actualiza ./data
 * usa ./wserver/app1.conf
 *
 * $ gcc -Wall -o appdata appdata.c bdb.c util.c -I ../include -ldb
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bdb.h>
#include <util.h>

typedef struct usuario { // clave en BDB nombre de usuario
	char _usuario[100];
	char clave[100];
} usuario;

int main(int argc, char **argv) {
	// handle para bd BDB de esta aplicacion
	DB *dbp = NULL;
	// hacer pruebas aqui y ejecutar desde linea de comandos
	clave_valor *param = ws_parametros_archivo("app1"); // usa app1.conf
	char *p = util_buscar(param,"db");
	if ( p == NULL ) {
		printf("main(): appdata: Error, parametro db no encontrado en ../wserver/app1.conf\n");
		return 1;
	}
	int rc = bdb_abrir(&dbp,p,DB_HASH);
	if ( rc ) {
		printf("main(): appdata: Error [%s] en apertura bd [%s]\n",db_strerror(rc),p);
		return 2;
	}
	bdb_cerrar(&dbp);
	util_borrar(param);
	
	param = ws_parametros_archivo("app2"); // usa app2.conf
	p = util_buscar(param,"db");
	if ( p == NULL ) {
		printf("main(): appdata: Error, parametro db no encontrado en ../wserver/app2.conf\n");
		return 3;
	}
	rc = bdb_abrir(&dbp,p,DB_HASH);
	if ( rc ) {
		printf("main(): appdata: Error [%s] en apertura bd [%s]\n",db_strerror(rc),p);
		return 4;
	}
	usuario u;
	strcpy(u._usuario,"grchere");
	strcpy(u.clave,"grchere");
	bdb_actualizo_reg(dbp,u._usuario,&u,sizeof(usuario));
	strcpy(u._usuario,"jromer");
	strcpy(u.clave,"jromer");
	bdb_actualizo_reg(dbp,u._usuario,&u,sizeof(usuario));	
	
	bdb_cerrar(&dbp);
	util_borrar(param);	
	
	printf("main(): appdata: fin! datos prueba generados, verifique ../data/*\n");
	
	return 0;
}


