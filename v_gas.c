#include <stdio.h>
#include <string.h> // Necessário para memcpy
#include <glib.h>

#include "gas.h"
#include "v_gas.h"

// ============================================================================
// FUNÇÕES DE ALOCAÇÃO E LIBERAÇÃO (VISÃO COMPUTACIONAL)
// ============================================================================

static VgasPopulacao *v_gas_alocar_populacao( const int n_pop, const GasLimites *lim ) {
    g_return_val_if_fail( lim && n_pop > 0, NULL );

    VgasPopulacao *v_pop = g_new0( VgasPopulacao, 1 );
    v_pop->pop = g_new0( GasPopulacao*, 4 );

    for ( int k = 0; k < 4; k++ ) {
        v_pop->pop[k] = gas_alocar_populacao( n_pop, lim[k].n_dim );
    }

    v_pop->elite = gas_alocar_populacao( 4, lim[0].n_dim );

    return v_pop;
}

static VgasGenitores *v_gas_alocar_genitores( const int n_gen, const GasLimites *lim ) {
    g_return_val_if_fail( lim && n_gen > 0, NULL );

    VgasGenitores *v_gen = g_new0( VgasGenitores, 1 );
    v_gen->gen = g_new0( GasGenitores*, 4 );

    for ( int k = 0; k < 4; k++ ) {
        v_gen->gen[k] = gas_alocar_genitores( n_gen, lim[k].n_dim );
    }
    return v_gen;
}

static void v_gas_liberar_populacao( VgasPopulacao *v_pop, const GasLimites *lim ) {
    g_return_if_fail( v_pop && lim );

    for ( int k = 0; k < 4; k++ ) {
        if ( v_pop->pop[k] != NULL ) {
            gas_liberar_populacao( v_pop->pop[k], lim[k].n_dim );
        }
    }
    g_free( v_pop->pop );
	 g_free( v_pop->elite );
    g_free( v_pop );
}

static void v_gas_liberar_genitores( VgasGenitores *v_gen, const GasLimites *lim ) {
    g_return_if_fail( v_gen && lim );

    for ( int k = 0; k < 4; k++ ) {
        if ( v_gen->gen[k] != NULL ) {
            gas_liberar_genitores( v_gen->gen[k], lim[k].n_dim );
        }
    }
    g_free( v_gen->gen );
    g_free( v_gen );
}

static void v_gas_populacao_inicial( VgasPopulacao *v_pop, const GasParametros *par, const GasLimites *lim ) {
    g_return_if_fail( v_pop && par && lim );

    for ( int k = 0; k < 4; k++ ) {
        gas_populacao_inicial( v_pop->pop[k], par, &lim[k] );
    }
}

// ============================================================================
// PIPELINE PRINCIPAL DE COEVOLUÇÃO
// ============================================================================

