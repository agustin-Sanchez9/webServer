#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int obtenerQueryString(char *destino, int largo);
void decodeQueyString(char *str);

int main() {
    char comando[256];

    if (!obtenerQueryString(comando, sizeof(comando))) {
        fprintf(stderr, "no fue posible obtener el query string\n");
        return 1;
    }

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
        _exit(1);
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


int obtenerQueryString(char *destino, int len) {
    const char *queryString = getenv("QUERY_STRING");
    if (queryString == NULL)
        return 0;
    if (strlen(queryString) >= len)
        return 0;
    strcpy(destino, queryString);
    return 1;
}

void decodeQueyString(char *string) {
    char *src = string;
    char *dst = string;
    while (*src) {
        if (src[0] == '%') {
            int value;
            sscanf(src + 1, "%2x", &value);
            *dst++ = (char)value;
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

