SUBDIRS = src
ACLOCAL_AMFLAGS = -I m4

clean-local:
	@rm aclocal.m4
	@rm -r autom4te.cache/
	@rm -f config.* wserver/.dirstamp
	@rm -f Makefile.in src/Makefile.in src/*.o wserver/*.o lib/*.o
	@rm -rf src/.deps/
	@rm -rf m4/
	@rm -f stamp-h1

#Genero librerias .so de aplicaciones en ./lib
apps:
	@echo Genero librerias de aplicaciones
	@mkdir -p lib
	@cd ./src && ./app1.sh
	@cd ./src && ./app2.sh
	@cd ./src && ./app3.sh
	@mkdir -p data
	@cp ./data.examples/* ./data/
	@echo Fin Genero librerias y datos de aplicaciones
	
