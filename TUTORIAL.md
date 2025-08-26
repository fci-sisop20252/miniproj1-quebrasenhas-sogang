# Guia para o Mini-Projeto 1: Quebra-Senhas Paralelo

Este tutorial vai guiar você através da implementação completa do mini-projeto de implementação de um quebra-senhas paralelo, utilizando os conceitos estudados sobre criação de processos.

## ⚠️ Aviso Importante - Uso Ético e Educacional

**Este projeto é exclusivamente educacional e tem como objetivo ensinar conceitos de sistemas operacionais, programação paralela e segurança computacional de forma responsável.**

- ✅ **Objetivo pedagógico**: Aprender fork(), exec(), wait() e paralelização
- ✅ **Conscientização sobre segurança**: Entender como senhas fracas são vulneráveis
- ❌ **NÃO use para atividades maliciosas**: Quebrar senhas reais é crime
- ❌ **NÃO teste em sistemas que não são seus**: Isso é invasão e é ilegal

**Lembre-se**: Com grandes poderes computacionais vêm grandes responsabilidades. Use este conhecimento para proteger sistemas, não para atacá-los.

## Contexto: Como Funcionam os Ataques de Quebra de Senhas

### O Problema da Segurança Digital

Na vida real, senhas não são armazenadas em texto plano nos sistemas. Em vez disso, elas passam por um processo chamado **hash criptográfico**:

```
Usuário digita: "abc123"
        ↓
Sistema calcula: MD5("abc123") = "e99a18c428cb38d5f260853678922e03"
        ↓  
Sistema armazena apenas o hash no banco de dados
```

**Por que isso é importante?**
- Se o banco de dados for comprometido, o atacante não vê as senhas diretamente
- Mesmo os administradores do sistema não conseguem ver suas senhas reais
- Para verificar login, o sistema calcula MD5(senha_digitada) e compara com o hash armazenado

### Como Funcionam os Ataques de Força Bruta

Quando um atacante obtém um hash MD5, ele precisa "reverter" o processo:

```
Hash encontrado: "900150983cd24fb0d6963f7d28e17f72"
        ↓
Atacante tenta: MD5("aaa") = "47bce5c74f589f4867dbd57e9ca9f808" ❌
Atacante tenta: MD5("aab") = "08c5433a60135c32e2962e7a04d70d6e" ❌  
Atacante tenta: MD5("abc") = "900150983cd24fb0d6963f7d28e17f72" ✅
        ↓
Senha descoberta: "abc"
```

**Características do ataque de força bruta:**
- **Computacionalmente intensivo**: Precisa testar milhões/bilhões de combinações
- **Totalmente paralelizável**: Cada tentativa é independente das outras
- **Limitado pela potência computacional**: Mais cores/threads = mais rápido
- **Vulnerabilidade de senhas fracas**: Senhas curtas/simples são quebradas rapidamente

## A Matemática Por Trás do Problema

Para um charset de tamanho `C` e senha de tamanho `L`:
- **Espaço total de busca**: C^L combinações
- **Exemplo**: charset="abc" (3 chars), senha tamanho 3 → 3³ = 27 combinações
- **Caso real**: charset="a-zA-Z0-9" (62 chars), senha tamanho 8 → 62⁸ = 218 trilhões de combinações

**Por que a paralelização é importante:**
- 1 core: 218 trilhões de tentativas sequenciais
- 4 cores: ~54 trilhões de tentativas por core (4x mais rápido)
- 16 cores: ~13 trilhões de tentativas por core (16x mais rápido)

## Visão Geral do Projeto

```
      ┌─────────────┐
      │ Coordinator │
      └──────┬──────┘
             │
   ┌─────────┴─────────┐
   │ fork() + exec()   ├─→ Worker 0 (busca aaa-czz)
   │ fork() + exec()   ├─→ Worker 1 (busca daa-fzz)
   │ fork() + exec()   ├─→ Worker 2 (busca gaa-izz)
   │ fork() + exec()   └─→ Worker 3 (busca jaa-lzz)
   └─────────┬─────────┘
             │
             ↓
    password_found.txt
```

## FASE 1: Compreensão do Projeto
### 1.1 O que é um Hash MD5?

**MD5 (Message Digest 5)** é uma função hash criptográfica que produz um valor de 128 bits (32 caracteres hexadecimais). Vamos entender suas características:

**Propriedades importantes do MD5:**
1. **Determinística**: A mesma entrada sempre produz a mesma saída
2. **Rápida**: Computacionalmente eficiente para calcular
3. **Aparentemente aleatória**: Pequenas mudanças na entrada causam grandes mudanças na saída
4. **Tamanho fixo**: Sempre produz exatamente 128 bits (32 caracteres hexadecimais)
5. **Unidirecional**: É computacionalmente inviável "reverter" o hash

