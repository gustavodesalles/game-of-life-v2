#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "gol.h"

int main(int argc, char **argv)
{
    int size, steps;
    cell_t **prev, **next, **tmp;
    FILE *f;
    stats_t stats_step = {0, 0, 0, 0};
    stats_t stats_total = {0, 0, 0, 0};

    if (argc != 3)
    {
        printf("ERRO! Você deve digitar %s <nome do arquivo do tabuleiro>!\n\n", argv[0]);
        return 0;
    }

    //Quantas threads?
    int n_threads = atoi(argv[2]);
    if (!n_threads) {
        printf("Número de threads deve ser > 0\n");
        return 1;
    }

    if ((f = fopen(argv[1], "r")) == NULL)
    {
        printf("ERRO! O arquivo de tabuleiro '%s' não existe!\n\n", argv[1]);
        return 0;
    }

    fscanf(f, "%d %d", &size, &steps);
    int n_celulas = size*size; //supondo que a matriz é sempre quadrada

    //Reduz número de threads para evitar inanição
    if (n_threads > n_celulas)
    {
        n_threads = n_celulas;
    }

    //Instancia array de threads
    pthread_t threads[n_threads];

    //Informações necessárias para realizar os cálculos
    aux info[n_threads];
    int begin = 0;
    int intervalo = n_celulas / n_threads;
    int resto = n_celulas % n_threads;

    //Popula struct com informações para cada thread
    for (int i = 0; i < n_threads; i++)
    {
        int end = begin + intervalo;
        if (resto) //caso n_celulas / n_threads seja ímpar
        {
            --resto;
            ++end;
        }
        info[i].size = size;
        info[i].begin = begin;
        info[i].end = end;
        begin = end;
    }
    
    prev = allocate_board(size);
    next = allocate_board(size);

    read_file(f, prev, size);

    fclose(f);

#ifdef DEBUG
    printf("Initial:\n");
    print_board(prev, size);
    print_stats(stats_step);
#endif

    for (int i = 0; i < steps; i++)
    {
        //Reseta valores de stats_step para o próximo passo
        stats_step.borns = 0;
        stats_step.loneliness = 0;
        stats_step.overcrowding = 0;
        stats_step.survivals = 0;

        //Inicia threads que executam "play" em paralelo
        for (int j = 0; j < n_threads; j++)
        {
            info[j].board = prev;
            info[j].newboard = next;
            info[j].stats = stats_step;
            pthread_create(&threads[j], NULL, play_parallel, (void *) &info[j]);
        }
        
        for (int j = 0; j < n_threads; j++)
        {
            pthread_join(threads[j], NULL);
            stats_step.borns += info[j].stats.borns;
            stats_step.survivals += info[j].stats.survivals;
            stats_step.loneliness += info[j].stats.loneliness;
            stats_step.overcrowding += info[j].stats.overcrowding;
        }

        stats_total.borns += stats_step.borns;
        stats_total.survivals += stats_step.survivals;
        stats_total.loneliness += stats_step.loneliness;
        stats_total.overcrowding += stats_step.overcrowding;

#ifdef DEBUG
        printf("Step %d ----------\n", i + 1);
        print_board(next, size);
        print_stats(stats_step);
#endif
        tmp = next;
        next = prev;
        prev = tmp;
    }

#ifdef RESULT
    printf("Final:\n");
    print_board(prev, size);
    print_stats(stats_total);
#endif

    // pthread_barrier_destroy(&barrier);
    free_board(prev, size);
    free_board(next, size);
}
