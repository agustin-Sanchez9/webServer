/*
 * cgi1.c
 * 
 * Copyright 2022 grchere <grchere@yahoo.com>
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
 * programa cgi muy simple que muestra el contenido del campo
 * nombre y apellido
 * 
 * para compilar:
 * $ gcc -Wall -o ../cgi-bin/cgi1.cgi cgi1.c cgi.c -I ../include 
 *
 * 
 */

#include <cgi.h>

int main(int argc, char **argv) {
	printf("Content-Type: text/html\r\n\r\n");
	printf("<!DOCTYPE html><body>");
	char buffer[1024], nombre[512], apellido[512];
	cgi_obtenercgi(buffer,1024);
	cgi_extraer(nombre,512,"nombre",buffer);
	cgi_extraer(apellido,512,"apellido",buffer);
	printf("<p>Nombre: %s</p><p>Apellido: %s</p>",nombre,apellido);
	printf("</body></html>");
	return 0;
}