**Exemplos de uso:**
```bash
# Compile o utilitário de teste (já implementado)
make test_hash

# Teste hashes conhecidos
./test_hash abc      # → 900150983cd24fb0d6963f7d28e17f72
./test_hash 123      # → 202cb962ac59075b964b07152d234b70  
./test_hash password # → 5f4dcc3b5aa765d61d8327deb882cf99

# Observe como pequenas mudanças causam hashes completamente diferentes:
./test_hash abc      # → 900150983cd24fb0d6963f7d28e17f72
./test_hash abd      # → 4911e516e5aa21d327512e0c8b197616
./test_hash ABC      # → 902fbdd2b1df0c4f70b4a5d23525e932
```

**Por que MD5 é usado em sistemas reais?**
- **Verificação de integridade**: Detectar se arquivos foram modificados
- **Armazenamento de senhas**: Não armazenar senhas em texto plano (embora MD5 seja considerado inseguro hoje)
- **Identificação única**: Gerar IDs únicos para dados
- **Distribuição de carga**: Hash para decidir qual servidor usar

**⚠️ Importante sobre Segurança:**
- MD5 é considerado **criptograficamente quebrado** desde 2005
- Sistemas modernos usam SHA-256, bcrypt, scrypt, ou Argon2
- Usamos MD5 aqui apenas por **simplicidade**
- Em sistemas reais, nunca use MD5 para senhas!

**Como usar a biblioteca fornecida:**
```c
#include "hash_utils.h"

char resultado[33];  // 32 chars + \0
md5_string("minha_senha", resultado);
printf("Hash: %s\n", resultado);
```

**⚠️ Implementação fornecida**: O MD5 já está completamente implementado em `hash_utils.c/h`! Você não precisa modificar estes arquivos.

### 1.2 O Problema do Quebra-Senhas (Força Bruta)

**Cenário**: Você é um pentester e obteve um hash MD5 de um sistema. Sua missão é descobrir a senha original para demonstrar a vulnerabilidade.

**O Desafio da Irreversibilidade:**
Como o MD5 é unidirecional, não podemos "calcular" a senha a partir do hash. A única opção é tentar todas as combinações possíveis até encontrar uma que produza o mesmo hash:

```
Hash alvo: "900150983cd24fb0d6963f7d28e17f72"

Tentativa 1: MD5("aaa") = "47bce5c74f589f4867dbd57e9ca9f808" ❌
Tentativa 2: MD5("aab") = "08c5433a60135c32e2962e7a04d70d6e" ❌
Tentativa 3: MD5("aac") = "2bb225f0ba9a58930757a868ed57d9a3" ❌
...
Tentativa 5: MD5("abc") = "900150983cd24fb0d6963f7d28e17f72" ✅ ENCONTROU!
```

**Parâmetros do Ataque:**
- **Hash alvo**: "900150983cd24fb0d6963f7d28e17f72" (que queremos quebrar)
- **Charset**: "abc" (conjunto de caracteres possíveis na senha)
- **Tamanho**: 3 (comprimento da senha)
- **Espaço de busca**: 3³ = 27 combinações possíveis

**Ordem de Verificação (lexicográfica):**
```
aaa → aab → aac → aba → abb → abc ← ENCONTROU!
abd → abe → aca → acb → acc → aca
baa → bab → bac → bba → bbb → bbc
bca → bcb → bcc → caa → cab → cac
cba → cbb → ccc
```

**Na Prática Real:**
- **Charset comum**: "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" (62 chars)
- **Senha de 6 caracteres**: 62⁶ = 56 bilhões de combinações
- **Senha de 8 caracteres**: 62⁸ = 218 trilhões de combinações
- **Tempo estimado**: Dias, semanas ou meses em um único computador

**Por isso a paralelização é essencial!**

### 1.3 Por que Paralelizar?

```
Sem paralelização (1 worker):
27 senhas ÷ 1 = 27 verificações sequenciais

Com paralelização (3 workers):
27 senhas ÷ 3 = 9 verificações por worker (em paralelo)
```

## Fase 2: Explorando o Código Base
### 2.1 Examine os arquivos fornecidos:

```bash
# Ver estrutura do projeto
ls -la src/

# Ler o coordinator.c com os TODOs
less src/coordinator.c

# Ler o worker.c com os TODOs
less src/worker.c
```

### 2.2 Entenda o fluxo:

1. **Coordinator** recebe: hash, tamanho, charset, num_workers
2. **Coordinator** divide o trabalho e cria workers usando fork/exec
3. **Workers** verificam suas senhas em paralelo
4. **Primeiro worker** que encontrar grava em password_found.txt
5. **Coordinator** aguarda todos com wait() e exibe resultado


