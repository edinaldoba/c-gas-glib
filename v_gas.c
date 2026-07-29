/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <math.h>
#include <stdio.h>
#include <string.h> // Necessário para memcpy
#include <glib.h>

#include "v_gas.h"



// ============================================================================
// PROTÓTIPOS DAS FUNÇÕES INTERNAS DE FITNESS (A SEREM IMPLEMENTADAS POR VOCÊ)
// ============================================================================

static double v_fitness_local( const double *x, const ImagemCinza *img, const int k ) {
   g_return_val_if_fail( x && img && img->image, 0.0 );

   int soma = 0; // Removido o '.0' pois a variável é inteira
   int x_idx = ( int )round( x[0] );
   int y_idx = ( int )round( x[1] );

   // Define a direção da varredura apontando para o centro da imagem
   int sinal_x, sinal_y;
   switch ( k ) {
   case 0:
      sinal_x =  1;
      sinal_y =  1;
      break; // Q0: Direita e Baixo
   case 1:
      sinal_x = -1;
      sinal_y =  1;
      break; // Q1: Esquerda e Baixo
   case 2:
      sinal_x = -1;
      sinal_y = -1;
      break; // Q2: Esquerda e Cima
   case 3:
      sinal_x =  1;
      sinal_y = -1;
      break; // Q3: Direita e Cima
   default:
      sinal_x = 1;
      sinal_y =  1;
      break;
   }

   int lado = 10;

   // Varredura da malha 10x10
   for ( int i = 0; i < lado; i++ ) {
      for ( int j = 0; j < lado; j++ ) {

         int atual_x = x_idx + ( j * sinal_x );
         int atual_y = y_idx + ( i * sinal_y );

         // Barreira de proteção contra Segmentation Fault
         if ( atual_x >= 0 && atual_x < img->ncol && atual_y >= 0 && atual_y < img->nrow ) {

            // Matrizes em C: [linha][coluna] -> [y][x]
            if ( img->image[atual_y][atual_x] < 10 ) {
               soma++;
            }
         }
      }
   }

   // Retorna a densidade de pixels escuros (de 0.0 a 1.0)
   return ( double )soma / ( lado * lado );
}


// ============================================================================
// CÁLCULO DA ÁREA DE QUALQUER POLÍGONO (SHOELACE FORMULA / FÓRMULA DE GAUSS)
// ============================================================================
static double v_gas_calcular_area_ancoras( const GasPopulacao *elite, const int n_ancoras ) {
   g_return_val_if_fail( elite && n_ancoras >= 3, 0.0 );

   double soma = 0.0;

   for ( int i = 0; i < n_ancoras; i++ ) {
      // O operador modulo (%) garante que o próximo vértice após o último seja o primeiro (0)
      int proximo = ( i + 1 ) % n_ancoras;

      // Coordenadas do vértice atual (i) e do próximo (proximo)
      double x_atual   = elite[i].x[0];
      double y_atual   = elite[i].x[1];

      double x_proximo = elite[proximo].x[0];
      double y_proximo = elite[proximo].x[1];

      // Produto cruzado em 2D (Determinante da matriz 2x2)
      soma += ( x_atual * y_proximo ) - ( x_proximo * y_atual );
   }

   // A área é a metade do módulo do determinante acumulado
   return fabs( soma ) / 2.0;
}



// Função auxiliar inline para distância euclidiana (muito rápida)
static inline double v_gas_distancia( const double p1[2], const double p2[2] ) {
   return hypot( p1[0] - p2[0], p1[1] - p2[1] ); // Nativa do C
}


// ============================================================================
// FUNÇÃO AUXILIAR: ERRO ORTOGONAL VIA PRODUTO ESCALAR NORMALIZADO
// ============================================================================
static double v_gas_erro_ortogonal( const double p0[2], const double p1[2],
                                    const double p2[2], const double p3[2],
                                    const double top_w, const double bot_w,
                                    const double left_h, const double right_h ) {

   // Vetores partindo de cada vértice
   // Canto 0 (Top-Esq): Vetor para P1 e Vetor para P3
   double dp0 = ( p1[0] - p0[0] ) * ( p3[0] - p0[0] ) + ( p1[1] - p0[1] ) * ( p3[1] - p0[1] );

   // Canto 1 (Top-Dir): Vetor para P0 e Vetor para P2
   double dp1 = ( p0[0] - p1[0] ) * ( p2[0] - p1[0] ) + ( p0[1] - p1[1] ) * ( p2[1] - p1[1] );

   // Canto 2 (Bot-Dir): Vetor para P1 e Vetor para P3
   double dp2 = ( p1[0] - p2[0] ) * ( p3[0] - p2[0] ) + ( p1[1] - p2[1] ) * ( p3[1] - p2[1] );

   // Canto 3 (Bot-Esq): Vetor para P2 e Vetor para P0
   double dp3 = ( p2[0] - p3[0] ) * ( p0[0] - p3[0] ) + ( p2[1] - p3[1] ) * ( p0[1] - p3[1] );

   // O erro é a média dos cossenos absolutos de cada quina.
   // Como a área na função principal é garantida > 100, não há risco de divisão por zero aqui.
   double cos0 = fabs( dp0 ) / ( top_w * left_h );
   double cos1 = fabs( dp1 ) / ( top_w * right_h );
   double cos2 = fabs( dp2 ) / ( bot_w * right_h );
   double cos3 = fabs( dp3 ) / ( bot_w * left_h );

   return ( cos0 + cos1 + cos2 + cos3 ) / 4.0;
}

