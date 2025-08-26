# System Calls para Gerenciamento de Processos - Mini-Projeto 1

Este documento detalha as system calls essenciais para o quebra-senhas paralelo: `fork()`, `exec()`, e `wait()`. 

**Foco**: Paralelização de processos. O MD5 já está implementado - concentre-se na coordenação dos workers.

## 1. fork() - Criação de Processos

### O que é fork()?

`fork()` cria uma cópia exata do processo atual. Após o fork:
- Existem DOIS processos idênticos rodando o mesmo código
- O processo original é chamado de **pai** (parent)
- O novo processo é chamado de **filho** (child)

### Como fork() funciona?

```c
#include <unistd.h>
#include <sys/types.h>

pid_t pid = fork();

if (pid < 0) {
    // ERRO - fork falhou
    perror("fork failed");
} else if (pid == 0) {
    // Este código roda APENAS no processo FILHO
    printf("Sou o filho! Meu PID: %d\n", getpid());
} else {
    // Este código roda APENAS no processo PAI
    printf("Sou o pai! Meu PID: %d, PID do filho: %d\n", getpid(), pid);
}
```

### Valores de Retorno do fork()

- **No processo pai**: retorna o PID do filho (número positivo)
- **No processo filho**: retorna 0
- **Em caso de erro**: retorna -1

### Exemplo Completo: Criando Múltiplos Processos

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int num_filhos = 3;
    pid_t pids[3];
    
    for (int i = 0; i < num_filhos; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            return 1;
        } else if (pid == 0) {
            // PROCESSO FILHO
            printf("Filho %d iniciado (PID: %d)\n", i, getpid());
            
            // Simular trabalho
            sleep(2 + i);  // Cada filho espera tempo diferente
            
            printf("Filho %d terminando\n", i);
            return i;  // Código de saída = número do filho
        } else {
            // PROCESSO PAI
            pids[i] = pid;
            printf("Pai criou filho %d com PID %d\n", i, pid);
        }
    }
    
    // Apenas o pai chega aqui
    printf("Pai aguardando todos os filhos...\n");
    
    // Aguardar todos os filhos
    for (int i = 0; i < num_filhos; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (WIFEXITED(status)) {
            printf("Filho com PID %d terminou com código %d\n", 
                   pids[i], WEXITSTATUS(status));
        }
    }
    
    return 0;
}
```

### Padrão para o Coordinator

```c
// No coordinator.c
for (int i = 0; i < num_workers; i++) {
    pid_t pid = fork();
    
    if (pid < 0) {
        // Erro crítico
        perror("Erro ao criar worker");
        exit(1);
    } else if (pid == 0) {
        // Processo filho - será substituído pelo worker
        // Próxima seção: execl()
        execl("./worker", "worker", /* argumentos */, NULL);
        
        // Se execl retornar, houve erro
        perror("Erro no execl");
        exit(1);
    } else {
        // Processo pai - armazenar PID para wait() posterior
        worker_pids[i] = pid;
    }
}
```

## 2. A familia exec() - Execução de Programas

### O que é exec()?

A família `exec()` substitui o processo atual por um novo programa. O processo mantém o mesmo PID, mas o código em execução é completamente substituído.

### Variantes da família exec()

Para este projeto, recomendo utilizar `execl()` (exec with list), que tem a seguinte sintaxe:

```c
int execl(const char *path, const char *arg0, ..., NULL);
```

- `path`: caminho do programa a executar
- `arg0`: nome do programa (convenção)
- `...`: argumentos do programa
- `NULL`: marca o fim dos argumentos

### Exemplo Básico de execl()

```c
// Substituir o processo atual pelo programa 'ls'
execl("/bin/ls", "ls", "-l", "-a", NULL);

// Se execl retornar, houve erro (o código abaixo só executa em caso de erro)
perror("execl failed");
exit(1);
```

### Padrão fork() + exec()

Este é o padrão mais comum em Unix/Linux:

```c
pid_t pid = fork();

if (pid == 0) {
    // Processo filho
    execl("./meu_programa", "meu_programa", "arg1", "arg2", NULL);
    
    // Só chega aqui se execl falhar
    perror("execl failed");
    exit(1);
} else if (pid > 0) {
    // Processo pai continua normalmente
    printf("Filho %d está executando meu_programa\n", pid);
}
```

### Outras Variantes úteis

```c
// execv - argumentos como array
char *args[] = {"ls", "-l", NULL};
execv("/bin/ls", args);

// execlp - busca no PATH
execlp("gcc", "gcc", "programa.c", "-o", "programa", NULL);

