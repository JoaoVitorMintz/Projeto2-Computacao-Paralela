#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "hash_table.h"

#define N 1000

struct timespec inicio, fim;

// Lê o manifest.txt e insere cada URL na hash table
void insert_in_ht(FILE *file, HashTable *ht) {
    char linha[N];
    while (fgets(linha, sizeof(linha), file)) {
        char URL[200];
        if (sscanf(linha, "%199s", URL) == 1) {
            ht_put(ht, URL);
        }
    }
}

// Lê o log, extrai as URLs e incrementa o contador de cada uma usando lock por bucket
void extrair_urls(FILE *file, HashTable *ht, omp_lock_t *locks) {
    char linha[N];
    char **linhas = (char **)malloc(sizeof(char *) * 12000000);
    int total = 0;

    // Carrega todas as linhas na memória primeiro (não dá pra paralelizar a leitura)
    while (fgets(linha, sizeof(linha), file)) {
        linhas[total] = strdup(linha);
        total++;
    }

    // Processamento paralelo
    #pragma omp parallel for
    for (int t = 0; t < total; t++) {
        char URL[200];
        char *pos = strstr(linhas[t], "GET ");

        if (pos != NULL) {
            pos += 4; 

            if (sscanf(pos, "%199[^ ]", URL) == 1) {

                // Calcula o bucket da URL mema função hash do hash_table.c
                // Precisamos saber qual bucket pra pegar o lock certo
                unsigned long hash = 5381;
                const char *s = URL;
                int c;
                while ((c = *s++)) {
                    hash = ((hash << 5) + hash) + c;
                }
                int bucket = hash % ht->size;

                // Pega o lock só do bucket desa URL
                // Assim as threads em buckets diferentes não si bloqueiam
                omp_set_lock(&locks[bucket]);

                CacheNode *node = ht_get(ht, URL);
                if (node != NULL) {
                    node->hit_count++;
                }

                omp_unset_lock(&locks[bucket]);
            }
        }

        free(linhas[t]);
    }

    free(linhas);
}

int main(int argc, char **argv) {
    HashTable *ht = ht_create(100000);

    // Verifica quantos argv existem 
    if (argc < 2) {
        printf("Uso: %s <arquivo>\n", argv[0]);
        return 1;
    }

    FILE *file_manifest = fopen("manifest.txt", "r");
    FILE *file = fopen(argv[1], "r");

    if (file_manifest == NULL || file == NULL) {
        printf("ERRO: Um dos arquivos não pode ser aberto.");
        if (file_manifest != NULL) fclose(file_manifest);
        if (file != NULL) fclose(file);
        ht_destroy(ht);
        return 1;
    }

    // Cria um lock pra cada bucket da tabela hash
    omp_lock_t *locks = malloc(sizeof(omp_lock_t) * ht->size);
    for (int i = 0; i < ht->size; i++) {
        omp_init_lock(&locks[i]);
    }

    printf("Iniciando análise paralela (lock por bucket):\n");

    clock_gettime(CLOCK_MONOTONIC, &inicio);

    insert_in_ht(file_manifest, ht);

    
    extrair_urls(file, ht, locks);

    ht_save_results(ht, "results.csv"); 

    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    printf("Tempo: %.4f segundos\n", tempo);

    // Destroi os locks
    for (int i = 0; i < ht->size; i++) {
        omp_destroy_lock(&locks[i]);
    }
    free(locks);

    // Fechando os arquivos que foram abertos:
    fclose(file_manifest);
    fclose(file);
    ht_destroy(ht);

    return 0;
}

