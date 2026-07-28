/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#include "gas.h"



static void display_tempo( const char *descricao, GTimer *cronometro );

typedef enum {
   GAS_TESTE_F5,
   GAS_TESTE_F6,
   GAS_TESTE_F10,
   GAS_TESTE_F11,
   GAS_TESTE_F13
} GasTesteId;


int main( void ) {
   g_autoptr( GTimer ) cronometro = g_timer_new();

   guint32 sementes[GAS_NUM_SEMENTES];
   gas_gerar_sementes( sementes );
   g_autoptr( GRand ) rand_context = g_rand_new_with_seed_array( sementes, G_N_ELEMENTS( sementes ) );

   // =========================================================
   // 2. CHAVE MESTRA: Mude apenas esta variável para alternar o teste
   // =========================================================
   GasTesteId teste_atual = GAS_TESTE_F10;

   // Estruturas base e ponteiros para as funções dinâmicas
   GasParametros par = { .rand = rand_context };
   GasLimites lim = { 0 };
   double limite_inf = 0.0, limite_sup = 0.0;

   double ( *gas_avaliar )( const double*, const int ) = NULL;
   int ( *gas_comparar )( const void*, const void* ) = NULL;

   // =========================================================
   // 3. CARREGAMENTO DOS PARÂMETROS ESPECÍFICOS DO TESTE
   // =========================================================
   switch ( teste_atual ) {
   case GAS_TESTE_F5:
      par.n_pop = 120;
      par.n_tor = 2;
      par.peso_disp = 1.9;
      par.toleracia = 1.0;

      lim.n_dim = 2;
      limite_inf = -50.0;
      limite_sup = +50.0;

      gas_avaliar = F5;
      gas_comparar = gas_comparar_objetivo_min;
      break;

   case GAS_TESTE_F6:
      par.n_pop = 220;
      par.n_tor = 2;
      par.peso_disp = 1.5;
      par.toleracia = 1.0e-3;

      lim.n_dim = 2;
      limite_inf = -100.0;
      limite_sup = +100.0;

      gas_avaliar = F6;
      gas_comparar = gas_comparar_objetivo_max;
      break;

   case GAS_TESTE_F10:
      par.n_pop = 850;
      par.n_tor = 2;
      par.peso_disp = 1.0;
      par.toleracia = 1.0e-4;

      lim.n_dim = 5;
      limite_inf = -6.0;
      limite_sup = +6.0;

      gas_avaliar = F10;
      gas_comparar = gas_comparar_objetivo_min;
      break;

   case GAS_TESTE_F11:
      par.n_pop = 330;
      par.n_tor = 2;
      par.peso_disp = 1.0;
      par.toleracia = 1.0e-2;

      lim.n_dim = 2;
      limite_inf = -500.0;
      limite_sup = +500.0;

      gas_avaliar = F11;
      gas_comparar = gas_comparar_objetivo_min;
      break;

   case GAS_TESTE_F13:
      par.n_pop = 60;
      par.n_tor = 10;
      par.peso_disp = 1.0;
      par.toleracia = 1.0e-4;

      lim.n_dim = 2;
      limite_inf = -10.0;
      limite_sup = +10.0;

      gas_avaliar = F13;
      gas_comparar = gas_comparar_objetivo_min;
      break;

   default:
      g_printerr( "Erro: Teste não reconhecido.\n" );
      return 1;
   }

   // =========================================================
   // 4. INICIALIZAÇÃO DEPENDENTE (Comum a todos os testes)
   // =========================================================
   par.n_gen = ( int )round( 0.82 * par.n_pop );
   par.p_rec = 0.75;
   par.p_mut = 0.95;

   lim.ini = g_new0( double, lim.n_dim );
   lim.fim = g_new0( double, lim.n_dim );

   for ( int j = 0; j < lim.n_dim; j++ ) {
      lim.ini[j] = limite_inf;
      lim.fim[j] = limite_sup;
   }

   // =========================================================
   // 5. EXECUÇÃO DO PIPELINE
   // =========================================================
   GasPopulacao melhor = gas_pipeline( &par, &lim, gas_avaliar, gas_comparar );

   // =========================================================
   // 6. LIMPEZA E RESULTADOS
   // =========================================================
   g_free( melhor.x );
   g_free( lim.ini );
   g_free( lim.fim );

   display_tempo( "Evolução", cronometro );

   return 0; // O g_autoptr(GRand) libera o rand_context automaticamente!
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



