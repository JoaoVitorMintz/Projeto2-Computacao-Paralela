// _GNU_SOURCE habilita extensões GNU: CLOCK_MONOTONIC, strdup, e outras
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "hash_table.h"

// Tamanho máximo de uma linha do log (IP + timestamp + URL + status + bytes)
#define MAX_LINHA 512

void insert_in_ht(FILE *file, HashTable *ht) {
    char linha[MAX_LINHA];

    while (fgets(linha, sizeof(linha), file)) {
        char URL[MAX_LINHA];

        /* sscanf com "%s" pega a palavra até o primeiro espaço/newline */
        if (sscanf(linha, "%511s", URL) == 1) {
            ht_put(ht, URL);
        }
    }
}

void processar_log_paralelo(FILE *file, HashTable *ht) {
    //PASSO 1: Carregar todas as linhas do arquivo em memória
    long capacidade = 1000000;   // começa reservando espaço para 1 milhão de linhas
    long total      = 0;

    // Array de ponteiros — cada posição vai apontar para uma linha do log
    char **linhas = malloc(capacidade * sizeof(char *));
    if (!linhas) {
        perror("Erro ao alocar array de linhas");
        return;
    }

    char buffer[MAX_LINHA];

    while (fgets(buffer, sizeof(buffer), file)) {

        // Se o array encheu, dobra a capacidade (realloc)
        if (total >= capacidade) {
            capacidade *= 2;
            linhas = realloc(linhas, capacidade * sizeof(char *));
            if (!linhas) {
                perror("Erro ao crescer array de linhas");
                return;
            }
        }

        // strdup copia a string e aloca memória para ela
        linhas[total++] = strdup(buffer);
    }
    #pragma omp parallel for schedule(dynamic, 1000)
    for (long i = 0; i < total; i++) {

        char URL[MAX_LINHA];
        char *pos = strstr(linhas[i], "GET ");

        if (pos != NULL) {
            pos += 4; // pula os 4 caracteres de "GET "
            if (sscanf(pos, "%511[^ ]", URL) == 1) {

                CacheNode *node = ht_get(ht, URL);

                if (node != NULL) {
                    // Definição da seção crítica para evitar que duas threads alterem, simultâneamente, o hit_count
                    #pragma omp critical
                    {
                        node->hit_count++;
                    }
                }
            }
        }

        // Libera a memória da linha após processá-la
        free(linhas[i]);
    }

    // Libera o array de ponteiros
    free(linhas);
}

int main(int argc, char **argv) {
    // Cria a tabela hash com 100.000 buckets (mesmo tamanho do sequencial)
    HashTable *ht = ht_create(100000);

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

    printf("Iniciando analise paralela (critical) com %d thread(s)...\n",
           omp_get_max_threads());  // mostra quantas threads o OpenMP vai usar

    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    // Fase 1: construção da tabela hash — SEMPRE sequencial
    insert_in_ht(file_manifest, ht);

    // Fase 2: processamento paralelo dos dois logs
    processar_log_paralelo(file, ht);

    // Fase 3: salvar resultados
    ht_save_results(ht, "results.csv");

    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo = (fim.tv_sec - inicio.tv_sec) +
                   (fim.tv_nsec - inicio.tv_nsec) / 1e9;

    printf("Tempo total: %.4f segundos\n", tempo);

    // Fechando os arquivos que foram abertos:
    fclose(file_manifest);
    fclose(file);
    ht_destroy(ht);


    return 0;
}
