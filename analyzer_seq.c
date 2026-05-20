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

// Versão caso seja necessário pegar a linha toda:
void insert_in_ht(FILE *file, HashTable *ht) {
    char linha[100]; // Vetor para armazenar cada linha do arquivo manifest.txt

    while(fgets(linha, sizeof(linha), file)) {
        char URL[100]; // Pega o tipo de URL (/api, /favicon, /docs e etc...)

        // EXEMPLO DE LINHA: /css/style-490426ffd727.css
        // TODO: Verificar se deveríamos pegar apenas "/css/style" ou tudo 
        if (sscanf(linha, "%199s", URL) == 1) { // Regex para pegar String pré "-"
            ht_put(ht, URL);
        }
    }    
}

// Versão caso seja necessário pegar a linha toda:
void extrair_urls(FILE *file, HashTable *ht) {
    char linha[100]; // Vetor para armazenar cada linha do arquivo log

    while(fgets(linha, sizeof(linha), file)) {
        char URL[100]; // Pega o tipo de URL (/api, /favicon, /docs e etc...)
        char *inicio = strstr(linha, "GET "); // Função que localiza subsequência, localizando primeira ocorrência

        if (inicio != NULL) {
            inicio += 4; // pula o "GET "

            // Verificar se deveríamos pegar apenas "/css/style" ou tudo (/api/data-250ab524602a.api/data)
            if (sscanf(inicio, "%99[^ ]", URL) == 1) { // ^ significa ler qualquer caracter exceto espaço
                /*
                Chamo ht_get, ela me dá um ponteiro (CacheNode *) para um nó (ou NULL).
                Guardo esse ponteiro em node para poder usar para node->hit_count++.
                */
                CacheNode *node = ht_get(ht, URL);


                if (node != NULL) {
                    node->hit_count++;
                }
            }
        }

    }
}

int main(int argc, char **argv) {
    HashTable* ht = ht_create(100000);

    // Verifica quantos argv existem (deve haver apenas 2, o numero de threads e o arquivo)
    if (argc < 2) {
        printf("Uso: %s <arquivo>\n", argv[0]);
        return 1;
    }

    FILE *file_manifest = fopen("manifest.txt", "r");
    FILE *file = fopen(argv[1], "r");

    if (file_manifest == NULL || file == NULL) {
        printf("ERRO: Um dos arquivos não pode ser aberto.");

        // Fechando os arquivos que foram abertos:
        if (file_manifest != NULL) fclose(file_manifest);
        if (file != NULL) fclose(file);

        // Destruindo HashTable que não foi usada
        ht_destroy(ht);
        return 1;
    }

    printf("Iniciando análise sequencial:\n");
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    
    // Insere URLs coletadas no manifest.txt para a *HashTable
    insert_in_ht(file_manifest, ht);

    // Lê cada URL coletada e faz o incremento dos valores coletados
    extrair_urls(file, ht);

    ht_save_results(ht, "results.csv"); // Salva o resultado da hashtable no arquivo 'results.csv'
    clock_gettime(CLOCK_MONOTONIC, &fim);

    double tempo = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    printf("Tempo: %.4f segundos\n", tempo);

    // Fechando os arquivos que foram abertos:
    fclose(file_manifest);
    fclose(file);

    ht_destroy(ht);

    return 0;
}

/*
REFERÊNCIAS:
https://www.ibm.com/docs/pt-br/i/7.6.0?topic=functions-strstr-locate-substring
*/
