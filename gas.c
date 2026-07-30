/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Não esqueça de incluir para usar o memcpy
#include <math.h>

#include "gnuplot.h"
#include "matriz.h"



double gas_max( const double *array, const int tam ) {
   g_return_val_if_fail( array && tam > 0, 0.0 );
   double max_val = array[0];
   for ( int i = 1; i < tam; i++ ) {
      max_val = MAX( max_val, array[i] );
   }
   return max_val;
}

static double gas_sum( const double *array, const int tam ) {
   g_return_val_if_fail( array && tam > 0, 0.0 );
   double soma = 0.0;
   for ( int i = 0; i < tam; i++ ) {
      soma += array[i];
   }
   return soma;
}

double gas_mean( const double *array, const int tam ) {
   g_return_val_if_fail( tam > 0, 0.0 );
   return gas_sum( array, tam ) / tam;
}




void gas_gerar_sementes( guint32 *sementes ) {
   g_return_if_fail( sementes );

   // 1. Pega os ciclos/tempo monotônico do processador em alta precisão (64 bits)
   gint64 ciclos_cpu = g_get_monotonic_time();

   // 2. Separa a parte alta e baixa do inteiro de 64 bits
   guint32 baixa = ( guint32 )( ciclos_cpu & 0xFFFFFFFF );
   guint32 alta  = ( guint32 )( ciclos_cpu >> 32 );

   // Imprime para manter a rastreabilidade se precisar reproduzir a execução
   // g_print( "Semente Monotonica (Ciclos): %" G_GINT64_FORMAT "\n", ciclos_cpu );

   // 3. Monta o array de sementes multiplicando por constantes de dispersão
   sementes[0] = baixa;
   sementes[1] = alta ^ 0x9E3779B9; // Proporção Áurea de 32-bit
   sementes[2] = baixa ^ 0x6789A;
   sementes[3] = ( baixa + alta ) ^ 0xBCDEF;
}


GasPopulacao *gas_alocar_populacao( const int n_pop, const int n_dim ) {
   g_return_val_if_fail( n_pop > 0 && n_dim > 0, NULL );

   GasPopulacao *pop = g_new0( GasPopulacao, n_pop );
   for ( int i = 0; i < n_pop; i++ ) {
      pop[i].x = g_new0( double, n_dim );
   }
   return pop;
}

GasGenitores *gas_alocar_genitores( const int n_gen, const int n_dim ) {
   g_return_val_if_fail( n_gen > 0 && n_dim > 0, NULL );

   GasGenitores *gen = g_new0( GasGenitores, n_gen );
   for ( int i = 0; i < n_gen; i++ ) {
      gen[i].x = g_new0( double, n_dim );
   }
   return gen;
}

void gas_liberar_populacao( GasPopulacao *pop, const int n_pop ) {
   g_return_if_fail( pop );
   for ( int i = 0; i < n_pop; i++ ) {
      if ( pop[i].x != NULL ) {
         g_free( pop[i].x );
      }
   }
   g_free( pop );
}

void gas_liberar_genitores( GasGenitores *gen, const int n_gen ) {
   g_return_if_fail( gen );
   for ( int i = 0; i < n_gen; i++ ) {
      if ( gen[i].x != NULL ) {
         g_free( gen[i].x );
      }
   }
   g_free( gen );
}



void gas_populacao_inicial( GasPopulacao *pop, const GasParametros *par, const GasLimites *lim ) {
   g_return_if_fail( pop && par && lim );

   for ( int i = 0; i < par->n_pop; i++ ) {
      for ( int j = 0; j < lim->n_dim; j++ ) {
         pop[i].x[j] = g_rand_double_range( par->rand, lim->ini[j], lim->fim[j] );
      }
   }
}




