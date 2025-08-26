# Conceitos de Programação em C - Mini-Projeto 1

Este documento cobre os conceitos essenciais de C necessários para implementar o quebra-senhas paralelo. O foco está na paralelização de processos, não no MD5 (que já está implementado).

## 1. Argumentos de Linha de Comando (argc e argv)

Todo programa C pode receber argumentos quando é executado. Estes são passados através da função `main`:

```c
int main(int argc, char *argv[]) {
    // argc = número de argumentos (incluindo o nome do programa)
    // argv = array de strings com os argumentos
}
```

### Estrutura do argv

- `argv[0]` - sempre contém o nome do programa
- `argv[1]` - primeiro argumento do usuário
- `argv[2]` - segundo argumento do usuário
- E assim por diante...

### Exemplo Prático - Coordinator

```c
// Execução: ./coordinator "hash123" 3 "abc" 4

int main(int argc, char *argv[]) {
    // argc será 5
    // argv[0] = "./coordinator"
    // argv[1] = "hash123"       (hash alvo)
    // argv[2] = "3"              (tamanho da senha)
    // argv[3] = "abc"            (charset)
    // argv[4] = "4"              (número de workers)
    
    // Validação básica
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <hash> <tamanho> <charset> <workers>\n", argv[0]);
        return 1;
    }
    
    // Conversão de strings para números
    int password_len = atoi(argv[2]);  // Converte "3" para 3
    int num_workers = atoi(argv[4]);   // Converte "4" para 4
}
```

### Validação de Argumentos

Sempre valide os argumentos antes de usá-los:

```c
// Verificar número de argumentos
if (argc < 3) {
    fprintf(stderr, "Erro: argumentos insuficientes\n");
    return 1;
}

// Validar conversões numéricas
int valor = atoi(argv[1]);
if (valor <= 0) {
    fprintf(stderr, "Erro: valor deve ser positivo\n");
    return 1;
}

// Validar strings não vazias
if (strlen(argv[2]) == 0) {
    fprintf(stderr, "Erro: string não pode ser vazia\n");
    return 1;
}
```

## 2. Operações com Arquivos usando System Calls

No projeto, usamos system calls (open, read, write, close) em vez das funções da biblioteca padrão (fopen, fread, etc).

### Abrindo Arquivos

```c
#include <fcntl.h>
#include <unistd.h>

// Abrir para leitura
int fd = open("arquivo.txt", O_RDONLY);
if (fd < 0) {
    perror("Erro ao abrir arquivo");
    return 1;
}

// Abrir para escrita (cria se não existir)
int fd = open("saida.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
// 0644 = permissões (rw-r--r--)
```

### Escrita Atômica (Importante para o Worker)

Para garantir que apenas um worker grave o resultado:

```c
// O_CREAT | O_EXCL falha se o arquivo já existir
int fd = open("password_found.txt", O_CREAT | O_EXCL | O_WRONLY, 0644);
if (fd >= 0) {
    // Este processo foi o primeiro a criar o arquivo!
    char buffer[100];
    int len = snprintf(buffer, sizeof(buffer), "%d:%s\n", worker_id, password);
    write(fd, buffer, len);
    close(fd);
} else {
    // Arquivo já existe - outro worker encontrou primeiro
    printf("Outro worker já encontrou a senha\n");
}
```

### Leitura de Arquivos

```c
char buffer[256];
ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
if (bytes_read > 0) {
    buffer[bytes_read] = '\0';  // Adicionar terminador
    printf("Conteúdo: %s\n", buffer);
}
close(fd);
```

### Verificar se Arquivo Existe

```c
#include <unistd.h>

if (access("arquivo.txt", F_OK) == 0) {
    printf("Arquivo existe\n");
} else {
    printf("Arquivo não existe\n");
}
```

## 3. Manipulação de Strings

### Comparação de Strings

```c
#include <string.h>

char *hash1 = "abc123";
char *hash2 = "abc123";

// strcmp retorna 0 se as strings são iguais
if (strcmp(hash1, hash2) == 0) {
    printf("Hashes são iguais!\n");
}

// Comparação lexicográfica
int result = strcmp("abc", "abd");
// result < 0 porque "abc" vem antes de "abd"
```

### Cópia de Strings

```c
char origem[] = "hello";
char destino[10];

// Copiar string
strcpy(destino, origem);

// Copiar com limite de tamanho (mais seguro)
strncpy(destino, origem, sizeof(destino) - 1);
destino[sizeof(destino) - 1] = '\0';  // Garantir terminação
```

### Encontrar Caractere em String

