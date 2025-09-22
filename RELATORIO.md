# Relatório: Mini-Projeto 1 - Quebra-Senhas Paralelo

**Aluno(s):** 
Rafael Lima 
RA: 10425819

Caio Henrique 
RA: 10425408

Lendy Naiara 
RA:

Guilherme Castro
RA: 10427775

---

## 1. Estratégia de Paralelização


**Como você dividiu o espaço de busca entre os workers?**

A divisão do espaço de busca foi feita de forma equidistante usando índices numéricos. Primeiro calculo o espaço total de combinações (charset_length^password_length) e divido igualmente entre os workers. O worker final recebe qualquer resto da divisão para garantir que todo o espaço seja coberto.

Algoritmo de divisão:

Calculo o espaço total de senhas possíveis

Divido pelo número de workers para obter a quantidade base por worker

Distribuo o resto da divisão para o último worker

Converto os índices inicial e final em senhas reais usando a função index_to_password

**Código relevante:** Cole aqui a parte do coordinator.c onde você calcula a divisão:
```c
long long passwords_per_worker = total_space / num_workers;
long long remaining = total_space % num_workers;

for (int i = 0; i < num_workers; i++) {
    long long start_index = i * passwords_per_worker;
    long long end_index = (i == num_workers - 1) ? 
                         (start_index + passwords_per_worker + remaining - 1) : 
                         (start_index + passwords_per_worker - 1);

    char start_password[password_len + 1];
    char end_password[password_len + 1];
    
    index_to_password(start_index, charset, charset_len, password_len, start_password);
    index_to_password(end_index, charset, charset_len, password_len, end_password);
}
```

---

## 2. Implementação das System Calls

**Descreva como você usou fork(), execl() e wait() no coordinator:**

o fork() foi utilizado para criar processos filhos idênticos ao processo pai. Em cada processo filho foi usado execl() para substituir a imagem do processo pelo programa worker, passando os argumentos necessários (hash alvo, senha inicial/final, charset, etc.). No processo pai, armazenamos os PIDs dos workers e usamos waitpid() em um loop para aguardar a conclusão de cada worker individualmente, capturando seus status de saída.

**Código do fork/exec:**
```c
printf("Iniciando workers...\n");

for (int i = 0; i < num_workers; i++) {
    long long start_index = i * passwords_per_worker;
    long long end_index = (i == num_workers - 1) ? 
                         (start_index + passwords_per_worker + remaining - 1) : 
                         (start_index + passwords_per_worker - 1);

    char start_password[password_len + 1];
    char end_password[password_len + 1];
    
    index_to_password(start_index, charset, charset_len, password_len, start_password);
    index_to_password(end_index, charset, charset_len, password_len, end_password);
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("Erro ao criar processo filho");
        exit(1);
    }
    
    if (pid == 0) {
        char worker_id[10];
        snprintf(worker_id, sizeof(worker_id), "%d", i);
        
        char start_idx_str[20];
        char end_idx_str[20];
        snprintf(start_idx_str, sizeof(start_idx_str), "%lld", start_index);
        snprintf(end_idx_str, sizeof(end_idx_str), "%lld", end_index);
        
        printf("Worker %d: verificando de %s a %s (%lld combinações)\n", 
               i, start_password, end_password, end_index - start_index + 1);
        
        execl("./worker", "./worker", target_hash, 
              start_password, end_password, charset, argv[2], worker_id, NULL);
        
        perror("Erro ao executar worker");
        exit(1);
    } else {
        workers[i] = pid;
        printf("Worker %d iniciado com PID %d\n", i, pid);
    }
}

printf("\nTodos os workers foram iniciados. Aguardando conclusão...\n");

int workers_completed = 0;
int workers_found_password = 0;
int status;
pid_t finished_pid;

for (int i = 0; i < num_workers; i++) {
    if (workers[i] > 0) {
        finished_pid = waitpid(workers[i], &status, 0);
    
        if (finished_pid == -1) {
            perror("Erro ao aguardar processo filho específico");
            continue;
        }
    
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code == 0) {
                printf("Worker %d terminou com sucesso\n", i);
            } else if (exit_code == 2) {
                printf("Worker %d encontrou a senha!\n", i);
                workers_found_password++;
            } else {
                printf("Worker %d terminou com código de erro %d\n", i, exit_code);
            }
        } else if (WIFSIGNALED(status)) {
            printf("Worker %d terminou devido ao sinal %d\n", i, WTERMSIG(status));
        }
        workers_completed++;
    }
}

while ((finished_pid = waitpid(-1, &status, 0)) > 0) {}

if (finished_pid == -1 && errno != ECHILD) {
    perror("Erro na limpeza final de processos");
}
```

