		Desarrollo de servidores web en C

wserver/server1
	servidor web de archivos
	implementa CGI basico
	basado en procesos pesados (fork())
	No soporta FastCGI
	No soporta https

wserver/server2
	servidor web de archivos
	implementa CGI basico
	basado en hilos (threads)
	permite aplicaciones web basadas en librerias dinamicas (.so)
	No soporta FastCGI
	No soporta https

INSTALACION
	PREREQUISITOS
		Unix/Linux con soporte de librerias dinamicas (.so)
		Instalar Berkeley DB 5.3 $ sudo apt-get install libdb5.3 libdb5.3-dev
		Instalar gcc, make etc  $ apt-get install build-essential manpages-dev gcc make 
		Instalar cliente git $ apt-get install git
		
   $ ./configure
   $ make
   $ make apps
   
   No ejecutar $ [sudo] make install
   Este software esta en desarrollo y no para uso en produccion. No requiere usuario root o sudoers,
   usar usuario no root.
   $ make apps  ejecuta src/app1.sh src/app2.sh src/app3.sh formas no standard de generar librerias .so que
   deben estar en directorio lib
   src/app1.c src/app2.c src/app3.c son apps web de ejemplo

DIRECTORIOS

├── build
│   carpeta usada por autotools
├── cgi-bin
│   carpeta para aplicaciones CGI
├── data
│   carpeta para guardar archivos y base de datos de aplicaciones web, archivos sin seguimiento git
├── data.examples
│   carpeta para guardar archivos y base de datos de aplicaciones web, NO MODIFICAR copiar a ../data
├── doc
│   carpeta de documentacion
├── include
│   carpeta de archivos include requeridos para compilar
├── lib
│   carpeta con aplicaciones web (que son librerias dinamicas .so)
├── src
│   carpeta de programas fuente
├── testing
│   carpeta testing para probar nuevas funcionalidades a agregar, codigo experimental
├── wserver
│   carpeta con archivos ejecutables, archivos de configuracion y archivo de mime types
│   requeridos por los ejecutables
└── www
    carpeta raiz de los servidores web
    ├── app1
    	carpeta de archivos estaticos de la app web app1
    ├── app2
    	carpeta de archivos estaticos de la app web app2
    ├── app3
    	carpeta de archivos estaticos de la app web app3
    ├── lib
    	carpeta de librerias web requeridas por apps web ej: jquery, bootstrap, etc
    │   └── jquery
    │       carpeta con libreria jquery para probar llamadas AJAX

   
DESARROLLO DE APPS WEB (solo wserver/server2)
   


Atte. Guillermo Cherencio
UNLu - UTN FRD
grchere@yahoo.com


