// acueducto.c - Práctica 4: procesos y pipes
//
// Hace lo mismo que estos comandos:
//   ./generador > numeros.txt
//   ./dec2rom < numeros.txt | ./rom2dec > vuelta.txt
//   diff numeros.txt vuelta.txt > /dev/null

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(void) {
    int fd;
    int tuberia[2];
    int estado;
    pid_t pid_generador, pid_dec2rom, pid_rom2dec, pid_diff;

    // 1. generador escribe los números en numeros.txt
    pid_generador = fork();
    if (pid_generador == -1) {
        perror("fork");
        exit(1);
    }
    if (pid_generador == 0) {
        fd = open("numeros.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open numeros.txt");
            exit(1);
        }
        dup2(fd, 1);    // la salida va al fichero
        close(fd);
        execlp("./generador", "generador", NULL);
        perror("execlp generador");
        exit(1);
    }
    waitpid(pid_generador, NULL, 0);    // esperamos a que acabe

    // 2. dec2rom lee numeros.txt y manda los romanos por la tubería a rom2dec,
    //    que escribe el resultado en vuelta.txt
    if (pipe(tuberia) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_dec2rom = fork();
    if (pid_dec2rom == -1) {
        perror("fork");
        exit(1);
    }
    if (pid_dec2rom == 0) {
        fd = open("numeros.txt", O_RDONLY);
        if (fd == -1) {
            perror("open numeros.txt");
            exit(1);
        }
        dup2(fd, 0);            // la entrada viene del fichero
        close(fd);
        dup2(tuberia[1], 1);    // la salida va a la tubería
        close(tuberia[0]);
        close(tuberia[1]);
        execlp("./dec2rom", "dec2rom", NULL);
        perror("execlp dec2rom");
        exit(1);
    }

    pid_rom2dec = fork();
    if (pid_rom2dec == -1) {
        perror("fork");
        exit(1);
    }
    if (pid_rom2dec == 0) {
        fd = open("vuelta.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open vuelta.txt");
            exit(1);
        }
        dup2(tuberia[0], 0);    // la entrada viene de la tubería
        dup2(fd, 1);            // la salida va al fichero
        close(tuberia[0]);
        close(tuberia[1]);
        close(fd);
        execlp("./rom2dec", "rom2dec", NULL);
        perror("execlp rom2dec");
        exit(1);
    }

    // el padre cierra la tubería, si no rom2dec no termina nunca
    close(tuberia[0]);
    close(tuberia[1]);
    waitpid(pid_dec2rom, NULL, 0);
    waitpid(pid_rom2dec, NULL, 0);

    // 3. diff compara los dos ficheros (su salida no se muestra)
    pid_diff = fork();
    if (pid_diff == -1) {
        perror("fork");
        exit(1);
    }
    if (pid_diff == 0) {
        fd = open("/dev/null", O_WRONLY);
        if (fd == -1) {
            perror("open /dev/null");
            exit(1);
        }
        dup2(fd, 1);
        close(fd);
        execlp("diff", "diff", "numeros.txt", "vuelta.txt", NULL);
        perror("execlp diff");
        exit(1);
    }
    waitpid(pid_diff, &estado, 0);

    // 4. diff devuelve 0 si los ficheros son iguales
    if (WIFEXITED(estado) && WEXITSTATUS(estado) == 0) {
        printf("Ambos documentos son idénticos\n");
        return 0;
    } else {
        printf("Hay discrepancia entre documentos\n");
        return 1;
    }
}
