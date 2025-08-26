# Arquivos do Mini-Projeto 1

Este diretório contém os arquivos fonte do quebra-senhas paralelo.

## Arquivos para os Estudantes

- **`coordinator.c`** - Template do processo coordenador com TODOs para implementar
- **`worker.c`** - Template do processo trabalhador com TODOs para implementar
- **`hash_utils.c`** - Biblioteca MD5 FORNECIDA (näo alterar)
- **`hash_utils.h`** - Header da biblioteca MD5 (não alterar)
- **`test_hash.c`** - Programa para testar a biblioteca MD5

## Como Usar

Os estudantes devem implementar os TODOs nos arquivos `coordinator.c` e `worker.c`:

```bash
# Compilar (vai ter warnings até implementarem os TODOs)
make all

# Testar (não vai funcionar até implementarem)
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2
```

## TODOs Principais

### coordinator.c
1. **TODO 1**: Validação de argumentos
2. **TODO 2**: Divisão do espaço de busca entre workers
3. **TODO 3-7**: Loop de criação de workers com fork() e execl()
4. **TODO 8**: Loop de wait() para aguardar workers
5. **TODO 9**: Leitura e parse do arquivo de resultado

### worker.c
1. **TODO 1**: Algoritmo de incremento de senha (increment_password)
2. **TODO 2**: Gravação atômica do resultado (save_result)
3. **TODO 3-6**: Loop principal de busca com MD5 e verificações

## Biblioteca MD5 Fornecida

A biblioteca MD5 já está implementada em `hash_utils.c`. Os estudantes devem usar:

```c
char hash[33];
md5_string("senha", hash);  // Calcula MD5 de "senha"
```

## Teste Manual do Worker

O worker pode ser testado individualmente:

```bash
./worker "900150983cd24fb0d6963f7d28e17f72" "aaa" "abc" "abc" "3" "0"
```

Parâmetros:
- Hash MD5 alvo
- Senha inicial do intervalo  
- Senha final do intervalo
- Charset
- Tamanho da senha
- ID do worker