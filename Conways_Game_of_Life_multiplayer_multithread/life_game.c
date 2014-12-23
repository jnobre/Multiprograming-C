#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/fcntl.h>
#include <sys/shm.h>
#include <limits.h>
#include <sys/mman.h>

#define DEAD 0
#define ALIVE 1
#define FALSE 0
#define TRUE 1
#define N 2
//#define DEBUG //uncomment this line to add debug messages
#define BUF_SIZE 4048
#define GENS 1000
#define SIZE 500
#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

typedef struct {
	int id;
	int inicio;
	int fim;
	int gen_actual;
} auxiliar;



int gens,freq,freq1,gens_actual[N][2];
int size,cont_freq[N];
int number_elements[5] = {0,0,0,0,0};
pthread_t thread[N];
int **matrix, **matrix_b;
pthread_mutex_t *mutex;
pthread_cond_t *cond_ready;
pthread_cond_t *cond_count;
pthread_mutex_t mutex_snapshot;
pthread_cond_t cond_snapshot;


int erro(char* text)
{
    printf("Erro:%s\n",text);
    exit(-1);
}

void init_semaphore()
{
    int i=0,j=0;



    cond_count = (pthread_cond_t *)malloc( sizeof(pthread_cond_t) * N );


    mutex=(pthread_mutex_t *) malloc( sizeof(pthread_mutex_t) * N );
    if(mutex == NULL)
        erro("Erro na memoria\n");
    cond_ready=(pthread_cond_t *) malloc( sizeof(pthread_cond_t) * N );
    if(cond_ready== NULL)
        erro("Erro na memoria\n");

    /* Initialize mutex and condition variable objects */
    for(j=0;j<N;j++)
        pthread_mutex_init(&mutex[j], NULL);

    //for(i=0;i<2;i++)
        for(j=0;j<N;j++)
            pthread_cond_init(&cond_ready[j], NULL);

    for(i=0;i<N;i++)
        pthread_cond_init(&cond_count[i], NULL);

}


int rand_index()
{
    return  rand()%size;
}




int is_in_range(int i, int j) {
    if (j < 0 || i < 0 || j >= size || i >= size)
        return FALSE;

    return TRUE;
}

int is_alive(int ** matrix, int i, int j) {
    if ( matrix[i][j] == ALIVE)
        return TRUE;
    return FALSE;
}

int put_single( int ** matrix, int ** matrix_b, int i, int j) {
    if (!is_in_range(i,j))
        return FALSE;

    if (is_alive(matrix,i,j))
        return FALSE;

    matrix[i][j] = matrix_b [i][j] = ALIVE;
    return TRUE;
}

int put_block( int ** matrix, int ** matrix_b, int i, int j) {
    if (!is_in_range(i,j))
        return FALSE;
    int pos = 0;
    for (pos = 0; pos < 2; pos++) {
        if (is_alive(matrix,(i+pos)%size,j))
            return FALSE;
        if (is_alive(matrix,i,(j+pos)%size))
            return FALSE;
    }
    if (is_alive(matrix,(i+pos)%size,(j+pos)%size))
        return FALSE;

    matrix [i][j] = matrix_b [i][j] = ALIVE;
    matrix [(i+1)%size][j] = matrix_b [(i+1)%size][j] = ALIVE;
    matrix [i][(j+1)%size] = matrix_b [i][(j+1)%size] = ALIVE;
    matrix [(i+1)%size][(j+1)%size] = matrix_b [(i+1)%size][(j+1)%size] = ALIVE;
    return TRUE;
}

int put_glider ( int ** matrix, int ** matrix_b, int i, int j) {
    if (!is_in_range(i,j))
        return FALSE;

    if (is_alive(matrix,(i+1)%size,j))
        return FALSE;
    if (is_alive(matrix,(i+2)%size,(j+1)%size))
        return FALSE;
    if (is_alive(matrix,i,(j+2)%size))
        return FALSE;
    if (is_alive(matrix,(i+1)%size,(j+2)%size))
        return FALSE;
    if (is_alive(matrix,(i+2)%size,(j+2)%size))
        return FALSE;

    matrix [(i+1)%size][j] = matrix_b [(i+1)%size][j] = ALIVE;
    matrix [(i+2)%size][(j+1)%size] = matrix_b [(i+2)%size][(j+1)%size] = ALIVE;
    matrix [i][(j+2)%size] = matrix_b [i][(j+2)%size] = ALIVE;
    matrix [(i+1)%size][(j+2)%size] = matrix_b [(i+1)%size][(j+2)%size] = ALIVE;
    matrix [(i+2)%size][(j+2)%size] = matrix_b [(i+2)%size][(j+2)%size] = ALIVE;
    return TRUE;
}

