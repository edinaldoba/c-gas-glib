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
// PROTÓTIPOS DAS FUNÇÕES INTERNAS DE FITNESS (A SEREM IMPLEMENTADAS POR VOCÊ)
// ============================================================================
// Função auxiliar para testar o fitness de uma escala (raio) específica
// static double avaliar_ancora_radial( int cx, int cy, const ImagemCinza *img, double raio_externo ) {
//     int acertos = 0;
//     int total = 0;
//
//     // Calcula o limite do bounding box para não varrer a imagem toda
//     int limite = (int)ceil( raio_externo * 1.05 );
//     double R2 = raio_externo * raio_externo; // Raio ao quadrado para evitar sqrt()
//
//     for ( int dy = -limite; dy <= limite; dy++ ) {
//         for ( int dx = -limite; dx <= limite; dx++ ) {
//
//             int px = cx + dx;
//             int py = cy + dy;
//
//             // Proteção contra Segmentation Fault
//             if ( px < 0 || px >= img->ncol || py < 0 || py >= img->nrow ) continue;
//
//             // Distância Euclidiana ao quadrado
//             double D2 = ( dx * dx ) + ( dy * dy );
//
//             // Distância normalizada ao quadrado (D2 / R2)
//             double U2 = D2 / R2;
//
//             if ( U2 > 1.1025 ) continue; // 1.1025 é 1.05 ao quadrado. Fora do alvo!
//
//             // Verifica se o pixel na imagem é escuro (usando o seu limiar original)
//             int pixel_escuro = ( img->image[py][px] < 10 );
//
//             // Define se a posição atual deveria ser escura baseada no seu desenho TikZ
//             // Proporções ao quadrado:
//             // Core (0.25^2 = 0.0625)
//             // Ring 1 (0.45^2 = 0.2025 até 0.65^2 = 0.4225)
//             // Ring 2 (0.85^2 = 0.7225 até 1.05^2 = 1.1025)
//             gboolean zona_escura = ( U2 <= 0.0625 ) ||
//                                    ( U2 >= 0.2025 && U2 <= 0.4225 ) ||
//                                    ( U2 >= 0.7225 && U2 <= 1.1025 );
//
//             if ( zona_escura ) {
//                 if ( pixel_escuro ) acertos++; // Bateu! Era pra ser escuro e é.
//             } else {
//                 if ( !pixel_escuro ) acertos++; // Bateu! Era pra ser claro (vazio) e é.
//             }
//             total++;
//         }
//     }
//
//     // Retorna o percentual de acertos (de 0.0 a 1.0)
//     return ( total == 0 ) ? 0.0 : ( (double)acertos / total );
// }

