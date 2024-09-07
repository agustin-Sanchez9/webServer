#include <bdb.h>

// graba o regraba clave-valor (ambos strings) en dbp
int bdb_actualizo(DB *dbp,char *sclave,char *svalor) {
	return bdb_actualizo_reg(dbp,sclave,svalor,strlen(svalor)+1);
}

// graba o regraba clave (string) - valor (struct) en dbp
int bdb_actualizo_reg(DB *dbp,char *sclave,void *svalor,int largo) {
	DBT clave, valor;
	memset(&clave, 0, sizeof(DBT));
	memset(&valor, 0, sizeof(DBT));
	clave.data = sclave;
	clave.size = strlen(sclave)+1;
	valor.data = svalor;
	valor.size = largo;
	int rc = dbp->put(dbp, NULL, &clave, &valor, 0); // si existe la clave se actualiza?		
	printf("bdb_actualizo_reg(): rc=%d\n",rc);
	return rc;
}

// obtengo dato entero de la BD
int bdb_get_iclave(DB *dbp,int *dest,char *clave) {
	return bdb_get_vclave(dbp,dest,sizeof(int),clave);
}
int bdb_get_dclave(DB *dbp,double *dest,char *clave) {
	return bdb_get_vclave(dbp,dest,sizeof(double),clave);
}
int bdb_get_fclave(DB *dbp,float *dest,char *clave) {
	return bdb_get_vclave(dbp,dest,sizeof(float),clave);
}
int bdb_get_lclave(DB *dbp,long *dest,char *clave) {
	return bdb_get_vclave(dbp,dest,sizeof(long),clave);
}
int bdb_get_llclave(DB *dbp,long long *dest,char *clave) {
	return bdb_get_vclave(dbp,dest,sizeof(long long),clave);
}

// obtengo string/struct/etc de la BD
int bdb_get_vclave(DB *dbp,void *dest,int largo,char *clave) {
	DBT rclave, rvalor;
	memset(&rclave, 0, sizeof(DBT));
	memset(&rvalor, 0, sizeof(DBT));
	rclave.data = clave;
	rclave.size = strlen(clave)+1;
	rvalor.data = dest;
	rvalor.ulen = largo;
	rvalor.flags = DB_DBT_USERMEM;
	int rc = dbp->get(dbp, NULL, &rclave, &rvalor, 0);
	return rc;
}

/*
 * Abre o crea bd clave-valor BDB db
 * Apertura simple de db no transaccional
 * devuelve rc devuelto por api BDB para chequear errores
 * si db no existe, la crea
 * si db existe, la abre
 * DBTYPE   DB_HASH, DB_BTREE, DB_RECNO
 */
int bdb_abrir(DB **dbp,const char *db,DBTYPE tipo) {
	u_int32_t flags; // database open flags
	int rc; // function return value
	/* Initialize the structure. This
	 * database is not opened in an environment,
	 * so the environment pointer is NULL. */
	rc = db_create(dbp,NULL,0);
	if (rc != 0) return rc;
	// Database open flags
	// If the database does not exist,  create it.
	flags = DB_CREATE | DB_EXCL; 
	// open the database 
	rc = (*dbp)->open(*dbp, 	// DB structure pointer
					NULL, 	// Transaction pointer
					db, 	// On-disk file that holds the database.
					NULL, 	// Optional logical database name
					tipo, 	// Database access method
					flags, 	// Open flags
					0); 	// File mode (using defaults)
	if (rc != 0) {
		// error en apertura de BD, intento no crear BD
		flags = 0;
		// open the database
		rc = (*dbp)->open(*dbp,NULL,db,NULL,tipo,flags,0);
	}
	return rc;
}

// cierra bd
int bdb_cerrar(DB **dbp) {
	int rc=0;
	if ( dbp ) rc=(*dbp)->close(*dbp, 0);
	return rc;
}

int bdb_borrar(DB *dbp,char *clave) {
	return bdb_vborrar(dbp,clave,strlen(clave)+1);
}

int bdb_vborrar(DB *dbp,void *clave,int largo) {
	DBT rclave;
	memset(&rclave, 0, sizeof(DBT));
	rclave.data = clave;
	rclave.size = largo;	
	int rc = dbp->del(dbp, NULL, &rclave, 0);
	return rc;
}

