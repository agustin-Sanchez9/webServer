#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>

typedef struct clave_valor {
	char *clave;
	char *valor;
	struct clave_valor *siguiente;
} clave_valor;

typedef struct peticion {
	char metodo[32];
	char recurso[1024];
	char recurso_fisico[1024];
	char protocolo[16];
	char version[16];
	clave_valor *cabecera;
	long largo;
	void *cuerpo;
} peticion;

typedef struct respuesta {
	char protocolo[16];
	char version[16];
	int estado;
	char mensaje[256];
	clave_valor *cabecera;
	long largo;
	void *cuerpo;
} respuesta;

typedef struct aplicacion {
	char nombre[32];
	char libreria[256];
	char url[256];
	void *lib; // puntero a libreria abierta .so
	void *buffer;
	long largo_buffer;
	clave_valor *param; // lista parametros de la app
	void *(*f_ini)(clave_valor *,clave_valor *,long *); // puntero a funcion inicial de aplicacion  void *f_init(long *);
	void (*f_fin)(void *,long *); // puntero a funcion final de aplicaicon void f_fin(void *,long *);
	void (*f_get)(void *,long *,clave_valor *,clave_valor *,peticion *,respuesta *);
} aplicacion;

// nuevo nodo lista
clave_valor *util_nuevo();
// borra lista
void util_borrar(clave_valor *lista);
void util_borrar_pet(peticion *p);
void util_borrar_res(respuesta *r);
// agregar a la lista
void util_agregar(clave_valor *lista,char *clave,char *valor);
void util_agregar_content(clave_valor *lista,long valor);
// buscar
char *util_buscar(clave_valor *lista,char *clave);
char *util_buscar_case(clave_valor *lista,char *clave);
char *util_buscar_default(clave_valor *lista,char *clave,char *defecto);
char *util_buscar_case_default(clave_valor *lista,char *clave,char *defecto);
void util_borrar_clave(clave_valor *lista,char *clave);
// listar
void util_listar(clave_valor *lista);


peticion *util_peticion(char *buffer,clave_valor *param);
respuesta *util_respuesta();
void util_procesar(int sd,clave_valor *param,clave_valor *mime,peticion *pet,respuesta *res);

// devuelve verdad si archivo existe, sino devuelve falso
int util_archivo_existe(char *archivo);
// devuelve la fecha-hora de ultima modificacion de archivo en el formato de fecha que requiere http
void util_archivo_fhmodif(char *to,int toLargo,char *archivo);
// devuelve la fecha-hora actual en el formato de fecha que requiere http
void util_fecha_hora(char *to,int toLargo);
// carga archivo en res, siempre y cuando exista
int util_cargar_archivo(respuesta *res,clave_valor *mime,char *archivo);
// carga archivo en res, el archivo html parametrizado a cargar cuando un recurso no existe
void util_cargar_archivo_noesta(clave_valor *param,clave_valor *mime,respuesta *res);
// devuelve verdad cuando str termina en suffix, caso contrario, devuelve falso
int util_termina_con(const char *str, const char *suffix);
// archivo tiene registros con formato: clave\tvalor\n
void util_cargar_mime(clave_valor *to,char *archivo);

void util_enviar_respuesta(int sd,respuesta *res);

//archivos de configuracion
clave_valor *ws_parametros_archivo(char *archivo);
clave_valor *ws_parametros(int argc,char **argv);

