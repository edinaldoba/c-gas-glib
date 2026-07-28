#!/bin/bash

# 1. Interrompe o script imediatamente se qualquer comando falhar
set -e

# 2. Garante que o diretório 'gnuplot' existe (se não existir, ele cria)
mkdir -p gnuplot

# 3. Limpa com segurança apenas os arquivos da pasta gnuplot.
# O '-f' ignora arquivos inexistentes e evita mensagens de erro desnecessárias!
rm -f gnuplot/*

# Nome do executável final
OUTPUT="v_gas"

echo "Compilando v_main.c, v_gas.c e gas.c..."

# Compilação incluindo as flags da GLib 2.0 e OpenMP
gcc -Wall -Wextra -fopenmp v_main.c v_gas.c gas.c -lm -o "$OUTPUT" $(pkg-config --cflags --libs glib-2.0)

echo "Compilação concluída com sucesso!"
echo "Executando o programa..."
echo "----------------------------------------"

# Executa o programa compilado
./"$OUTPUT"

# Entra na pasta gnuplot e plota os gráficos
cd gnuplot
    gnuplot -persist plotPontos.txt
    gnuplot -persist plotEvolucao.txt
    gnuplot -persist plotDispersao.txt
cd ..
