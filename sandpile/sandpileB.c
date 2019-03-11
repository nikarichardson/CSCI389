#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>


// A Litany of Global Variables
int	pthread_cond_init(pthread_cond_t *cv, const pthread_condattr_t *cattr);
int rows;
int columns;
int** sandPile;
int** next;
char s[] = "s";
char n[] = "n";
int qvar;                       // global variable which will indicate the status of the grid's quiescence
int rows_divided;
int x_1st_mid;
int y_1st_mid;
int x_2nd_mid;
int y_2nd_mid;
int x_3rd_mid;
int y_3rd_mid;


typedef struct _barrier {
    int num_threads;
    int threads_in;             // participating threads
    pthread_mutex_t access;     // protects access to thread count
    pthread_cond_t cv;          // condition variable on which threads wait
} barrier;


void row_border(int columns){
    int k;
    while (k < columns){
        printf("+ - - ");
        k++;
    }
    printf("+");
    printf("\n");
    k = 0;
}

void row_values(int columns, int j, char str[]){
    // j:= rows    b:= column counter
    
    if (strcmp(str,s) == 0){
        printf("|");
        int b;
        b = 0;
        while (b < columns){
            int val = sandPile[j][b];
            printf("  %d  |",val);
            b++;
        }
        
    } else {
        printf("|");
        int b;
        b = 0;
        while (b < columns){
            int val = next[j][b];
            printf("  %d  |",val);
            b++;
        }
    }
    

    
}

void output_grid(char str[]){
    if (strcmp(str,"s") == 0){
        int m;
        int j;
        // Begin_Print := printing grid display of sandpile simulation code.
        
        printf("\n");
        row_border(columns);
        m = 0;
        j = 0;
        while (m < rows){
            row_values(columns,j,s);
            j++;
            m++;
            printf("\n");
        }
        row_border(columns);
        
        // End_Print := printing grid display of sandpile simulation code.
        
    } else {
        int m;
        int j;
        // Begin_Print := printing grid display of sandpile simulation code.
        
        printf("\n");
        row_border(columns);
        m = 0;
        j = 0;
        while (m < rows){
            row_values(columns,j,n);
            j++;
            m++;
            printf("\n");
        }
        row_border(columns);
        
        // End_Print := printing grid display of sandpile simulation code.
        
    }
  
    
}

void topple(int r_start, int c_start, int r_end, int c_end){
    
    for (int i = r_start; i < r_end; i++ ) {
        for (int j = c_start; j < c_end; j++ ) {
            int left_neighbor = 1;
            int right_neighbor = 1;
            int up_neighbor = 1;
            int down_neighbor = 1;
            
            // Segmentation Fault Fix
            if (i-1 < 0){
                up_neighbor = 0;
            }
            
            if (i+1 > rows){
                down_neighbor = 0;
            }
            
            if (j-1 < 0){
                left_neighbor = 0;
            }
            
            if ((j+1) > columns){
                right_neighbor = 0;
            }
            
            if (sandPile[i][j] >= 4){
                sandPile[i][j] -= 4;
                next[i][j] = sandPile[i][j];
                
                // Update := update neighbor's height
                if (up_neighbor != 0){
                    next[i-1][j] += 1;
                    sandPile[i-1][j] += 1;
                }
                
                if (left_neighbor != 0){
                    next[i][j-1] += 1;
                    sandPile[i][j-1] += 1;
                }
                
                if (down_neighbor != 0){
                    next[i+1][j] += 1;
                    sandPile[i+1][j] += 1;
                }
                
                    
                if (right_neighbor != 0){
                    next[i][j+1] += 1;
                    sandPile[i][j+1] += 1;
                }
            }
            
            
           // Update:= copy next into sandPile array.
           memcpy(*sandPile, *next, sizeof(sandPile));
        }
    }
    
}

int must_topple(){
    int ret;
    ret = 0;
    for (int i = 0; i < rows; i++ ) {
        for (int j = 0; j < columns; j++ ) {
            if (next[i][j] >= 4){
                ret = 1;
            }
        }
    }
    return ret;
}

