/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include <stdint.h>

#include "gas.h"
#include "v_gas.h"


typedef struct {
   uint8_t r, g, b;
} PixelRGB;

typedef struct {
   char key[5];         // Identificador do formato (ex: "P3", "P6")
   int ncol, nrow;      // Dimensões da imagem
   int max;             // Valor máximo para os canais de cor
   PixelRGB **image;    // Matriz bidimensional de estruturas de pixel RGB
} ImagemColorida;


// ============================================================================
// PROTÓTIPOS INTERNOS
// ============================================================================
static void display_tempo( const char *descricao, GTimer *cronometro );

static int** alocar_matriz_pixels( int nrow, int ncol );
static void liberar_matriz_pixels( int **matriz, int nrow );
static PixelRGB** alocar_matriz_pixels_colorida( int nrow, int ncol );
static void liberar_matriz_pixels_colorida( PixelRGB **matriz, int nrow );

static gboolean carregar_imagem_colorida_nativa(const char *caminho, ImagemColorida *img);
static void rgb2gray( ImagemColorida *PPM, ImagemCinza *PGM );
static void redimensionar_imagem_bilinear( ImagemCinza *origem, ImagemCinza *destino, int dim );


// ============================================================================
// FUNÇÃO PRINCIPAL
// ============================================================================
int main( void ) {
   g_autoptr( GTimer ) cronometro = g_timer_new();

   guint32 sementes[GAS_NUM_SEMENTES];
   gas_gerar_sementes( sementes );
   g_autoptr( GRand ) rand_context = g_rand_new_with_seed_array( sementes, G_N_ELEMENTS( sementes ) );

   // Quando o feedback_visual é TRUE, temos uma execução normal de gas_pipeline() com feedback visual no gnuplot
   // Quando o feedback_visual é FALSE, executamos um teste em paralelo com 10 mil execuções de gas_pipeline()
   gboolean feedback_visual = TRUE;

   GasParametros par = {
      .n_pop          = 60,    // Tamanho da população
      .n_gen          = 24,    // Quantidade de indivíduos substituídos a cada geração
      .n_tor          = 2,     // Número de indivíduos envolvidos no torneio
      .n_obj          = 4,     // Número de objetivos da coevolução (Mantido)
      .p_rec          = 0.80,  // Leve aumento para garantir mistura genética na pop menor
      .p_mut          = 0.90,  // Mantido altíssimo (Essencial para busca contínua)
      .peso_disp      = 1.8,   // Peso de dispersão (Força repulsiva)
      .toleracia      = 3.0e-1,// Tolerância baseada no coeficiente de dispersão médio global
      .max_geracoes   = 65,    // Limite máximo de gerações
      .total_geracoes = 0,
      .limiar         = 1,
      .rand           = rand_context
   };

   // Alocação e leitura segura da imagem
   g_autofree ImagemColorida *img_rgb_orig = g_new0( ImagemColorida, 1 );
   g_autofree ImagemCinza *img_gray = g_new0( ImagemCinza, 1 );
   g_autofree ImagemCinza *img_gray_alloc = g_new0( ImagemCinza, 1 );

   // Seleção da imagem de teste
   carregar_imagem_colorida_nativa( "./img/img_distorcida.png", img_rgb_orig );

   // carregar_imagem_colorida_nativa( "./img/imgh.png", img_rgb_orig );
   // carregar_imagem_colorida_nativa( "./img/imgv.png", img_rgb_orig );
   // carregar_imagem_colorida_nativa( "./img/imgv_ruido_severo.png", img_rgb_orig );

   rgb2gray( img_rgb_orig, img_gray );
   redimensionar_imagem_bilinear( img_gray, img_gray_alloc, 960 );

   GasLimites *lim = v_gas_limites( img_gray_alloc->nrow, img_gray_alloc->ncol, par.n_obj );

   int sucessos = 0;
   int32_t total_geracoes = 0;
   GasPopulacao *melhor = NULL;

   // Constante para os testes paralelos
   const int NUM_TESTES = 10000;

   if ( feedback_visual ) {
      melhor = v_gas_pipeline( img_gray_alloc, &par, lim, feedback_visual );

   } else {

      #pragma omp parallel for schedule(static) reduction(+:sucessos, total_geracoes)
      for ( int i = 0; i < NUM_TESTES; i++ ) {

         // 1. Isolamento de Threads: Cópia local dos parâmetros
         GasParametros par_local = par;
         par_local.total_geracoes = 0; // Zera o contador para esta execução específica
         par_local.rand = g_rand_new(); // Motor estocástico isolado

         GasPopulacao *melhor_local = v_gas_pipeline( img_gray_alloc, &par_local, lim, feedback_visual );

         // Se não atingir o fitness mínimo, faz o resgate ajustando a dispersão
         // double media_fitness = ( melhor_local[0].fitness + melhor_local[1].fitness +
         //                          melhor_local[2].fitness + melhor_local[3].fitness ) / 4.0;
         //
         // if ( media_fitness < 0.98 ) {
         //    // CORREÇÃO: Libera a memória da primeira tentativa antes de sobrescrever o ponteiro!
         //    gas_liberar_populacao( melhor_local, par_local.n_obj );
         //
         //    par_local.peso_disp = 2.8; // Força a exploração para quebrar o mínimo local
         //    par_local.max_geracoes = 100;
         //    melhor_local = v_gas_pipeline( img_gray_alloc, &par_local, lim, feedback_visual );
         // }

         // .img/img_distorcida.png
         static const double gabarito_real[4][2] = {
            { 143.0, 125.0 }, // Âncora A (k = 0)
            { 847.0, 116.0 }, // Âncora B (k = 1)
            { 859.0, 689.0 }, // Âncora C (k = 2)
            { 138.0, 679.0 }  // Âncora D (k = 3)
         };

         // ./img/imgh.png
         // static const double gabarito_real[4][2] = {
         //    { 216.0, 147.0 }, // Âncora A (k = 0)
         //    { 857.0, 147.0 }, // Âncora B (k = 1)
         //    { 857.0, 650.0 }, // Âncora C (k = 2)
         //    { 216.0, 650.0 }  // Âncora D (k = 3)
         // };

         // ./img/imgv.png
         // static const double gabarito_real[4][2] = {
         //    { 203.0, 194.0 }, // Âncora A (k = 0)
         //    { 655.0, 194.0 }, // Âncora B (k = 1)
         //    { 655.0, 872.0 }, // Âncora C (k = 2)
         //    { 203.0, 872.0 }  // Âncora D (k = 3)
         // };

         // ./img/imgv_ruido_severo.png
         // static const double gabarito_real[4][2] = {
         //    { 420.0,  81.0 }, // Âncora A (k = 0)
         //    { 908.0,  81.0 }, // Âncora B (k = 1)
         //    { 908.0, 813.0 }, // Âncora C (k = 2)
         //    { 420.0, 813.0 }  // Âncora D (k = 3)
         // };

         // Critério de tolerância máxima em pixels para cada âncora
         const double tolerancia_pixels = 2.5;
         gboolean convergiu = TRUE;

         for ( int k = 0; k < par_local.n_obj; k++ ) {
            double dx = melhor_local[k].x[0] - gabarito_real[k][0];
            double dy = melhor_local[k].x[1] - gabarito_real[k][1];

            // Distância Euclidiana real ao centro do alvo
            double erro_distancia = sqrt( ( dx * dx ) + ( dy * dy ) );

            if ( erro_distancia > tolerancia_pixels ) {
               convergiu = FALSE;
               break; // Não precisa checar os outros objetivos
            }
         }

         if ( convergiu ) {
            sucessos++;
         }

         // CORREÇÃO: Acumula o esforço computacional desta thread no total global
         total_geracoes += par_local.total_geracoes;

         // CORREÇÃO: Limpa a memória das âncoras e do gerador de números aleatórios desta thread
         gas_liberar_populacao( melhor_local, par_local.n_obj );
         g_rand_free( par_local.rand );
      }

      // Paramos o relógio aqui para calcular as métricas exatas do benchmark
      double tempo_decorrido = g_timer_elapsed( cronometro, NULL );

      double taxa_sucesso = ( ( double )sucessos / NUM_TESTES ) * 100.0;
      double media_geracoes = ( double )total_geracoes / NUM_TESTES;
      double media_avaliacoes = media_geracoes * par.n_gen * par.n_obj;
      double tempo_por_imagem_ms = ( tempo_decorrido / NUM_TESTES ) * 1000.0;

      printf( "=======================================================\n" );
      printf( "📊 BENCHMARK DO MOTOR ESTOCÁSTICO (VÉRTICE GAS)\n" );
      printf( "=======================================================\n" );
      printf( "🎯 Taxa de Convergência: %.2f%%\n", taxa_sucesso );
      printf( "🔄 Média de Gerações:    %.2f gerações/execução\n", media_geracoes );
      printf( "⚡ Média de Avaliações:   %d avaliações/imagem\n", ( int )round( media_avaliacoes ) );
      printf( "⏱ Tempo Total (10k):     %.3f segundos\n", tempo_decorrido );
      printf( "🏎  Tempo Médio por Img:  %.3f ms\n", tempo_por_imagem_ms );
   }

   // ------------------------------------------------------------------------
   // LIMPEZA DE MEMÓRIA (DEEP FREE) DA THREAD PRINCIPAL
   // ------------------------------------------------------------------------
   if ( img_gray->image )       liberar_matriz_pixels( img_gray->image, img_gray->nrow );
   if ( img_gray_alloc->image ) liberar_matriz_pixels( img_gray_alloc->image, img_gray_alloc->nrow );
   if ( img_rgb_orig->image )   liberar_matriz_pixels_colorida( img_rgb_orig->image, img_rgb_orig->nrow );

   for ( int k = 0; k < par.n_obj; k++ ) {
      g_free( lim[k].ini );
      g_free( lim[k].fim );
   }
   g_free( lim );

   if ( melhor ) gas_liberar_populacao( melhor, par.n_obj );

   // Exibe o tempo apenas se rodou com feedback visual (se foi benchmark, a tabela já mostrou)
   if ( feedback_visual ) {
      display_tempo( "Evolução Visual", cronometro );
   }
   printf( "=======================================================\n" );

   return 0;
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
static int** alocar_matriz_pixels( int nrow, int ncol ) {
   int **matriz = g_new( int*, nrow );
   for ( int i = 0; i < nrow; i++ ) {
      matriz[i] = g_new0( int, ncol ); // g_new0 já zera a memória
   }
   return matriz;
}

static void liberar_matriz_pixels( int **matriz, int nrow ) {
   if ( !matriz ) return;

   for ( int i = 0; i < nrow; i++ ) {
      g_free( matriz[i] );
   }
   g_free( matriz );
}


static PixelRGB** alocar_matriz_pixels_colorida( int nrow, int ncol ) {
   PixelRGB **matriz = g_new( PixelRGB*, nrow );
   for ( int i = 0; i < nrow; i++ ) {
      matriz[i] = g_new0( PixelRGB, ncol ); // g_new0 já zera a memória
   }
   return matriz;
}

static void liberar_matriz_pixels_colorida( PixelRGB **matriz, int nrow ) {
   for ( int i = 0; i < nrow; i++ ) {
      g_free( matriz[i] );
   }
   g_free( matriz );
   matriz = NULL;
}


// Função de reconstrução que lida com 3 canais (RGB) ou 4 canais (RGBA)
static void reconstruir_matriz_colorida(const guchar *pixels_1d, ImagemColorida *img,
                                       int largura, int altura,
                                       int rowstride, int canais) {
    g_return_if_fail(pixels_1d != NULL);
    g_return_if_fail(img != NULL);

    if (img->image != NULL) {
        liberar_matriz_pixels_colorida(img->image, img->nrow);
    }

    img->ncol = largura;
    img->nrow = altura;
    img->max = 255;
    g_strlcpy(img->key, "P6", sizeof(img->key));

    img->image = alocar_matriz_pixels_colorida(altura, largura);
    if (!img->image) return;

    // Se for RGB puro (3 canais) e sem padding, podemos usar o memcpy ultraveloz!
    if (canais == 3 && rowstride == (largura * 3)) {
        size_t tamanho_linha_util = largura * sizeof(PixelRGB);

        // #pragma omp parallel for
        for (int y = 0; y < altura; y++) {
            const guchar *origem_linha = pixels_1d + (y * rowstride);
            memcpy(img->image[y], origem_linha, tamanho_linha_util);
        }
    } else {
        // Caso a imagem tenha Alpha (4 canais) ou padding no rowstride,
        // copiamos pixel a pixel pulando o canal A (transparência)
        // #pragma omp parallel for
        for (int y = 0; y < altura; y++) {
            const guchar *origem_linha = pixels_1d + (y * rowstride);
            for (int x = 0; x < largura; x++) {
                const guchar *p = origem_linha + (x * canais);
                img->image[y][x].r = p[0];
                img->image[y][x].g = p[1];
                img->image[y][x].b = p[2];
            }
        }
    }
}

static gboolean carregar_imagem_colorida_nativa(const char *caminho, ImagemColorida *img) {
    g_return_val_if_fail(caminho != NULL, FALSE);
    g_return_val_if_fail(img != NULL, FALSE);

    GError *erro = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(caminho, &erro);

    if (!pixbuf) {
        g_printerr("[ERRO] Ao carregar imagem (%s): %s\n", caminho, erro->message);
        g_clear_error(&erro);
        return FALSE;
    }

    int largura = gdk_pixbuf_get_width(pixbuf);
    int altura = gdk_pixbuf_get_height(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int canais = gdk_pixbuf_get_n_channels(pixbuf); // 3 para RGB, 4 para RGBA
    const guchar *pixels_1d = gdk_pixbuf_read_pixels(pixbuf);

    // Passamos o número de canais para a função auxiliar
    reconstruir_matriz_colorida(pixels_1d, img, largura, altura, rowstride, canais);

    g_object_unref(pixbuf);
    return TRUE;
}


static void rgb2gray( ImagemColorida *PPM, ImagemCinza *PGM ) {
   if ( !PPM || !PGM ) return;

   g_strlcpy( PGM->key, "P5", sizeof( PGM->key ) );
   PGM->ncol = PPM->ncol;
   PGM->nrow = PPM->nrow;
   PGM->max  = PPM->max;

   // 1. Pré-alocação segura da matriz 2D
   PGM->image = alocar_matriz_pixels( PGM->nrow, PGM->ncol );

   // #pragma omp parallel for schedule(static)
   for ( int i = 0; i < PGM->nrow; i++ ) {
      for ( int j = 0; j < PGM->ncol; j++ ) {

         unsigned char r = PPM->image[i][j].r;
         unsigned char g = PPM->image[i][j].g;
         unsigned char b = PPM->image[i][j].b;

         // Luminância Rec.709
         PGM->image[i][j] = ( 2126*r + 7152*g + 722*b ) / 10000;
      }
   }

}

static void redimensionar_imagem_bilinear( ImagemCinza *origem, ImagemCinza *destino, int dim ) {
   if ( !origem || !destino || dim <= 0 ) return;

   gboolean deitada = ( origem->ncol > origem->nrow );
   destino->ncol = deitada ? dim : ( dim * origem->ncol ) / origem->nrow;
   destino->nrow = deitada ? ( dim * origem->nrow ) / origem->ncol : dim;

   g_strlcpy( destino->key, origem->key, sizeof( destino->key ) );
   destino->max = origem->max;

   if ( destino->image != NULL ) {
      liberar_matriz_pixels( destino->image, destino->nrow );
   }

   destino->image = alocar_matriz_pixels( destino->nrow, destino->ncol );
   if ( !destino->image ) return;

   // Fatores de proporção (mapeamento reverso alinhando os cantos)
   float x_ratio = ( ( float )( origem->ncol - 1 ) ) / ( destino->ncol > 1 ? destino->ncol - 1 : 1 );
   float y_ratio = ( ( float )( origem->nrow - 1 ) ) / ( destino->nrow > 1 ? destino->nrow - 1 : 1 );

   // Paralelização OpenMP ativada.
   // schedule(static) é perfeito aqui porque o custo computacional de cada linha é exatamente igual.
   // #pragma omp parallel for schedule(static)
   for ( int i = 0; i < destino->nrow; i++ ) {
      // OTIMIZAÇÃO 1: Variáveis declaradas aqui dentro são PRIVADAS para cada thread
      float src_y = y_ratio * i;
      int y = ( int )src_y;
      float y_diff = src_y - y;
      float y_inv = 1.0f - y_diff;

      int y_next = ( y + 1 < origem->nrow ) ? y + 1 : y;

      for ( int j = 0; j < destino->ncol; j++ ) {
         float src_x = x_ratio * j;
         int x = ( int )src_x;
         float x_diff = src_x - x;
         float x_inv = 1.0f - x_diff;

         int x_next = ( x + 1 < origem->ncol ) ? x + 1 : x;

         // OTIMIZAÇÃO 2: Captura limpa e independente
         int a = origem->image[y][x];
         int b = origem->image[y][x_next];
         int c = origem->image[y_next][x];
         int d = origem->image[y_next][x_next];

         float pixel_interpolado = a * x_inv * y_inv +
                                   b * x_diff * y_inv +
                                   c * x_inv * y_diff +
                                   d * x_diff * y_diff;

         // OTIMIZAÇÃO 3: Escrita sem colisão, cada thread escreve na sua própria linha 'i'
         destino->image[i][j] = ( int )( pixel_interpolado + 0.5f );
      }
   }
}