void gas_projetar_pca( const GasPopulacao *pop, GasPopulacao *pop_2d, int n_pop, int n_dim ) {
   g_return_if_fail( pop != NULL && pop_2d != NULL );
   g_return_if_fail( n_pop > 1 && n_dim > 2 ); // O PCA só faz sentido se dimensão > 2 e pop > 1

   // 1. Alocação das matrizes de trabalho usando sua biblioteca
   Matrix X           = mat_new( n_pop, n_dim );
   Matrix media       = mat_new( 1, n_dim );
   Matrix Cov         = mat_new( n_dim, n_dim );
   Matrix autovalores = mat_new( n_dim, 1 );
   Matrix autovetores = mat_new( n_dim, n_dim );
   Matrix X_2d        = mat_new( n_pop, 2 );

   // 2. Extração dos dados da struct GasPopulacao para a Matriz contígua X
   for ( int i = 0; i < n_pop; i++ ) {
      for ( int j = 0; j < n_dim; j++ ) {
         X.data[i * n_dim + j] = pop[i].x[j];
      }
   }

   // 3. Pipeline PCA Purista de Alto Desempenho
   mat_centralizar_na_origem( X, media );

   // Cov = X^T * X
   mat_transpose_by_mul( X, Cov );
   // Ajuste para covariância amostral (divide por n - 1)
   mat_mul_esc_inplace( Cov, 1.0 / ( double )( n_pop - 1 ) );

   // Extração dos Autovalores e Autovetores via LAPACKE
   mat_eigen_symm( Cov, autovalores, autovetores );

   // Projeção nos 2 maiores autovetores
   mat_projetar_pca_2d( X, autovetores, X_2d );

   // 4. Mapeamento de volta para a struct de saída (pop_2d)
   for ( int i = 0; i < n_pop; i++ ) {
      // Importante: Assumimos que pop_2d[i].x já está previamente alocado com tamanho 2
      pop_2d[i].x[0] = X_2d.data[i * 2 + 0]; // Eixo Principal 1 (Maior variância)
      pop_2d[i].x[1] = X_2d.data[i * 2 + 1]; // Eixo Principal 2 (Segunda maior variância)

      // Copiar o fitness é excelente caso você queira colorir os pontos no Gnuplot por qualidade!
      pop_2d[i].fitness = pop[i].fitness;
   }

   // 5. Limpeza de memória (Assumindo que sua struct Matrix usa free nativo)
   mat_free( X );
   mat_free( media );
   mat_free( Cov );
   mat_free( autovalores );
   mat_free( autovetores );
   mat_free( X_2d );
}



void gas_torneio( const GasPopulacao *pop, GasGenitores *gen, const int n_dim, const GasParametros *par,
                  int( gas_comparar )( const void* a, const void* b ) ) {
   g_return_if_fail( pop && gen && par && gas_comparar );

   for ( int i = 0; i < par->n_gen; i++ ) {
      int rnd1 = g_rand_int_range( par->rand, 0, par->n_pop );

      for ( int k = 0; k < par->n_tor - 1; k++ ) {
         int rnd2 = g_rand_int_range( par->rand, 0, par->n_pop );

         if ( gas_comparar( &pop[rnd2], &pop[rnd1] ) == 1 ) {
            rnd1 = rnd2;
         }
      }
      memcpy( gen[i].x, pop[rnd1].x, n_dim * sizeof( double ) );
   }
}



void gas_crossover_aritmetico( GasPopulacao *pop, const GasGenitores *gen, const int n_dim, const GasParametros *par ) {
   g_return_if_fail( pop && gen && par );

   for ( int i = 0; i < par->n_gen - 1; i += 2 ) {
      double rnd = g_rand_double_range( par->rand, 0.0, 1.0 );

      if ( rnd < par->p_rec ) {
         double a = g_rand_double_range( par->rand, 0.0, 1.0 );

         for ( int j = 0; j < n_dim; j++ ) {
            pop[i].x[j] = a * gen[i].x[j] + ( 1.0 - a ) * gen[i + 1].x[j];
            pop[i + 1].x[j] = a * gen[i + 1].x[j] + ( 1.0 - a ) * gen[i].x[j];
         }

      } else {
         memcpy( pop[i].x, gen[i].x, n_dim * sizeof( double ) );
         memcpy( pop[i + 1].x, gen[i + 1].x, n_dim * sizeof( double ) );
      }
   }
}