## FASE 3: Implementação do Worker - Algoritmo de Busca
### 3.1 Comece pelo incremento de senha (TODO 1 no worker.c)

O incremento funciona como um contador:
```
aaa → aab → aac → aba → abb → abc → aca → ...
```

**Uma possível implementação do increment_password:**
```c
int increment_password(char *password, const char *charset, int charset_len, int password_len) {
    
    // Perceba que o password é passado por referência, ou seja, as alterações serão refletidas fora da função
    for (int i = password_len - 1; i >= 0; i--) {
        // Encontrar índice atual do caractere no charset
        int index = 0;
        while (index < charset_len && charset[index] != password[i]) {
        // Enquanto o indice for menor do que o tamanho do charset e o caractere atual não for igual ao caractere na senha
            index++; // Incrementa o índice para apontar para o próximo caractere do charset
        }
        
        // Um erro deve acontecer quando o caractere não está no charset
        if (index >= charset_len) return 0; 

        // Tenta incrementar
        if (index + 1 < charset_len) {
            password[i] = charset[index + 1];
            return 1;  // Sucesso! Encontramos o caracter do charset para aquela posição
        } else {
            password[i] = charset[0];  // Reset e vai pro próximo dígito
        }
    }
    // Percorreu todo o espaço de busca e não encontrou
    return 0;
}
```

### 3.2 Teste o incremento isoladamente

Adicione temporariamente no main do worker:
```c
// CÓDIGO DE TESTE - REMOVER DEPOIS
char test[4] = "aaa";
for (int i = 0; i < 10; i++) {
    printf("Senha %d: %s\n", i, test);
    increment_password(test, "abc", 3, 3);
}
return 0;  // Sair após teste
```

Compile e teste:
```bash
make worker
./worker teste teste teste abc 3 0
# Deve mostrar: aaa, aab, aac, aba, abb, ...
```

## FASE 4: Completando o Worker
### 4.1 Implemente a verificação de hash (TODOs 4 e 5)
No loop principal do worker:
```c
// TODO 4: Calcular hash
md5_string(current_password, computed_hash);

// TODO 5: Comparar com alvo
// Use a funcao strcmp para comparar strings
if (strcmp(computed_hash, target_hash) == 0) {
    printf("[Worker %d] SENHA ENCONTRADA: %s\n", worker_id, current_password);
    save_result(worker_id, current_password);
    break;
}
```

### 4.2 Implemente a gravação em arquivo (TODO 2)

```c
void save_result(int worker_id, const char *password) {
    int fd = open(RESULT_FILE, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd >= 0) {
        char buffer[256];
        int len = snprintf(buffer, sizeof(buffer), "%d:%s\n", worker_id, password);
        write(fd, buffer, len);
        close(fd);
        printf("[Worker %d] Resultado salvo!\n", worker_id);
    }
}
```

### 4.3 Teste o worker isoladamente

```bash
# Calcular hash de "abc"
echo -n "abc" | md5sum
# Resultado: 900150983cd24fb0d6963f7d28e17f72

# Testar worker diretamente
./worker "900150983cd24fb0d6963f7d28e17f72" "aaa" "acc" "abc" 3 0

# Verificar resultado
cat password_found.txt
# Deve mostrar: 0:abc
```

## FASE 5: Implementação do Coordinator
### 5.1 Testando Workers em Paralelo (Manual)

Antes de implementar o coordinator, teste workers manualmente:

```bash
# Terminal 1
./worker "900150983cd24fb0d6963f7d28e17f72" "aaa" "azz" "abc" 3 0 &

# Terminal 2
./worker "900150983cd24fb0d6963f7d28e17f72" "baa" "bzz" "abc" 3 1 &

# Terminal 3
./worker "900150983cd24fb0d6963f7d28e17f72" "caa" "czz" "abc" 3 2 &

# Aguardar e verificar
wait
cat password_found.txt
```

## FASE 6: Implementando fork(), exec() e wait() no Coordinator
### 6.1 Entenda o padrão fork()

```c
pid_t pid = fork();
if (pid < 0) {
    // ERRO
} else if (pid == 0) {
    // APENAS O FILHO EXECUTA AQUI
    exit(0);
} else {
    // APENAS O PAI EXECUTA AQUI
}
```

### 6.2 Implemente a criação de workers (TODOs 3-7 no coordinator.c)

**TODO 3-4: Criar processos com fork()**
- Use um loop para criar `num_workers` processos
- Para cada iteração, calcule o intervalo de senhas desse worker
- Chame `fork()` e armazene o PID retornado
- Lembre-se: fork() retorna 0 no filho, PID no pai, -1 em erro

