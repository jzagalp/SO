#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(void) {

    // ---------- PASO 1: generador > numeros.txt ----------
    pid_t pid_generador = fork();
    if (pid_generador == -1) {
        perror("fork generador");
        exit(1);
    }
    if (pid_generador == 0) {
        // hijo: abro numeros.txt para escribir.
        // O_CREAT lo crea si no existe y O_TRUNC lo vacia si ya existia
        int fd_numeros = open("numeros.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_numeros == -1) {
            perror("open numeros.txt");
            exit(1);
        }
        dup2(fd_numeros, STDOUT_FILENO);  // la salida estandar (1) ahora va al fichero
        close(fd_numeros);                // ya esta copiado en el 1, este no hace falta

        execlp("./generador", "generador", NULL);
        perror("execlp generador");       // solo se llega aqui si execlp falla
        exit(1);
    }
    // padre: espero a que generador termine de escribir todos los numeros
    waitpid(pid_generador, NULL, 0);

    return 0;
}