---

## 3. Comunicação Entre Processos

**Como você garantiu que apenas um worker escrevesse o resultado?**
Implementamos uma escrita atômica usando a flag O_CREAT | O_EXCL na syscall open(). Quando um worker encontra a senha, ele tenta criar o arquivo de resultado com essas flags. O O_EXCL garante que a operação só terá sucesso se o arquivo não existir, criando uma seção crítica atômica. Se múltiplos workers encontrarem a senha simultaneamente, apenas o primeiro a chamar open() conseguirá criar o arquivo - os outros receberão erro porque o arquivo já existe. Isso evita condições de corrida onde vários processos poderiam sobrescrever o resultado ou corromper o arquivo.

**Como o coordinator consegue ler o resultado?**
O coordinator verifica a existência do arquivo password_found.txt após todos os workers terminarem. Se o arquivo existe, ele é aberto e lido linha por linha. O coordinator faz o parse da informação extraindo o ID do worker e a senha encontrada, separando pelo caractere ':'. Em seguida, verificamos se a senha está correta recalculando seu hash MD5 e comparando com o hash alvo, prevenindo falsos positivos.

---

## 4. Análise de Performance
Complete a tabela com tempos reais de execução:
Tabela com tempos reais de execução:

Teste	1 Worker	2 Workers	4 Workers	Speedup (4w)
Hash: 202cb962ac59075b964b07152d234b70
Charset: "0123456789"
Tamanho: 3
Senha: "123"	0.005s	0.002s	0.002s	2.5
Hash: 5d41402abc4b2a76b9719d911017c592
Charset: "abcdefghijklmnopqrstuvwxyz"
Tamanho: 5
Senha: "hello"	1.906s	2.161s	0.277s

**O speedup foi linear? Por quê?**
O speedup foi linear? Por quê?

Não foi linear. Com 2 workers ficou até mais lento que com 1 worker, mostrando que criar processos tem um custo. Cada processo novo precisa ser criado e gerenciado pelo sistema, o que gasta tempo. Só quando usamos mais workers é que o benefício do paralelismo compensa esse custo. Quanto menor o problema, mais esse custo atrapalha. Por isso o speedup não é proporcional.

---

## 5. Desafios e Aprendizados
**Qual foi o maior desafio técnico que você enfrentou?**
O maior desafio foi implementar corretamente a comunicação entre processos. Tivemos dificuldade com o algoritmo de incremento de senhas, que exigia tratar cada caractere como um dígito em base variável. Outro problema foi o gerenciamento de processos zombies, resolvido com uso adequado de waitpid(). A sincronização na escrita do arquivo de resultado também foi complexa, mas solucionada com operações atômicas usando O_EXCL. Estes desafios nos mostraram a importância do planejamento cuidadoso em programação paralela.

---

## Comandos de Teste Utilizados

```bash
# Teste básico
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Teste de performance
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste com senha maior
time ./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 4
```
---

**Checklist de Entrega:**
- [X] Código compila sem erros
- [X] Todos os TODOs foram implementados
- [X] Testes passam no `./tests/simple_test.sh`
- [X] Relatório preenchido
