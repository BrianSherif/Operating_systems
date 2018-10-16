#include <stdio.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include <stdlib.h>

pthread_mutex_t mutex;
static int P = 100;
static int N = 10000;
static char coins[20];

// Function definitions
void *strategy_1(void *);
void *strategy_2(void *);
void *strategy_3(void *);
void run_threads(int n, void *(proc)(void *));
void print_coins(const char *, const char *, int);
static double timeit(int n, void *(*proc)(void *));

int main(int argc, char **argv)
{
    int opt, i;
    double time;

    //Initialize coin array
    for (i = 0; i < 10; i++)
        coins[i] = 'O';
    for (i = 10; i < 20; i++)
        coins[i] = 'X';

    while ((opt = getopt(argc, argv, "p:n:")) != -1)
    {
        switch(opt)
        {
            case 'n':
            {
                N = atoi(optarg);
                if(N < 0)
                {
                    perror("Invalid argument");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 'p':
            {
                P = atoi(optarg);
                if(P < 0)
                {
                    perror("Invalid argument");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case '?':
            {
                perror("Invalid option");
                abort();
            }
        }
    }

    print_coins(coins, "global", 1);
    time = timeit(P, (void *) strategy_1);
    print_coins(coins, "global", 0);
    printf("%d threads x %d flips: %f ms\n\n", P, N, time);

    print_coins(coins, "iterative", 1);
    time = timeit(P, (void *) strategy_2);
    print_coins(coins, "table", 0);
    printf("%d threads x %d flips: %f ms\n\n", P, N, time);

    print_coins(coins, "coin", 1);
    time = timeit(P, (void *) strategy_3);
    print_coins(coins, "coin", 0);
    printf("%d threads x %d flips: %f ms\n\n", P, N, time);

    return EXIT_SUCCESS;
}

void *strategy_1(void *ptr)
{
    int i, j;
    pthread_mutex_lock(&mutex);
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < 20; j++)
        {
            if (coins[j] == 'O')
                coins[j] = 'X';
            else
                coins[j] = 'O';
        }
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *strategy_2(void *ptr)
{
    int i, j;
    for (i = 0; i < N; i++)
    {
        pthread_mutex_lock(&mutex);
        for (j = 0; j < 20; j++)
        {
            if (coins[j] == 'O')
                coins[j] = 'X';
            else
                coins[j] = 'O';
        }
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void *strategy_3(void *ptr)
{
    int i, j;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < 20; j++)
        {
            pthread_mutex_lock(&mutex);
            if (coins[j] == 'O')
                coins[j] = 'X';
            else if(coins[j] == 'X')
                coins[j] = 'O';
            pthread_mutex_unlock(&mutex);
        }
    }

    return NULL;
}

void run_threads(int n, void *(proc)(void *))
{
    pthread_t thread_id[n];
    int i;
    for(i = 0; i < n; i++)
    {
        if(pthread_create(&thread_id[i], NULL, *proc, NULL))
        {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }
    }

    for(i = 0; i < n; i++)
    {
        if(pthread_join(thread_id[i], NULL))
        {
            perror("Error joining threads");
            exit(EXIT_FAILURE);
        }
    }
}

void print_coins(const char *coins, const char *msg, int first)
{
    int i;
    printf("coins: ");
    for(i = 0; i < 20; i++)
    {
        printf("%c", coins[i]);
    }
    if(first)
        printf(" (start - %s lock)\n", msg);
    else
        printf(" (end - %s lock)\n", msg);
}

static double timeit(int n, void *(*proc)(void *))
{
    clock_t t1, t2;
    t1 = clock();
    run_threads(n, proc);
    t2 = clock();
    return ((double)t2 - (double)t1) / CLOCKS_PER_SEC * 1000;
}
