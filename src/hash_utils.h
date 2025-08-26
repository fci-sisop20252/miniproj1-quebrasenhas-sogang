#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MD5_DIGEST_LENGTH 16

/**
 * Computa o hash MD5 de uma string de entrada
 * 
 * @param input String de entrada para calcular o hash
 * @param output Buffer de saída com pelo menos 33 bytes (32 caracteres hex + '\0')
 * 
 * Exemplo de uso:
 *   char hash[33];
 *   md5_string("senha123", hash);
 *   printf("MD5: %s\n", hash);  // Imprime: 482c811da5d5b4bc6d497ffa98491e38
 */
void md5_string(const char *input, char output[33]);

#endif // HASH_UTILS_H