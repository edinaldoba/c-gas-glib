/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <math.h>
#include <stdio.h>
#include <string.h> // Necessário para memcpy
#include <glib.h>

#include "gas.h"
#include "gnuplot.h"
#include "v_gas.h"



// ============================================================================
// FUNÇÃO DE FITNESS LOCAL (ALTAMENTE OTIMIZADA)
// ============================================================================
static double v_fitness_local( const double *x, const ImagemCinza *img, const int k ) {
   g_return_val_if_fail( x && img && img->image, 0.0 );

   ( void )k;

   int cx = ( int )round( x[0] );
   int cy = ( int )round( x[1] );

   if ( cx < 0 || cx >= img->ncol || cy < 0 || cy >= img->nrow ) return 0.0;

   // Vetores de direção: 0(Leste), 1(Oeste), 2(Sul), 3(Norte)
   int dx[4] = { 1, -1,  0,  0 };
   int dy[4] = { 0,  0,  1, -1 };

   int raio_max = 40;
   double fitness_total = 0.0;

   int centro_eh_preto = ( img->image[cy][cx] < 10 );

   // NOVO: Array para guardar o 'r' da última transição em cada uma das 4 direções
   int ultimo_r[4] = { 0, 0, 0, 0 };

   for ( int dir = 0; dir < 4; dir++ ) {
      int transicoes = 0;
      int estado_atual = centro_eh_preto;

      for ( int r = 1; r <= raio_max; r++ ) {
         int px = cx + ( dx[dir] * r );
         int py = cy + ( dy[dir] * r );

         if ( px < 0 || px >= img->ncol || py < 0 || py >= img->nrow ) break;

         int pixel_escuro = ( img->image[py][px] < 10 );

         if ( pixel_escuro != estado_atual ) {
            transicoes++;
            estado_atual = pixel_escuro;

            // NOVO: Atualiza a distância da transição mais recente
            ultimo_r[dir] = r;
         }
      }

      int pontuacao = 5 - abs( transicoes - 5 );
      if ( pontuacao < 0 ) pontuacao = 0;

      fitness_total += pontuacao;
   }

   double fitness_normalizado = fitness_total / 20.0;

   if ( !centro_eh_preto ) {
      fitness_normalizado *= 0.75;
   }
   // NOVO: Desempate do Platô! Só aplicamos se ele encontrou o alvo (20 pontos)
   else if ( fitness_total == 20.0 ) {

      // Calcula a diferença de distância das bordas opostas
      // Se estiver perfeitamente centralizado, erro_x e erro_y serão 0 (ou no máximo 1 por conta do grid de pixels)
      int erro_x = abs( ultimo_r[0] - ultimo_r[1] ); // Diferença entre Leste e Oeste
      int erro_y = abs( ultimo_r[2] - ultimo_r[3] ); // Diferença entre Sul e Norte

      // Aplicamos uma penalidade minúscula (0.0001 por pixel de assimetria)
      // Ex: Se o candidato está 3 pixels pro lado direito, erro_x = 6. Penalidade = 0.0006.
      // O fitness cai de 1.0000 para 0.9994.
      double penalidade_simetria = ( erro_x + erro_y ) * 0.015;

      // O centro absoluto mantém 1.0000 (ou o mais próximo disso possível)
      fitness_normalizado -= penalidade_simetria;
   }

   return fitness_normalizado;
}


