#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO COORDENADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * 
 * Este programa coordena múltiplos workers para quebrar senhas MD5 em paralelo.
 * O MD5 JÁ ESTÁ IMPLEMENTADO - você deve focar na paralelização (fork/exec/wait).
 * 
 * Uso: ./coordinator <hash_md5> <tamanho> <charset> <num_workers>
 * 
 * Exemplo: ./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4
 * 
 * SEU TRABALHO: Implementar os TODOs marcados abaixo
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"

/**
 * Calcula o tamanho total do espaço de busca
 * 
 * @param charset_len Tamanho do conjunto de caracteres
 * @param password_len Comprimento da senha
 * @return Número total de combinações possíveis
 */
long long calculate_search_space(int charset_len, int password_len) {
    long long total = 1;
    for (int i = 0; i < password_len; i++) {
        total *= charset_len;
    }
    return total;
}

/**
 * Converte um índice numérico para uma senha
 * Usado para definir os limites de cada worker
 * 
 * @param index Índice numérico da senha
 * @param charset Conjunto de caracteres
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @param output Buffer para armazenar a senha gerada
 */
void index_to_password(long long index, const char *charset, int charset_len, 
                       int password_len, char *output) {
    for (int i = password_len - 1; i >= 0; i--) {
        output[i] = charset[index % charset_len];
        index /= charset_len;
    }
    output[password_len] = '\0';
}

/**
 * Função principal do coordenador
 */
