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
GasPopulacao *v_gas_pipeline( ImagemCinza *img, const GasParametros *par,
                              const GasLimites *lim, gboolean feedback_visual );

#endif // V_GAS_H
