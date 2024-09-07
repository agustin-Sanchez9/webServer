/*
 * fcgi1.c
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
 * compilar:
 * $ gcc -Wall -o fcgi1 fcgi1.c -lfcgi
 * 
 */

#include "fcgi_stdio.h" /* fcgi library; put it first*/

#include <stdlib.h>

int count;

void initialize(void)
{
  count=0;
}

int main(void)
{
/* Initialization. */  
  initialize();

/* Response loop. */
  while (FCGI_Accept() >= 0)   {
    FCGI_printf("Content-type: text/html\r\n"
           "\r\n"
           "<title>FastCGI Hello! (C, fcgi_stdio library)</title>"
           "<h1>FastCGI Hello! (C, fcgi_stdio library)</h1>"
           "Request number %d running on host <i>%s</i>\n",
            ++count, getenv("SERVER_HOSTNAME"));
  }
  
  return 0;
}