**TODO 5: Tratamento no processo pai**
- Se `pid > 0`, você está no pai: armazene o PID no array `workers[i]`
- Imprima informações sobre o worker criado (ID, PID, intervalo)
- Continue o loop para criar o próximo worker

**TODO 6-7: Executar worker no processo filho**
- Se `pid == 0`, você está no filho
- Converta os argumentos numéricos para strings (use `sprintf`)
- Use `execl("./worker", "worker", ...)` com todos os 6 argumentos
- Se execl retornar, houve erro - trate com perror() e exit(1)

### 6.3 Implemente a espera pelos workers (TODO 8)

**Aguardando todos os workers terminarem:**
- Use um loop que executa `num_workers` vezes
- Chame `wait(&status)` para aguardar qualquer filho terminar
- Identifique qual worker terminou comparando o PID retornado com seu array
- Use `WIFEXITED(status)` para verificar se terminou normalmente
- Use `WEXITSTATUS(status)` para obter o código de saída

### 6.4 Implemente a leitura do resultado (TODO 9)

**Verificando se a senha foi encontrada:**
- Abra o arquivo `RESULT_FILE` com `open()` no modo O_RDONLY
- Se o arquivo existir (fd >= 0), leia seu conteúdo com `read()`
- Faça parse do formato "worker_id:password" usando `strchr()`
- Use `md5_string()` para verificar se a senha encontrada está correta
- Exiba o resultado para o usuário

## FASE 7: Testes e Análise
### 7.1 Realizar o primeiro teste

```bash
# Compilar tudo
make clean
make all

# Teste simples - senha "abc"
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Saída esperada:
# === Quebra de Senhas Paralela ===
# ...
# ✓ Senha encontrada!
# Senha: abc
```

### 7.2 Teste com charset numérico
```bash
# Hash de "123"
./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4
```

### 7.3 Teste com mais workers
```bash
# Mesmo teste, mais workers (avaliando o tempo)
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 2
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 8
```

### 7.4 Teste de stress
```bash
# Senha mais longa
./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 8
```

Finalize preenchendo o `RELATORIO_TEMPLATE.md` com suas respostas para as 5 questões principais:

1. **Estratégia de Paralelização**: Como você dividiu o espaço de busca entre os workers
2. **Implementação das System Calls**: Como usou fork(), execl() e wait() no coordinator
3. **Comunicação Entre Processos**: Como garantiu escrita atômica e fez parse do resultado
4. **Análise de Performance**: Preencha a tabela de tempos e calcule o speedup
5. **Desafios e Aprendizados**: Qual foi o maior desafio técnico que enfrentou

## Checklist Final

Antes de entregar, verifique:

- [ ] **Compilação**: `make clean && make all` funciona sem erros
- [ ] **Teste básico**: `./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4` encontra "abc"
- [ ] **Testes automatizados**: `./tests/simple_test.sh` passa
- [ ] **Comunicação**: Apenas um worker escreve no arquivo password_found.txt
- [ ] **Performance**: Tempo geralmente diminui com mais workers
- [ ] **TODOs implementados**: coordinator.c e worker.c completos
- [ ] **Relatório**: RELATORIO_TEMPLATE.md preenchido

## Problemas Comuns e Soluções Rápidas

### "undefined reference to md5_string"
```bash
make clean
make all  # Usa o Makefile que linka corretamente
```

### "Worker não encontra senha que existe"
- Verifique se o intervalo do worker contém a senha
- Adicione prints de debug no incremento
- Teste o worker isoladamente: `./worker "hash" "aaa" "azz" "abc" 3 0`

### "Coordinator trava e não termina"
- Verifique se todos os workers terminam
- Use `ps aux | grep worker` para ver workers ativos
- Certifique-se de que wait() está correto

### "Múltiplas linhas em password_found.txt"
- Verifique se está usando O_CREAT | O_EXCL na abertura
- Apenas um worker deve conseguir criar o arquivo

## Recursos Adicionais

- **`docs/SYSCALLS.md`** - Detalhes completos de fork/exec/wait
- **`docs/CONCEITOS_C.md`** - Conceitos de C para o projeto
- **`docs/DEBUGGING.md`** - Técnicas de debugging sem strace
- **`./tests/simple_test.sh`** - Script de teste local

## 🎯 Objetivos de Aprendizado

Ao completar este mini-projeto, você terá dominado:
- ✅ **Paralelização de processos** com fork/exec/wait
- ✅ **Comunicação entre processos** via arquivos
- ✅ **Sincronização** e coordenação de múltiplos workers

**Foco**: O MD5 é apenas o contexto - o importante é aprender paralelização!