/*
 * pi_p.c
 * -----------------------------------------
 * Versión paralela del cálculo de pi usando Pthreads.
 *
 * Idea base:
 *
 *   pi ≈ integral_0^1 f(x) dx
 *   donde f(x) = 4 / (1 + x^2)
 *
 * Se divide el rango de iteraciones [0, n) entre H hilos.
 * Cada hilo calcula una suma parcial de f(x_i) y el hilo principal
 * suma esos resultados para obtener la aproximación final.
 *
 * Uso:
 *      ./pi_p               -> H = 4 (por defecto), n = 2 000 000 000
 *      ./pi_p H             -> usa H hilos, n por defecto
 *      ./pi_p H n           -> usa H hilos y n subintervalos
 *
 * Parámetros:
 *  - H: número de hilos (entero positivo).
 *  - n: número de subintervalos (entero positivo).
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

/* Constantes de configuración y referencia */
static const int    N_INTERVALOS_POR_DEFECTO = 2000000000;
static const int    HILOS_POR_DEFECTO       = 4;
static const double PI_REFERENCIA           = 3.141592653589793238462643;

/*
 * DatosHilo
 * -----------------------------------------
 * Estructura que describe la porción de trabajo asignada a un hilo:
 *  - indice_inicio: primer índice de iteración (inclusive).
 *  - indice_fin   : último índice de iteración (exclusive).
 *  - paso         : ancho del subintervalo, h = 1.0 / n.
 */
typedef struct {
    int    indice_inicio;
    int    indice_fin;
    double paso;
} DatosHilo;

/* Prototipos de funciones internas */
static double calcular_pi_paralelo(int numero_intervalos, int numero_hilos);
static void  *trabajo_suma_parcial(void *argumento);
static double obtener_tiempo(void);
static void   mostrar_uso(const char *nombre_programa);

int main(int argc, char **argv)
{
    int numero_hilos     = HILOS_POR_DEFECTO;
    int numero_intervalos = N_INTERVALOS_POR_DEFECTO;

    if (argc >= 2) {
        numero_hilos = atoi(argv[1]);
    }
    if (argc >= 3) {
        numero_intervalos = atoi(argv[2]);
    }

    if (numero_hilos <= 0) {
        fprintf(stderr,
                "Advertencia: número de hilos inválido (%d). Se usará 1 hilo.\n",
                numero_hilos);
        numero_hilos = 1;
    }

    if (numero_intervalos <= 0 || numero_intervalos > 2147483647) {
        fprintf(stderr,
                "Error: el valor de n debe estar entre 1 y 2147483647.\n");
        mostrar_uso(argv[0]);
        return EXIT_FAILURE;
    }

    double tiempo_inicio   = obtener_tiempo();
    double pi_aproximado   = calcular_pi_paralelo(numero_intervalos,
                                                  numero_hilos);
    double tiempo_fin      = obtener_tiempo();

    printf("\nConfiguración:\n");
    printf("  n (subintervalos) = %d\n", numero_intervalos);
    printf("  H (hilos)         = %d\n", numero_hilos);

    printf("\npi se aproxima a      = %.20f\n", pi_aproximado);
    printf("Error absoluto        = %.20f\n",
           fabs(pi_aproximado - PI_REFERENCIA));
    printf("Tiempo paralelo (s)   = %.6f\n",
           tiempo_fin - tiempo_inicio);

    return EXIT_SUCCESS;
}

/*
 * mostrar_uso
 * -----------------------------------------
 * Muestra brevemente cómo usar el programa.
 */
static void mostrar_uso(const char *nombre_programa)
{
    fprintf(stderr,
            "Uso:\n"
            "  %s              -> H = %d, n = %d\n"
            "  %s H            -> H hilos, n por defecto\n"
            "  %s H n          -> H hilos y n subintervalos\n",
            nombre_programa,
            HILOS_POR_DEFECTO,
            N_INTERVALOS_POR_DEFECTO,
            nombre_programa,
            nombre_programa);
}

/*
 * trabajo_suma_parcial
 * -----------------------------------------
 * Función ejecutada por cada hilo.
 *
 * Parámetro:
 *  - argumento: puntero a DatosHilo, con el rango [indice_inicio, indice_fin)
 *    y el valor del paso h.
 *
 * Comportamiento:
 *  - Recorre su subrango de índices.
 *  - Para cada i, calcula x_i = h * (i + 0.5).
 *  - Acumula localmente 4 / (1 + x_i^2).
 *  - Reserva memoria para un double (malloc) donde almacena
 *    la suma parcial.
 *  - Retorna dicho puntero mediante pthread_exit.
 */
