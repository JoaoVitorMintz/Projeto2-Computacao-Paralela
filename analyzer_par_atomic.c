/*
Pelo que pesquisei, serve para permitir o uso de mais funcionalidades que não fazem parte da biblioteca
padrão do C, tive que colocar para o CLOCK_MONOTONIC ser definido.
*/
#define _POSIX_C_SOURCE 199309L 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "hash_table.h"

struct timespec inicio, fim;

void insert_in_ht(FILE *file, HashTable *ht) {
    char linha[100]; // Vetor para armazenar cada linha do arquivo manifest.txt

    while(fgets(linha, sizeof(linha), file)) {
        char URL[100]; // Pega o tipo de URL (/api, /favicon, /docs e etc...)

        // EXEMPLO DE LINHA: /css/style-490426ffd727.css
        if (sscanf(linha, "%99[^-]", URL) == 1) { // Regex para pegar String pré "-"
            ht_put(ht, URL);
        }
    }    
}

void extrair_urls(FILE *file, HashTable *ht) {
    char linha[100]; // Vetor para armazenar cada linha do arquivo manifest.txt

    while(fgets(linha, sizeof(linha), file)) {
        char URL[100]; // Pega o tipo de URL (/api, /favicon, /docs e etc...)
        char *inicio = strstr(linha, "GET "); // Função que localiza subsequência, localizando primeira ocorrência

        if (inicio != NULL) {
            inicio += 4; // pula o "GET "

            if (sscanf(inicio, "%99[^-]", URL) == 1) {
                CacheNode *node = ht_get(ht, URL);

                if (node != NULL) {
                    node->hit_count++;
                }
            }
        }

    }

}

int main() {
    HashTable* ht = ht_create(100000);

    FILE *file_manifest = fopen("manifest.txt", "r");
    FILE *file_distribuido = fopen("log_distribuido.txt", "r");
    FILE *file_concorrente = fopen("log_concorrente.txt", "r");

    if (file_manifest == NULL || file_distribuido == NULL || file_concorrente == NULL) {
        printf("ERRO: Um dos arquivos não pode ser aberto.");

        // Fechando os arquivos que foram abertos:
        if (file_manifest != NULL) fclose(file_manifest);
        if (file_distribuido != NULL) fclose(file_distribuido);
        if (file_concorrente != NULL) fclose(file_concorrente);

        // Destruindo HashTable que não foi usada
        ht_destroy(ht);
        return 1;
    }

    printf("Iniciando análise sequencial:\n");
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    
    // Insere URLs coletadas no manifest.txt para a *HashTable
    insert_in_ht(file_manifest, ht);

    // Lê cada URL coletada e faz o incremento dos valores coletados
    extrair_urls(file_distribuido, ht);
    extrair_urls(file_concorrente, ht);

    ht_save_results(ht, "results.csv"); // Salva o resultado da hashtable no arquivo 'results.csv'
    clock_gettime(CLOCK_MONOTONIC, &fim);

    double tempo = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    printf("Tempo: %.4f segundos\n", tempo);

    // Fechando os arquivos que foram abertos:
    fclose(file_manifest);
    fclose(file_distribuido);
    fclose(file_concorrente);

    ht_destroy(ht);

    return 0;
}
