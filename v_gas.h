#ifndef V_GAS_H
#define V_GAS_H

#include "gas.h"


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

GasPopulacao *v_gas_pipeline( const ImagemCinza *img,
                              const GasParametros *par,
                              const GasLimites *lim,
                              double (*v_gas_avaliar)(const double*, GasPopulacao*, const int, const ImagemCinza *img),
                              int (*gas_comparar)(const void* a, const void* b) );

#endif // V_GAS_H
