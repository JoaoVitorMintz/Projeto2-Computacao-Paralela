CFLAGS = -fopenmp

all: analyzer_seq

analyzer_seq: analyzer_seq.c
	gcc -O2 $(CFLAGS) analyzer_seq.c hash_table.c -o analyzer_seq