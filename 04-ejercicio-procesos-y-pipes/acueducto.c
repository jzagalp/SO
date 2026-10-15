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

    // ---------- PASO 2: dec2rom < numeros.txt (de momento sale por pantalla) ----------
    pid_t pid_dec2rom = fork();
    if (pid_dec2rom == -1) {
        perror("fork dec2rom");
        exit(1);
    }
    if (pid_dec2rom == 0) {
        // hijo: numeros.txt solo hay que leerlo, asi que basta con O_RDONLY
        // (si pongo O_TRUNC aqui se borra el fichero!)
        int fd_entrada = open("numeros.txt", O_RDONLY);
        if (fd_entrada == -1) {
            perror("open numeros.txt");
            exit(1);
        }
        dup2(fd_entrada, STDIN_FILENO);   // la entrada estandar (0) ahora es el fichero
        close(fd_entrada);

        execlp("./dec2rom", "dec2rom", NULL);
        perror("execlp dec2rom");
        exit(1);
    }
    waitpid(pid_dec2rom, NULL, 0);

    return 0;
}
