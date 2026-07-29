# Executáveis finais
TARGET_GAS   = gas
TARGET_VGAS  = v_gas

# Compilador e Flags
CC      = gcc
CFLAGS  = -Wall -Wextra -O3 -fopenmp $(shell pkg-config --cflags glib-2.0)
LIBS    = $(shell pkg-config --libs glib-2.0) -llapacke -llapack -lblas -lm

# Fontes comuns compartilhados por ambos os projetos
COMMON_SRCS = gas.c funcoes.c matriz.c
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

# Fontes específicos de cada versão
GAS_SRCS  = main.c $(COMMON_SRCS)
GAS_OBJS  = $(GAS_SRCS:.c=.o)

VGAS_SRCS = v_main.c v_gas.c $(COMMON_SRCS)
VGAS_OBJS = $(VGAS_SRCS:.c=.o)

# Regra padrão: compila os dois executáveis
all: build_dir $(TARGET_GAS) $(TARGET_VGAS)

# Garantia da pasta gnuplot
build_dir:
	@mkdir -p gnuplot

# Linkagem do GA Padrão
$(TARGET_GAS): $(GAS_OBJS)
	$(CC) $(GAS_OBJS) -o $@ $(CFLAGS) $(LIBS)
	@echo "----------------------------------------"
	@echo "GA Padrão compilado: ./$(TARGET_GAS)"
	@echo "----------------------------------------"

# Linkagem do GA Co-evolutivo
$(TARGET_VGAS): $(VGAS_OBJS)
	$(CC) $(VGAS_OBJS) -o $@ $(CFLAGS) $(LIBS)
	@echo "----------------------------------------"
	@echo "GA Co-evolutivo compilado: ./$(TARGET_VGAS)"
	@echo "----------------------------------------"

# Compilação genérica dos objetos (.c -> .o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Executa o GA Padrão e gera os gráficos
run: $(TARGET_GAS)
	@rm -f gnuplot/*
	./$(TARGET_GAS)
	@cd gnuplot && gnuplot -persist plotPontos.txt 2>/dev/null || true
	@cd gnuplot && gnuplot -persist plotEvolucao.txt 2>/dev/null || true
	@cd gnuplot && gnuplot -persist plotDispersao.txt 2>/dev/null || true

# Executa o GA Co-evolutivo e gera os gráficos
vrun: $(TARGET_VGAS)
	@rm -f gnuplot/*
	./$(TARGET_VGAS)
	@cd gnuplot && gnuplot -persist plotPontos.txt 2>/dev/null || true
	@cd gnuplot && gnuplot -persist plotEvolucao.txt 2>/dev/null || true
	@cd gnuplot && gnuplot -persist plotDispersao.txt 2>/dev/null || true

# Limpeza completa de todos os objetos e executáveis
clean:
	rm -f *.o $(TARGET_GAS) $(TARGET_VGAS) gnuplot/*
	@echo "Limpeza concluída."

.PHONY: all run vrun clean build_dir