void *a_topple(void *b){
    int c;
    barrier *barr = (barrier *)b;
    // Up to 2nd_mid

    while (must_topple() == 1){
        while (qvar != 1){                                          // while not quiescent
            // --- Barrier_Wait:= Critical Section Begin ---
            pthread_mutex_lock(&barr->access);
            
            if (must_topple() == 0 || qvar == 1){
                break;
            }
            
            // Topple_Region:= apply toppling rule to Region A.
            topple(0,0,x_2nd_mid,y_2nd_mid);
          
            if (barr->num_threads == 3){       // if thread is last to enter
                c = pthread_cond_broadcast(&barr->cv);             // signal all condition variables
                // --- Critical Section End ---
                pthread_mutex_unlock(&barr->access);
                barr->num_threads = 0;
            } else {                                                // if thread is not last, update count
                barr->num_threads+=1;
                /* wait on condition variable */
                c = pthread_cond_wait(&barr->cv, &barr->access);
                barr->num_threads = 0;
                // --- Critical Section End ---
                pthread_mutex_unlock(&barr->access);
            }
            
           
        }
    }
    return 0;
}

void *b_topple(void *b){
    int c;
    barrier *barr = (barrier *)b;
    pthread_cond_t condition = barr->cv;
    pthread_mutex_t mu = barr->access;
    // From 2nd_mid to 1st_mid
    
    while (must_topple() == 1){
        while (qvar != 1){                                          // while not quiescent
      
            // --- Barrier_Wait:= Critical Section Begin ---
            pthread_mutex_lock(&barr->access);
            
            if (must_topple() == 0 || qvar == 1){
                break;
            }
            
            
            // Topple_Region:= apply toppling rule to Region B.
            topple(x_2nd_mid,y_2nd_mid,x_1st_mid,y_1st_mid);
            
            
            if (barr->num_threads == 3){       // if thread is last to enter
                c = pthread_cond_broadcast(&barr->cv);             // signal all condition variables
                // --- Critical Section End ---
                pthread_mutex_unlock(&barr->access);
                barr->num_threads = 0;
            } else {                                                // if thread is not last, update count
                barr->num_threads+=1;
                /* wait on condition variable */
                c = pthread_cond_wait(&barr->cv, &barr->access);
                barr->num_threads = 0;
                // --- Critical Section End ---
                pthread_mutex_unlock(&barr->access);
            }
    
        }
    }
    return 0;
    
}

void *c_topple(void *b){
    // From 1st_mid to 3rd_mid
    int c;
    barrier *barr = (barrier *)b;
    pthread_cond_t condition = barr->cv;
    pthread_mutex_t mu = barr->access;

    while (must_topple() == 1){
        while (qvar != 1){                                          // while not quiescent
        
            // --- Barrier_Wait:= Critical Section Begin ---
            pthread_mutex_lock(&barr->access);
            
            if (must_topple() == 0 || qvar == 1){
                break;
            }
            
            
            // Topple_Region:= apply toppling rule to Region C.
            topple(x_1st_mid,y_1st_mid,x_3rd_mid,y_3rd_mid);
            
            
            if (barr->num_threads == 3){       // if thread is last to enter
                c = pthread_cond_broadcast(&barr->cv);             // signal all condition variables
                // --- Critical Section End ---
                pthread_mutex_unlock(&barr->access);
            
                barr->num_threads = 0;
            } else {                                                // if thread is not last, update count
                barr->num_threads+=1;
    
                /* wait on condition variable */
                c = pthread_cond_wait(&barr->cv, &barr->access);
                barr->num_threads = 0;
                // --- Critical Section End ---
                pthread_mutex_unlock(&barr->access);
            }
            
         
        }
    }
    return 0;
}