// execvp - array + PATH
char *args[] = {"python3", "script.py", NULL};
execvp("python3", args);
```

## 3. wait() - Sincronização de Processos

### O que é wait()?

`wait()` e `waitpid()` permitem que um processo pai aguarde seus filhos terminarem.

### wait() Básico

```c
#include <sys/wait.h>

int status;
pid_t child_pid = wait(&status);

if (child_pid > 0) {
    printf("Filho %d terminou\n", child_pid);
    
    // Analisar o status de saída
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        printf("Código de saída: %d\n", exit_code);
    }
}
```

### waitpid() - Aguardar Processo Específico

```c
pid_t child_pid = fork();
if (child_pid == 0) {
    // Código do filho
    sleep(5);
    return 42;  // Código de saída
}

// Pai aguarda filho específico
int status;
waitpid(child_pid, &status, 0);

if (WIFEXITED(status)) {
    printf("Filho retornou: %d\n", WEXITSTATUS(status));  // Imprime 42
}
```

### Aguardar Todos os Filhos

```c
// Método 1: wait() em loop
int num_children = 5;
for (int i = 0; i < num_children; i++) {
    int status;
    pid_t pid = wait(&status);
    if (pid > 0) {
        printf("Filho %d terminou\n", pid);
    }
}

// Método 2: waitpid() com -1
while (waitpid(-1, &status, 0) > 0) {
    // -1 significa "qualquer filho"
    if (WIFEXITED(status)) {
        printf("Um filho terminou com código %d\n", WEXITSTATUS(status));
    }
}
```

### Análise do Status

```c
int status;
pid_t pid = wait(&status);

if (WIFEXITED(status)) {
    // Processo terminou normalmente
    int exit_code = WEXITSTATUS(status);
    printf("Exit code: %d\n", exit_code);
} else if (WIFSIGNALED(status)) {
    // Processo foi terminado por um sinal
    int signal = WTERMSIG(status);
    printf("Terminado pelo sinal: %d\n", signal);
} else if (WIFSTOPPED(status)) {
    // Processo foi parado (não terminado)
    int signal = WSTOPSIG(status);
    printf("Parado pelo sinal: %d\n", signal);
}
```

## Fluxo de Processos no Mini-Projeto

```
Coordinator (processo principal)
    |
    ├── fork() → Worker 0 (busca intervalo aaa-bzz)
    │             └── execl("./worker", hash, "aaa", "bzz", charset, ...)
    │
    ├── fork() → Worker 1 (busca intervalo caa-dzz)  
    │             └── execl("./worker", hash, "caa", "dzz", charset, ...)
    │
    ├── fork() → Worker 2 (busca intervalo eaa-fzz)
    │             └── execl("./worker", hash, "eaa", "fzz", charset, ...)
    │
    └── wait() para todos os workers
        └── Ler password_found.txt e exibir resultado
```

**Cada worker**: Usa MD5 fornecido + implementa busca no seu intervalo

## Erros Comuns e Soluções

### 1. Zombie Processes

**Problema**: Filhos terminam mas o pai não faz wait()
```c
// ERRADO - cria zumbis
for (int i = 0; i < 10; i++) {
    if (fork() == 0) {
        exit(0);  // Filho termina
    }
}
sleep(100);  // Pai não faz wait - filhos viram zumbis
```

**Solução**: Sempre fazer wait()
```c
// CORRETO
for (int i = 0; i < 10; i++) {
    if (fork() == 0) {
        exit(0);
    }
}
while (wait(NULL) > 0);  // Aguardar todos
```

### 2. Fork Bomb

**Problema**: Fork dentro de loop sem controle
```c
// PERIGO! Não execute isto!
while (1) {
    fork();  // Crescimento exponencial de processos
}
```

**Solução**: Sempre controlar quem faz fork
```c
// CORRETO
for (int i = 0; i < NUM_WORKERS; i++) {
    if (fork() == 0) {
        // Filho faz seu trabalho e SAI
        do_work();
        exit(0);  // Importante!
    }
}
```

### 3. Exec Não Substitui o Processo

**Problema**: Esquecer que exec falhou
```c
// INCOMPLETO
execl("./programa", "programa", NULL);
printf("Continuando...\n");  // Só executa se execl falhar!
```

**Solução**: Tratar falha do exec
```c
// CORRETO
execl("./programa", "programa", NULL);
perror("execl failed");
exit(1);  // Sair em caso de erro
```
## Referências Úteis

- `man 2 fork` - Manual do fork()
- `man 3 exec` - Manual da família exec()
- `man 2 wait` - Manual do wait()
- `man 2 waitpid` - Manual do waitpid()