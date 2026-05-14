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
./analyzer_par_critical <arquivo>
```

Sendo que o **arquivo** é **log_concorrente.txt** e **log_distribuido.txt**. Após rodar o código para um determinado arquivo, realize esse comando dentro do termial no ambiente Linux/UNIX:

```terminal
sort results.csv > results_<log_realizado>_sorted.csv
```

Este comando realiza a ordenação do arquivo.csv, importante para comparar com o **gabarito_concorrente.csv** ou **gabarito_distribuido.csv**. Lembre-se que: cada arquivo gerado ao rodar o script deve ser ordenado e definido se é concorrente ou distribuido.

## 📝 Explicação geral da lógica do código:

## 📖 Para compreender melhor a lógica do projeto:
 - Leia o arquivo "Projeto Prático 2 - Computação Paralela - V3.pdf" para mais detalhes sobre o projeto e sua estrutura.
