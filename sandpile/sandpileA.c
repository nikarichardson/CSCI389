#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int rows;
int columns;
int** sandPile;
int** next;
char s[] = "s";
char n[] = "n";

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

void topple(){
    
    for (int i = 0; i < rows; i++ ) {
        for (int j = 0; j < columns; j++ ) {
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


void quiesce(){
    int x;
    int y;
    
    // Center:= defined position of the approximated center grid cell
    x = (rows / 2);
    y = (columns / 2);
    
    output_grid(s);
    while (must_topple() == 1){
        topple();
        output_grid(n);
        sleep(5);
    }
    
    printf("\nQuiescence ...\n");
    
}



int main(int argc, char **argv){
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
    
    
    int x;
    int y;
    
    // Center:= defined position of the approximated center grid cell
    x = (rows / 2);
    y = (columns / 2);

    for (int l = 0; l < rows; l++ ) {
        for (int j = 0; j < columns; j++ ) {
            if (l == x & j == y){
                sandPile[l][j] = height;
                
            } else {
                sandPile[l][j] = 0;
            }
        }
    }
    
    for (int l = 0; l < rows; l++ ) {
        for (int g = 0; g < columns; g++ ) {
            if (l == x & g == y){
                next[l][g] = height-4;
                
            } else {
                next[l][g] = 0;
            }
        }
    }
    
    // Output := sandpile contents
    for (int u = 0; u < rows; u++) {
        for (int k = 0; k < columns; k++) {
            printf("sandPile[%d][%d] : %d\n",u,k, sandPile[u][k]);
        }
        printf("\n");
    }

    int m;
    int j;
    

    
    quiesce();
    
}