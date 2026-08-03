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



/**
 * @brief Obtém o limiar de preto adaptativo amostrando a População Inicial (LHS).
 *
 * @param img Ponteiro para a imagem em escala de cinza.
 * @param pop Vetor com a população inicializada pelo Hipercubo Latino.
 * @param par Estrutura de parâmetros contendo o tamanho n_pop.
 * @return int Limiar dinâmico [15, 80] para considerar um pixel como "escuro/tinta".
 */
// Função de comparação para o qsort
// static int comparar_int( const void *a, const void *b ) {
//    return ( *( int * )a - *( int * )b );
// }
// static int obter_limiar_preto_dinamico( const ImagemCinza *img, const GasPopulacao *pop, const GasParametros *par ) {
//    g_return_val_if_fail( img && img->image && pop && par && par->n_pop > 0, 20 );
//
//    // Aloca um array temporário no stack para os tons coletados
//    int *amostras = g_newa( int, par->n_pop );
//    int n_validos = 0;
//
//    // 1. Extrai o tom do pixel em cada ponto inicializado pelo LHS
//    for ( int i = 0; i < par->n_pop; i++ ) {
//       int cx = ( int )round( pop[i].x[0] );
//       int cy = ( int )round( pop[i].x[1] );
//
//       // Proteção de limites de borda
//       if ( cx >= 0 && cx < img->ncol && cy >= 0 && cy < img->nrow ) {
//          amostras[n_validos++] = img->image[cy][cx];
//       }
//    }
//
//    if ( n_validos == 0 ) return 20; // Fallback de segurança
//
//
//
//    // 2. Ordena os tons amostrados (do mais escuro para o mais claro)
//    qsort( amostras, n_validos, sizeof( int ), comparar_int );
//
//    // 3. Pegamos o tom do percentil 5% (índice k) das amostras mais escuras
//    int idx_percentil = ( int )( n_validos * 0.05 );
//    int tom_tinta_amostrado = amostras[idx_percentil];
//
//    // 4. Calculamos o limiar dando uma margem de +25 tons acima da tinta detectada
//    int limiar_dinamico = tom_tinta_amostrado + 25;
//
//    // Trava o limiar em uma faixa física plausível [15, 80]
//    // - Nunca menor que 15 (para evitar falso positivo em papéis brancos demais)
//    // - Nunca maior que 80 (para não considerar sombras intensas de celular como tinta)
//    return CLAMP( limiar_dinamico, 15, 80 );
// }



