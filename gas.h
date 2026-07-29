#ifndef GAS_H
#define GAS_H

#include <glib.h>


typedef struct {
   int n_pop, n_gen, n_tor, n_obj;
   double p_rec, p_mut, peso_disp, toleracia;
   GRand *rand; // <- Ponteiro para o gerador de números aleatórios
} GasParametros;

typedef struct {
   double *ini, *fim;
   int n_dim;
} GasLimites;

typedef struct {
   double *x, fitness;
} GasPopulacao;

typedef struct {
   double *x;
} GasGenitores;



double gas_max( const double *array, const int tam );
double gas_mean( const double *array, const int tam );

#define GAS_NUM_SEMENTES 4
void gas_gerar_sementes( guint32 *sementes );

GasPopulacao *gas_alocar_populacao( const int n_pop, const int n_dim );
GasGenitores *gas_alocar_genitores( const int n_gen, const int n_dim );

void gas_liberar_populacao( GasPopulacao *pop, const int n_pop );
void gas_liberar_genitores( GasGenitores *gen, const int n_gen );

void gas_populacao_inicial( GasPopulacao *pop, const GasParametros *par, const GasLimites *lim );

void gas_projetar_pca( const GasPopulacao *pop, GasPopulacao *pop_2d, int n_pop, int n_dim );

void gas_torneio( const GasPopulacao *pop, GasGenitores *gen, const int n_dim, const GasParametros *par,
                  int( gas_comparar )( const void* a, const void* b ) );
void gas_crossover_aritmetico( GasPopulacao *pop, const GasGenitores *gen, const int n_dim, const GasParametros *par );
void gas_mutacao_direcional( GasPopulacao *pop, const double *coef_disp, const int n_dim, const GasParametros *par );
void gas_mutacao_creep( GasPopulacao *pop, const double *coef_disp, const GasLimites *lim, const GasParametros *par );
void gas_coeficiente_dispersao( const GasPopulacao *pop, double *coef_disp, const GasParametros *par, const int n_dim );

int gas_comparar_objetivo_max( const void* a, const void* b );
int gas_comparar_objetivo_min( const void* a, const void* b );

double F5(  const double *x, const int n_dim );
double F6(  const double *x, const int n_dim );
double F10( const double *x, const int n_dim );
double F11( const double *x, const int n_dim );
double F13( const double *x, const int n_dim );

void gas_display_gnuplot( const GasLimites *lim, const int geracao );
void gas_gravar_pontos( const GasPopulacao *pop, const int n_pop, const int geracao );
void gas_display_terminal( const GasPopulacao *pop, const int n_dim, const double dispersao_max, const int geracao );

GasPopulacao *gas_pipeline( const GasParametros *par, const GasLimites *lim, gboolean feedback_visual,
                            double( gas_avaliar )( const double*, const int ),
                            int( gas_comparar )( const void* a, const void* b ) );




#endif
