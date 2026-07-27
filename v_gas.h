#ifndef V_GAS_H
#define V_GAS_H

#include "gas.h"

// ============================================================================
// ESTRUTURAS DO ALGORITMO GENÉTICO MULTIOBJETIVO (COEVOLUÇÃO)
// ============================================================================

// O problema deve se adaptar aos gas e não o contrário.
// lim[0]->n_dim = lim[1]->n_dim = lim[2]->n_dim = lim[3]->n_dim
typedef struct {
    GasLimites *lim;
} VgasLimites;

typedef struct {
    GasPopulacao **pop;
    GasPopulacao  *elite;
} VgasPopulacao;

typedef struct {
    GasGenitores **gen;
} VgasGenitores;

// ============================================================================
// ESTRUTURAS DE VISÃO COMPUTACIONAL
// ============================================================================

typedef struct {
    char  key[5];    // Identificador do formato (ex: "P2", "P5")
    int   ncol;      // Dimensão X da imagem (Largura)
    int   nrow;      // Dimensão Y da imagem (Altura)
    int   max;       // Valor máximo de intensidade do pixel
    int **image;     // Matriz bidimensional de tons de cinza (alocada dinamicamente)
} ImagemCinza;

// ============================================================================
// ASSINATURAS DE FUNÇÕES
// ============================================================================

void liberar_matriz_pixels( int **matriz, int nrow );

void imread_gray( ImagemCinza *IMG, const char *arquivo );

GasPopulacao *v_gas_pipeline( GasParametros *par,
                              VgasLimites *v_lim,
                              double (*v_gas_avaliar)(const double*, GasPopulacao*, const int, ImagemCinza *img),
                              int (*gas_comparar)(const void* a, const void* b) );

#endif // V_GAS_H