int put_lightweight_space_ship( int ** matrix, int ** matrix_b, int i, int j) {
    if (!is_in_range(i,j))
        return FALSE;

    if (is_alive(matrix,i,j))
        return FALSE;
    if (is_alive(matrix,(i+3)%size,j))
        return FALSE;
    if (is_alive(matrix,(i+4)%size,(j+1)%size))
        return FALSE;
    if (is_alive(matrix,i,(j+2)%size))
        return FALSE;
    if (is_alive(matrix,(i+4)%size,(j+2)%size))
        return FALSE;
    if (is_alive(matrix,(i+1)%size,(j+3)%size))
        return FALSE;
    if (is_alive(matrix,(i+2)%size,(j+3)%size))
        return FALSE;
    if (is_alive(matrix,(i+3)%size,(j+3)%size))
        return FALSE;
    if (is_alive(matrix,(i+4)%size,(j+3)%size))
        return FALSE;


    matrix [i][j] = matrix_b [i][j] = ALIVE;
    matrix [(i+3)%size][j] = matrix_b [(i+3)%size][j] = ALIVE;
    matrix [(i+4)%size][(j+1)%size] = matrix_b [(i+4)%size][(j+1)%size] = ALIVE;
    matrix [i][(j+2)%size] = matrix_b [i][(j+2)%size] = ALIVE;
    matrix [(i+4)%size][(j+2)%size] = matrix_b [(i+4)%size][(j+2)%size] = ALIVE;
    matrix [(i+1)%size][(j+3)%size] = matrix_b [(i+1)%size][(j+3)%size] = ALIVE;
    matrix [(i+2)%size][(j+3)%size] = matrix_b [(i+2)%size][(j+3)%size] = ALIVE;
    matrix [(i+3)%size][(j+3)%size] = matrix_b [(i+3)%size][(j+3)%size] = ALIVE;
    matrix [(i+4)%size][(j+3)%size] = matrix_b [(i+4)%size][(j+3)%size] = ALIVE;
    return TRUE;
}

