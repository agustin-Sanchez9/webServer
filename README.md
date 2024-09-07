# webserver

		Desarrollo de servidores web basicos en C

wserver/server1
- 	servidor web de archivos
- 	implementa CGI basico
- 	basado en procesos pesados (fork())
- 	No soporta FastCGI
- 	No soporta https


wserver/server2
- 	servidor web de archivos
- 	implementa CGI basico
- 	basado en procesos livianos (hilos o threads)
- 	permite aplicaciones web basadas en librerias dinamicas (.so)
- 	No soporta FastCGI
- 	No soporta https

	
Software no productivo, solo para uso educativo, asignatura Sistemas Operativos, UTN FRD

## ¿Como empezar?

Una vez realizada la instalacion, puede comenzar a usar el servidor 2 (por ejemplo) de la siguiente manera (verifique antes wserver/server2.conf para ver el puerto y otros parametros):

```
$ cd wserver
$ ./server2
```


Luego abra su navegador web en el mismo computador en donde esta ejecutando ./server2 e ingrese la siguiente URL:

`localhost:8181`

Alli podra ver la pagina www/index.html y puede chequear las opciones disponibles para servidor 2, si algo no funciona, es posible que jquery no este funcionando (en tal caso, descomente codigo para chequear que jquery anda bien dentro de index.html) o bien los datos no estan bien (verifique que en directorio data/*.db se encuentran las bases de datos Berkeley DB requeridas), si no estan las BD's, tal vez olvido hacer $ make apps<br> 
Tambien puede copiar el contenido de la carpeta data.examples/ a data/<br>
Verifique que en la carpeta lib/ se encuentren todas las aplicaciones compiladas, es decir, todas las librerias *.so (libapp1.so, libapp2.so, libapp3.so .. ) si no se encuentran, puede hacer:
```
$ cd src 
$ ./app1.sh
$ ./app2.sh
$ ./app3.sh
```
Esto permitira compilar y generar nuevamente las liberias requeridas por server 2 en lib/

## Nombre
Desarrollo de servidores web en lenguaje C para uso academico.

## Descripcion
Servidor web a ser utilizado en [TP1](https://www.grch.com.ar/docs/so/trabajos.practicos/obligatorios/Unidad_I_IVtp1.pdf) y [TP2](https://www.grch.com.ar/docs/so/trabajos.practicos/obligatorios/Unidad_V_VIItp2.pdf) , practicas de la asignatura Sistemas Operativos [UTN FRD](https://www.frd.utn.edu.ar/) 

## Instalacion

	PREREQUISITOS
		Unix/Linux con soporte de librerias dinamicas (.so)
		Instalar Berkeley DB 5.3 `$ sudo apt-get install libdb5.3 libdb5.3-dev`
		Instalar gcc, make, automake etc  `$ apt-get install build-essential manpages-dev gcc make automake` 
		Instalar cliente git `$ apt-get install git`
		
```
   $ git clone https://gitlab.com/grchere/webserver.git
   $ cd webserver
   $ ./configure
   $ make
   $ make apps
```
   
   **No ejecutar** `$ [sudo] make install`<br>
   **No modificar archivos** en data.examples/<br>
   Puede copiar archivos de data.examples/ a data/<br>
   Archivos en data/ no tienen seguimiento git, directorio en donde ubicar archivos de datos de las aplicaciones web.<br>
   **Este software esta en desarrollo y no para uso en produccion**. No requiere usuario root o sudoers,
   usar usuario no root.
   `$ make apps`  ejecuta src/app1.sh src/app2.sh src/app3.sh formas no standard de generar librerias .so que
   deben estar en directorio lib/
   src/app1.c src/app2.c src/app3.c son las aplicaciones web de ejemplo para server 2<br>

## Forma de Uso
Para usar servidor 1:
```
$ cd wserver
$ ./server1
```


Para usar servidor 2:
```
$ cd wserver
$ ./server2
```

verificar previamente los archivos de configuracion server1.conf y server2.conf
Atencion! verificar la direccion IP del servidor dentro de server1.conf y server2.conf
para localhost usar IP 127.0.0.1 caso contrario usar la IP actual $ sudo ifconfig
ver los mensajes en la consola, no hay archivo log
Ejecutar su navegador web en la URL:  `localhost:8181`    (o bien el puerto tcp que haya configurado)

## Soporte
Guillermo Cherencio  grchere@yahoo.com  (primero leer este documento, README y leer en directorio doc/)

## ¿Como seguimos?
Agregar soporte para HTTPS
Agregar log de mensajes en servidor
Mejorar rendimiento con implementacion de memoria cache?
Pruebas de aplicaciones web mas grandes, compartir datos entre aplicaciones.
...

## Contribuciones
Aceptamos colaboracion de alumnos y colegas que deseen sumarse al proyecto y mejorar el software, depurar bugs, etc. preeviamente contactarme por e-mail 
Requisitos para desarrollar, instalar herramientas build system de la FSF, algunas de las herramientas que tuve que instalar en mi pc:

```
$ sudo apt-get install autotools-dev autoconf automake libtool libtool-bin
$ sudo apt-get install libltdl-dev libsigsegv2 m4 
$ sudo apt-get install shtool
```


## Autores y Reconocimientos
Guillermo Cherencio  grchere@yahoo.com

## Licencia
GNU GPL

## Estado del Proyecto
Se realizaran revisiones anuales (antes del inicio de cada cursada), se utiliza en la asignatura Sistemas Operativos en UTN FRD SO, [ver carpeta](https://www.grch.com.ar/docs/so/) para enseñar conceptos de sistemas operativos, desarrollo web, C, etc.

Atte. Guillermo Cherencio.
