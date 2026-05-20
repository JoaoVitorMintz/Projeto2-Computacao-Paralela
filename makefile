CFLAGS = -fopenmp

all: analyzer_seq analyzer_par_atomic analyzer_par_atomic_padded analyzer_par_critical analyzer_par_lock

analyzer_seq: analyzer_seq.c
	gcc -O2 $(CFLAGS) analyzer_seq.c hash_table.c -o analyzer_seq

analyzer_par_atomic: analyzer_par_atomic.c
	gcc -O2 $(CFLAGS) analyzer_par_atomic.c hash_table.c -o analyzer_par_atomic

analyzer_par_atomic_padded: analyzer_par_atomic.c
	gcc -O2 $(CFLAGS) -DPADDED analyzer_par_atomic.c hash_table.c -o analyzer_par_atomic_padded

analyzer_par_critical: analyzer_par_critical.c
	gcc -O2 $(CFLAGS) analyzer_par_critical.c hash_table.c -o analyzer_par_critical

analyzer_par_lock: analyzer_par_lock.c
	gcc -O2 $(CFLAGS) analyzer_par_lock.c hash_table.c -o analyzer_par_lock
