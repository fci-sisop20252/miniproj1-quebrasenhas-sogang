# Makefile para o Mini-Projeto 1: Quebra-Senhas Paralelo
# Sistemas Operacionais - 2025

CC = gcc
CFLAGS = -Wall -g
SRCDIR = src
BINARIES = coordinator worker test_hash

# Alvos principais
all: coordinator worker test_hash

# Quebra-senhas paralelo - Componentes para implementar
coordinator: $(SRCDIR)/coordinator.c $(SRCDIR)/hash_utils.c $(SRCDIR)/hash_utils.h
	$(CC) $(CFLAGS) -o coordinator $(SRCDIR)/coordinator.c $(SRCDIR)/hash_utils.c

worker: $(SRCDIR)/worker.c $(SRCDIR)/hash_utils.c $(SRCDIR)/hash_utils.h
	$(CC) $(CFLAGS) -o worker $(SRCDIR)/worker.c $(SRCDIR)/hash_utils.c

# Hash MD5 - Utilitário fornecido (pronto)
test_hash: $(SRCDIR)/test_hash.c $(SRCDIR)/hash_utils.c $(SRCDIR)/hash_utils.h
	$(CC) $(CFLAGS) -o test_hash $(SRCDIR)/test_hash.c $(SRCDIR)/hash_utils.c

# Teste rápido do projeto
test: all
	@echo "=== Teste Rápido do Mini-Projeto ==="
	@echo "Testando hash MD5:"
	@./test_hash abc
	@echo ""
	@echo "Executando quebra-senhas (senha 'abc'):"
	@./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Limpeza
clean:
	rm -f $(BINARIES)
	rm -f password_found.txt
	rm -f *.o

# Ajuda
help:
	@echo "Makefile para o Mini-Projeto 1: Quebra-Senhas Paralelo"
	@echo ""
	@echo "Alvos disponíveis:"
	@echo "  all         - Compila coordinator, worker e test_hash"
	@echo "  coordinator - Compila o processo coordenador (TODO: implementar)"
	@echo "  worker      - Compila o processo trabalhador (TODO: implementar)"
	@echo "  test_hash   - Compila o utilitário de teste MD5 (fornecido)"
	@echo "  test        - Executa teste rápido do projeto"
	@echo "  clean       - Remove todos os binários e arquivos temporários"
	@echo "  help        - Mostra esta mensagem de ajuda"
	@echo ""
	@echo "Exemplo de uso:"
	@echo "  make all                     # Compila todos os componentes"
	@echo "  make test                    # Testa o projeto rapidamente"
	@echo "  make clean                   # Limpa os binários"
	@echo ""
	@echo "Para testes completos, execute: ./tests/simple_test.sh"

.PHONY: all clean help test