static void *trabajo_suma_parcial(void *argumento)
{
    DatosHilo *datos = (DatosHilo *)argumento;
    double suma_local = 0.0;

    for (int i = datos->indice_inicio; i < datos->indice_fin; ++i) {
        double x = datos->paso * ((double)i + 0.5);
        suma_local += 4.0 / (1.0 + x * x);
    }

    double *resultado = (double *)malloc(sizeof(double));
    if (resultado == NULL) {
        /* En caso de error de memoria, se retorna NULL.
         * El hilo principal decidirá cómo manejarlo. */
        pthread_exit(NULL);
    }

    *resultado = suma_local;
    pthread_exit(resultado);
}

/*
 * calcular_pi_paralelo
 * -----------------------------------------
 * Divide el trabajo de la integración numérica entre
 * 'numero_hilos' hilos, recolecta las sumas parciales y
 * retorna la aproximación final de pi.
 *
 * Parámetros:
 *  - numero_intervalos: número total de subintervalos.
 *  - numero_hilos     : número de hilos a crear.
 *
 * Retorna:
 *  - Aproximación de pi como número de doble precisión.
 *
 * Notas:
 *  - En caso de error grave de memoria o creación de hilos,
 *    se imprime un mensaje y el programa termina con EXIT_FAILURE.
 */
static double calcular_pi_paralelo(int numero_intervalos, int numero_hilos)
{
    const double paso = 1.0 / (double)numero_intervalos;

    /* Reservamos arreglos para IDs de hilos y sus datos */
    pthread_t *hilos = (pthread_t *)malloc(sizeof(pthread_t) * numero_hilos);
    DatosHilo *datos_hilos =
        (DatosHilo *)malloc(sizeof(DatosHilo) * numero_hilos);

    if (hilos == NULL || datos_hilos == NULL) {
        fprintf(stderr, "Error: fallo al reservar memoria para hilos.\n");
        free(hilos);
        free(datos_hilos);
        exit(EXIT_FAILURE);
    }

    /* Particionamiento del rango [0, n) en bloques casi iguales */
    int tam_bloque = numero_intervalos / numero_hilos;
    int resto      = numero_intervalos % numero_hilos;

    int inicio_actual = 0;

    for (int h = 0; h < numero_hilos; ++h) {
        int extra = (h < resto) ? 1 : 0;

        datos_hilos[h].indice_inicio = inicio_actual;
        datos_hilos[h].indice_fin    = inicio_actual + tam_bloque + extra;
        datos_hilos[h].paso          = paso;

        inicio_actual = datos_hilos[h].indice_fin;

        int codigo = pthread_create(&hilos[h],
                                    NULL,
                                    trabajo_suma_parcial,
                                    &datos_hilos[h]);
        if (codigo != 0) {
            fprintf(stderr,
                    "Error al crear el hilo %d (código %d).\n", h, codigo);
            /* Esperamos los hilos ya creados antes de abortar */
            for (int j = 0; j < h; ++j) {
                pthread_join(hilos[j], NULL);
            }
            free(hilos);
            free(datos_hilos);
            exit(EXIT_FAILURE);
        }
    }

    /* Recolección de resultados parciales */
    double suma_global = 0.0;

    for (int h = 0; h < numero_hilos; ++h) {
        void *retorno = NULL;
        int codigo = pthread_join(hilos[h], &retorno);
        if (codigo != 0) {
            fprintf(stderr,
                    "Error en pthread_join para el hilo %d (código %d).\n",
                    h, codigo);
            continue;
        }

        if (retorno != NULL) {
            double *suma_parcial = (double *)retorno;
            suma_global += *suma_parcial;
            free(suma_parcial);
        } else {
            fprintf(stderr,
                    "Advertencia: el hilo %d retornó NULL.\n", h);
        }
    }

    free(hilos);
    free(datos_hilos);

    return paso * suma_global;
}

/*
 * obtener_tiempo
 * -----------------------------------------
 * Retorna un timestamp en segundos, usando un reloj
 * de alta resolución (CLOCK_MONOTONIC).
 */
static double obtener_tiempo(void)
{
    struct timespec instante;

    clock_gettime(CLOCK_MONOTONIC, &instante);

    return (double)instante.tv_sec + (double)instante.tv_nsec * 1e-9;
}