int put_pulsar ( int ** matrix, int ** matrix_b, int i, int j) {
    if (!is_in_range(i,j))
        return FALSE;

    if (is_alive(matrix,(i+2)%size,j))
        return FALSE;
    if (is_alive(matrix,(i+3)%size,j))
        return FALSE;
    if (is_alive(matrix,(i+4)%size,j))
        return FALSE;
    if (is_alive(matrix,(i+8)%size,j))
        return FALSE;
    if (is_alive(matrix,(i+9)%size,j))
        return FALSE;
    if (is_alive(matrix,(i+10)%size,j))
        return FALSE;

    if (is_alive(matrix,i,(j+2)%size))
        return FALSE;
    if (is_alive(matrix,(i+5)%size,(j+2)%size))
        return FALSE;
    if (is_alive(matrix,(i+7)%size,(j+2)%size))
        return FALSE;
    if (is_alive(matrix,(i+12)%size,(j+2)%size))
        return FALSE;

    if (is_alive(matrix,i,(j+3)%size))
        return FALSE;
    if (is_alive(matrix,(i+5)%size,(j+3)%size))
        return FALSE;
    if (is_alive(matrix,(i+7)%size,(j+3)%size))
        return FALSE;
    if (is_alive(matrix,(i+12)%size,(j+3)%size))
        return FALSE;

    if (is_alive(matrix,i,(j+4)%size))
        return FALSE;
    if (is_alive(matrix,(i+5)%size,(j+4)%size))
        return FALSE;
    if (is_alive(matrix,(i+7)%size,(j+4)%size))
        return FALSE;
    if (is_alive(matrix,(i+12)%size,(j+4)%size))
        return FALSE;

    if (is_alive(matrix,(i+2)%size,(j+5)%size))
        return FALSE;
    if (is_alive(matrix,(i+3)%size,(j+5)%size))
        return FALSE;
    if (is_alive(matrix,(i+4)%size,(j+5)%size))
        return FALSE;
    if (is_alive(matrix,(i+8)%size,(j+5)%size))
        return FALSE;
    if (is_alive(matrix,(i+9)%size,(j+5)%size))
        return FALSE;
    if (is_alive(matrix,(i+10)%size,(j+5)%size))
        return FALSE;

    if (is_alive(matrix,(i+2)%size,(j+7)%size))
        return FALSE;
    if (is_alive(matrix,(i+3)%size,(j+7)%size))
        return FALSE;
    if (is_alive(matrix,(i+4)%size,(j+7)%size))
        return FALSE;
    if (is_alive(matrix,(i+8)%size,(j+7)%size))
        return FALSE;
    if (is_alive(matrix,(i+9)%size,(j+7)%size))
        return FALSE;
    if (is_alive(matrix,(i+10)%size,(j+7)%size))
        return FALSE;

    if (is_alive(matrix,i,(j+8)%size))
        return FALSE;
    if (is_alive(matrix,(i+5)%size,(j+8)%size))
        return FALSE;
    if (is_alive(matrix,(i+7)%size,(j+8)%size))
        return FALSE;
    if (is_alive(matrix,(i+12)%size,(j+8)%size))
        return FALSE;

    if (is_alive(matrix,i,(j+9)%size))
        return FALSE;
    if (is_alive(matrix,(i+5)%size,(j+9)%size))
        return FALSE;
    if (is_alive(matrix,(i+7)%size,(j+9)%size))
        return FALSE;
    if (is_alive(matrix,(i+12)%size,(j+9)%size))
        return FALSE;

    if (is_alive(matrix,i,(j+10)%size))
        return FALSE;
    if (is_alive(matrix,(i+5)%size,(j+10)%size))
        return FALSE;
    if (is_alive(matrix,(i+7)%size,(j+10)%size))
        return FALSE;
    if (is_alive(matrix,(i+12)%size,(j+10)%size))
        return FALSE;

    if (is_alive(matrix,(i+2)%size,(j+12)%size))
        return FALSE;
    if (is_alive(matrix,(i+3)%size,(j+12)%size))
        return FALSE;
    if (is_alive(matrix,(i+4)%size,(j+12)%size))
        return FALSE;
    if (is_alive(matrix,(i+8)%size,(j+12)%size))
        return FALSE;
    if (is_alive(matrix,(i+9)%size,(j+12)%size))
        return FALSE;
    if (is_alive(matrix,(i+10)%size,(j+12)%size))
        return FALSE;

    matrix [(i+2)%size][j] = matrix_b [(i+2)%size][j] = ALIVE;
    matrix [(i+3)%size][j] = matrix_b [(i+3)%size][j] = ALIVE;
    matrix [(i+4)%size][j] = matrix_b [(i+4)%size][j] = ALIVE;
    matrix [(i+8)%size][j] = matrix_b [(i+8)%size][j] = ALIVE;
    matrix [(i+9)%size][j] = matrix_b [(i+9)%size][j] = ALIVE;
    matrix [(i+10)%size][j] = matrix_b [(i+10)%size][j] = ALIVE;
    matrix [i][(j+2)%size] = matrix_b [i][(j+2)%size] = ALIVE;
    matrix [(i+5)%size][(j+2)%size] = matrix_b [(i+5)%size][(j+2)%size] = ALIVE;
    matrix [(i+7)%size][(j+2)%size] = matrix_b [(i+7)%size][(j+2)%size] = ALIVE;
    matrix [(i+12)%size][(j+2)%size] = matrix_b [(i+12)%size][(j+2)%size] = ALIVE;
    matrix [i][(j+3)%size] = matrix_b [i][(j+3)%size]  = ALIVE;
    matrix [(i+5)%size][(j+3)%size] = matrix_b [(i+5)%size][(j+3)%size] = ALIVE;
    matrix [(i+7)%size][(j+3)%size] = matrix_b [(i+7)%size][(j+3)%size] = ALIVE;
    matrix [(i+12)%size][(j+3)%size] = matrix_b [(i+12)%size][(j+3)%size] = ALIVE;
    matrix [i][(j+4)%size] = matrix_b [i][(j+4)%size] = ALIVE;
    matrix [(i+5)%size][(j+4)%size] = matrix_b [(i+5)%size][(j+4)%size] = ALIVE;
    matrix [(i+7)%size][(j+4)%size] = matrix_b [(i+7)%size][(j+4)%size] = ALIVE;
    matrix [(i+12)%size][(j+4)%size] = matrix_b [(i+12)%size][(j+4)%size] = ALIVE;
    matrix [(i+2)%size][(j+5)%size] = matrix_b [(i+2)%size][(j+5)%size] = ALIVE;
    matrix [(i+3)%size][(j+5)%size] = matrix_b [(i+3)%size][(j+5)%size] = ALIVE;
    matrix [(i+4)%size][(j+5)%size] = matrix_b [(i+4)%size][(j+5)%size] = ALIVE;
    matrix [(i+8)%size][(j+5)%size] = matrix_b [(i+8)%size][(j+5)%size] = ALIVE;
    matrix [(i+9)%size][(j+5)%size] = matrix_b [(i+9)%size][(j+5)%size] = ALIVE;
    matrix [(i+10)%size][(j+5)%size] = matrix_b [(i+10)%size][(j+5)%size] = ALIVE;
    matrix [(i+2)%size][(j+7)%size] = matrix_b [(i+2)%size][(j+7)%size] = ALIVE;
    matrix [(i+3)%size][(j+7)%size] = matrix_b [(i+3)%size][(j+7)%size] = ALIVE;
    matrix [(i+4)%size][(j+7)%size] = matrix_b [(i+4)%size][(j+7)%size] = ALIVE;
    matrix [(i+8)%size][(j+7)%size] = matrix_b [(i+8)%size][(j+7)%size] = ALIVE;
    matrix [(i+9)%size][(j+7)%size] = matrix_b [(i+9)%size][(j+7)%size] = ALIVE;
    matrix [(i+10)%size][(j+7)%size] = matrix_b [(i+10)%size][(j+7)%size] = ALIVE;
    matrix [i][(j+8)%size] = matrix_b [i][(j+8)%size] = ALIVE;
    matrix [(i+5)%size][(j+8)%size] = matrix_b [(i+5)%size][(j+8)%size] = ALIVE;
    matrix [(i+7)%size][(j+8)%size] = matrix_b [(i+7)%size][(j+8)%size] = ALIVE;
    matrix [(i+12)%size][(j+8)%size] = matrix_b [(i+12)%size][(j+8)%size] = ALIVE;
    matrix [i][(j+9)%size] = matrix_b [i][(j+9)%size] = ALIVE;
    matrix [(i+5)%size][(j+9)%size] = matrix_b [(i+5)%size][(j+9)%size] = ALIVE;
    matrix [(i+7)%size][(j+9)%size] = matrix_b [(i+7)%size][(j+9)%size] = ALIVE;
    matrix [(i+12)%size][(j+9)%size] = matrix_b [(i+12)%size][(j+9)%size] = ALIVE;
    matrix [i][(j+10)%size] = matrix_b [i][(j+10)%size] = ALIVE;
    matrix [(i+5)%size][(j+10)%size] = matrix_b [(i+5)%size][(j+10)%size] = ALIVE;
    matrix [(i+7)%size][(j+10)%size] = matrix_b [(i+7)%size][(j+10)%size] = ALIVE;
    matrix [(i+12)%size][(j+10)%size] = matrix_b [(i+12)%size][(j+10)%size] = ALIVE;
    matrix [(i+2)%size][(j+12)%size] = matrix_b [(i+2)%size][(j+12)%size] = ALIVE;
    matrix [(i+3)%size][(j+12)%size] = matrix_b [(i+3)%size][(j+12)%size] = ALIVE;
    matrix [(i+4)%size][(j+12)%size] = matrix_b [(i+4)%size][(j+12)%size] = ALIVE;
    matrix [(i+8)%size][(j+12)%size] = matrix_b [(i+8)%size][(j+12)%size] = ALIVE;
    matrix [(i+9)%size][(j+12)%size] = matrix_b [(i+9)%size][(j+12)%size] = ALIVE;
    matrix [(i+10)%size][(j+12)%size] = matrix_b [(i+10)%size][(j+12)%size] = ALIVE;
    return TRUE;
}


