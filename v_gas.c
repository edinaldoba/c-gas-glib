#include <stdio.h>
#include <string.h> // Necessário para memcpy
#include <glib.h>

#include "v_gas.h"

// ============================================================================
// PIPELINE PRINCIPAL DE COEVOLUÇÃO
// ============================================================================
GasPopulacao *v_gas_pipeline( const ImagemCinza *img,
                              const GasParametros *par,
                              const GasLimites *lim,
                              double (*v_gas_avaliar)(const double*, GasPopulacao*, const int, const ImagemCinza *img),
                              int (*gas_comparar)(const void* a, const void* b) )
{
    g_return_val_if_fail( par && lim && v_gas_avaliar && gas_comparar, NULL );

    // Alocação da matriz de dispersão
    double **coef_disp = g_new0( double*, par->n_obj );
    GasPopulacao **pop = g_new0( GasPopulacao*, par->n_obj );
    GasGenitores **gen = g_new0( GasGenitores*, par->n_obj );

    for ( int k = 0; k < par->n_obj; k++ ) {
        coef_disp[k] = g_new0( double, lim[k].n_dim );

        pop[k] = gas_alocar_populacao( par->n_pop, lim[k].n_dim );
        gen[k] = gas_alocar_genitores( par->n_gen, lim[k].n_dim );

        gas_populacao_inicial( pop[k], par, &lim[k] );
    }

    GasPopulacao *elite = gas_alocar_populacao( par->n_obj, lim[0].n_dim );

    int geracao = 0;
    g_autofree double *dispersao_max = g_new0( double, par->n_obj );

    // Geração 0 (Avaliação Exploratória)
    for ( int k = 0; k < par->n_obj; k++ ) {
        for ( int i = 0; i < par->n_pop; i++ ) {
            pop[k][i].fitness = v_gas_avaliar( pop[k][i].x, NULL, lim[k].n_dim, img );
        }
        qsort( pop[k], par->n_pop, sizeof(GasPopulacao), gas_comparar );

        memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof(double) );
        elite[k].fitness = pop[k][par->n_pop - 1].fitness;

        gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
    }

    // Laço Evolutivo
    do {
        geracao++;

        for ( int k = 0; k < par->n_obj; k++ ) {
            gas_torneio( pop[k], gen[k], lim[k].n_dim, par, gas_comparar );
            gas_crossover_aritmetico( pop[k], gen[k], lim[k].n_dim, par );
            gas_mutacao_creep( pop[k], coef_disp[k], &lim[k], par );

            for ( int i = 0; i < par->n_pop; i++ ) {
                pop[k][i].fitness = v_gas_avaliar( pop[k][i].x, elite, lim[k].n_dim, img );
            }
            qsort( pop[k], par->n_pop, sizeof(GasPopulacao), gas_comparar );

            memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof(double) );
            elite[k].fitness = pop[k][par->n_pop - 1].fitness;

            gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
            dispersao_max[k] = gas_max( coef_disp[k], lim[k].n_dim );
        }

    } while ( gas_max(dispersao_max, par->n_obj) > par->toleracia && geracao < 1000 );

    // ------------------------------------------------------------------------
    // LIMPEZA DE RECURSOS DO PIPELINE
    // ------------------------------------------------------------------------
    for ( int k = 0; k < par->n_obj; k++ ) {
        g_free( coef_disp[k] );
        gas_liberar_populacao( pop[k], lim[k].n_dim );
        gas_liberar_genitores( gen[k], lim[k].n_dim );
    }

    g_free( pop );
    g_free( gen );
    g_free( coef_disp );

    // Retorna as âncoras limpas e seguras
    return elite;
}


