# 🧬 Algoritmos Genéticos de Alta Performance em C

![C](https://img.shields.io/badge/Linguagem-C%20Puro-blue.svg)
![GLib](https://img.shields.io/badge/Dependência-GLib%202.0-green.svg)
![Gnuplot](https://img.shields.io/badge/Visualização-Gnuplot-red.svg)

Um *pipeline* robusto, modular e altamente otimizado para otimização estocástica usando Algoritmos Genéticos (GAs). Desenvolvido em C puro, o projeto foca em máxima performance e gerenciamento seguro de memória com a biblioteca GLib.

## 🚀 Principais Características

* **Arquitetura Modular:** Configuração baseada em ponteiros de função e estruturas dinâmicas, permitindo a troca instantânea de funções de avaliação (benchmark) e parâmetros evolutivos.
* **Segurança e Estabilidade:** Gerenciamento de recursos rigoroso utilizando as macros e tipagens da GLib (`g_autoptr`, `g_new0`, `g_free`), garantindo execução contínua sem vazamentos de memória (*memory leaks*).
* **Visualização em Tempo Real:** Integração direta com o **Gnuplot** para plotagem automática da evolução do *fitness*, decaimento da dispersão e animação 2D da população a cada geração.

## 🔬 Operadores Matemáticos e Heurísticas Inéditas

Além dos operadores clássicos da literatura (Seleção por Torneio, Crossover Aritmético), este repositório traz implementações exclusivas que garantem altíssima taxa de convergência (acima de 99,95%) em funções de teste complexas:

* **Modelagem Matemático-Evolutiva:**
  * **Teoria dos Jogos (Equilíbrio de Nash):** Coevolução de subpopulações concorrentes implementada em C/GLib.
  * **Convergência Geométrica (Área de Gauss):** Aplicação da *Shoelace Formula* como operador de fitness para preservação proporcional do quadrilátero.
  
  $$A = \frac{1}{2} \left\vert{} \sum_{i=0}^{n-1} (x_i y_{i+1} - x_{i+1} y_i) \right\vert{}$$
  
* **Visualização Dinâmica N-D via PCA (LAPACKE/CBLAS):**
  * Pipeline nativo de Análise de Componentes Principais (PCA) construído sobre rotinas de álgebra linear de alto desempenho (cblas_dgemm, cblas_dgemv e LAPACKE_dsyevd).
  * Mapeamento estocástico $N\text{D} \to 2\text{D}$ em tempo real via decomposição em autovalores/autovetores da matriz de covariância amostral centralizada:
  
  $$\mathbf{C} = \frac{1}{N-1} (\mathbf{X} - \bar{\mathbf{X}})^T (\mathbf{X} - \bar{\mathbf{X}})$$
  
  * Permite projetar a hiper-nuvem de indivíduos na direção das duas maiores componentes de variância sem impacto perceptível no tempo de CPU, viabilizando o diagnóstico visual contínuo no Gnuplot para espaços de busca de alta dimensionalidade.
  
* **Coeficiente de Dispersão Dinâmico:** Uma métrica estatística autoral que mensura o espalhamento da população no espaço de busca em tempo real.
* **Mutação Direcional Sensível à Dispersão:** Um operador de mutação inédito que calibra o tamanho do passo (*step size*) estocástico com base no Coeficiente de Dispersão, acelerando a exploração inicial e refinando a convergência final.
* **Mutação Creep Adaptativa:** Um operador clássico que faz uso do Coeficiente de Dispersão para ajustar a magnitude da perturbação gerada de forma dinâmica, respeitando rigorosamente a distância para as fronteiras do espaço de busca.

## 📊 Funções de Teste (Benchmarks) Incluídas

O sistema já vem configurado com as seguintes funções de teste clássicas para algoritmos de otimização contínua:
* **F5:** Função de De Jong V (Shekel's Foxholes) - 2D
* **F6:** Função de Schaffer - 2D
* **F10:** Função de Rastrigin - 5D
* **F11:** Função de Schwefel - 2D
* **F13:** Função de Shubert - 2D

## 🛠️ Dependências e Compilação

Para compilar e executar o projeto, você precisará das seguintes ferramentas instaladas no seu ambiente Linux:
* `gcc` (com suporte a OpenMP)
* `libglib2.0-dev`
* `gnuplot`

**Exemplo de compilação via terminal:**
```bash
make run
make vrun
