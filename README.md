# Projeto 2 - Computação Paralela

## 👥 Alunos
 - João Vitor Garcia Aguiar Mintz
 - Mateus Kage Moya
 - Giovanni Barreiro Garitano de Castro
 - Yan Andreotti dos Santos

## ✅ Objetivos do projeto:

## ⚙️ Como rodar:

Dentro do termial no ambiente Linux/UNIX:

```terminal
make
./analyzer_seq <arquivo>
./analyzer_par_atomic <arquivo>
./analyzer_par_atomic_padded <arquivo>
./analyzer_par_critical <arquivo>
./analyzer_par_lock <arquivo>
```

Sendo que o **arquivo** é **log_concorrente.txt** e **log_distribuido.txt**. Após rodar o código para um determinado arquivo, realize esse comando dentro do termial no ambiente Linux/UNIX:

```terminal
sort results.csv > results_<log_realizado>_sorted.csv
diff -s results_<log_realizado>_sorted.csv gabarito_<log_realizado>.csv
```

Este comando realiza a ordenação do arquivo.csv, importante para comparar com o **gabarito_concorrente.csv** ou **gabarito_distribuido.csv**. Lembre-se que: cada arquivo gerado ao rodar o script deve ser ordenado e definido se é concorrente ou distribuido.

## 📝 Explicação geral da lógica do código:

### Analisador Sequencial:
Inicialmente, é criado a Hashtable para que ela possa receber os dados posteriormente. Com o sucesso no processo de abertura dos arquivos e validado os parâmetros para a execução, a versão sequencial entra na função `insert_in_ht` que irá coletar as URLs do arquivo `manifest.txt` e a insere na Hashtable com valor de `hit_count` inicial igual à 0.

Após a criação da Hashtable com os dados já carregados, inicia-se a função `extrair_urls` que inicia alocando um veotr para armazenar acada linha do arquivo log, em seguida, é realizado um parsing da linha. Realiza-se inicialmente o `strstr(linha, "GET ")` que é uma função para determinar a localização de uma determinada subsequência, no caso: "GET ". Em seguida, verifica se o ponto de inicio é diferente de NULL (isto é, não começa após o espaço), sendo necessário, portanto, pular 4 posições do vetor para iniciar exatamente do ponto que queremos coletar para comparar com os dados da Hashtable. Após isso, é utilizado um regex **"%99[^ ]"** para pegar todos os 99 ou menos caracteres até o próximo **" "**. Após isso, será gerado uma variável *node do tipo CacheNode que buscará a Hashtable de valor idêntico ao da linha e, caso seja encontrado, irá incrementar o `hit_count`.

Após todo esse processo, é calculado o tempo, o arquivo é fechado e a hashtable destruiída.

### Analisador Paralelo Crítico:
A versão paralela com `#pragma omp critical` segue a mesma ideia da versão sequencial na etapa de construção da tabela hash, inserindo inicialmente as URLs do arquivo de manifesto na estrutura de dados. Depois disso, o arquivo de log é lido de forma sequencial e todas as linhas são carregadas para a memória, permitindo que o processamento posterior seja distribuído entre várias threads.

Após o carregamento das linhas, o programa utiliza `#pragma omp parallel for` para dividir o trabalho entre as threads. Cada thread percorre um subconjunto das linhas, procura a ocorrência de `"GET "`, extrai a URL acessada e realiza a busca dessa URL na tabela hash. Como a estrutura da tabela não é modificada depois de sua construção, as operações de busca podem ocorrer em paralelo sem comprometer a integridade dos dados.

O ponto crítico dessa abordagem está na atualização do campo `hit_count` de cada nó encontrado. Como várias threads podem tentar incrementar esse contador ao mesmo tempo, o código utiliza `#pragma omp critical` para transformar essa região em uma seção crítica, garantindo que apenas uma thread por vez execute o incremento. Isso evita condições de corrida (*race conditions*) e assegura a corretude do resultado.

Apesar de correta, essa estratégia pode limitar o desempenho. Como a diretiva `critical` impõe exclusão mútua global para o trecho protegido, até mesmo threads que acessam URLs diferentes precisam esperar umas pelas outras para atualizar seus respectivos contadores. Assim, a solução preserva a consistência dos dados, mas pode reduzir o ganho de paralelismo em comparação com abordagens mais refinadas, como uso de operações atômicas ou mecanismos de sincronização mais localizados.

Além disso, o programa mede o tempo total de execução com `clock_gettime`, permitindo comparar experimentalmente o custo dessa sincronização em relação às demais versões do projeto. Ao final, os resultados são gravados em `results.csv`, possibilitando a ordenação e comparação com os arquivos de gabarito fornecidos.


## 📖 Para compreender melhor a lógica do projeto:
 - Leia o arquivo "Projeto Prático 2 - Computação Paralela - V3.pdf" para mais detalhes sobre o projeto e sua estrutura.
