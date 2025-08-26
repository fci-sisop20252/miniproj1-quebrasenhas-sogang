#include <stdio.h>
#include <string.h>
#include "hash_utils.h"

/**
 * Programa de teste para verificar a implementação MD5
 * 
 * Uso:
 *   ./test_hash           - Executa testes padrão
 *   ./test_hash "string"  - Calcula MD5 de uma string específica
 */

typedef struct {
    const char *input;
    const char *expected_hash;
} TestCase;

int main(int argc, char *argv[]) {
    // Se argumentos fornecidos, calcula hash da string
    if (argc > 1) {
        char hash[33];
        md5_string(argv[1], hash);
        printf("Input: %s\n", argv[1]);
        printf("MD5:   %s\n", hash);
        return 0;
    }
    
    // Casos de teste conhecidos
    TestCase tests[] = {
        {"", "d41d8cd98f00b204e9800998ecf8427e"},
        {"a", "0cc175b9c0f1b6a831c399e269772661"},
        {"abc", "900150983cd24fb0d6963f7d28e17f72"},
        {"message digest", "f96b697d7cb7938d525a2f31aaf161d0"},
        {"abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b"},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", 
         "d174ab98d277d9f5a5611c2c9f419d9f"},
        {"12345678901234567890123456789012345678901234567890123456789012345678901234567890",
         "57edf4a22be3c955ac49da2e2107b67a"},
        {"123", "202cb962ac59075b964b07152d234b70"},
        {"password", "5f4dcc3b5aa765d61d8327deb882cf99"},
        {"hello", "5d41402abc4b2a76b9719d911017c592"}
    };
    
    int num_tests = sizeof(tests) / sizeof(TestCase);
    int passed = 0;
    int failed = 0;
    
    printf("=== Teste da Implementação MD5 ===\n\n");
    
    for (int i = 0; i < num_tests; i++) {
        char hash[33];
        md5_string(tests[i].input, hash);
        
        printf("Teste %d:\n", i + 1);
        printf("  Input:    \"%s\"\n", tests[i].input);
        printf("  Esperado: %s\n", tests[i].expected_hash);
        printf("  Obtido:   %s\n", hash);
        
        if (strcmp(hash, tests[i].expected_hash) == 0) {
            printf("  Status:   ✓ PASSOU\n");
            passed++;
        } else {
            printf("  Status:   ✗ FALHOU\n");
            failed++;
        }
        printf("\n");
    }
    
    printf("=== Resumo dos Testes ===\n");
    printf("Total:   %d\n", num_tests);
    printf("Passou:  %d\n", passed);
    printf("Falhou:  %d\n", failed);
    
    if (failed == 0) {
        printf("\n✓ Todos os testes passaram! A implementação MD5 está correta.\n");
        return 0;
    } else {
        printf("\n✗ Alguns testes falharam. Verifique a implementação MD5.\n");
        return 1;
    }
}