void init( int ** matrix, int ** matrix_b )
{
    int i, j;

    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            matrix[i][j] = DEAD;
            matrix_b[i][j] = DEAD;
        }
    }
    #ifdef DEBUG
    fprintf( stderr, "Boundary set\n");
    #endif
    int cnt;

    for (;number_elements[0] > 0; number_elements[0]--) {
        cnt = 0;
        while (!put_single(matrix, matrix_b, rand_index(), rand_index())) if (++cnt > 3) break;
        #ifdef DEBUG
        if (cnt <= 3)
            fprintf( stderr,"Single created!");
        #endif
    }

    for (;number_elements[1] > 0; number_elements[1]--) {
        cnt = 0;
        while (!put_block(matrix, matrix_b, rand_index(), rand_index())) if (++cnt > 3) break;
        #ifdef DEBUG
        if (cnt <= 3)
            fprintf( stderr,"Block created!");
        #endif
    }
    for (;number_elements[2] > 0; number_elements[2]--) {
        cnt = 0;
        while (!put_glider(matrix, matrix_b, rand_index(), rand_index())) if (++cnt > 3) break;
        #ifdef DEBUG
        if (cnt <= 3)
            fprintf( stderr,"Glider created!");
        #endif
    }
    for (;number_elements[3] > 0; number_elements[3]--) {
        cnt = 0;
        while (!put_lightweight_space_ship(matrix, matrix_b, rand_index(), rand_index())) if (++cnt > 3) break;
        #ifdef DEBUG
        if (cnt <= 3)
            fprintf( stderr,"Space ship created!");
        #endif
    }
    for (;number_elements[4] > 0; number_elements[4]--) {
        cnt = 0;
        while (!put_pulsar(matrix, matrix_b, rand_index(), rand_index())) if (++cnt > 3) break;
        #ifdef DEBUG
        if (cnt <= 3)
            fprintf( stderr,"Pulsar created!");
        #endif
    }

}

