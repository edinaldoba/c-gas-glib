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
GasLimites *v_gas_limites( const int nrow, const int ncol, const int n_obj );

GasPopulacao *v_gas_pipeline( const ImagemCinza *img, GasParametros *par,
                              const GasLimites *lim, const gboolean feedback_visual );

#endif // V_GAS_H
