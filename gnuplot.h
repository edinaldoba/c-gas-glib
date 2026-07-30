#ifndef GNUPLOT_H
#define GNUPLOT_H

#include "gas.h"

void gas_display_gnuplot( const GasLimites *lim, const int geracao );

void gas_gravar_pontos( const GasPopulacao *pop, const int n_pop, const int geracao );

void gas_display_terminal( const GasPopulacao *pop, const int n_dim, const double dispersao_media, const int geracao );

void v_gas_display_terminal( const GasPopulacao *elite, const double *dispersao_media, const int geracao );


#endif