void cleanup(int **matrix, int **matrix_b) {
    int i;

    for(i=0;i<N;i++)
    {
        pthread_mutex_destroy(&mutex[i]);
        pthread_cond_destroy(&cond_ready[i]);
    }

    for (i = 0; i < size; i++)
    {
        free( matrix[i] );
        free( matrix_b[i] );

    }
    free( matrix );
    free( matrix_b );
    free( cond_ready );
    free(mutex);

}




void print(auxiliar aux)
{
    int i=0,j=0;
    printf("Geracao %d\n",gens_actual[aux.id][0]+1);
   for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            printf("%d ", matrix_b[j][i] );
        }
        printf ("\n");
    }
  //printf("Sou a thread %d e vou imprimiar a geracaco %d\n",aux.id,gens_actual[aux.id][0]);
    printf("\n\n");
}


void evolve(  int ** matrix, int ** matrix_b,auxiliar aux,int geracao_actual)
{
    int i, j, sum, tmp, vizinho[2];

    //verificar thread's vizinhas
    if(aux.id-1<0)
    {
        vizinho[0]=N-1;
        vizinho[1]=aux.id+1;
    }
    else if(aux.id+1>=N)
    {
        vizinho[0]=aux.id-1;
        vizinho[1]=0;
    }
    else
    {
        vizinho[0]=aux.id-1;
        vizinho[1]=aux.id+1;
    }

  //  printf("Sou a thread %d e os meus vizinhos sao esquerda thread %d direita thread %d\n",aux.id,vizinho[0],vizinho[1]);

    //começar pelas células interiores
    for (j = aux.inicio+1; j < aux.fim; j++)
    {
        //printf("Sou thread %d do 1º for e estou no indice J == %d\n",aux.id,j);
            for (i = 0; i < size; i++)
            {

                int x,y;
                if (i == 0)
                    x = size - 1;
                else
                    x = (i-1)%size;
                if (j == 0)
                    y = size - 1;
                else
                    y = (j-1)%size;

                sum =
                matrix[x][y] +
                matrix[x][j] +
                matrix[x][(j+1)%size] +
                matrix[i][y] +
                matrix[i][(j+1)%size] +
                matrix[(i+1)%size][y] +
                matrix[(i+1)%size][j] +
                matrix[(i+1)%size][(j+1)%size];


                if (sum < 2 || sum > 3)
                    matrix_b[i][j] = DEAD ;
                else if (sum == 3)
                    matrix_b[i][j] = ALIVE ;
                else
                    matrix_b[i][j] = matrix[i][j] ;

            }
    }

    geracao_actual=gens_actual[aux.id][0];

    for (j = aux.inicio; j <= aux.fim; j=j+(aux.fim-aux.inicio))
    {
        //bloqueia mutex da respectiva thread
        pthread_mutex_lock(&mutex[aux.id]);

        //se for primeira linha e estiver geracao passada
        if(j==aux.inicio && gens_actual[vizinho[0]][0] < geracao_actual)
        {
            //se o vizinho da esquerda estiver numa geracao passada
            while(gens_actual[vizinho[0]][0] < geracao_actual)
            {
                //espera pela actualizacao da geracao
                pthread_cond_wait(&cond_ready[aux.id], &mutex[aux.id]);
            }

        }

        if(j==aux.fim && gens_actual[vizinho[1]][0] < geracao_actual)
        {
            while(gens_actual[vizinho[1]][0] < geracao_actual)
            {
                pthread_cond_wait(&cond_ready[aux.id], &mutex[aux.id]);
            }
        }
        //desbloqueia respectivo mutex
        pthread_mutex_unlock(&mutex[aux.id]);


        for (i = 0; i < size; i++)
        {
            int x,y;
            if (i == 0)
                x = size - 1;
            else
                x = (i-1)%size;
            if (j == 0)
                y = size - 1;
            else
                y = (j-1)%size;

            sum =
            matrix[x][y] +
            matrix[x][j] +
            matrix[x][(j+1)%size] +
            matrix[i][y] +
            matrix[i][(j+1)%size] +
            matrix[(i+1)%size][y] +
            matrix[(i+1)%size][j] +
            matrix[(i+1)%size][(j+1)%size];


            if (sum < 2 || sum > 3)
                matrix_b[i][j] = DEAD ;
            else if (sum == 3)
                matrix_b[i][j] = ALIVE ;
            else
                matrix_b[i][j] = matrix[i][j];
        }
    }

    gens_actual[aux.id][1]++;

    if(gens_actual[vizinho[1]][1]==gens_actual[aux.id][1])
    {
        pthread_cond_signal(&cond_count[vizinho[1]]);
    }


    if(gens_actual[vizinho[0]][1]==gens_actual[aux.id][1])
    {
        pthread_cond_signal(&cond_count[vizinho[0]]);
    }

    pthread_mutex_lock(&mutex[aux.id]);
    while(gens_actual[vizinho[1]][1] != gens_actual[aux.id][1] || gens_actual[vizinho[0]][1] != gens_actual[aux.id][1])
    {
        pthread_cond_wait(&cond_count[aux.id], &mutex[aux.id]);
    }
    pthread_mutex_unlock(&mutex[aux.id]);

   //snapshot
   /* if(gens_actual[aux.id][0]+1 == freq)
    {

        //verifica se e o ultimo

        for(i=0;i<N;i++)
        {
                if(gens_actual[i][1] == gens_actual[aux.id][1] )
                {
                    cont_freq[aux.id]++;
                }
        }
        //se foi o ultimo

        if(cont_freq[aux.id]==N)
        {
             print(aux);
            freq=freq+freq1;
            pthread_cond_broadcast(&cond_snapshot);
        }
        else
        {
             pthread_mutex_lock(&mutex_snapshot);
             pthread_cond_wait(&cond_snapshot,&mutex_snapshot);
             pthread_mutex_unlock(&mutex_snapshot);

        }
        cont_freq[aux.id]=0;

    }*/

    //actualiza matriz's
    for (i = 0; i < size; i++)
    {
        for (j = aux.inicio; j <= aux.fim; j++)
        {
            tmp = matrix[i][j];
            matrix[i][j] = matrix_b[i][j];
            matrix_b[i][j] = tmp;
            #ifdef DEBUG
            printf("%d ", matrix[j][i] );
            #endif
        }
        #ifdef DEBUG
        printf ("\n");
        #endif
    }
    #ifdef DEBUG
    printf("\n");
    #endif

   //actualizar
    gens_actual[aux.id][0]++;

    if(gens_actual[aux.id][0] < gens){

        if(gens_actual[aux.id][0]==gens_actual[vizinho[0]][0])
        {
            pthread_cond_signal(&cond_ready[vizinho[0]]);
        }

        if(gens_actual[aux.id][0]==gens_actual[vizinho[1]][0])
        {
             pthread_cond_signal(&cond_ready[vizinho[1]]);
        }

     }

}

