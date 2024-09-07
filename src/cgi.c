#include <cgi.h>

/**********************FUNCIONES PROCESOS CGI************************/

/**
 * Ojo con los espacios intermedios, se reemplazan por +
 * Ojo con mayusculas y minusculas
 * buffer contiene:   variable1=valor1&variable2=valor2....
 * 
 * Toma buffer, busca tag y copia valor en destino
 * destino tiene que tener espacio suficiente
 * devuelve 1 (verdad) si pudo hacer extraccion
 * sino devuelve 0 (falso)
 */
int cgi_extraer(char *destino,int largo,const char *tag,char *buffer) {
	char *p = strcasestr(buffer,tag), *out = destino;
	int n=0;
	if ( p ) {
		p+=strlen(tag)+1;
		if ( *p ) {
			while(*p && *p != '&' && n < largo) {
				if (*p == '\n' || *p == '\r' || *p == '\t') { p++;continue; }
				if (*p == '+'){ *out = ' '; }
				else {
					// posible caracter codificado %00 .. %FF
					if (*p == '%' && cgi_esHexa(*(p+1)) && cgi_esHexa(*(p+2)) ) {
						*out = cgi_charHexa(p);
						p+=2;
					} else {
						*out = *p;
					}
				}
				out++;p++;n++;
			}
			*out='\0';
			return 1;
		}
	}
	return 0;
}
// funciones de conveniencia
int cgi_iextraer(int *destino,const char *tag,char *buffer) { // extrae entero
	char tmp[32];
	int rc;
	rc = cgi_extraer(tmp,32,tag,buffer);
	if ( rc ) *destino = atoi(tmp); 
	return rc;
}
int cgi_dextraer(double *destino,const char *tag,char *buffer) { // extrae double
	char tmp[32];
	int rc;
	rc = cgi_extraer(tmp,32,tag,buffer);
	if ( rc ) *destino = atof(tmp); 
	return rc;
}
int cgi_fextraer(float *destino,const char *tag,char *buffer) { // extrae float
	char tmp[32];
	int rc;
	rc = cgi_extraer(tmp,32,tag,buffer);
	if ( rc ) *destino = (float) atof(tmp); 
	return rc;
}
int cgi_lextraer(long *destino,const char *tag,char *buffer) { // extrae long
	char tmp[32];
	int rc;
	rc = cgi_extraer(tmp,32,tag,buffer);
	if ( rc ) *destino = atol(tmp); 
	return rc;
}
int cgi_cextraer(char *destino,const char *tag,char *buffer) { // extrae 1 char
	char tmp[5];
	int rc;
	rc = cgi_extraer(tmp,5,tag,buffer);
	if ( rc ) *destino = tmp[0]; 
	return rc;
}

/**
 * Devuelve 1 (verdad) si c es un digito hexadecimal
 * Sino devuelve 0 (falso)
 */
int cgi_esHexa(char c) {
	char c2 = (char) toupper(c);
	if ( c2 >= '0' && c2 <= '9' ) return 1;
	if ( c2 >= 'A' && c2 <= 'F' ) return 1;
	return 0;
}
/**
 * p apunta a "%2A..." por ejemplo, esta funcion devuelve
 * el caracter ascii del valor hexa 2A
 */
char cgi_charHexa(char *p) {
	char hexa[3];
	hexa[0] = *(p+1);
	hexa[1] = *(p+2);
	hexa[2] = '\0';
	return (char) strtol(hexa,NULL,16);
}

/**
 * Esta funcion obtiene el string cgi cargado por el servidor
 * web para luego ser procesado
 * Puede tratarse de una llamada de tipo GET o POST
 * GET el string cgi se encuentra en variable QUERY_STRING
 * POST el string se ingresa por teclado
 * Esta funcion sirve para ambos casos
 * Retorna 1 si todo Ok
 * Retorna 0 si hubo error
 */
int cgi_obtenercgi(char *destino,int largo) {
	if ( getenv("REQUEST_METHOD") == NULL ) return 0;
	if ( strcasecmp(getenv("REQUEST_METHOD"),"GET") == 0 ) { // es GET
		if ( getenv("QUERY_STRING") == NULL ) return 0;
		if ( strlen(getenv("QUERY_STRING")) > largo ) {
			// el contenido de query_string supera a largo!
			return 0;
		}
		strcpy(destino,getenv("QUERY_STRING"));
	} else { // es POST
		int lei = read(STDIN_FILENO,destino,largo-1);
		if ( lei >= 0 ) destino[lei]='\0';
	}
	return 1;
}

// genero session id de largo caracteres
// dest debe tener espacio suficiente para largo+1 caracteres, 
// esta funcion agrega \0 luego del largo-esimo caracter generado
void cgi_sesion(char *dest,int largo) {
    srand(time(NULL) + rand());
    int i;
    for(i=0;i<largo;i++,dest++) {
		int i = rand() % 127;
		while(i < 32) i = rand() % 127;
		*dest = (char) i;
    }
    *dest = '\0';
}

long long cgi_fecha_en_millis() { // fecha-hora actual en milisegundos 
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (((long long)tv.tv_sec)*1000LL)+(tv.tv_usec/1000LL);
}
