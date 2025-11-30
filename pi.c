/*
 *  OpenMP lecture exercises
 *  Copyright (C) 2011 by Christian Terboven <terboven@rz.rwth-aachen.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */


/*
 * pi.c
 * -----------------------------------------
 * Cálculo aproximado de pi mediante integración numérica
 * usando la regla del punto medio sobre la función:
 *
 *      f(x) = 4 / (1 + x^2)   en el intervalo [0, 1]
 *
 * La integral de esta función es pi, por lo que:
 *
 *      pi ≈ sum_{i=0}^{n-1} f(x_i) * h
 *
 * donde h = 1/n y x_i es el punto medio de cada subintervalo.
 *
 * Uso:
 *      ./pi               -> usa n por defecto (2 000 000 000)
 *      ./pi n             -> usa el valor de n indicado
 *
 * Parámetros:
 *  - n: número de subintervalos (entero positivo).
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* Constantes de configuración y referencia */
static const int    N_INTERVALOS_POR_DEFECTO = 2000000000;
static const double PI_REFERENCIA            = 3.141592653589793238462643;

/* Prototipos de funciones internas */
static double funcion_integrando(double x);
static double calcular_pi_secuencial(int numero_intervalos);
static double obtener_tiempo(void);

int main(int argc, char **argv)
{
    int    numero_intervalos = N_INTERVALOS_POR_DEFECTO;
    double pi_aproximado     = 0.0;
    double tiempo_inicio     = 0.0;
    double tiempo_fin        = 0.0;

    /* Permitir que el usuario sobreescriba el número de intervalos */
    if (argc >= 2) {
        numero_intervalos = atoi(argv[1]);
    }

    if (numero_intervalos <= 0 || numero_intervalos > 2147483647) {
        fprintf(stderr,
                "Error: el valor de n debe estar entre 1 y 2147483647.\n");
        return EXIT_FAILURE;
    }

    /* Medimos solo el tiempo del cálculo numérico de pi */
    tiempo_inicio   = obtener_tiempo();
    pi_aproximado   = calcular_pi_secuencial(numero_intervalos);
    tiempo_fin      = obtener_tiempo();

    printf("\nConfiguración:\n");
    printf("  n (subintervalos) = %d\n", numero_intervalos);

    printf("\npi se aproxima a      = %.20f\n", pi_aproximado);
    printf("Error absoluto        = %.20f\n",
           fabs(pi_aproximado - PI_REFERENCIA));
    printf("Tiempo secuencial (s) = %.6f\n",
           tiempo_fin - tiempo_inicio);

    return EXIT_SUCCESS;
}

/*
 * funcion_integrando
 * -----------------------------------------
 * f(x) = 4 / (1 + x^2)
 * Función cuya integral en [0, 1] es pi.
 */
static double funcion_integrando(double x)
{
    return 4.0 / (1.0 + x * x);
}

/*
 * calcular_pi_secuencial
 * -----------------------------------------
 * Realiza la integración numérica usando la regla
 * del punto medio con 'numero_intervalos' subintervalos.
 *
 * Parámetros:
 *  - numero_intervalos: cantidad de subintervalos (n > 0).
 *
 * Retorna:
 *  - Aproximación de pi como número de doble precisión.
 */
static double calcular_pi_secuencial(int numero_intervalos)
{
    const double paso = 1.0 / (double)numero_intervalos;
    double suma       = 0.0;

    for (int i = 0; i < numero_intervalos; ++i) {
        double x_punto_medio = paso * ((double)i + 0.5);
        suma += funcion_integrando(x_punto_medio);
    }

    return paso * suma;
}

/*
 * obtener_tiempo
 * -----------------------------------------
 * Retorna un timestamp en segundos, usando un reloj
 * de alta resolución (CLOCK_MONOTONIC).
 *
 * Esta función es útil para medir tiempos de ejecución.
 */
static double obtener_tiempo(void)
{
    struct timespec instante;

    clock_gettime(CLOCK_MONOTONIC, &instante);

    return (double)instante.tv_sec + (double)instante.tv_nsec * 1e-9;
}