// ============================================================================
// FUNÇÃO QUE CALCULA OS LIMITES DOS 4 QUADRANTES (4 OBJETIVOS)
// ============================================================================
GasLimites *v_gas_limites( const int nrow, const int ncol, const int n_obj ) {
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



// ============================================================================
// FUNÇÃO DE FITNESS LOCAL (ALTAMENTE OTIMIZADA)
// ============================================================================
static double v_fitness_local( const double *x, const ImagemCinza *img, const int limiar, const int k ) {
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

   int centro_eh_preto = ( img->image[cy][cx] < limiar );

   // NOVO: Array para guardar o 'r' da última transição em cada uma das 4 direções
   int ultimo_r[4] = { 0, 0, 0, 0 };

   for ( int dir = 0; dir < 4; dir++ ) {
      int transicoes = 0;
      int estado_atual = centro_eh_preto;

      for ( int r = 1; r <= raio_max; r++ ) {
         int px = cx + ( dx[dir] * r );
         int py = cy + ( dy[dir] * r );

         if ( px < 0 || px >= img->ncol || py < 0 || py >= img->nrow ) break;

         int pixel_escuro = ( img->image[py][px] < limiar );

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
      // GG, deixe-me te dar um feedback (essa penalização mudou de 0.75 para 0.90)
      // Afinei este parâmetro a luz dos testes em massa
      fitness_normalizado *= 0.75;
   }
   // NOVO: Desempate do Platô! Só aplicamos se ele encontrou o alvo (20 pontos)
   else if ( fitness_total == 20.0 ) {

      // Calcula a diferença de distância das bordas opostas
      // Se estiver perfeitamente centralizado, erro_x e erro_y serão 0 (ou no máximo 1 por conta do grid de pixels)
      int erro_x = abs( ultimo_r[0] - ultimo_r[1] ); // Diferença entre Leste e Oeste
      int erro_y = abs( ultimo_r[2] - ultimo_r[3] ); // Diferença entre Sul e Norte

      // O número 0.015 também foi um ajuste fino verificado nos testes em massa
      double penalidade_simetria = ( erro_x + erro_y ) * 0.015;

      // O centro absoluto mantém 1.0000 (ou o mais próximo disso possível)
      fitness_normalizado -= penalidade_simetria;
   }

   return fitness_normalizado;
}


// Fitness Local reforçado com varredura em forma de asterisco (Cruz + Xis)
// static double v_fitness_local_cruz_mais_xis( const double *x, const ImagemCinza *img, const int k ) {
//    g_return_val_if_fail( x && img && img->image, 0.0 );
//
//    ( void )k;
//
//    int cx = ( int )round( x[0] );
//    int cy = ( int )round( x[1] );
//
//    if ( cx < 0 || cx >= img->ncol || cy < 0 || cy >= img->nrow ) return 0.0;
//
//    // Vetores de direção: 8 direções
//    // [0,1]: Leste (+x), Oeste (-x)
//    // [2,3]: Sul (+y), Norte (-y) (Considerando a origem da imagem no canto superior esquerdo)
//    // [4,5]: Sudeste (+x, +y), Noroeste (-x, -y)
//    // [6,7]: Nordeste (+x, -y), Sudoeste (-x, +y)
//    int dx[8] = { 1, -1,  0,  0,  1, -1,  1, -1 };
//    int dy[8] = { 0,  0,  1, -1,  1, -1, -1,  1 };
//
//    // O raio máximo euclidiano da âncora
//    int raio_max_ortogonal = 40;
//    // Nas diagonais, r avança em dois eixos (distância real = r * sqrt(2)).
//    // Limitamos r a ~28 para que a varredura não passe do raio real de 40 pixels.
//    int raio_max_diagonal = 28;
//
//    double fitness_total = 0.0;
//
//    int centro_eh_preto = ( img->image[cy][cx] < 10 );
//
//    // Array para guardar o 'r' da última transição em cada uma das 8 direções
//    int ultimo_r[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
//
//    for ( int dir = 0; dir < 8; dir++ ) {
//       int transicoes = 0;
//       int estado_atual = centro_eh_preto;
//
//       // Ajusta o limite do laço dependendo se a direção é diagonal ou ortogonal
//       int passos_maximos = ( dx[dir] != 0 && dy[dir] != 0 ) ? raio_max_diagonal : raio_max_ortogonal;
//
//       for ( int r = 1; r <= passos_maximos; r++ ) {
//          int px = cx + ( dx[dir] * r );
//          int py = cy + ( dy[dir] * r );
//
//          if ( px < 0 || px >= img->ncol || py < 0 || py >= img->nrow ) break;
//
//          int pixel_escuro = ( img->image[py][px] < 10 );
//
//          if ( pixel_escuro != estado_atual ) {
//             transicoes++;
//             estado_atual = pixel_escuro;
//
//             // Atualiza o índice do passo da última transição
//             ultimo_r[dir] = r;
//          }
//       }
//
//       int pontuacao = 5 - abs( transicoes - 5 );
//       if ( pontuacao < 0 ) pontuacao = 0;
//
//       fitness_total += pontuacao;
//    }
//
//    // A pontuação máxima agora é 40 (8 direções * 5 pontos)
//    double fitness_normalizado = fitness_total / 40.0;
//
//    if ( !centro_eh_preto ) {
//       fitness_normalizado *= 0.75;
//    }
//    // Desempate do Platô! Só aplicamos se ele encontrou o alvo (40 pontos inteiros)
//    else if ( fitness_total == 40.0 ) {
//
//       // Diferenças de distância (em passos) das bordas opostas
//       int erro_x     = abs( ultimo_r[0] - ultimo_r[1] ); // Leste vs Oeste
//       int erro_y     = abs( ultimo_r[2] - ultimo_r[3] ); // Sul vs Norte
//       int erro_diag1 = abs( ultimo_r[4] - ultimo_r[5] ); // Sudeste vs Noroeste
//       int erro_diag2 = abs( ultimo_r[6] - ultimo_r[7] ); // Nordeste vs Sudoeste
//
//       // O multiplicador 0.0075 ajusta perfeitamente o aumento para 4 eixos!
//       double penalidade_simetria = ( erro_x + erro_y ) * 0.0075 + ( erro_diag1 + erro_diag2 ) * 0.0075;
//
//       fitness_normalizado -= penalidade_simetria;
//    }
//
//    return fitness_normalizado;
// }


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
      const double w1, const int limiar, const int k ) {
   g_return_val_if_fail( x && img, 0.0 );

   // Pesos da combinação linear para gerações > 0 (podem ser ajustados depois)
   // printf( "%lf\n", w1 ); getchar();
   const double w2 = 1.0 - w1;

   // 1. O fitness local sempre é calculado, independentemente da geração
   // Avalia o contraste/textura da imagem exatamente na coordenada 'x'
   double f_local = v_fitness_local( x, img, limiar, k );

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
GasPopulacao *v_gas_pipeline( const ImagemCinza *img, GasParametros *par,
                              const GasLimites *lim, const gboolean feedback_visual ) {
   g_return_val_if_fail( img && par && lim, NULL );

   // Alocação da matriz de dispersão e populações
   double **coef_disp = g_new0( double*, par->n_obj );
   GasPopulacao **pop = g_new0( GasPopulacao*, par->n_obj );
   GasGenitores **gen = g_new0( GasGenitores*, par->n_obj );

   double disp_max = 0.0;
   g_autofree double *dispersao_media = g_new0( double, par->n_obj );

   // Inicialização e cálculo da Dispersão Máxima Teórica (Uniforme)
   for ( int k = 0; k < par->n_obj; k++ ) {
      coef_disp[k] = g_new0( double, lim[k].n_dim );

      for ( int j = 0; j < lim[k].n_dim; j++ ) {
         disp_max += par->peso_disp * ( lim[k].fim[j] - lim[k].ini[j] ) / sqrt( 12.0 );
      }

      pop[k] = gas_alocar_populacao( par->n_pop, lim[k].n_dim );
      gen[k] = gas_alocar_genitores( par->n_gen, lim[k].n_dim );

      gas_populacao_inicial_uniforme( pop[k], par, &lim[k] );

      // A dispersão inicial já é pré-calculada no setup
      gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
      dispersao_media[k] = gas_mean( coef_disp[k], lim[k].n_dim );
   }

   disp_max /= ( lim[0].n_dim * par->n_obj );

   GasPopulacao *elite = gas_alocar_populacao( par->n_obj, lim[0].n_dim );
   int geracao = 0;
   double dispersao_media_global = 0.0;

   // =========================================================================
   // GERAÇÃO 0: AVALIAÇÃO EXPLORATÓRIA
   // =========================================================================

   // Passo 1: Calcular a dispersão global e o w1 oficial da Geração 0
   dispersao_media_global = gas_mean( dispersao_media, par->n_obj );
   double proporcao_inicial = dispersao_media_global / disp_max;
   double w1 = CLAMP( 0.90 * proporcao_inicial + 0.10, 0.0, 1.0 );

   // Passo 2: Avaliar todo mundo com o w1 perfeitamente sincronizado
   for ( int k = 0; k < par->n_obj; k++ ) {
      for ( int i = 0; i < par->n_pop; i++ ) {
         pop[k][i].fitness = v_gas_fitness_coevolutivo( pop[k][i].x, NULL, img, w1, par->limiar, k );
      }
      qsort( pop[k], par->n_pop, sizeof( GasPopulacao ), gas_comparar_objetivo_max );

      memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof( double ) );
      elite[k].fitness = pop[k][par->n_pop - 1].fitness;
   }

   //--------------- FEEDBACK VISUAL (GERAÇÃO 0) ------------------------//
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
      fprintf( p_dispersao, "%d %.8f\n", geracao, dispersao_media_global );
   }
   //--------------------------------------------------------------------//

   // =========================================================================
   // LAÇO EVOLUTIVO CO-EVOLUTIVO (Equilíbrio de Nash)
   // =========================================================================
   do {
      geracao++;

      // ----------------------------------------------------------------------
      // ETAPA 1: DINÂMICA POPULACIONAL (Reprodução e Movimento)
      // ----------------------------------------------------------------------
      for ( int k = 0; k < par->n_obj; k++ ) {
         gas_torneio( pop[k], gen[k], lim[k].n_dim, par, gas_comparar_objetivo_max );
         gas_crossover_aritmetico( pop[k], gen[k], lim[k].n_dim, par );
         gas_mutacao_creep( pop[k], coef_disp[k], &lim[k], par );

         gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
         dispersao_media[k] = gas_mean( coef_disp[k], lim[k].n_dim );
      }

      // ----------------------------------------------------------------------
      // ETAPA 2: SINCRONIZAÇÃO TÉRMICA GLOBAL (O W1 Universal da Geração)
      // ----------------------------------------------------------------------
      // ========================================================================================
      // ⚠️ ALERTA PARA O EDINALDO DO FUTURO ⚠️
      // NUNCA tente calcular um 'w1' individual para cada objetivo (k) baseado apenas
      // na sua dispersão isolada! O peso 'w1' DEVE ser estritamente GLOBAL.
      //
      // Por que manter 'dispersao_media_global'?
      // O sucesso da Coevolução (Equilíbrio de Nash) exige que TODAS as sub-populações
      // obedeçam às mesmas "regras do jogo" simultaneamente. Se um objetivo usa w1 alto
      // (focado na imagem local) e outro usa w1 baixo (focado na geometria), o aspecto
      // colaborativo é penalizado. A geometria do gabarito quebra, a ancoragem deforma
      // e a eficácia fantástica (>99.8%) do GA despenca!
      // ========================================================================================
      dispersao_media_global = gas_mean( dispersao_media, par->n_obj );
      double proporcao = dispersao_media_global / disp_max;
      w1 = CLAMP( 0.90 * proporcao + 0.10, 0.0, 1.0 );

      // ----------------------------------------------------------------------
      // ETAPA 3: AVALIAÇÃO E EQUILÍBRIO DE NASH (Via Gauss-Seidel)
      // ----------------------------------------------------------------------
      for ( int k = 0; k < par->n_obj; k++ ) {

         // Avalia apenas os filhos recém-gerados
         for ( int i = 0; i < par->n_gen; i++ ) {
            pop[k][i].fitness = v_gas_fitness_coevolutivo( pop[k][i].x, elite, img, w1, par->limiar, k );
         }
         qsort( pop[k], par->n_pop, sizeof( GasPopulacao ), gas_comparar_objetivo_max );

         // Atualiza o Elite DESTE grupo (Gauss-Seidel)
         memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof( double ) );
         elite[k].fitness = pop[k][par->n_pop - 1].fitness;
      }

      //--------------- FEEDBACK VISUAL (FIM DA GERAÇÃO) ------------------------//
      if ( feedback_visual ) {
         for ( int k = 0; k < par->n_obj; k++ ) {
            fitness_elite[k] = elite[k].fitness;
            gas_gravar_pontos( pop[k], par->n_pop, geracao );
         }
         fprintf( p_fitness,   "%d %.8f\n", geracao, gas_mean( fitness_elite, par->n_obj ) );
         fprintf( p_dispersao, "%d %.8f\n", geracao, dispersao_media_global );
      }
      //-------------------------------------------------------------------------//

   } while ( dispersao_media_global > par->toleracia && geracao < par->max_geracoes );


   //--------------- FEEDBACK VISUAL (RESUMO FINAL) ------------------------//
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
   //------------------------------------------------------------------------//

   // =========================================================================
   // LIMPEZA DE RECURSOS DO PIPELINE
   // =========================================================================
   for ( int k = 0; k < par->n_obj; k++ ) {
      g_free( coef_disp[k] );
      gas_liberar_populacao( pop[k], par->n_pop );
      gas_liberar_genitores( gen[k], par->n_gen );
   }

   g_free( pop );
   g_free( gen );
   g_free( coef_disp );

   // Registra o número de gerações no parâmetro
   par->total_geracoes += geracao;

   // Retorna as âncoras limpas e seguras
   return elite;
}


