/*
 * acueducto.c
 *
 * Practica 4: ejercicio de procesos y pipes
 * Sistemas Operativos - UPNA
 *
 * Hace lo mismo que estos comandos de la shell:
 *
 *   ./generador > numeros.txt
 *   ./dec2rom < numeros.txt | ./rom2dec > vuelta.txt
 *   diff numeros.txt vuelta.txt > /dev/null
 *
 * y segun lo que devuelva diff dice si los dos ficheros son iguales.
 *
 * Compilar y ejecutar (en la carpeta donde estan generador, dec2rom y rom2dec):
 *   gcc -Wall -o acueducto acueducto.c
 *   ./acueducto
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FICHERO_NUMEROS "numeros.txt"
#define FICHERO_VUELTA  "vuelta.txt"

int main(void) {
    pid_t pid_generador, pid_dec2rom, pid_rom2dec, pid_diff;
    int tuberia[2];          /* tuberia[0] = lectura, tuberia[1] = escritura */
    int fd;
    int estado_generador, estado_dec2rom, estado_rom2dec, estado_diff;

    /* ------------------------------------------------------------------ */
    /* Paso 1: ./generador > numeros.txt                                   */
    /* ------------------------------------------------------------------ */
    pid_generador = fork();
    if (pid_generador == -1) {
        perror("fork generador");
        exit(1);
    }

    if (pid_generador == 0) {
        /* Hijo: la salida estandar va al fichero numeros.txt */
        fd = open(FICHERO_NUMEROS, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open " FICHERO_NUMEROS);
            exit(1);
        }
        if (dup2(fd, 1) == -1) {
            perror("dup2 generador");
            exit(1);
        }
        close(fd);

        execlp("./generador", "generador", NULL);
        perror("execlp generador");    /* solo llega aqui si execlp falla */
        exit(1);
    }

    /* Padre: hay que esperar a que generador acabe antes de leer el fichero */
    if (waitpid(pid_generador, &estado_generador, 0) == -1) {
        perror("waitpid generador");
        exit(1);
    }
    if (!WIFEXITED(estado_generador) || WEXITSTATUS(estado_generador) != 0) {
        fprintf(stderr, "Error: generador no ha terminado correctamente\n");
        exit(1);
    }

    /* ------------------------------------------------------------------ */
    /* Paso 2: ./dec2rom < numeros.txt | ./rom2dec > vuelta.txt            */
    /* ------------------------------------------------------------------ */
    if (pipe(tuberia) == -1) {
        perror("pipe");
        exit(1);
    }

    /* Primer hijo: dec2rom */
    pid_dec2rom = fork();
    if (pid_dec2rom == -1) {
        perror("fork dec2rom");
        exit(1);
    }

    if (pid_dec2rom == 0) {
        /* La entrada estandar viene de numeros.txt (solo lectura) */
        fd = open(FICHERO_NUMEROS, O_RDONLY);
        if (fd == -1) {
            perror("open " FICHERO_NUMEROS);
            exit(1);
        }
        if (dup2(fd, 0) == -1) {
            perror("dup2 dec2rom entrada");
            exit(1);
        }
        close(fd);

        /* La salida estandar va al extremo de escritura de la tuberia */
        if (dup2(tuberia[1], 1) == -1) {
            perror("dup2 dec2rom salida");
            exit(1);
        }
        close(tuberia[0]);   /* dec2rom no lee de la tuberia */
        close(tuberia[1]);   /* ya esta copiado en el descriptor 1 */

        execlp("./dec2rom", "dec2rom", NULL);
        perror("execlp dec2rom");
        exit(1);
    }

    /* Segundo hijo: rom2dec */
    pid_rom2dec = fork();
    if (pid_rom2dec == -1) {
        perror("fork rom2dec");
        exit(1);
    }

    if (pid_rom2dec == 0) {
        /* La entrada estandar viene del extremo de lectura de la tuberia */
        if (dup2(tuberia[0], 0) == -1) {
            perror("dup2 rom2dec entrada");
            exit(1);
        }
        close(tuberia[0]);   /* ya esta copiado en el descriptor 0 */
        close(tuberia[1]);   /* si no se cierra, rom2dec nunca recibe fin de fichero */

        /* La salida estandar va a vuelta.txt */
        fd = open(FICHERO_VUELTA, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open " FICHERO_VUELTA);
            exit(1);
        }
        if (dup2(fd, 1) == -1) {
            perror("dup2 rom2dec salida");
            exit(1);
        }
        close(fd);

        execlp("./rom2dec", "rom2dec", NULL);
        perror("execlp rom2dec");
        exit(1);
    }

    /* Padre: cierra los dos extremos de la tuberia ANTES de esperar.
       Si el padre dejara abierto tuberia[1], rom2dec se quedaria
       esperando datos para siempre y acueducto no terminaria. */
    close(tuberia[0]);
    close(tuberia[1]);

    if (waitpid(pid_dec2rom, &estado_dec2rom, 0) == -1) {
        perror("waitpid dec2rom");
        exit(1);
    }
    if (waitpid(pid_rom2dec, &estado_rom2dec, 0) == -1) {
        perror("waitpid rom2dec");
        exit(1);
    }

    if (!WIFEXITED(estado_dec2rom) || WEXITSTATUS(estado_dec2rom) != 0) {
        fprintf(stderr, "Error: dec2rom no ha terminado correctamente\n");
        exit(1);
    }
    if (!WIFEXITED(estado_rom2dec) || WEXITSTATUS(estado_rom2dec) != 0) {
        fprintf(stderr, "Error: rom2dec no ha terminado correctamente\n");
        exit(1);
    }

    /* ------------------------------------------------------------------ */
    /* Paso 3: diff numeros.txt vuelta.txt > /dev/null                     */
    /* ------------------------------------------------------------------ */
    pid_diff = fork();
    if (pid_diff == -1) {
        perror("fork diff");
        exit(1);
    }

    if (pid_diff == 0) {
        /* No queremos ver las diferencias, solo el codigo de salida */
        fd = open("/dev/null", O_WRONLY);
        if (fd == -1) {
            perror("open /dev/null");
            exit(1);
        }
        if (dup2(fd, 1) == -1) {
            perror("dup2 diff");
            exit(1);
        }
        close(fd);

        /* diff esta en el PATH, por eso no lleva ./ delante */
        execlp("diff", "diff", FICHERO_NUMEROS, FICHERO_VUELTA, NULL);
        perror("execlp diff");
        exit(2);   /* diff usa 2 para indicar error */
    }

    if (waitpid(pid_diff, &estado_diff, 0) == -1) {
        perror("waitpid diff");
        exit(1);
    }

    /* ------------------------------------------------------------------ */
    /* Paso 4: resultado segun el codigo de salida de diff                 */
    /* ------------------------------------------------------------------ */
    if (WIFEXITED(estado_diff) && WEXITSTATUS(estado_diff) == 0) {
        printf("Ambos documentos son idénticos\n");
        return 0;
    } else {
        printf("Hay discrepancia entre documentos\n");
        return 1;
    }
}
