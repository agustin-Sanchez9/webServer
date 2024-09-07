#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

/****************FUNCIONES PARA PROCESOS CGI*********************/
int cgi_extraer(char *destino,int largo,const char *tag,char *buffer);
int cgi_iextraer(int *destino,const char *tag,char *buffer); // extrae entero
int cgi_dextraer(double *destino,const char *tag,char *buffer); // extrae double
int cgi_fextraer(float *destino,const char *tag,char *buffer); // extrae float
int cgi_lextraer(long *destino,const char *tag,char *buffer); // extrae long
int cgi_cextraer(char *destino,const char *tag,char *buffer); // extrae 1 char
int cgi_esHexa(char c);
char cgi_charHexa(char *p);
int cgi_obtenercgi(char *destino,int largo);
void cgi_sesion(char *dest,int largo); // genero session id de largo caracteres
long long cgi_fecha_en_millis(); // fecha-hora actual en milisegundos 
