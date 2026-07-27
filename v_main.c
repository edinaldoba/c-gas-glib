/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <math.h>

#include "gas.h"
#include "v_gas.h"

// ============================================================================
// PROTÓTIPOS INTERNOS
// ============================================================================
static void         display_tempo( const char *descricao, GTimer *cronometro );
static VgasLimites *v_gas_limites( int nrow, int ncol );

// ============================================================================
// FUNÇÃO PRINCIPAL
// ============================================================================
int main ( void ) {
    g_autoptr( GTimer ) cronometro = g_timer_new();

    guint32 sementes[GAS_NUM_SEMENTES];
    gas_gerar_sementes( sementes );
    g_autoptr( GRand ) rand_context = g_rand_new_with_seed_array( sementes, G_N_ELEMENTS(sementes) );

    GasParametros par = {
        .n_pop     = 100,
        .n_gen     = 82,
        .n_tor     = 2,
        .p_rec     = 0.75,
        .p_mut     = 0.95,
        .peso_disp = 1.0,
        .toleracia = 1.0e-2,
        .rand      = rand_context
    };

    int ncol = 960;
    int nrow = 640;
    VgasLimites *v_lim = v_gas_limites( nrow, ncol );

    // Ponteiro de função para a avaliação (a ser estruturada)
    double (*v_gas_avaliar)(const double*, GasPopulacao*, const int, ImagemCinza *img) = NULL;

    // Executa o pipeline de evolução
    GasPopulacao *melhor = v_gas_pipeline( &par, v_lim, v_gas_avaliar, gas_comparar_objetivo_max );

    // ------------------------------------------------------------------------
    // LIMPEZA DE MEMÓRIA (DEEP FREE)
    // ------------------------------------------------------------------------
    for ( int k = 0; k < 4; k++ ) {
        g_free( melhor[k].x );

        // Libera as alocações internas dos limites feitas em v_gas_limites
        g_free( v_lim->lim[k].ini );
        g_free( v_lim->lim[k].fim );
    }
    g_free( melhor );
    g_free( v_lim->lim );
    g_free( v_lim );

    display_tempo( "Evolução", cronometro );

    return 0;
}

// ============================================================================
// IMPLEMENTAÇÕES DAS FUNÇÕES
// ============================================================================

static VgasLimites *v_gas_limites( int nrow, int ncol ) {
    // Validação estrita padrão GLib
    g_return_val_if_fail( nrow > 0 && ncol > 0, NULL );

    int n_dim = 2; // 0: eixo X (colunas), 1: eixo Y (linhas)

    // Calcula os pontos médios e limites máximos da imagem
    double mid_x = (ncol - 1.0) / 2.0;
    double mid_y = (nrow - 1.0) / 2.0;
    double max_x = ncol - 1.0;
    double max_y = nrow - 1.0;

    // Matrizes de limites para os 4 quadrantes (Sentido horário)
    // k=0 (Topo-Esquerda)   | k=1 (Topo-Direita)
    // ----------------------+----------------------
    // k=3 (Base-Esquerda)   | k=2 (Base-Direita)
    double ini_x[4] = { 0.0,   mid_x, mid_x, 0.0   };
    double fim_x[4] = { mid_x, max_x, max_x, mid_x };

    double ini_y[4] = { 0.0,   0.0,   mid_y, mid_y };
    double fim_y[4] = { mid_y, mid_y, max_y, max_y };

    VgasLimites *v_lim = g_new0( VgasLimites, 1 );

    // Aloca o array principal de limites (4 quadrantes)
    v_lim->lim = g_new0( GasLimites, 4 );

    for ( int k = 0; k < 4; k++ ) {
        v_lim->lim[k].n_dim = n_dim;
        v_lim->lim[k].ini   = g_new0( double, n_dim );
        v_lim->lim[k].fim   = g_new0( double, n_dim );

        // Preenche os limites da Dimensão 0 (Eixo X)
        v_lim->lim[k].ini[0] = ini_x[k];
        v_lim->lim[k].fim[0] = fim_x[k];

        // Preenche os limites da Dimensão 1 (Eixo Y)
        v_lim->lim[k].ini[1] = ini_y[k];
        v_lim->lim[k].fim[1] = fim_y[k];
    }

    return v_lim;
}

static void display_tempo( const char *descricao, GTimer *cronometro ) {
    double tempo_segundos = g_timer_elapsed( cronometro, NULL );

    if ( tempo_segundos > 60.0 ) {
        int minutos = ( int )( tempo_segundos / 60 );
        double segundos_restantes = tempo_segundos - ( minutos * 60 );
        printf( "⏱ %s concluída em %d min e %.2f seg.\n", descricao, minutos, segundos_restantes );
    } else {
        printf( "⏱ %s concluída em %.3f segundos.\n", descricao, tempo_segundos );
    }
}
