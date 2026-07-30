/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <glib.h>
#include <math.h>

#include "gas.h"
#include "gnuplot.h"


void gas_display_gnuplot( const GasLimites *lim, const int geracao ) {
   g_return_if_fail( lim );

   FILE *p_plot;

   // 1. Gráfico de Evolução
   p_plot = fopen( "gnuplot/plotEvolucao.txt", "w" );
   if ( p_plot ) {
      fprintf( p_plot, "reset\n" );
      fprintf( p_plot, "set terminal wxt size 920,600 enhanced font 'Verdana,16' persist\n" );
      fprintf( p_plot, "set grid\n" );
      fprintf( p_plot, "set xrange [0:%d]\n", geracao );
      fprintf( p_plot, "set xlabel 'Geração'\n" );
      fprintf( p_plot, "set ylabel 'Avaliação'\n" );
      fprintf( p_plot, "plot 'E.pts' title 'Evolução da Avaliação do Mais Apto' with lines lt 3 lw 2\n" );
      fclose( p_plot );
   }

   // 2. Gráfico de Dispersão
   p_plot = fopen( "gnuplot/plotDispersao.txt", "w" );
   if ( p_plot ) {
      fprintf( p_plot, "reset\n" );
      fprintf( p_plot, "set terminal wxt size 900,600 enhanced font 'Verdana,16' persist\n" );
      fprintf( p_plot, "set grid\n" );
      fprintf( p_plot, "set xrange [0:%d]\n", geracao );
      fprintf( p_plot, "set xlabel 'Geração'\n" );
      fprintf( p_plot, "set ylabel 'Dispersão'\n" );
      fprintf( p_plot, "plot 'D.pts' title 'Evolução do Coeficiente de Dispersão' with lines lt 3 lw 2\n" );
      fclose( p_plot );
   }

   // 3. Animação dos Pontos (Limpo, Inteligente e Profissional)
   p_plot = fopen( "gnuplot/plotPontos.txt", "w" );
   if ( p_plot ) {
      fprintf( p_plot, "reset\n" );
      fprintf( p_plot, "set terminal wxt size 800,800 enhanced font 'Verdana,16' persist\n" );
      fprintf( p_plot, "set grid\n" );
      fprintf( p_plot, "set xrange [%.1f:%.1f]\n", lim->ini[0], lim->fim[0] );
      fprintf( p_plot, "set yrange [%.1f:%.1f]\n", lim->ini[1], lim->fim[1] );
      fprintf( p_plot, "set size ratio -1\n" );
      fprintf( p_plot, "set pointsize 2\n" );
      // Exemplo: define a escala de cor fixa entre o pior e o melhor fitness conhecido
      fprintf( p_plot, "set cbrange [0:1]\n" ); // Ajuste para os limites do seu fitness
      fprintf( p_plot, "set colorbox\n" );        // Exibe a barra lateral de cores

      // fprintf( p_plot, "set object 1 rectangle from graph 0,0 to graph 1,1 behind fillcolor rgb '#7f7f7f' fillstyle solid 1.0\n" );
      // // Ajusta a cor dos eixos/linhas para branco para dar contraste com o fundo escuro
      // fprintf( p_plot, "set border lc rgb 'white'\n" );
      // fprintf( p_plot, "set key tc rgb 'white'\n" );
      // fprintf( p_plot, "set tics tc rgb 'white'\n" );

      // Gradiente clássico estilo Jet: 0 = Azul (mínimo), 1 = Vermelho (máximo)
      fprintf( p_plot, "set palette defined ( 0 'dark-blue', 0.5 'yellow', 1.0 'red' )\n" );

      // Usamos o laço nativo do gnuplot para iterar sobre os arquivos .pts gerados
      fprintf( p_plot, "do for [i=0:%d] {\n", geracao );
      // 1:2:3 -> X na col 1, Y na col 2, Cor (Fitness) na col 3
      // pt 7 -> ponto preenchido
      // palette -> aplica o mapa de cores ativo no Gnuplot conforme o fitness da col 3
      fprintf( p_plot, "    plot sprintf('geracao_%%d.pts', i) using 1:2:3 title sprintf('Geração: %%d', i) with points pt 1 palette\n" );
      fprintf( p_plot, "    pause 0.015\n" );
      fprintf( p_plot, "}\n" );

      fclose( p_plot );
   }
}



void gas_gravar_pontos( const GasPopulacao *pop, const int n_pop, const int geracao ) {
   g_return_if_fail( pop && n_pop > 0 );

   char arquivo[256];
   snprintf( arquivo, sizeof( arquivo ), "gnuplot/geracao_%d.pts", geracao );
   FILE *p_geracao = fopen( arquivo, "a" );
   if ( p_geracao ) {
      for ( int i = 0; i < n_pop; i++ ) {
         fprintf( p_geracao, "%.8f %.8f %.8f\n", pop[i].x[0], pop[i].x[1], pop[i].fitness );
      }
      fclose( p_geracao );
   }
}


void gas_display_terminal( const GasPopulacao *pop, const int n_dim, const double dispersao_media, const int geracao ) {
   g_return_if_fail( pop && n_dim > 0 );

   printf( "Geração: %d\n", geracao );
   printf( "Mais Apto: " );
   for ( int i = 0; i < n_dim; i++ ) {
      printf( "%.8f  ", pop->x[i] );
   }
   printf( "\nAvaliação do Mais Apto: %.8f\n", pop->fitness );
   printf( "Coeficiente de Dispersão: %.8f\n\n", dispersao_media );
}

void v_gas_display_terminal( const GasPopulacao *elite, const double *dispersao_media, const int geracao ) {
   g_return_if_fail( elite && dispersao_media );

   printf( "Geração: %d\n", geracao );

   printf( "Dispersão: %.4f  %.4f  %.4f  %.4f\n",
           dispersao_media[0], dispersao_media[1], dispersao_media[2], dispersao_media[3] );

   printf( "Fitness:   %.4f  %.4f  %.4f  %.4f\n\n",
           elite[0].fitness, elite[1].fitness, elite[2].fitness, elite[3].fitness );

   printf( "A( %4d, %4d )    B( %4d, %4d )\nD( %4d, %4d )    C( %4d, %4d )\n\n",

           ( int )round( elite[0].x[0] ), ( int )round( elite[0].x[1] ), ( int )round( elite[1].x[0] ), ( int )round( elite[1].x[1] ),
           ( int )round( elite[3].x[0] ), ( int )round( elite[3].x[1] ), ( int )round( elite[2].x[0] ), ( int )round( elite[2].x[1] ) );

}