GasPopulacao *v_gas_pipeline( GasParametros *par,
                              VgasLimites *v_lim,
                              double(v_gas_avaliar)(const double*, GasPopulacao*, const int, ImagemCinza *img),
                              int(gas_comparar)(const void* a, const void* b) )
{
    GasPopulacao *melhor = NULL;
    g_return_val_if_fail( par && v_lim && v_gas_avaliar && gas_comparar, melhor );

    GasLimites *lim = v_lim->lim; // Ponteiro de trabalho

    // Alocação da matriz de dispersão
    double **coef_disp = g_new0( double*, 4 );
    for ( int k = 0; k < 4; k++ ) {
        coef_disp[k] = g_new0( double, lim[k].n_dim );
    }

    VgasPopulacao *v_pop = v_gas_alocar_populacao( par->n_pop, lim );
    VgasGenitores *v_gen = v_gas_alocar_genitores( par->n_gen, lim );

    // Ponteiros de trabalho
    GasPopulacao **pop = v_pop->pop;
    GasGenitores **gen = v_gen->gen;

    int geracao = 0;
    g_autofree double *dispersao_max = g_new0( double, 4 );

    v_gas_populacao_inicial( v_pop, par, lim );

    // Alocação e leitura segura da imagem
    ImagemCinza *img = g_new0( ImagemCinza, 1 );
    imread_gray( img, "img_teste.png" );

    // Geração 0 (Avaliação Exploratória)
    for ( int k = 0; k < 4; k++ ) {
        for ( int i = 0; i < par->n_pop; i++ ) {
            pop[k][i].fitness = v_gas_avaliar( pop[k][i].x, NULL, lim[k].n_dim, img );
        }
        qsort( pop[k], par->n_pop, sizeof(GasPopulacao), gas_comparar );

        memcpy( v_pop->elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof(double) );
		  v_pop->elite[k].fitness = pop[k][par->n_pop - 1].fitness;

        gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
    }

    // Laço Evolutivo
    do {
        geracao++;

        for ( int k = 0; k < 4; k++ ) {
            gas_torneio( pop[k], gen[k], lim[k].n_dim, par, gas_comparar );
            gas_crossover_aritmetico( pop[k], gen[k], lim[k].n_dim, par );
            gas_mutacao_creep( pop[k], coef_disp[k], &lim[k], par );

            for ( int i = 0; i < par->n_pop; i++ ) {
                pop[k][i].fitness = v_gas_avaliar( pop[k][i].x, v_pop->elite, lim[k].n_dim, img );
            }
            qsort( pop[k], par->n_pop, sizeof(GasPopulacao), gas_comparar );

            memcpy( v_pop->elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof(double) );
				v_pop->elite[k].fitness = pop[k][par->n_pop - 1].fitness;

            gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
            dispersao_max[k] = gas_max( coef_disp[k], lim[k].n_dim );
        }

    } while ( gas_max(dispersao_max, 4) > par->toleracia && geracao < 1000 );

    // ------------------------------------------------------------------------
    // EXTRAÇÃO DOS VENCEDORES (DEEP COPY)
    // ------------------------------------------------------------------------
    melhor = g_new0( GasPopulacao, 4 );

    for ( int k = 0; k < 4; k++ ) {
        melhor[k].x = g_new0( double, lim[k].n_dim );
        memcpy( melhor[k].x, v_pop->elite[k].x, lim[k].n_dim * sizeof(double) );
        melhor[k].fitness = v_pop->elite[k].fitness;
    }

    // ------------------------------------------------------------------------
    // LIMPEZA DE RECURSOS DO PIPELINE
    // ------------------------------------------------------------------------
    for ( int k = 0; k < 4; k++ ) {
        g_free( coef_disp[k] );
    }
    g_free( coef_disp );

    v_gas_liberar_populacao( v_pop, lim );
    v_gas_liberar_genitores( v_gen, lim );

    liberar_matriz_pixels( img->image, img->nrow );
    g_free( img );

    // Retorna as 4 âncoras limpas e seguras
    return melhor;
}

// ============================================================================
// MANIPULAÇÃO DE IMAGENS
// ============================================================================

void liberar_matriz_pixels( int **matriz, int nrow ) {
    if ( !matriz ) return;

    for ( int i = 0; i < nrow; i++ ) {
        g_free( matriz[i] );
    }
    g_free( matriz );
}

void imread_gray( ImagemCinza *IMG, const char *arquivo ) {
    if ( !IMG || !arquivo ) return;

    FILE *p = fopen( arquivo, "rb" );
    if ( !p ) {
        fprintf( stderr, "Erro: Não foi possível abrir o arquivo %s.\n", arquivo );
        return;
    }

    if ( fscanf( p, "%9s\n%d %d\n%d\n", IMG->key, &IMG->ncol, &IMG->nrow, &IMG->max ) != 4 ) {
        fprintf( stderr, "Erro: Falha ao ler o cabeçalho PPM do arquivo.\n" );
        fclose( p );
        return;
    }

    snprintf( IMG->key, sizeof(IMG->key), "P5" ); // Mais seguro que sprintf

    // 1. Pré-alocação segura da matriz 2D com GLib
    IMG->image = g_new0( int*, IMG->nrow );
    for ( int i = 0; i < IMG->nrow; i++ ) {
        IMG->image[i] = g_new0( int, IMG->ncol );
    }

    // 2. I/O em Bloco (Leitura Massiva)
    size_t total_pixels = ( size_t )IMG->ncol * IMG->nrow;
    unsigned char *buffer_gigante = g_new( unsigned char, total_pixels * 3 );

    if ( fread( buffer_gigante, 3, total_pixels, p ) != total_pixels ) {
        fprintf( stderr, "Aviso: Fim de arquivo inesperado. A imagem pode estar cortada.\n" );
    }
    fclose( p );

    // 3. Processamento CPU-Bound Paralelizado via OpenMP
    // #pragma omp parallel for schedule(static)
    for ( int i = 0; i < IMG->nrow; i++ ) {
        size_t offset_linha = ( size_t )i * IMG->ncol * 3;

        for ( int j = 0; j < IMG->ncol; j++ ) {
            size_t idx = offset_linha + ( j * 3 );

            unsigned char r = buffer_gigante[idx];
            unsigned char g = buffer_gigante[idx + 1];
            unsigned char b = buffer_gigante[idx + 2];

            int luminancia = ( 2126 * r + 7152 * g + 722 * b ) / 10000;
            IMG->image[i][j] = luminancia;
        }
    }

    g_free( buffer_gigante ); // Limpeza via GLib
}
