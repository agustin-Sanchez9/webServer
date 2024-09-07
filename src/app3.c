/*
 * app3.c
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
 * app3 simple ejecutor de comandos con popen()
 * 
 * $ gcc -fPIC -Wall -g -c -o cgi.o cgi.c -I . -L .
 * $ gcc -fPIC -Wall -g -c -o util.o util.c -I . -L .
 * $ gcc -fPIC -Wall -g -c -o app3.o app3.c  -I . -L . 
 * $ gcc -fPIC -shared -Wl,-soname,libapp3.so -o libapp3.so app3.o util.o cgi.o -lc 
 * $ mv -f libapp3.so ../lib
 * $ rm *.o
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <cgi.h>

void *app_ini(clave_valor *pserver,clave_valor *papp,long *largo); // puntero a funcion inicial de aplicacion  void *f_init(long *);
void app_fin(void *buffer,long *largo); // puntero a funcion final de aplicaicon void f_fin(void *,long *);
void app_get(void *buffer,long *largo,clave_valor *pserver,clave_valor *papp,peticion *pet,respuesta *ret);
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
	return NULL;
}
void app_fin(void *buffer,long *largo) { // puntero a funcion final de aplicaicon void f_fin(void *,long *);
	// no hago nada, no tengo ningun buffer, asi que buffer debe ser NULL y largo 0L
}
void app_get(void *buffer,long *largo,clave_valor *pserver,clave_valor *papp,peticion *pet,respuesta *ret) {
	int bloque = 1024,tam_salida=bloque;
	char *salida = (char *) malloc(bloque);
	memset(salida,0,tam_salida);
	if ( strcmp(pet->recurso,"/app3.listar") == 0 ) {
		char scmd[200];
		memset(scmd,0,200);
		printf("app_get(3): pet->cuerpo=[%s]\n",(char *) pet->cuerpo);
		cgi_extraer(scmd,200,"comando",(char *) pet->cuerpo);
		printf("app_get(3): comando a ejecutar=[%s]\n",scmd);
		FILE *f = popen(scmd,"r");
		if ( f == NULL ) {
			snprintf(salida,tam_salida,"Error, no pude ejecutar [%s]",scmd);
		} else {
			// leo salida del comando
			char linea[500];
			int bytes_salida=5;
			strcpy(salida,"<pre>");
			int sin_lineas = 1;
			while (fgets(linea,500,f) != NULL) {
				sin_lineas=0;
				printf("app_get(3): linea=[%s]\n",linea);
				int nlinea = strlen(linea);
				if ( (bytes_salida+nlinea+50) > tam_salida ) {
					// necesito ampliar salida, le agrego otro bloque mas
					tam_salida+=bloque;
					salida = (char *) realloc(salida,tam_salida);
				}
				bytes_salida+=nlinea;
				strcat(salida,linea);
			}
			strcat(salida,"</pre>");
			if ( sin_lineas ) snprintf(salida,tam_salida,"Error, no pude ejecutar [%s], no obtuve salida!",scmd);
			pclose(f);		
		}
	}
	ret->cuerpo = salida;
	ret->largo = strlen(salida)+1;
	util_agregar_content(ret->cabecera,ret->largo);
	util_agregar(ret->cabecera,"Content-Type","text/html");
}