// ============================================================================
// FUNÇÃO DE FITNESS GEOMÉTRICO (ATUALIZADA)
// ============================================================================
static double v_fitness_geometrico( const double *x, const GasPopulacao *elite, const int k ) {
   g_return_val_if_fail( x && elite && k >= 0 && k < 4, 0.0 );

   // 1. O PULO DO GATO: Simulação do polígono
   GasPopulacao simulacao[4];
   for ( int i = 0; i < 4; i++ ) {
      simulacao[i].x = ( i == k ) ? ( double * )x : elite[i].x;
   }

   // 2. Calcula a área e as dimensões teóricas ideais
   double area = v_gas_calcular_area_ancoras( simulacao, 4 );
   if ( area < 100.0 ) return 0.0; // Esta barreira garante que as arestas > 0

   double largura_ideal = sqrt( area * ( 14.0 / 11.0 ) );
   double altura_ideal  = sqrt( area * ( 11.0 / 14.0 ) );

   // 3. Extrai as distâncias reais
   double top_w = v_gas_distancia( simulacao[0].x, simulacao[1].x );
   double bot_w = v_gas_distancia( simulacao[3].x, simulacao[2].x );
   double left_h  = v_gas_distancia( simulacao[0].x, simulacao[3].x );
   double right_h = v_gas_distancia( simulacao[1].x, simulacao[2].x );

   double largura_real = ( top_w + bot_w ) / 2.0;
   double altura_real  = ( left_h + right_h ) / 2.0;

   // 4. Avaliação de Erros Geométricos
   double erro_w = fabs( largura_real - largura_ideal ) / largura_ideal;
   double erro_h = fabs( altura_real - altura_ideal ) / altura_ideal;

   // NOVO: Cálculo do erro de ortogonalidade aproveitando as arestas já calculadas
   double erro_ortogonal = v_gas_erro_ortogonal( simulacao[0].x, simulacao[1].x,
                           simulacao[2].x, simulacao[3].x,
                           top_w, bot_w, left_h, right_h );

   // 5. Fitness (Erro Relativo Normalizado)
   // Os três erros gravitam de 0.0 a 1.0 (ou mais em deformações severas).
   double f_geo = 1.0 / ( 1.0 + erro_w + erro_h + erro_ortogonal );

   return f_geo;
}

// ============================================================================
// FUNÇÃO GLOBAL DE AVALIAÇÃO (COEVOLUÇÃO)
// ============================================================================

double v_gas_fitness_coevolutivo( const double *x, const GasPopulacao *elite, const ImagemCinza *img,
                                  const double coef_disp, const int k ) {
   g_return_val_if_fail( x && img, 0.0 );

   // Pesos da combinação linear para gerações > 0 (podem ser ajustados depois)
   const double w1 = coef_disp / 250.0;
   const double w2 = 1.0 - w1;

   // 1. O fitness local sempre é calculado, independentemente da geração
   // Avalia o contraste/textura da imagem exatamente na coordenada 'x'
   double f_local = v_fitness_local( x, img, k );

   // 2. GERAÇÃO 0: Se a elite for NULL, não há como calcular a geometria
   if ( elite == NULL ) {
      return f_local;
   }

   // 3. GERAÇÕES > 0: A elite existe, ativando a pressão evolutiva geométrica
   // Avalia como a coordenada 'x' se comporta em relação às outras 3 âncoras
   double f_geo = v_fitness_geometrico( x, elite, k );

   // 4. FITNESS ATRIBUÍDO (Equilíbrio de Nash)
   double f_atribuido = ( w1 * f_local ) + ( w2 * f_geo );

   return f_atribuido;
}







