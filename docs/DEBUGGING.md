# Guia de Debugging - Mini-Projeto 1

Este guia ajuda a identificar e resolver os problemas mais comuns durante o desenvolvimento do quebra-senhas paralelo. Foque nos problemas de paralelização - o MD5 já está implementado.

## 1. Problemas de Compilação

### Erro: "undefined reference to `md5_string`"

**Causa**: Esqueceu de compilar/linkar hash_utils.c

**Solução**:
```bash
# ERRADO
gcc -o coordinator src/coordinator.c

# CORRETO
gcc -o coordinator src/coordinator.c src/hash_utils.c
# ou simplesmente
make coordinator
```

### Erro: "implicit declaration of function 'fork'"

**Causa**: Headers necessários não incluídos

**Solução**: Adicione os headers corretos
```c
#include <unistd.h>      // para fork(), execl()
#include <sys/types.h>   // para pid_t
#include <sys/wait.h>    // para wait(), waitpid()
```

### Warning: "format '%d' expects argument of type 'int'"

**Causa**: Tipos incompatíveis no printf

**Solução**:
```c
// Para long long
long long valor = 1000000;
printf("%lld\n", valor);  // Use %lld para long long

// Para size_t
size_t tamanho = strlen(str);
printf("%zu\n", tamanho);  // Use %zu para size_t
```

### Compilação com Todos os Warnings

```bash
# Compile sempre com -Wall para ver todos os warnings
gcc -Wall -g -o programa programa.c

# Para debugging mais detalhado
gcc -Wall -Wextra -g -O0 -o programa programa.c
```

## 2. Debugging de Processos

### Debugging de System Calls

Use prints de debug e verificações manuais:

```c
// No coordinator.c
pid_t pid = fork();
if (pid < 0) {
    perror("fork failed");
    printf("DEBUG: Fork falhou no worker %d\n", i);
} else if (pid == 0) {
    printf("DEBUG: Filho %d iniciando execl\n", i);
    execl("./worker", "worker", target_hash, start_pwd, end_pwd, charset, 
          password_len_str, worker_id_str, NULL);
    printf("DEBUG: Se chegou aqui, execl falhou!\n");
    exit(1);
} else {
    printf("DEBUG: Pai criou worker %d com PID %d\n", i, pid);
}
```

### Verificando Processos em Execução

```bash
# Ver todos os processos do usuário
ps aux | grep $USER

# Ver árvore de processos
pstree -p $$  # $$ é o PID do shell atual

# Monitorar processos em tempo real
watch -n 1 'ps aux | grep worker'

# Ver processos zumbi
ps aux | grep defunct
```

### Matando Processos Travados

```bash
# Matar um processo específico
kill PID

# Forçar término
kill -9 PID

# Matar todos os workers
pkill worker

# Matar toda a árvore de processos
kill -- -PGID  # PGID = Process Group ID
```

## 3. Debugging de Comunicação Entre Processos

### Problema: "Arquivo password_found.txt não é criado"

**Debugging**:
```bash
# Verificar permissões do diretório
ls -ld .

# Testar criação manual
touch test_file.txt
```

**Possíveis causas**:
1. Worker não encontrou a senha no intervalo
2. Erro no algoritmo de comparação
3. Permissões incorretas no diretório

**Teste isolado**:
```c
// Teste simples de criação do arquivo
int fd = open("test.txt", O_CREAT | O_EXCL | O_WRONLY, 0644);
if (fd < 0) {
    perror("open failed");
} else {
    write(fd, "teste\n", 6);
    close(fd);
}
```

### Problema: "Race Condition - Múltiplos workers escrevem"

**Sintoma**: Arquivo corrompido ou múltiplas linhas

**Solução**: Use O_EXCL para garantir exclusividade
```c
// Apenas UM processo conseguirá criar o arquivo
int fd = open(RESULT_FILE, O_CREAT | O_EXCL | O_WRONLY, 0644);
if (fd >= 0) {
    // Este processo ganhou a "corrida"
    write(fd, resultado, strlen(resultado));
    close(fd);
}
```

**Verificação**:
```bash
# Script para testar race condition
for i in {1..10}; do
    rm -f password_found.txt
    ./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4
    echo "Tentativa $i:"
    if [ -f password_found.txt ]; then
        lines=$(wc -l < password_found.txt)
        echo "Linhas no arquivo: $lines"
        cat password_found.txt
    fi
done
```

## 4. Debugging de Algoritmos

### Teste do Incremento de Senha

```c
// Adicione este código temporário no worker.c
void test_increment() {
    char password[4] = "aaa";
    char *charset = "abc";
    
    printf("Testando incremento:\n");
    for (int i = 0; i < 30; i++) {
        printf("%d: %s\n", i, password);
        if (!increment_password(password, charset, 3, 3)) {
            printf("Overflow!\n");
            break;
        }
    }
}
```

### Verificação do Hash MD5

```bash
# Testar hash de uma string conhecida
./test_hash abc
# Deve retornar: 900150983cd24fb0d6963f7d28e17f72

# Comparar com md5sum do sistema
echo -n "abc" | md5sum
# Deve dar o mesmo resultado
```

### Debug do Particionamento

```c
// No coordinator, adicione prints de debug
printf("DEBUG: Worker %d\n", i);
printf("  Início: %lld (%s)\n", start_index, start_password);
printf("  Fim: %lld (%s)\n", end_index, end_password);
printf("  Total: %lld senhas\n", end_index - start_index + 1);
```

## 5. Análise de Performance

### Medindo Tempo de Execução

```bash
# Tempo total
time ./coordinator "hash" 4 "0123456789" 4

# Tempo detalhado (real, user, sys)
/usr/bin/time -v ./coordinator "hash" 4 "0123456789" 4
```