```c
char charset[] = "abcdef";
char letra = 'd';

// strchr retorna ponteiro para a primeira ocorrência
char *posicao = strchr(charset, letra);
if (posicao != NULL) {
    int indice = posicao - charset;  // Calcula o índice (3 neste caso)
    printf("'%c' está na posição %d\n", letra, indice);
}
```

### Conversão String para Número

```c
char *str_numero = "123";
int numero = atoi(str_numero);  // numero = 123

// Para validação mais robusta
char *endptr;
long valor = strtol(str_numero, &endptr, 10);
if (*endptr != '\0') {
    printf("Conversão falhou - caracteres inválidos\n");
}
```

## 4. Geração de Senhas - Algoritmo de Incremento

O worker precisa gerar todas as senhas em seu intervalo. Isso é feito incrementando a senha como se fosse um número em uma base especial:

```c
// Exemplo: charset = "abc", tamanho = 3
// Sequência: aaa, aab, aac, aba, abb, abc, aca, ...

int increment_password(char *password, const char *charset, int charset_len, int password_len) {
    // Começar do último caractere (mais à direita)
    for (int i = password_len - 1; i >= 0; i--) {
        // Encontrar posição atual do caractere no charset
        char *pos = strchr(charset, password[i]);
        if (pos == NULL) return 0;  // Erro: caractere inválido
        
        int index = pos - charset;
        index++;  // Próximo caractere
        
        if (index < charset_len) {
            // Ainda há caracteres disponíveis
            password[i] = charset[index];
            return 1;  // Sucesso
        } else {
            // Overflow - voltar ao primeiro e continuar
            password[i] = charset[0];
            // Loop continua para incrementar a próxima posição
        }
    }
    return 0;  // Overflow total
}
```

### Exemplo de Uso do Incremento

```c
char senha[4] = "aaa";  // 3 caracteres + '\0'
char *charset = "abc";
int charset_len = 3;
int password_len = 3;

// Gerar primeiras 10 senhas
for (int i = 0; i < 10; i++) {
    printf("Senha %d: %s\n", i, senha);
    if (!increment_password(senha, charset, charset_len, password_len)) {
        printf("Fim do espaço de busca\n");
        break;
    }
}
// Saída: aaa, aab, aac, aba, abb, abc, aca, acb, acc, baa
```

## 5. Formatação de Saída

### printf vs fprintf

```c
// printf escreve na saída padrão (stdout)
printf("Mensagem normal\n");

// fprintf permite escolher onde escrever
fprintf(stdout, "Para a tela\n");           // Equivale a printf
fprintf(stderr, "Erro: algo deu errado\n"); // Para erros
```

### snprintf - Formatação Segura para Buffers

```c
char buffer[100];
int worker_id = 3;
char *password = "abc123";

// snprintf evita overflow de buffer
int len = snprintf(buffer, sizeof(buffer), 
                   "Worker %d encontrou: %s", worker_id, password);

// len contém o número de caracteres escritos (sem contar '\0')
```

## 6. Comunicação entre Processos via Arquivo

O coordinator e os workers comunicam-se através do arquivo `password_found.txt`:

### Formato do Arquivo

```
worker_id:password
```

Exemplo: `2:abc123` significa que o worker 2 encontrou a senha "abc123"

### Parsing do Resultado (no Coordinator)

```c
char buffer[256];
// Assumindo que buffer contém "2:abc123\n"

// Encontrar o ':' 
char *colon = strchr(buffer, ':');
if (colon != NULL) {
    *colon = '\0';  // Substitui ':' por '\0' para separar
    
    int worker_id = atoi(buffer);      // "2"
    char *password = colon + 1;        // "abc123\n"
    
    // Remover newline se houver
    char *newline = strchr(password, '\n');
    if (newline) *newline = '\0';
    
    printf("Worker %d encontrou: %s\n", worker_id, password);
}
```

## 7. Usando a Biblioteca MD5 Fornecida

Você **não precisa implementar MD5** - apenas usar a biblioteca fornecida:

```c
#include "hash_utils.h"

char hash[33];  // 32 caracteres + '\0'
md5_string("senha123", hash);
printf("Hash: %s\n", hash);  // Imprime o hash MD5
```

A biblioteca está completa e testada. Foque na paralelização!

## Dicas Importantes

- **Sempre inicialize strings**: Use `memset` ou atribuição direta
- **Cuidado com buffer overflow**: Use `strncpy` em vez de `strcpy`
- **Verifique retornos de funções**: Especialmente `open`, `read`, `write`
- **Libere recursos**: Sempre faça `close(fd)` após usar um arquivo
- **Use `perror`**: Para mensagens de erro descritivas