// ============================================================================
// PIPELINE PRINCIPAL DE COEVOLUÇÃO
// ============================================================================
GasPopulacao *v_gas_pipeline( const ImagemCinza *img,
                              const GasParametros *par,
                              const GasLimites *lim,
                              gboolean feedback_visual,
                              double ( *v_gas_avaliar )( const double*, const GasPopulacao*, const ImagemCinza *img,
                                    const double coef_disp, const int ),
                              int ( *gas_comparar )( const void* a, const void* b ) ) {
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
   g_autofree double *dispersao_media = g_new0( double, par->n_obj );

   // Geração 0 (Avaliação Exploratória)
   for ( int k = 0; k < par->n_obj; k++ ) {
      gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
      dispersao_media[k] = gas_mean( coef_disp[k], lim[k].n_dim );

      for ( int i = 0; i < par->n_pop; i++ ) {
         pop[k][i].fitness = v_gas_avaliar( pop[k][i].x, NULL, img, dispersao_media[k], k );
      }
      qsort( pop[k], par->n_pop, sizeof( GasPopulacao ), gas_comparar );

      memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof( double ) );
      elite[k].fitness = pop[k][par->n_pop - 1].fitness;
   }

   //--------------- FEEDBACK VISUAL ------------------------//
   FILE *p_dispersao = NULL;
   FILE *p_fitness = NULL;
   g_autofree double *fitness_elite = NULL;
   if ( feedback_visual ) {
      p_dispersao = fopen( "gnuplot/D.pts", "w" );
      p_fitness   = fopen( "gnuplot/E.pts", "w" );
      fitness_elite = g_new0( double, par->n_obj );
      for ( int k = 0; k < par->n_obj; k++ ) {
         fitness_elite[k] = elite[k].fitness;
         gas_gravar_pontos( pop[k], par->n_pop, geracao );
      }
      fprintf( p_fitness,   "%d %.8f\n", geracao, gas_mean( fitness_elite,   par->n_obj ) );
      fprintf( p_dispersao, "%d %.8f\n", geracao, gas_mean( dispersao_media, par->n_obj ) );
   }
   //-------------------------------------------------------//

   // Laço Evolutivo
   do {
      geracao++;

      for ( int k = 0; k < par->n_obj; k++ ) {
         gas_torneio( pop[k], gen[k],        lim[k].n_dim, par, gas_comparar );
         gas_crossover_aritmetico( pop[k], gen[k],        lim[k].n_dim, par );
         gas_mutacao_creep( pop[k], coef_disp[k], &lim[k],       par );

         gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
         dispersao_media[k] = gas_mean( coef_disp[k], lim[k].n_dim );

         for ( int i = 0; i < par->n_pop; i++ ) {
            pop[k][i].fitness = v_gas_avaliar( pop[k][i].x, elite, img, dispersao_media[k], k );
         }
         qsort( pop[k], par->n_pop, sizeof( GasPopulacao ), gas_comparar );

         memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof( double ) );
         elite[k].fitness = pop[k][par->n_pop - 1].fitness;
      }

      //--------------- FEEDBACK VISUAL ------------------------//
      if ( feedback_visual ) {
         for ( int k = 0; k < par->n_obj; k++ ) {
            fitness_elite[k] = elite[k].fitness;
            gas_gravar_pontos( pop[k], par->n_pop, geracao );
         }
         fprintf( p_fitness,   "%d %.8f\n", geracao, gas_mean( fitness_elite,   par->n_obj ) );
         fprintf( p_dispersao, "%d %.8f\n", geracao, gas_mean( dispersao_media, par->n_obj ) );
      }
      //--------------------------------------------------------//

   } while ( gas_mean( dispersao_media, par->n_obj ) > par->toleracia && geracao < 1000 );

   //--------------- FEEDBACK VISUAL ------------------------//
   if ( feedback_visual ) {
      for ( int k = 0; k < par->n_obj; k++ ) {
         gas_display_terminal( &pop[k][par->n_pop - 1], lim[k].n_dim, dispersao_media[k], geracao );
      }
      GasLimites v_lim = {
         .n_dim = lim[0].n_dim,
         .ini = g_new0( double, lim[0].n_dim ),
         .fim = g_new0( double, lim[0].n_dim )
      };
      memcpy( v_lim.ini, lim[0].ini, v_lim.n_dim * sizeof( double ) );
      memcpy( v_lim.fim, lim[2].fim, v_lim.n_dim * sizeof( double ) );

      gas_display_gnuplot( &v_lim, geracao );

      g_free( v_lim.ini );
      g_free( v_lim.fim );

      if ( p_fitness )   fclose( p_fitness );
      if ( p_dispersao ) fclose( p_dispersao );
   }
   //--------------------------------------------------------//

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


