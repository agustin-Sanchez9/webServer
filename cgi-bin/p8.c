#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// se utilizara la variable de entorno QUERY_STRING para la obtencion de los datos para los comandos
// y se manipularan para que el servidor pueda ejecutarlos
// se separaran los argumentos con "%20", un espacio en hexa, esto para no usar caracteres que puedan
// conformar parte de un comando como pueden ser el "-" o el "+", etc.

int obtenerQueryString(char *destino, int largo);
void decodeQueyString(char *str);

int main() {
    char comando[256];

    // obtengo el comando
    if (!obtenerQueryString(comando, sizeof(comando))) {
        fprintf(stderr, "no fue posible obtener el query string\n");
        return 1;
    }

    // reemplazo "%20" por " "
    decodeQueyString(comando);

    printf("Content-Type: text/html\r\n\r\n");
    printf("<html><body>\n");
    printf("<p>metodo GET comando %s</p>\n",comando);

    // parseo
    char *cmd = strtok(comando, " ");
    char *args[256];
    int i = 0;
    while (cmd != NULL && i < 255) {
        args[i++] = cmd;
        cmd = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (args[0]) {
        printf("<p>cmd=%s</p>\n", args[0]);
    }
    if (i > 1) {
        printf("<p>args=");
        for (int k = 1; k < i; k++) {
            printf("%s", args[k]);
        }
        printf("</p>\n");
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        printf("fallo de pipe()");
        return 1;
    }
        
    pid_t pid;
    int status;
    char buffer[1024];
    ssize_t count;

    pid = fork();
    if (pid == -1) {
        printf("fallo de fork()");
        return 1;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        execvp(args[0], args);
        // si execvp falla continuara con el exit asi aseguro que no quede un huerfano o zombie.
        exit(1); 
    } else {
        close(pipefd[1]);
        printf("<p></p>"); // para dejar espacio
        while ((count = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[count] = '\0'; 
            printf("<pre>%s</pre>\n", buffer);
        }
        close(pipefd[0]);
        waitpid(pid, &status, 0);
    }
    printf("</body></html>\n");
    return 0;
}

// obtener la cadena de consulta QUERY_STRING. return 1 indica exito y 0 indica error o fracaso de operacion
int obtenerQueryString(char *buffer, int len) {
    // si se recibe "http://ejemplo.com/p8.cgi?ps%20-lax" entonces QUERY_STRING es "ps%20-lax"
    // y lo obtengo con getenv
    char *queryString = getenv("QUERY_STRING");
    // no se obtuvo nada
    if (queryString == NULL) 
        return 0;  
    // lo botenido supera el tamaño del buffer
    if (strlen(queryString) >= len) 
        return 0; 
    // se copia la cadena al buffer. Se sabe que no desborda el buffer por el chequeo anterior
    strcpy(buffer, queryString); 
    return 1; 
}


// necesario para decodificar el caracter "%20" en hexa a un espacio en ascii
void decodeQueyString(char *string) {
    char *original = string;
    char *decoded = string;
    while (*original) {
        if (original[0] == '%') {
            int value;
            sscanf(original + 1, "%2x", &value);
            *decoded++ = (char)value;
            original += 3; // incremento 3 posiciones por el % y el numero hexa
        } else {
            *decoded++ = *original++; // si no es hexa se copia tal como esta y se avanza ambos punteros
        }
    }
    *decoded = '\0'; // marco final de cadena
}