// Fitness Local reforçado com varredura em forma de asterisco (Cruz + Xis)
static double v_fitness_local_cruz_mais_xis( const double *x, const ImagemCinza *img, const int k ) {
   g_return_val_if_fail( x && img && img->image, 0.0 );

   ( void )k;

   int cx = ( int )round( x[0] );
   int cy = ( int )round( x[1] );

   if ( cx < 0 || cx >= img->ncol || cy < 0 || cy >= img->nrow ) return 0.0;

   // Vetores de direção: 8 direções
   // [0,1]: Leste (+x), Oeste (-x)
   // [2,3]: Sul (+y), Norte (-y) (Considerando a origem da imagem no canto superior esquerdo)
   // [4,5]: Sudeste (+x, +y), Noroeste (-x, -y)
   // [6,7]: Nordeste (+x, -y), Sudoeste (-x, +y)
   int dx[8] = { 1, -1,  0,  0,  1, -1,  1, -1 };
   int dy[8] = { 0,  0,  1, -1,  1, -1, -1,  1 };

   // O raio máximo euclidiano da âncora
   int raio_max_ortogonal = 40;
   // Nas diagonais, r avança em dois eixos (distância real = r * sqrt(2)).
   // Limitamos r a ~28 para que a varredura não passe do raio real de 40 pixels.
   int raio_max_diagonal = 28;

   double fitness_total = 0.0;

   int centro_eh_preto = ( img->image[cy][cx] < 10 );

   // Array para guardar o 'r' da última transição em cada uma das 8 direções
   int ultimo_r[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

   for ( int dir = 0; dir < 8; dir++ ) {
      int transicoes = 0;
      int estado_atual = centro_eh_preto;

      // Ajusta o limite do laço dependendo se a direção é diagonal ou ortogonal
      int passos_maximos = ( dx[dir] != 0 && dy[dir] != 0 ) ? raio_max_diagonal : raio_max_ortogonal;

      for ( int r = 1; r <= passos_maximos; r++ ) {
         int px = cx + ( dx[dir] * r );
         int py = cy + ( dy[dir] * r );

         if ( px < 0 || px >= img->ncol || py < 0 || py >= img->nrow ) break;

         int pixel_escuro = ( img->image[py][px] < 10 );

         if ( pixel_escuro != estado_atual ) {
            transicoes++;
            estado_atual = pixel_escuro;

            // Atualiza o índice do passo da última transição
            ultimo_r[dir] = r;
         }
      }

      int pontuacao = 5 - abs( transicoes - 5 );
      if ( pontuacao < 0 ) pontuacao = 0;

      fitness_total += pontuacao;
   }

   // A pontuação máxima agora é 40 (8 direções * 5 pontos)
   double fitness_normalizado = fitness_total / 40.0;

   if ( !centro_eh_preto ) {
      fitness_normalizado *= 0.75;
   }
   // Desempate do Platô! Só aplicamos se ele encontrou o alvo (40 pontos inteiros)
   else if ( fitness_total == 40.0 ) {

      // Diferenças de distância (em passos) das bordas opostas
      int erro_x     = abs( ultimo_r[0] - ultimo_r[1] ); // Leste vs Oeste
      int erro_y     = abs( ultimo_r[2] - ultimo_r[3] ); // Sul vs Norte
      int erro_diag1 = abs( ultimo_r[4] - ultimo_r[5] ); // Sudeste vs Noroeste
      int erro_diag2 = abs( ultimo_r[6] - ultimo_r[7] ); // Nordeste vs Sudoeste

      // O multiplicador 0.0075 ajusta perfeitamente o aumento para 4 eixos!
      double penalidade_simetria = ( erro_x + erro_y ) * 0.0075 + ( erro_diag1 + erro_diag2 ) * 0.0075;

      fitness_normalizado -= penalidade_simetria;
   }

   return fitness_normalizado;
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

   // 2. Extrai as distâncias reais PRIMEIRO (Para resolver o "Ovo e a Galinha")
   double top_w   = v_gas_distancia( simulacao[0].x, simulacao[1].x );
   double bot_w   = v_gas_distancia( simulacao[3].x, simulacao[2].x );
   double left_h  = v_gas_distancia( simulacao[0].x, simulacao[3].x );
   double right_h = v_gas_distancia( simulacao[1].x, simulacao[2].x );

   double largura_real = ( top_w + bot_w ) / 2.0;
   double altura_real  = ( left_h + right_h ) / 2.0;

   // Barreira contra colapso geométrico (arestas muito pequenas)
   if ( largura_real < 50.0 || altura_real < 50.0 ) return 0.0;

   // 3. Detecção Automática da Direção da Página
   // Se o GA formou um retângulo mais largo que alto, assumimos gabarito Horizontal ('h')
   // Caso contrário, assumimos Vertical ('v')
   double proporcao_alvo;
   if ( largura_real > altura_real ) {
      proporcao_alvo = 14.0 / 11.0; // Horizontal
   } else {
      proporcao_alvo = 10.0 / 15.0; // Vertical
   }

   // 4. Calcula a área e as dimensões teóricas ideais
   double area = v_gas_calcular_area_ancoras( simulacao, 4 );

   // Como proporcao_alvo = Largura / Altura, deduzimos as dimensões ideais a partir da área:
   double largura_ideal = sqrt( area * proporcao_alvo );
   double altura_ideal  = sqrt( area / proporcao_alvo );

   // 5. Avaliação de Erros Geométricos
   double erro_w = fabs( largura_real - largura_ideal ) / largura_ideal;
   double erro_h = fabs( altura_real - altura_ideal ) / altura_ideal;

   // Cálculo do erro de ortogonalidade aproveitando as arestas já calculadas
   double erro_ortogonal = v_gas_erro_ortogonal( simulacao[0].x, simulacao[1].x,
                           simulacao[2].x, simulacao[3].x,
                           top_w, bot_w, left_h, right_h );

   // 6. Fitness (Erro Relativo Normalizado)
   // Os três erros gravitam de 0.0 a 1.0 (ou mais em deformações severas).
   double f_geo = 1.0 / ( 1.0 + erro_w + erro_h + erro_ortogonal );

   return f_geo;
}



// ============================================================================
// FUNÇÃO GLOBAL DE AVALIAÇÃO (COEVOLUÇÃO)
// ============================================================================

static double v_gas_fitness_coevolutivo( const double *x, const GasPopulacao *elite, const ImagemCinza *img,
                                         const double w1, const int k ) {
   g_return_val_if_fail( x && img, 0.0 );

   // Pesos da combinação linear para gerações > 0 (podem ser ajustados depois)
   // printf( "%lf\n", w1 ); getchar();
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
GasPopulacao *v_gas_pipeline( ImagemCinza *img, const GasParametros *par,
                              const GasLimites *lim, gboolean feedback_visual ) {
   g_return_val_if_fail( par && lim, NULL );

   // Alocação da matriz de dispersão
   double **coef_disp = g_new0( double*, par->n_obj );

   GasPopulacao **pop = g_new0( GasPopulacao*, par->n_obj );
   GasGenitores **gen = g_new0( GasGenitores*, par->n_obj );

   double disp_max = 0.0;

   for ( int k = 0; k < par->n_obj; k++ ) {
      coef_disp[k] = g_new0( double, lim[k].n_dim );

      for ( int j = 0; j < lim[k].n_dim; j++ ) {
         disp_max += par->peso_disp * ( lim[k].fim[j] - lim[k].ini[j] ) / sqrt( 12.0 );
      }

      pop[k] = gas_alocar_populacao( par->n_pop, lim[k].n_dim );
      gen[k] = gas_alocar_genitores( par->n_gen, lim[k].n_dim );

      gas_populacao_inicial( pop[k], par, &lim[k] );
   }

   disp_max = disp_max / ( lim[0].n_dim * par->n_obj );

   // printf("%lf\n",disp_max);getchar();

   GasPopulacao *elite = gas_alocar_populacao( par->n_obj, lim[0].n_dim );

   int geracao = 0;
   g_autofree double *dispersao_media = g_new0( double, par->n_obj );
   double dispersao_media_geral = 0.0;

   // Geração 0 (Avaliação Exploratória)
   for ( int k = 0; k < par->n_obj; k++ ) {
      gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
      dispersao_media[k] = gas_mean( coef_disp[k], lim[k].n_dim );

      // ========================================================================================
      // ⚠️ ALERTA PARA O EDINALDO DO FUTURO ⚠️
      // NUNCA tente calcular um 'w1' individual para cada objetivo (k) baseado apenas
      // na sua dispersão isolada! O peso 'w1' DEVE ser estritamente GLOBAL.
      //
      // Por que manter 'dispersao_media_geral'?
      // O sucesso da Coevolução (Equilíbrio de Nash) exige que TODAS as sub-populações
      // obedeçam às mesmas "regras do jogo" simultaneamente. Se um objetivo usa w1 alto
      // (focado na imagem local) e outro usa w1 baixo (focado na geometria), o aspecto
      // colaborativo é penalizado. A geometria do gabarito quebra, a ancoragem deforma
      // e a eficácia fantástica (>99.8%) do GA despenca!
      // ========================================================================================

      dispersao_media_geral = gas_mean( dispersao_media, par->n_obj );
      double proporcao = dispersao_media_geral / disp_max;

      // w1 inicia em ~0.95 e cai de forma igual e sincronizada para as 4 âncoras.
      // O CLAMP garante matematicamente que o peso nunca saia de [0, 1].
      double w1 = CLAMP( 0.96 * proporcao, 0.0, 1.0 );

      for ( int i = 0; i < par->n_pop; i++ ) {
         pop[k][i].fitness = v_gas_fitness_coevolutivo( pop[k][i].x, NULL, img, w1, k );
      }
      qsort( pop[k], par->n_pop, sizeof( GasPopulacao ), gas_comparar_objetivo_max );

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
      fprintf( p_fitness,   "%d %.8f\n", geracao, gas_mean( fitness_elite, par->n_obj ) );
      fprintf( p_dispersao, "%d %.8f\n", geracao, dispersao_media_geral );
   }
   //-------------------------------------------------------//

   // Laço Evolutivo
   do {
      geracao++;

      for ( int k = 0; k < par->n_obj; k++ ) {
         gas_torneio( pop[k], gen[k], lim[k].n_dim, par, gas_comparar_objetivo_max );
         gas_crossover_aritmetico( pop[k], gen[k], lim[k].n_dim, par );
         gas_mutacao_creep( pop[k], coef_disp[k], &lim[k], par );

         gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
         dispersao_media[k] = gas_mean( coef_disp[k], lim[k].n_dim );

         // A proporção normalizada: Inicia próxima de 1.0 e cai em direção a 0
         dispersao_media_geral = gas_mean( dispersao_media, par->n_obj );
         double proporcao = dispersao_media_geral / disp_max;

         // w1 inicia em ~0.9 e cai. O CLAMP garante matematicamente que o peso nunca saia de [0, 1]
         double w1 = CLAMP( 0.96 * proporcao, 0.0, 1.0 );

         for ( int i = 0; i < par->n_gen; i++ ) { // GG, isso estava errado a dias. Não se avalia membros antigos da população
            pop[k][i].fitness = v_gas_fitness_coevolutivo( pop[k][i].x, elite, img, w1, k );
         }
         qsort( pop[k], par->n_pop, sizeof( GasPopulacao ), gas_comparar_objetivo_max );

         memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof( double ) );
         elite[k].fitness = pop[k][par->n_pop - 1].fitness;
      }

      //--------------- FEEDBACK VISUAL ------------------------//
      if ( feedback_visual ) {
         for ( int k = 0; k < par->n_obj; k++ ) {
            fitness_elite[k] = elite[k].fitness;
            gas_gravar_pontos( pop[k], par->n_pop, geracao );
         }
         fprintf( p_fitness,   "%d %.8f\n", geracao, gas_mean( fitness_elite, par->n_obj ) );
         fprintf( p_dispersao, "%d %.8f\n", geracao, dispersao_media_geral );
      }
      //--------------------------------------------------------//

   } while ( dispersao_media_geral > par->toleracia && geracao < 100 );

   //--------------- FEEDBACK VISUAL ------------------------//
   if ( feedback_visual ) {
      v_gas_display_terminal( elite, dispersao_media, geracao );

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
      gas_liberar_populacao( pop[k], par->n_pop );
      gas_liberar_genitores( gen[k], par->n_gen );
   }

   g_free( pop );
   g_free( gen );
   g_free( coef_disp );

   // Retorna as âncoras limpas e seguras
   return elite;
}