void gas_mutacao_direcional( GasPopulacao *pop, const double *coef_disp, const int n_dim, const GasParametros *par ) {
   g_return_if_fail( pop && coef_disp && par );

   double fator_escala = 1.0 / sqrt( n_dim );

   for ( int i = 0; i < par->n_gen; i++ ) {
      double rnd_mut = g_rand_double_range( par->rand, 0.0, 1.0 );

      if ( rnd_mut < par->p_mut ) {
         for ( int j = 0; j < n_dim; j++ ) {
            double rnd_dir = g_rand_double_range( par->rand, -1.0, 1.0 );
            pop[i].x[j] = pop[i].x[j] + rnd_dir * coef_disp[j] * fator_escala;
         }
      }
   }
}


// GG, eu adaptei o meu coeficiente de dispersão lindo e maravilhoso na mutação creep. Ficou perfeito!
void gas_mutacao_creep( GasPopulacao *pop, const double *coef_disp, const GasLimites *lim, const GasParametros *par ) {
   g_return_if_fail( pop && coef_disp && lim && par );

   for ( int i = 0; i < par->n_gen; i++ ) {
      double rnd_mut = g_rand_double_range( par->rand, 0.0, 1.0 );

      if ( rnd_mut < par->p_mut ) {
         double fator_escala = 1.0 / sqrt( lim->n_dim );

         for ( int j = 0; j < lim->n_dim; j++ ) {
            double rnd_step = g_rand_double_range( par->rand, 0.0, 1.0 );

            if ( g_rand_boolean( par->rand ) ) {
               pop[i].x[j] = pop[i].x[j] + rnd_step * MIN( lim->fim[j] - pop[i].x[j], coef_disp[j] * fator_escala );

            } else {
               pop[i].x[j] = pop[i].x[j] - rnd_step * MIN( pop[i].x[j] - lim->ini[j], coef_disp[j] * fator_escala );
            }
         }
      }
   }
}

void gas_coeficiente_dispersao( const GasPopulacao *pop, double *coef_disp, const GasParametros *par, const int n_dim ) {
   g_return_if_fail( pop && coef_disp && par );

   for ( int j = 0; j < n_dim; j++ ) {
      double inv_n_pop = 1.0 / par->n_pop;
      double soma = 0.0;

      for ( int i = 0; i < par->n_pop; i++ ) {
         soma += pop[i].x[j];
      }
      double media = soma * inv_n_pop;
      soma = 0.0;

      for ( int i = 0; i < par->n_pop; i++ ) {
         double diff = pop[i].x[j] - media;
         soma += diff * diff;
      }
      coef_disp[j] = par->peso_disp * sqrt( soma * inv_n_pop );
   }
}



int gas_comparar_objetivo_max( const void* a, const void* b ) {
   const GasPopulacao *arg1 = ( const GasPopulacao * )a;
   const GasPopulacao *arg2 = ( const GasPopulacao * )b;
   if ( arg1->fitness < arg2->fitness ) return -1;
   if ( arg1->fitness > arg2->fitness ) return 1;
   return 0;
}

int gas_comparar_objetivo_min( const void* a, const void* b ) {
   const GasPopulacao *arg1 = ( const GasPopulacao * )a;
   const GasPopulacao *arg2 = ( const GasPopulacao * )b;
   if ( arg1->fitness < arg2->fitness ) return 1;
   if ( arg1->fitness > arg2->fitness ) return -1;
   return 0;
}