// static double avaliar_ancora_radial( int cx, int cy, const ImagemCinza *img, double raio_externo ) {
//    int acertos = 0;
//    int total = 0;
//
//    int limite = ( int )ceil( raio_externo * 1.05 );
//
//    // Inversão para trocar a divisão cara dentro do loop por uma multiplicação
//    double inv_R = 1.0 / raio_externo;
//
//    // Varreremos linearmente do centro até a borda (complexidade O(R))
//    for ( int r = 0; r <= limite; r++ ) {
//
//       // Distância normalizada linear [0.0, 1.05]
//       double u = r * inv_R;
//
//       // Proporções lineares (raízes dos seus U2 antigos):
//       // Core: <= 0.25
//       // Ring 1: 0.45 a 0.65
//       // Ring 2: 0.85 a 1.05
//       gboolean zona_escura = ( u <= 0.25 ) ||
//                              ( u >= 0.45 && u <= 0.65 ) ||
//                              ( u >= 0.85 && u <= 1.05 );
//
//       // Deslocamento para as 4 diagonais ( r * cos(45°) )
//       int d = ( int )round( r * 0.70710678 );
//
//       // 8 pontos distribuídos em formato de asterisco (*) no raio 'r'
//       // Vetores: E, W, S, N, SE, NW, SW, NE
//       int pontos[8][2] = {
//          { cx + r, cy }, { cx - r, cy }, { cx, cy + r }, { cx, cy - r },
//          { cx + d, cy + d }, { cx - d, cy - d }, { cx - d, cy + d }, { cx + d, cy - d }
//       };
//
//       // Se r == 0, todos os 8 pontos são o centro. Evita checar o mesmo pixel 8 vezes.
//       int num_pontos_a_testar = ( r == 0 ) ? 1 : 8;
//
//       for ( int i = 0; i < num_pontos_a_testar; i++ ) {
//          int px = pontos[i][0];
//          int py = pontos[i][1];
//
//          if ( px < 0 || px >= img->ncol || py < 0 || py >= img->nrow ) continue;
//
//          int pixel_escuro = ( img->image[py][px] < 10 );
//
//          // Verifica tanto os anéis pretos quanto os brancos
//          if ( zona_escura ) {
//             if ( pixel_escuro ) acertos++;
//          } else {
//             if ( !pixel_escuro ) acertos++;
//          }
//          total++;
//       }
//    }
//
//    return ( total == 0 ) ? 0.0 : ( ( double )acertos / total );
// }
//
// static double v_fitness_local( const double *x, const ImagemCinza *img, const int k ) {
//    g_return_val_if_fail( x && img && img->image, 0.0 );
//
//    // O quadrante 'k' não determina mais a direção da varredura porque círculos são simétricos!
//    // Mas ele pode ser mantido na assinatura para compatibilidade com a arquitetura.
//    ( void )k;
//
//    int cx = ( int )round( x[0] );
//    int cy = ( int )round( x[1] );
//
//    // 1. Array com as 5 escalas diferentes (raio externo em pixels)
//    // Ajuste esses valores baseados na resolução aproximada do escaneamento do gabarito
//    double escalas[] = { 15.0, 20.0, 25.0, 30.0, 35.0 };
//    int n_escalas = sizeof( escalas ) / sizeof( escalas[0] );
//
//    double max_fitness = 0.0;
//
//    // 2. Testa todas as escalas e fica com a que tiver o maior nível de acerto
//    for ( int i = 0; i < n_escalas; i++ ) {
//       double fit = avaliar_ancora_radial( cx, cy, img, escalas[i] );
//       if ( fit > max_fitness ) {
//          max_fitness = fit;
//       }
//    }
//
//    // Como os anéis cobrem muita área, a chance de um falso positivo (bater 100%) em sujeira é zero.
//    // Retornamos a escala que melhor "encaixou" no formato do alvo.
//    return max_fitness;
// }


