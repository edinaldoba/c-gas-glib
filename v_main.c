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
static void display_tempo( const char *descricao, GTimer *cronometro );
static GasLimites *v_gas_limites( const int nrow, const int ncol, const int n_obj );
static void liberar_matriz_pixels( int **matriz, int nrow );
static void imread_gray( ImagemCinza *IMG, const char *arquivo );

// ============================================================================
// FUNÇÃO PRINCIPAL
// ============================================================================
int main( void ) {
   g_autoptr( GTimer ) cronometro = g_timer_new();

   guint32 sementes[GAS_NUM_SEMENTES];
   gas_gerar_sementes( sementes );
   g_autoptr( GRand ) rand_context = g_rand_new_with_seed_array( sementes, G_N_ELEMENTS( sementes ) );

   GasParametros par = {
      .n_pop     = 200,
      .n_gen     = 164,
      .n_tor     = 2,
      .n_obj     = 4,    // Número de objetivos da coevolução
      .p_rec     = 0.75,
      .p_mut     = 0.95,
      .peso_disp = 1.5,
      .toleracia = 5.0e-1,
      .rand      = rand_context
   };


   // Alocação e leitura segura da imagem
   ImagemCinza *img = g_new0( ImagemCinza, 1 );
   imread_gray( img, "./img/img.ppm" );

   GasLimites *lim = v_gas_limites( img->nrow, img->ncol, par.n_obj );

   // int sucessos = 0;

   // #pragma omp parallel for schedule(static)
   // for ( int i = 0; i < 10000; i++ ) {
      GasPopulacao *melhor = v_gas_pipeline( img, &par, lim, v_gas_fitness_coevolutivo, gas_comparar_objetivo_max );
      // if( melhor->fitness > 0.999 ) sucessos++;
      gas_liberar_populacao( melhor, lim[0].n_dim );
   // }
   // printf( "Convergência de %.2f%%\n", (float)sucessos / 100.0 );

   liberar_matriz_pixels( img->image, img->nrow );
   g_free( img );

   // ------------------------------------------------------------------------
   // LIMPEZA DE MEMÓRIA (DEEP FREE)
   // ------------------------------------------------------------------------
   for ( int k = 0; k < par.n_obj; k++ ) {
      g_free( lim[k].ini );
      g_free( lim[k].fim );
   }
   g_free( lim );

   display_tempo( "Evolução", cronometro );

   return 0;
}

// ============================================================================
// IMPLEMENTAÇÕES DAS FUNÇÕES
// ============================================================================
static GasLimites *v_gas_limites( const int nrow, const int ncol, const int n_obj ) {
   // Validação estrita padrão GLib
   // Como esta função desenha limites para 4 quadrantes, travamos n_obj em 4.
   g_return_val_if_fail( nrow > 0 && ncol > 0 && n_obj == 4, NULL );

   int n_dim = 2; // 0: eixo X (colunas), 1: eixo Y (linhas)

   // Calcula os pontos médios e limites máximos da imagem
   double mid_x = ( ncol - 1.0 ) / 2.0;
   double mid_y = ( nrow - 1.0 ) / 2.0;
   double max_x = ncol - 1.0;
   double max_y = nrow - 1.0;

   // Constantes espaciais (Devem ter tamanho constante [4] para inicializar com chaves)
   // k=0 (Topo-Esquerda)   | k=1 (Topo-Direita)
   // ----------------------+----------------------
   // k=3 (Base-Esquerda)   | k=2 (Base-Direita)
   double ini_x[4] = { 0.0,   mid_x, mid_x, 0.0   };
   double fim_x[4] = { mid_x, max_x, max_x, mid_x };

   double ini_y[4] = { 0.0,   0.0,   mid_y, mid_y };
   double fim_y[4] = { mid_y, mid_y, max_y, max_y };

   // Aloca o array principal de limites usando a variável n_obj
   GasLimites *lim = g_new0( GasLimites, n_obj );

   for ( int k = 0; k < n_obj; k++ ) {
      lim[k].n_dim = n_dim;
      lim[k].ini   = g_new0( double, n_dim );
      lim[k].fim   = g_new0( double, n_dim );

      // Preenche os limites da Dimensão 0 (Eixo X)
      lim[k].ini[0] = ini_x[k];
      lim[k].fim[0] = fim_x[k];

      // Preenche os limites da Dimensão 1 (Eixo Y)
      lim[k].ini[1] = ini_y[k];
      lim[k].fim[1] = fim_y[k];
   }

   return lim;
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




// ============================================================================
// MANIPULAÇÃO DE IMAGENS
// ============================================================================
static void liberar_matriz_pixels( int **matriz, int nrow ) {
   if ( !matriz ) return;

   for ( int i = 0; i < nrow; i++ ) {
      g_free( matriz[i] );
   }
   g_free( matriz );
}

static void imread_gray( ImagemCinza *IMG, const char *arquivo ) {
   if ( !IMG || !arquivo ) return;

   FILE *p = fopen( arquivo, "rb" );
   if ( !p ) {
      fprintf( stderr, "Erro: Não foi possível abrir o arquivo %s.\n", arquivo );
      return;
   }

   if ( fscanf( p, "%9s\n%d %d\n%d\n", IMG->key, &IMG->ncol, &IMG->nrow, &IMG->max ) != 4 ) {
      fprintf( stderr, "Erro: Falha ao ler o cabeçalho PPM do arquivo.\n" );
      // fclose( p );
      // return;
   }

   snprintf( IMG->key, sizeof( IMG->key ), "P5" ); // Mais seguro que sprintf

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
