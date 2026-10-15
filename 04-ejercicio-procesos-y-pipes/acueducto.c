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

    // ---------- PASOS 2 y 3: dec2rom < numeros.txt | rom2dec > vuelta.txt ----------
    // la pipe se crea en el padre ANTES de los fork, asi los dos hijos la heredan
    int fds[2];   // fds[0] = extremo de lectura, fds[1] = extremo de escritura
    if (pipe(fds) == -1) {
        perror("pipe");
        exit(1);
    }

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

        dup2(fds[1], STDOUT_FILENO);      // lo que escriba dec2rom va a la pipe
        close(fds[0]);                    // dec2rom no lee de la pipe
        close(fds[1]);                    // ya esta copiado en el 1

        execlp("./dec2rom", "dec2rom", NULL);
        perror("execlp dec2rom");
        exit(1);
    }

    pid_t pid_rom2dec = fork();
    if (pid_rom2dec == -1) {
        perror("fork rom2dec");
        exit(1);
    }
    if (pid_rom2dec == 0) {
        // hijo: rom2dec lee de la pipe
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
        close(fds[1]);   // importante: si rom2dec tiene abierta la escritura, nunca le llega el EOF

        // y escribe en vuelta.txt (se crea o se vacia igual que numeros.txt)
        int fd_salida = open("vuelta.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_salida == -1) {
            perror("open vuelta.txt");
            exit(1);
        }
        dup2(fd_salida, STDOUT_FILENO);
        close(fd_salida);

        execlp("./rom2dec", "rom2dec", NULL);
        perror("execlp rom2dec");
        exit(1);
    }

    // padre: no usa la pipe, asi que cierra los dos extremos ANTES de esperar.
    // si el padre se deja abierto fds[1], rom2dec se queda esperando el EOF para siempre
    close(fds[0]);
    close(fds[1]);

    waitpid(pid_dec2rom, NULL, 0);
    waitpid(pid_rom2dec, NULL, 0);

    return 0;
}