void *worker(auxiliar *aux)
{
    int gen;

    for( gen=0 ; gen<gens ; gen++ )
    {
        //geracao actual e a gen e calcula a geracao+1
       evolve(matrix, matrix_b,*aux,gen);

    }

    pthread_exit(NULL);
}



int main( int argn, char * argv[] )
{
    int i,inicio=0,fim=0,j=0,tmp=0;
    auxiliar aux[N];
    init_semaphore();

    srand(12345);
    #ifdef DEBUG
    int j;
    #endif

    if (argn < 4) {
        fprintf(stderr, "Argumentos insuficientes. Utilizar: life_game <dim_matriz> <num_gerações> <frequencia snapshot>\n");
        exit(-1);
    }

    gens = atoi(argv[2]);
    if (gens == 0) gens = GENS;
    size =  atoi(argv[1]);
    if (size == 0) size = SIZE;
    freq = atoi(argv[3]);
    if (freq == 0) freq = size/2;
    freq1=freq;

    number_elements [0] = 5; // 5 single
    number_elements [1] = 5; // 5 blocks
    number_elements [2] = 3; // 3 gliders
    number_elements [3] = 2; // 2 lightweigh space ships
    number_elements [4] = 1; // 1 pulsar

    matrix = (int **)malloc( sizeof(int *) * (size) );
    matrix_b = (int **)malloc( sizeof(int *) * (size) );
    for (i = 0; i < size; i++)
    {
        matrix[i] = (int *)malloc( sizeof(int) * (size) );
        matrix_b[i] = (int *)malloc( sizeof(int) * (size) );
    }

    init( matrix, matrix_b );

    #ifdef DEBUG
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            printf("%d ", matrix[j][i] );
        }
        printf ("\n");
    }
    #endif
    for(i=0;i < N; i++)
        for(j=0;j<2;j++)
            gens_actual[i][j]=0;

    fim=size/N;
    //dividir por N thread's
    for (i = 0; i < N; i++) {
        aux[i].id=i;
        aux[i].inicio=inicio;
        aux[i].fim=fim;
        aux[i].gen_actual=0;
        cont_freq[i]=0;
       // gens_actual[i]=0;
       pthread_create(&thread[i], NULL, (void*) &worker,(void*) &aux[i]);
       // printf("Sou a thread id == %d e vou começaar a trabalhar %d e acabar %d\n",i,inicio,fim);
        //calcular indeces das thread
        if(i==N-2)
        {
            inicio=tmp+(size/N)+1;
            tmp=inicio-1;
            fim=size;
        }
        else
        {
            inicio=tmp+(size/N)+1;
            tmp=inicio-1;
            fim=fim+size/N;
        }
    }

    //esperar que as thread acabem o seu trabalho
    for(i=0;i<N;i++)
    {
        pthread_join(thread[i],NULL);
    }

  printf("\n\nGeracao %d\n",gens);
   for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            printf("%d ", matrix[j][i] );
        }
        printf ("\n");
    }

    cleanup(matrix, matrix_b);

    exit( 0 );
}
