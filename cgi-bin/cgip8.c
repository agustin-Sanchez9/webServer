#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {

    printf("Content-Type: text/html\r\n\r\n");
	printf("<!DOCTYPE html><body>");

    // creo el fd a usar en el pipe
    int pipefd[2];

    // creacion del pipe y chequeo de error
    if (pipe(pipefd) == -1) {
        printf("fallo de pipe()\n");
        exit(0);
    }

    pid_t pid = fork();
    // chequeo de error de fork
    if (pid == -1) {
        printf("fallo de fork()\n");
        exit(0);
    }

    // proceso hijo
    if (pid == 0) { 
        // cerrar el extremo de lectura del pipe
        close(pipefd[0]); 

        // redirigir la salida estandar al extremo de escritura del pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            printf("fallo de dup2\n");
            exit(0);
        }
        
        // cerrar el descriptor duplicado
        close(pipefd[1]); 

        char *const args[] = { "ps", "-lax", NULL };

        // ejecutar el comando
        execvp(args[0], &args[0]);


    } 
    // proceso padre
    else { 
        // cerrar el extremo de escritura del pipe
        close(pipefd[1]); 

        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            printf("%s", buffer);
        }

        // cerrar el extremo de lectura del pipe
        close(pipefd[0]); 

        // esperar al hijo
        if (waitpid(pid, NULL, 0) == -1) {
            printf("fallo waitpid");
            exit(0);
        }
    }
    printf("</body></html>");
    return 0;
}
