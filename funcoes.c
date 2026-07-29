/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <glib.h>
#include <math.h>

#include "funcoes.h"


double F5( const double *x, const int n_dim ) {
   // Validação de segurança no padrão da GLib
   g_return_val_if_fail( x && n_dim > 0, 0.0 );

   // 1. Transformamos a1 e a2 em uma única matriz 2D.
   // 2. O modificador 'static const' é crucial aqui: ele diz ao compilador para alocar
   //    essa matriz na memória apenas uma vez (no segmento de dados), em vez de empurrar
   //    50 números inteiros para a pilha (stack) a cada milissegundo que a função for chamada.
   static const double a[2][25] = {
      {-32, -16,  0, 16, 32, -32, -16,  0, 16, 32, -32, -16,  0, 16, 32, -32, -16,  0, 16, 32, -32, -16,  0, 16, 32},
      {-32, -32, -32, -32, -32, -16, -16, -16, -16, -16,  0,  0,  0,  0,  0, 16, 16, 16, 16, 16, 32, 32, 32, 32, 32}
   };

   const double K = 500.0;
   double soma = 0.0;

   // Proteção para garantir que o laço não tente ler uma 3ª dimensão inexistente na matriz 'a'
   int dim_max = ( n_dim < 2 ) ? n_dim : 2;

   for ( int j = 0; j < 25; j++ ) {
      double soma_potencias = 0.0;

      // Aproveitando o n_dim para iterar sobre as dimensões de forma flexível e expansível
      for ( int d = 0; d < dim_max; d++ ) {
         soma_potencias += pow( x[d] - a[d][j], 6.0 );
      }

      // Uso explícito de '1.0' e '(double)' para evitar conversões implícitas
      soma += 1.0 / ( ( double )j + soma_potencias );
   }

   return 1.0 / ( ( 1.0 / K ) + soma );
}


double F6( const double *x, const int n_dim ) {
   g_return_val_if_fail( x && n_dim > 0, 0.0 );

   double soma = 0.0;

   // 1. Multiplicação direta em vez de pow(x[j], 2)
   for ( int j = 0; j < n_dim; j++ ) {
      soma += x[j] * x[j];
   }

   // 2. Fragmentação da equação para evitar múltiplos pow() e melhorar a leitura
   double temp_sin = sin( sqrt( soma ) );
   double numerador = ( temp_sin * temp_sin ) - 0.5;

   double temp_denom = 1.0 + 0.001 * soma;
   double denominador = temp_denom * temp_denom;

   return 0.5 - ( numerador / denominador );
}

double F10( const double *x, const int n_dim ) { // Função de Rastrigin I
   g_return_val_if_fail( x && n_dim > 0, 0.0 );

   const double A = 10.0;
   double soma = 0.0;

   // Iteramos sobre as dimensões (n_dim) em vez de usar um 'ndim' global
   for ( int j = 0; j < n_dim; j++ ) {
      // 1. Substituímos pow(x[j], 2) pela multiplicação direta x[j] * x[j]
      // 2. Utilizamos a constante G_PI nativa da GLib (que já possui precisão máxima)
      // 3. Garantimos que 2.0 seja tratado como double
      soma += ( x[j] * x[j] ) - A * cos( 2.0 * G_PI * x[j] );
   }

   return ( A * n_dim ) + soma;
}

double F11( const double *x, const int n_dim ) { // Função de Schwefel
   g_return_val_if_fail( x && n_dim > 0, 0.0 );

   const double V = 418.9829;
   double soma = 0.0;

   // Iteramos sobre as dimensões (n_dim) para suportar ND, além de 2D
   for ( int j = 0; j < n_dim; j++ ) {
      // 1. Utilizamos fabs(x[j]) para obter o módulo |x_i| garantindo compatibilidade com double
      // 2. sqrt() extrai a raiz quadrada do módulo
      // 3. Multiplicamos -x[j] pelo seno do resultado
      soma += -x[j] * sin( sqrt( fabs( x[j] ) ) );
   }

   return ( V * n_dim ) + soma;
}

double F13( const double *x, const int n_dim ) { // Função de Shubert
   g_return_val_if_fail( x && n_dim > 0, 0.0 );

   double produtorio = 1.0;

   // Iteramos sobre as dimensões (i)
   for ( int i = 0; i < n_dim; i++ ) {
      double somatorio_interno = 0.0;

      // Somatório interno j de 1 a 5
      for ( int j = 1; j <= 5; j++ ) {
         // j é convertido implicitamente para double nas operações
         somatorio_interno += j * cos( ( j + 1 ) * x[i] + j );
      }

      produtorio *= somatorio_interno;
   }

   return produtorio;
}