void *d_topple(void *b){
    // From 3rd_mid to max(rows) & max(columns)
    int c;
    barrier *barr = (barrier *)b;
    pthread_cond_t condition = barr->cv;
    pthread_mutex_t mu = barr->access;

    while (must_topple() == 1){
        while (qvar != 1){
            
            // --- Barrier_Wait:= Critical Section Begin ---
            pthread_mutex_lock(&barr->access);
            
            if (must_topple() == 0 || qvar == 1){
                break;
            }
            
            
            // Topple_Region:= apply toppling rule to Region D.
            topple(x_3rd_mid,y_3rd_mid,rows,columns);
    
            if (barr->num_threads == (barr->threads_in - 1)){       // if thread is last to enter
                c = pthread_cond_broadcast(&barr->cv);             // signal all condition variables
                // --- Critical Section End ---
                pthread_mutex_unlock(&barr->access);
                barr->num_threads = 0;
                
            } else {                                                // if thread is not last, update count
                barr->num_threads+=1;
    
                /* wait on condition variable */
                c = pthread_cond_wait(&barr->cv, &barr->access);
                barr->num_threads = 0;
                // --- Critical Section End ---
                pthread_mutex_unlock(&barr->access);
            }
            
            
        }
    }
    return 0;
}


void quiesce(){
    
    //output_grid(s);
    
    pthread_cond_t cnd;
    pthread_cond_init(&cnd, NULL);               // initialize condition variable to default value
    pthread_mutex_t a;
    pthread_mutex_init(&a,NULL);
    
    // Initializing a four-thread barrier.
    barrier *barr = malloc(sizeof(barrier));
    barr->num_threads = 0;                       // counts how many threads have reached the barrier
    barr->threads_in = 4;                        // initializes the number of participating threads
    barr->cv = cnd;
    barr->access = a;

    
    // Create all four regional pthreads.
    rows_divided = 0;
    
    pthread_t ta;
    pthread_t tb;
    pthread_t tc;
    pthread_t td;
    
    while (must_topple() == 1){
        pthread_create(&ta,NULL,a_topple,(void *)barr);
        pthread_create(&tb,NULL,b_topple,(void *)barr);
        pthread_create(&tc,NULL,c_topple,(void *)barr);
        pthread_create(&td,NULL,d_topple,(void *)barr);

    }
    qvar = 1;
}


int main(int argc, char **argv){
    qvar = 0;
    rows = atoi(argv[1]);
    
    columns = atoi(argv[2]);
    int height = atoi(argv[3]);
    
    
    sandPile = malloc(rows * sizeof(int*));
    for (int r = 0; r < rows; r++) {
        sandPile[r] = malloc(columns * sizeof(int));
    }
    
    next = malloc(rows * sizeof(int*));
    for (int h = 0; h < rows; h++) {
        next[h] = malloc(columns * sizeof(int));
    }
    
    x_1st_mid = (rows / 2);
    y_1st_mid = (columns / 2);
    
    x_2nd_mid = (x_1st_mid / 2);
    y_2nd_mid = (y_1st_mid / 2);
    
    x_3rd_mid = (rows - x_1st_mid);
    y_3rd_mid = (columns - y_1st_mid);

    for (int l = 0; l < rows; l++ ) {
        for (int j = 0; j < columns; j++ ) {
            if (l == x_1st_mid & j == y_1st_mid){
                sandPile[l][j] = height;
                
            } else {
                sandPile[l][j] = 0;
            }
        }
    }
    
    for (int l = 0; l < rows; l++ ) {
        for (int g = 0; g < columns; g++ ) {
            if (l == x_1st_mid & g == y_1st_mid){
                next[l][g] = height-4;
                
            } else {
                next[l][g] = 0;
            }
        }
    }
    
    int m;
    int j;
    
    clock_t start = clock();
    quiesce();
    
    if (qvar == 1){
        clock_t end = clock();
        printf("\n         start time: %lu, end time: %lu\n",start,end);
        double time_spent = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("\n         time spent: %f \n",time_spent);
        printf("\n");
    }
    
   
   
}