GasPopulacao *gas_pipeline( const GasParametros *par, const GasLimites *lim, gboolean feedback_visual,
                            double( gas_avaliar )( const double*, const int ),
                            int( gas_comparar )( const void* a, const void* b ) ) {
   g_return_val_if_fail( par && lim && gas_avaliar && gas_comparar, NULL );

   double *coef_disp = g_new0( double, lim->n_dim );
   GasPopulacao *pop = gas_alocar_populacao( par->n_pop, lim->n_dim );
   GasGenitores *gen = gas_alocar_genitores( par->n_gen, lim->n_dim );

   int geracao = 0;
   double dispersao_media;

   gas_populacao_inicial( pop, par, lim );

   for ( int i = 0; i < par->n_pop; i++ ) {
      pop[i].fitness = gas_avaliar( pop[i].x, lim->n_dim );
   }
   qsort( pop, par->n_pop, sizeof( GasPopulacao ), gas_comparar );
   gas_coeficiente_dispersao( pop, coef_disp, par, lim->n_dim );

   //--------------- FEEDBACK VISUAL ------------------------//
   GasPopulacao *pop_2d = NULL;
   FILE *p_dispersao = NULL;
   FILE *p_fitness = NULL;
   if ( feedback_visual ) {
      p_dispersao = fopen( "gnuplot/D.pts", "w" );
      p_fitness   = fopen( "gnuplot/E.pts", "w" );
      dispersao_media = gas_mean( coef_disp, lim->n_dim );
      fprintf( p_dispersao, "%d %.8f\n", geracao, dispersao_media );
      fprintf( p_fitness, "%d %.8f\n", geracao, pop[par->n_pop - 1].fitness );
      if ( lim->n_dim > 2 ) {
         pop_2d = gas_alocar_populacao( par->n_pop, 2 );
         gas_projetar_pca( pop, pop_2d, par->n_pop, lim->n_dim );
         gas_gravar_pontos( pop_2d, par->n_pop, geracao );
      } else {
         gas_gravar_pontos( pop, par->n_pop, geracao );
      }
   }
   //-------------------------------------------------------//

   do {
      /****************************** ALGORITMOS GENÉTICOS ******************************/
      geracao = geracao + 1;

      gas_torneio( pop, gen, lim->n_dim, par, gas_comparar );
      gas_crossover_aritmetico( pop, gen, lim->n_dim, par );
      gas_mutacao_creep( pop, coef_disp, lim, par );
      // gas_mutacao_direcional( pop, coef_disp, lim->n_dim, par );

      for ( int i = 0; i < par->n_gen; i++ ) {
         pop[i].fitness = gas_avaliar( pop[i].x, lim->n_dim );
      }
      qsort( pop, par->n_pop, sizeof( GasPopulacao ), gas_comparar );
      gas_coeficiente_dispersao( pop, coef_disp, par, lim->n_dim );
      dispersao_media = gas_mean( coef_disp, lim->n_dim );

      //--------------- FEEDBACK VISUAL ------------------------//
      if ( feedback_visual ) {
         fprintf( p_dispersao, "%d %.8f\n", geracao, dispersao_media );
         fprintf( p_fitness, "%d %.8f\n", geracao, pop[par->n_pop - 1].fitness );
         if ( lim->n_dim > 2 ) {
            gas_projetar_pca( pop, pop_2d, par->n_pop, lim->n_dim );
            gas_gravar_pontos( pop_2d, par->n_pop, geracao );
         } else {
            gas_gravar_pontos( pop, par->n_pop, geracao );
         }
      }
      //--------------------------------------------------------//

   } while ( dispersao_media > par->toleracia && geracao < 1000 ); // <--- FIM DO LOOP WHILE

   //--------------- FEEDBACK VISUAL ------------------------//
   if ( feedback_visual ) {
      gas_display_terminal( &pop[par->n_pop - 1], lim->n_dim, dispersao_media, geracao );
      gas_display_gnuplot( lim, geracao );
      if ( p_fitness )   fclose( p_fitness );
      if ( p_dispersao ) fclose( p_dispersao );
      if ( pop_2d ) gas_liberar_populacao( pop_2d, par->n_pop );
   }
   //--------------------------------------------------------//

   GasPopulacao *melhor = gas_alocar_populacao( 1, lim->n_dim );
   melhor->fitness = pop[par->n_pop - 1].fitness;
   memcpy( melhor->x, pop[par->n_pop - 1].x, lim->n_dim * sizeof( double ) );

   //-------- Liberar memória ----------------
   g_free( coef_disp );
   gas_liberar_populacao( pop, par->n_pop );
   gas_liberar_genitores( gen, par->n_gen );

   return melhor;
}

