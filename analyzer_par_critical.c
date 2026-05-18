/*
 * analyzer_par_critical.c — Versão PARALELA com #pragma omp critical
 *
 * A ideia central desta versão é:
 *   1. Construir a tabela hash sequencialmente (igual ao sequencial).
 *   2. Carregar todas as linhas do log NA MEMÓRIA (ainda sequencial,
 *      porque arquivo não pode ser lido por várias threads ao mesmo tempo).
 *   3. Processar essas linhas EM PARALELO com OpenMP.
 *   4. Proteger o hit_count++ com #pragma omp critical — só uma thread
 *      por vez pode entrar nesse bloco, as outras esperam na fila.
 */

/* _GNU_SOURCE habilita extensões GNU: CLOCK_MONOTONIC, strdup, e outras */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>          /* Biblioteca do OpenMP — necessária para #pragma omp */
#include "hash_table.h"

/* Tamanho máximo de uma linha do log (IP + timestamp + URL + status + bytes) */
#define MAX_LINHA 512

/* ─────────────────────────────────────────────────────────────────────────────
 * insert_in_ht
 * Lê o manifest.txt e insere cada URL na hash table com hit_count = 0.
 * Igual ao sequencial — isso nunca é paralelizado (spec §4.2).
 * ───────────────────────────────────────────────────────────────────────────── */
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

/* ─────────────────────────────────────────────────────────────────────────────
 * processar_log_paralelo
 * Recebe um arquivo de log aberto e processa todas as linhas em paralelo.
 *
 * PASSO 1 — Carregar linhas em memória (sequencial):
 *   Não é possível paralelizar a leitura de arquivo. Então primeiro lemos
 *   tudo e guardamos cada linha em um array dinâmico de strings.
 *
 * PASSO 2 — Processar em paralelo (#pragma omp parallel for):
 *   Cada thread pega um subconjunto das linhas e trabalha de forma
 *   independente: encontra o "GET ", extrai a URL, busca na hash table.
 *   A busca (ht_get) é segura em paralelo porque a estrutura da tabela
 *   não muda mais — só o hit_count é alterado.
 *
 * PASSO 3 — #pragma omp critical:
 *   O hit_count++ é a única parte perigosa. Se duas threads incrementarem
 *   ao mesmo tempo, uma atualização pode se perder (race condition).
 *   O critical cria um "portão": só uma thread passa por vez.
 * ───────────────────────────────────────────────────────────────────────────── */
void processar_log_paralelo(FILE *file, HashTable *ht) {

    /* ── PASSO 1: Carregar todas as linhas do arquivo em memória ── */

    long capacidade = 1000000;   /* começa reservando espaço para 1 milhão de linhas */
    long total      = 0;

    /* Array de ponteiros — cada posição vai apontar para uma linha do log */
    char **linhas = malloc(capacidade * sizeof(char *));
    if (!linhas) {
        perror("Erro ao alocar array de linhas");
        return;
    }

    char buffer[MAX_LINHA];

    while (fgets(buffer, sizeof(buffer), file)) {

        /* Se o array encheu, dobra a capacidade (realloc) */
        if (total >= capacidade) {
            capacidade *= 2;
            linhas = realloc(linhas, capacidade * sizeof(char *));
            if (!linhas) {
                perror("Erro ao crescer array de linhas");
                return;
            }
        }

        /* strdup copia a string e aloca memória para ela */
        linhas[total++] = strdup(buffer);
    }

    /* ── PASSO 2 e 3: Processar em paralelo com critical ── */

    /*
     * #pragma omp parallel for divide o intervalo [0, total) entre as threads.
     * Cada thread executa o corpo do for para um subconjunto de índices.
     *
     * schedule(dynamic, 1000): distribui blocos de 1000 linhas dinamicamente.
     * Isso equilibra a carga porque algumas linhas têm URLs mais curtas/longas.
     */
    #pragma omp parallel for schedule(dynamic, 1000)
    for (long i = 0; i < total; i++) {

        char URL[MAX_LINHA];

        /*
         * Procura "GET " na linha. O log tem o formato:
         *   IP - - [timestamp] "GET /url HTTP/1.1" status bytes
         * strstr retorna um ponteiro para onde "GET " começa.
         */
        char *pos = strstr(linhas[i], "GET ");

        if (pos != NULL) {
            pos += 4; /* pula os 4 caracteres de "GET " */

            /*
             * %511[^ ] lê tudo até o próximo espaço (que separa a URL do "HTTP/1.1").
             * Isso garante que pegamos a URL completa.
             */
            if (sscanf(pos, "%511[^ ]", URL) == 1) {

                /*
                 * ht_get é seguro em paralelo: a estrutura da tabela hash
                 * (quais buckets existem, quais URLs estão lá) não muda mais
                 * depois do manifest. Múltiplas threads podem buscar ao mesmo
                 * tempo sem problema.
                 */
                CacheNode *node = ht_get(ht, URL);

                if (node != NULL) {
                    /*
                     * SEÇÃO CRÍTICA — o coração desta versão.
                     *
                     * Problema sem critical:
                     *   Thread A lê hit_count = 5
                     *   Thread B lê hit_count = 5  ← ao mesmo tempo!
                     *   Thread A escreve 6
                     *   Thread B escreve 6          ← deveria ser 7!
                     *   Resultado: perdemos um incremento. (race condition)
                     *
                     * Com #pragma omp critical:
                     *   Só UMA thread por vez entra neste bloco.
                     *   As outras ficam esperando na fila.
                     *   Garantia de corretude — mas sacrifica velocidade,
                     *   porque mesmo threads acessando URLs DIFERENTES
                     *   ficam bloqueadas umas pelas outras.
                     *   (Esse é o ponto fraco que o professor quer que você observe.)
                     */
                    #pragma omp critical
                    {
                        node->hit_count++;
                    }
                }
            }
        }

        /* Libera a memória da linha após processá-la */
        free(linhas[i]);
    }

    /* Libera o array de ponteiros */
    free(linhas);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * main
 * ───────────────────────────────────────────────────────────────────────────── */
int main(int argc, char **argv) {
    /* Cria a tabela hash com 100.000 buckets (mesmo tamanho do sequencial) */
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
           omp_get_max_threads());  /* mostra quantas threads o OpenMP vai usar */

    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    /* Fase 1: construção da tabela hash — SEMPRE sequencial */
    insert_in_ht(file_manifest, ht);

    /* Fase 2: processamento paralelo dos dois logs */
    processar_log_paralelo(file, ht);

    /* Fase 3: salvar resultados */
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