static double v_fitness_local( const double *x, const ImagemCinza *img, const int k ) {
   g_return_val_if_fail( x && img && img->image, 0.0 );

   // O quadrante 'k' não é mais necessário para a direção geométrica,
   // pois a cruz é simétrica. Mantido na assinatura por arquitetura.
   ( void )k;

   int cx = ( int )round( x[0] );
   int cy = ( int )round( x[1] );

   // Se o chute do GA caiu fora dos limites da imagem, fitness = 0 direto
   if ( cx < 0 || cx >= img->ncol || cy < 0 || cy >= img->nrow ) return 0.0;

   // Vetores de direção para a cruz: Leste, Oeste, Sul, Norte
   int dx[4] = { 1, -1,  0,  0 };
   int dy[4] = { 0,  0,  1, -1 };

   int raio_max = 40;
   double fitness_total = 0.0;

   // O centro verdadeiro do alvo é PRETO. Guardamos o estado do pixel central.
   int centro_eh_preto = ( img->image[cy][cx] < 10 );

   // Lançamos 4 "raios" a partir do centro
   for ( int dir = 0; dir < 4; dir++ ) {
      int transicoes = 0;
      int estado_atual = centro_eh_preto;

      // Caminha do pixel 1 até o raio máximo de 40
      for ( int r = 1; r <= raio_max; r++ ) {
         int px = cx + ( dx[dir] * r );
         int py = cy + ( dy[dir] * r );

         // Se bater na borda da imagem, interrompe este raio
         if ( px < 0 || px >= img->ncol || py < 0 || py >= img->nrow ) break;

         int pixel_escuro = ( img->image[py][px] < 10 );

         // Se mudou de cor (Branco->Preto ou Preto->Branco), conta a transição
         if ( pixel_escuro != estado_atual ) {
            transicoes++;
            estado_atual = pixel_escuro;
         }
      }

      // Matemática do Gradiente: 5 - |transicoes - 5|
      // Perfeito (5) = 5 pontos. Deslocado (4 ou 6) = 4 pontos. Ruído (>=10) = 0 pontos.
      int pontuacao = 5 - abs( transicoes - 5 );
      if ( pontuacao < 0 ) pontuacao = 0; // Impede pontuação negativa em áreas de muito ruído

      fitness_total += pontuacao;
   }

   // A pontuação máxima teórica é 20 (4 direções * 5 pontos perfeitos)
   // Normalizamos para o intervalo de [0.0 a 1.0]
   double fitness_normalizado = fitness_total / 20.0;

   // Refinamento final: Se o GA tentar centralizar a âncora em uma das "valas" brancas
   // do alvo, ele conseguirá no máximo 4 transições. Mas para garantir que ele seja
   // expulso das zonas brancas e caia no "miolo" preto, aplicamos uma penalidade.
   if ( !centro_eh_preto ) {
      fitness_normalizado *= 0.75;
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

   // 2. Calcula a área e as dimensões teóricas ideais
   double area = v_gas_calcular_area_ancoras( simulacao, 4 );
   if ( area < 100.0 ) return 0.0; // Esta barreira garante que as arestas > 0

   // double area_elite = v_gas_calcular_area_ancoras( elite, 4 );

   double largura_ideal = sqrt( area * ( 14.0 / 11.0 ) );
   double altura_ideal  = sqrt( area * ( 11.0 / 14.0 ) );

   // 3. Extrai as distâncias reais
   double top_w   = v_gas_distancia( simulacao[0].x, simulacao[1].x );
   double bot_w   = v_gas_distancia( simulacao[3].x, simulacao[2].x );
   double left_h  = v_gas_distancia( simulacao[0].x, simulacao[3].x );
   double right_h = v_gas_distancia( simulacao[1].x, simulacao[2].x );

   double largura_real = ( top_w + bot_w ) / 2.0;
   double altura_real  = ( left_h + right_h ) / 2.0;

   // 4. Avaliação de Erros Geométricos
   // double erro_area = fabs( area - area_elite ) / area_elite;
   double erro_w    = fabs( largura_real - largura_ideal ) / largura_ideal;
   double erro_h    = fabs( altura_real - altura_ideal ) / altura_ideal;

   // NOVO: Cálculo do erro de ortogonalidade aproveitando as arestas já calculadas
   double erro_ortogonal = v_gas_erro_ortogonal( simulacao[0].x, simulacao[1].x,
                           simulacao[2].x, simulacao[3].x,
                           top_w, bot_w, left_h, right_h );

   // 5. Fitness (Erro Relativo Normalizado)
   // Os três erros gravitam de 0.0 a 1.0 (ou mais em deformações severas).
   // (void)erro_area;
   double f_geo = 1.0 / ( 1.0 + erro_w + erro_h + erro_ortogonal );

   return f_geo;
}

// ============================================================================
// FUNÇÃO GLOBAL DE AVALIAÇÃO (COEVOLUÇÃO)
// ============================================================================

double v_gas_fitness_coevolutivo( const double *x, const GasPopulacao *elite, const ImagemCinza *img,
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
GasPopulacao *v_gas_pipeline( const ImagemCinza *img, const GasParametros *par, const GasLimites *lim,
                              gboolean feedback_visual ) {
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