### Testando Diferentes Configurações

```bash
# Script para testar speedup
for workers in 1 2 4 8; do
    echo "Testando com $workers workers:"
    time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" $workers
    echo ""
done
```

### Identificando Gargalos

- **Criação de processos**: Muitos workers pode ter overhead
- **Comunicação**: Apenas o primeiro worker que encontra escreve
- **Balanceamento**: Workers devem receber trabalho similar

## 6. Debugging com GDB

### Comandos Básicos

```bash
# Compilar com símbolos de debug
gcc -g -o coordinator src/coordinator.c src/hash_utils.c

# Iniciar GDB
gdb ./coordinator

# Dentro do GDB
(gdb) break main           # Breakpoint no main
(gdb) run "hash" 3 "abc" 2 # Executar com argumentos
(gdb) next                  # Próxima linha
(gdb) step                  # Entrar na função
(gdb) print variable        # Ver valor da variável
(gdb) continue             # Continuar execução
(gdb) quit                 # Sair
```

### Debugging de Fork com GDB

```bash
# No GDB, escolher seguir pai ou filho após fork
(gdb) set follow-fork-mode child  # Seguir o filho
(gdb) set follow-fork-mode parent # Seguir o pai (padrão)

# Ver todos os processos
(gdb) info inferiors

# Trocar entre processos
(gdb) inferior 2
```

### Debugging de Segmentation Fault

```bash
# Habilitar core dumps
ulimit -c unlimited

# Rodar programa
./coordinator "hash" 3 "abc" 4
# Segmentation fault (core dumped)

# Analisar core dump
gdb ./coordinator core
(gdb) backtrace  # Ver onde ocorreu o erro
(gdb) frame 0    # Ir para o frame do erro
(gdb) list       # Ver código
(gdb) print var  # Ver valores das variáveis
```

## 7. Problemas Comuns e Soluções

### "Worker não está encontrando senha que deveria existir"

**Checklist de debugging**:
1. Hash está correto?
   ```bash
   echo -n "abc" | md5sum
   ```

2. Charset contém todos os caracteres?
   ```c
   printf("Charset: '%s'\n", charset);
   ```

3. Intervalo do worker contém a senha?
   ```c
   printf("Worker %d: %s até %s\n", id, start, end);
   ```

4. Incremento está funcionando?
   ```c
   // Adicione um contador
   printf("Senhas verificadas: %lld\n", count);
   ```

### "Coordinator trava e não termina"

**Possíveis causas**:
1. Worker travou e não terminou
2. Wait está incorreto

**Debugging**:
```bash
# Ver se workers ainda estão rodando
ps aux | grep worker

# Ver estado dos processos
ps -eo pid,ppid,state,cmd | grep -E "coordinator|worker"

# Verificar estado dos processos
ps -eo pid,ppid,state,cmd | grep -E "coordinator|worker"
```

### "Muitos processos zumbi"

**Sintoma**:
```bash
ps aux | grep defunct
# user 12345 0.0 0.0 0 0 ? Z 10:30 0:00 [worker] <defunct>
```

**Solução**: Adicionar wait() apropriado
```c
// Limpar todos os zumbis
while (waitpid(-1, NULL, WNOHANG) > 0);
```

## 8. Script de Teste Automatizado

```bash
#!/bin/bash
# debug_test.sh

echo "=== Teste de Debugging do Quebra-Senhas ==="

# Limpar ambiente
rm -f password_found.txt trace.txt

# Compilar com debug
echo "Compilando..."
make clean
make CFLAGS="-Wall -g -DDEBUG" all

# Teste 1: Hash conhecido
echo -e "\n[Teste 1] Senha conhecida 'abc'"
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2
if [ -f password_found.txt ]; then
    echo "✓ Arquivo criado"
    cat password_found.txt
else
    echo "✗ Arquivo não criado"
fi

# Teste 2: Execução básica
echo -e "\n[Teste 2] Execução com senha numérica"
echo "Verificando execução:"
./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste 3: Verificar zumbis
echo -e "\n[Teste 3] Verificando processos zumbi"
./coordinator "hash_invalido" 2 "ab" 10 &
COORD_PID=$!
sleep 1
ZOMBIES=$(ps aux | grep defunct | wc -l)
kill $COORD_PID 2>/dev/null
wait $COORD_PID 2>/dev/null
echo "Processos zumbi encontrados: $ZOMBIES"

echo -e "\n=== Teste Completo ==="
```

## 9. Macros de Debug

Adicione ao código para facilitar debugging:

```c
// No início do arquivo
#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) \
        fprintf(stderr, "[DEBUG %s:%d] " fmt "\n", \
                __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...) do {} while(0)
#endif

// Uso no código
DEBUG_PRINT("Worker %d iniciado", worker_id);
DEBUG_PRINT("Verificando senha: %s", current_password);

// Compilar com debug
gcc -DDEBUG -o programa programa.c
```

## 10. Checklist Final de Debugging

Antes de considerar o projeto completo:

- [ ] Compila sem warnings com `-Wall`
- [ ] Não há processos zumbis após execução
- [ ] Arquivo `password_found.txt` é criado corretamente
- [ ] Apenas um worker escreve no arquivo (sem race condition)
- [ ] Todos os workers terminam apropriadamente
- [ ] Coordinator aguarda todos os workers
- [ ] Não há memory leaks (verificar com valgrind)
- [ ] Performance é proporcional ao número de workers
- [ ] Funciona com diferentes charsets e tamanhos
- [ ] Trata erros apropriadamente (fork, exec, open falhas)