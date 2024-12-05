#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS 200
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

// Función para dividir la entrada en tokens
void parse_input(char *input, char **args) {
    char *token = strtok(input, " \t\n");
    int i = 0;
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

// Función para manejar la ejecución con redirecciones y tuberías
void execute_command(char *input) {
    char *args[MAX_ARGS];
    char *cmd1[MAX_ARGS], *cmd2[MAX_ARGS];
    int pipefd[2];
    int in_redirect = 0, out_redirect = 0, pipe_flag = 0;
    char *in_file = NULL, *out_file = NULL;
    char fileName[MAX_INPUT];

    // Detectar redirecciones y tuberías
    char *pipe_pos = strchr(input, '|');
    char *in_pos = strchr(input, '<');
    char *out_pos = strchr(input, '>');

    if (pipe_pos) {
        *pipe_pos = '\0';
        parse_input(input, cmd1);
        parse_input(pipe_pos + 1, cmd2);
        pipe_flag = 1;
    } else if (in_pos || out_pos) {
        parse_input(input, args);
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "<") == 0) {
                in_redirect = 1;
                in_file = args[i + 1];
                args[i] = NULL;
            } else if (strcmp(args[i], ">") == 0) {
                out_redirect = 1;
                out_file = args[i + 1];
                args[i] = NULL;
            }
        }
    } else {
        parse_input(input, args);
    }

    // Manejar tuberías
    if (pipe_flag) {
        pipe(pipefd);
        if (fork() == 0) { // Primer proceso
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execvp(cmd1[0], cmd1);
            fprintf(stderr, RED "%s: no se encontró la orden\n"RESET, cmd1[0]);
            exit(1);
        }
        if (fork() == 0) { // Segundo proceso
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            execvp(cmd2[0], cmd2);
            fprintf(stderr, RED "%s: no se encontró la orden\n"RESET, cmd2[0]);
            exit(1);
        }
        close(pipefd[0]);
        close(pipefd[1]);
        wait(NULL);
        wait(NULL);
        return;
    }

    // Crear un proceso hijo para ejecutar el comando
    pid_t pid = fork();
    if (pid == 0) {
        if (in_redirect) {
            int fd = open(in_file, O_RDONLY);
            if (fd < 0) {
                perror(RED"Error al abrir el archivo de entrada"RESET);
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (out_redirect) {
            int fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror(RED"Error al abrir el archivo de salida"RESET);
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        execvp(args[0], args);
        //perror(args[0]);
        fprintf(stderr, RED "%s: no se encontró la orden\n"RESET, args[0]);
        exit(1);
    } else if (pid < 0) {
        perror(RED "Error al crear el proceso"RESET);
    } else {
        wait(NULL);
    }
}


int main() {
    char input[MAX_INPUT];
    const char* username = getenv("USER");

    while (1) {
        printf(YELLOW "%s> " RESET, username);
        fflush(stdout);
        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            break;
        }
        if (strcmp(input, "exit\n") == 0) {
            break;
        }
        execute_command(input);
    }

    return 0;
}
