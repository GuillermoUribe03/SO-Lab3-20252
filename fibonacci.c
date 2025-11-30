/*
 * fibonacci.c
 * -----------------------------------------
 * Genera una secuencia de N números de Fibonacci utilizando
 * un hilo trabajador (worker thread).
 *
 * El hilo principal:
 *  - Lee N desde la línea de comandos.
 *  - Reserva un arreglo compartido de tamaño N.
 *  - Empaqueta el puntero al arreglo y el valor N en una
 *    estructura de argumentos.
 *  - Crea un hilo trabajador, pasándole esa estructura.
 *  - Espera a que el hilo termine (pthread_join).
 *  - Imprime la secuencia resultante.
 *
 * El hilo trabajador:
 *  - Recibe la estructura con el puntero al arreglo y N.
 *  - Rellena el arreglo con los primeros N términos de la
 *    sucesión de Fibonacci.
 *
 * Convención usada:
 *  F(0) = 0
 *  F(1) = 1
 *  F(2) = 1
 *  F(3) = 2
 *  ...
 *
 * Uso:
 *      ./fibonacci N
 *
 * Parámetros:
 *  - N: entero mayor o igual a 0.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* Tipo de dato para los valores de Fibonacci */
typedef unsigned long long tipo_fibonacci;

/*
 * ArgumentosFibonacci
 * -----------------------------------------
 * Estructura para pasar múltiples parámetros al hilo:
 *  - arreglo: puntero al arreglo compartido donde se almacenará la secuencia.
 *  - cantidad: número de términos a generar (N >= 0).
 */
typedef struct {
    tipo_fibonacci *arreglo;
    int             cantidad;
} ArgumentosFibonacci;

/* Prototipos de funciones internas */
static void *trabajador_fibonacci(void *argumento);
static void  mostrar_uso(const char *nombre_programa);

int main(int argc, char **argv)
{
    if (argc < 2) {
        mostrar_uso(argv[0]);
        return EXIT_FAILURE;
    }

    int cantidad = atoi(argv[1]);
    if (cantidad < 0) {
        fprintf(stderr,
                "Error: N debe ser un entero mayor o igual a 0.\n");
        return EXIT_FAILURE;
    }

    /* Caso N = 0: no hay nada que generar ni imprimir */
    if (cantidad == 0) {
        return EXIT_SUCCESS;
    }

    /* Reserva dinámica para el arreglo de Fibonacci */
    tipo_fibonacci *secuencia =
        (tipo_fibonacci *)malloc(sizeof(tipo_fibonacci) * (size_t)cantidad);
    if (secuencia == NULL) {
        perror("Error en malloc para secuencia");
        return EXIT_FAILURE;
    }

    /* Reserva y carga de la estructura de argumentos para el hilo */
    ArgumentosFibonacci *argumentos =
        (ArgumentosFibonacci *)malloc(sizeof(ArgumentosFibonacci));
    if (argumentos == NULL) {
        perror("Error en malloc para ArgumentosFibonacci");
        free(secuencia);
        return EXIT_FAILURE;
    }

    argumentos->arreglo  = secuencia;
    argumentos->cantidad = cantidad;

    pthread_t hilo_trabajador;
    int codigo = pthread_create(&hilo_trabajador,
                                NULL,
                                trabajador_fibonacci,
                                (void *)argumentos);
    if (codigo != 0) {
        fprintf(stderr,
                "Error al crear el hilo (código %d).\n", codigo);
        free(argumentos);
        free(secuencia);
        return EXIT_FAILURE;
    }

    /* Esperamos a que el hilo termine antes de acceder al arreglo */
    codigo = pthread_join(hilo_trabajador, NULL);
    if (codigo != 0) {
        fprintf(stderr,
                "Error en pthread_join (código %d).\n", codigo);
        free(argumentos);
        free(secuencia);
        return EXIT_FAILURE;
    }

    /* Impresión de la secuencia generada */
    for (int i = 0; i < cantidad; ++i) {
        if (i > 0) {
            printf(" ");
        }
        printf("%llu", (unsigned long long)secuencia[i]);
    }
    printf("\n");

    free(argumentos);
    free(secuencia);

    return EXIT_SUCCESS;
}

/*
 * mostrar_uso
 * -----------------------------------------
 * Muestra un mensaje de ayuda con el formato de uso del programa.
 */
static void mostrar_uso(const char *nombre_programa)
{
    fprintf(stderr, "Uso: %s N\n", nombre_programa);
    fprintf(stderr, "  N: número de términos de Fibonacci (N >= 0).\n");
}

/*
 * trabajador_fibonacci
 * -----------------------------------------
 * Función que ejecuta el hilo trabajador.
 *
 * Parámetro:
 *  - argumento: puntero a ArgumentosFibonacci con:
 *      - arreglo  : arreglo compartido donde se escribirá la secuencia.
 *      - cantidad : número de términos a generar.
 *
 * Comportamiento:
 *  - Maneja casos pequeños (N = 1, N = 2).
 *  - Para N >= 3, calcula los términos de forma iterativa:
 *      F(i) = F(i - 1) + F(i - 2)
 *  - No retorna datos mediante pthread_exit; la comunicación se
 *    realiza a través del arreglo compartido.
 */
static void *trabajador_fibonacci(void *argumento)
{
    ArgumentosFibonacci *argumentos = (ArgumentosFibonacci *)argumento;
    tipo_fibonacci *arreglo         = argumentos->arreglo;
    int cantidad                    = argumentos->cantidad;

    if (cantidad <= 0) {
        pthread_exit(NULL);
    }

    /* Casos base */
    if (cantidad >= 1) {
        arreglo[0] = 0ULL;
    }
    if (cantidad >= 2) {
        arreglo[1] = 1ULL;
    }

    /* Caso general: cálculo iterativo para i >= 2 */
    for (int i = 2; i < cantidad; ++i) {
        arreglo[i] = arreglo[i - 1] + arreglo[i - 2];
    }

    pthread_exit(NULL);
}