int main(int argc, char *argv[]) {
    // TODO 1: Validar argumentos de entrada
    // Verificar se argc == 5 (programa + 4 argumentos)
    // Se não, imprimir mensagem de uso e sair com código 1
    
    // IMPLEMENTE AQUI: verificação de argc e mensagem de erro
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <hash_md5> <tamanho> <charset> <num_workers>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s \"900150983cd24fb0d6963f7d28e17f72\" 3 \"abc\" 4\n", argv[0]);
        return 1;
    }
    
    // Parsing dos argumentos (após validação)
    const char *target_hash = argv[1];
    int password_len = atoi(argv[2]);
    const char *charset = argv[3];
    int num_workers = atoi(argv[4]);
    int charset_len = strlen(charset);
    
    // TODO: Adicionar validações dos parâmetros
    // - password_len deve estar entre 1 e 10
    // - num_workers deve estar entre 1 e MAX_WORKERS
    // - charset não pode ser vazio
    
    if (password_len < 1 || password_len > 10) {
        fprintf(stderr, "Erro: Tamanho da senha deve estar entre 1 e 10\n");
        return 1;
    }
    
    if (num_workers < 1 || num_workers > MAX_WORKERS) {
        fprintf(stderr, "Erro: Número de workers deve estar entre 1 e %d\n", MAX_WORKERS);
        return 1;
    }
    
    if (charset_len == 0) {
        fprintf(stderr, "Erro: Charset não pode ser vazio\n");
        return 1;
    }
    
    printf("=== Mini-Projeto 1: Quebra de Senhas Paralelo ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Número de workers: %d\n", num_workers);
    
    // Calcular espaço de busca total
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaço de busca total: %lld combinações\n\n", total_space);
    
    // Remover arquivo de resultado anterior se existir
    unlink(RESULT_FILE);
    
    // Registrar tempo de início
    time_t start_time = time(NULL);
    
    // TODO 2: Dividir o espaço de busca entre os workers
    // Calcular quantas senhas cada worker deve verificar
    // DICA: Use divisão inteira e distribua o resto entre os primeiros workers
    
    // IMPLEMENTE AQUI:
    long long passwords_per_worker = total_space / num_workers;
    long long remaining = total_space % num_workers;
    
    // Arrays para armazenar PIDs dos workers
    pid_t workers[MAX_WORKERS];
    
    // TODO 3: Criar os processos workers usando fork()
    printf("Iniciando workers...\n");
    
    // IMPLEMENTE AQUI: Loop para criar workers
    for (int i = 0; i < num_workers; i++) {
        // TODO: Calcular intervalo de senhas para este worker
        long long start_index = i * passwords_per_worker;
        long long end_index = start_index + passwords_per_worker - 1;
        
        // Distribuir o resto entre os primeiros workers
        if (i < remaining) {
            start_index += i;
            end_index += i + 1;
        } else {
            start_index += remaining;
            end_index += remaining;
        }
        
        // TODO: Converter indices para senhas de inicio e fim
        char start_password[password_len + 1];
        char end_password[password_len + 1];
        
        index_to_password(start_index, charset, charset_len, password_len, start_password);
        index_to_password(end_index, charset, charset_len, password_len, end_password);
        
        // TODO 4: Usar fork() para criar processo filho
        pid_t pid = fork();
        
        if (pid == -1) {
            // TODO 7: Tratar erros de fork() e execl()
            perror("Erro ao criar processo filho");
            exit(1);
        }
        
        if (pid == 0) {
            // TODO 6: No processo filho: usar execl() para executar worker
            char worker_id[10];
            snprintf(worker_id, sizeof(worker_id), "%d", i);
            
            char start_idx_str[20];
            char end_idx_str[20];
            snprintf(start_idx_str, sizeof(start_idx_str), "%lld", start_index);
            snprintf(end_idx_str, sizeof(end_idx_str), "%lld", end_index);
            
            printf("Worker %d: verificando de %s a %s (%lld combinações)\n", 
                   i, start_password, end_password, end_index - start_index + 1);
            
            execl("./worker", "./worker", target_hash, charset, 
                  start_idx_str, end_idx_str, worker_id, NULL);
            
            // Se execl falhar
            perror("Erro ao executar worker");
            exit(1);
        } else {
            // TODO 5: No processo pai: armazenar PID
            workers[i] = pid;
            printf("Worker %d iniciado com PID %d\n", i, pid);
        }
    }
    
    printf("\nTodos os workers foram iniciados. Aguardando conclusão...\n");
    
    // TODO 8: Aguardar todos os workers terminarem usando wait()
    // IMPORTANTE: O pai deve aguardar TODOS os filhos para evitar zumbis
    
    // IMPLEMENTE AQUI:
    int workers_completed = 0;
    int workers_found_password = 0;
    
    while (workers_completed < num_workers) {
        int status;
        pid_t finished_pid = wait(&status);
        
        if (finished_pid == -1) {
            perror("Erro ao aguardar processo filho");
            break;
        }
        
        // Identificar qual worker terminou
        for (int i = 0; i < num_workers; i++) {
            if (workers[i] == finished_pid) {
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
                break;
            }
        }
    }
    
    // Registrar tempo de fim
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, start_time);
    
    printf("\n=== Resultado ===\n");
    
    // TODO 9: Verificar se algum worker encontrou a senha
    // Ler o arquivo password_found.txt se existir
    
    // IMPLEMENTE AQUI:
    FILE *result_file = fopen(RESULT_FILE, "r");
    if (result_file != NULL) {
        char line[256];
        if (fgets(line, sizeof(line), result_file)) {
            // Fazer parse do formato "worker_id:password"
            char *colon = strchr(line, ':');
            if (colon != NULL) {
                *colon = '\0';
                char *worker_id = line;
                char *found_password = colon + 1;
                
                // Remover nova linha se existir
                char *newline = strchr(found_password, '\n');
                if (newline != NULL) *newline = '\0';
                
                // Verificar o hash usando md5_string()
                char *calculated_hash = md5_string(found_password);
                if (calculated_hash != NULL && strcmp(calculated_hash, target_hash) == 0) {
                    printf("✅ SENHA ENCONTRADA pelo worker %s: %s\n", worker_id, found_password);
                    printf("Hash verificado: %s\n", calculated_hash);
                } else {
                    printf("❌ Falso positivo detectado: %s\n", found_password);
                }
                free(calculated_hash);
            }
        }
        fclose(result_file);
    } else if (workers_found_password > 0) {
        printf("❌ Workers relataram sucesso mas arquivo de resultado não encontrado\n");
    } else {
        printf("❌ Senha não encontrada no espaço de busca\n");
    }
    
    // Estatísticas finais (opcional)
    // TODO: Calcular e exibir estatísticas de performance
    printf("\n=== Estatísticas ===\n");
    printf("Tempo total de execução: %.2f segundos\n", elapsed_time);
    printf("Taxa média: %.2f combinações/segundo\n", total_space / elapsed_time);
    printf("Workers que completaram: %d/%d\n", workers_completed, num_workers);
    printf("Workers que encontraram a senha: %d\n", workers_found_password);
    
    return 0;
}
