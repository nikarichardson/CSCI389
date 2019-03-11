//
//  d-way heap.c
//  
//
//

#include "dwayheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <strings.h> 


int size;
int l = 0;

Heap create_heap(int n, int d)
{
    Heap myheap;
    myheap.d = d;
    myheap.size = n;
    myheap.data = (int*)malloc(sizeof(int) * n);
    
    return myheap;
}



void inserth(Heap *h, int v, int d)
{
    int i = (l + 1);
    int t = h->data[i];
    int pindex = (i-1)/d;
    h->data[i] = v;
    l ++;
    
    while (h->data[pindex] < h->data[i]){
        int k = h->data[pindex];
        h->data[pindex] = h->data[i];
        h->data[i] = k;
        i = pindex;
        pindex = (i-1)/d;
    }
    
}

void printHeap(Heap *h){
    printf("\n\nPrinting the heap...\n ");
    printf("\n");
    for (int i=0; i < (h->size); i++){
        printf(" %d ", h->data[i]);
    }
    printf("\n");
    
}


Heap randominsert(Heap *h, int n, int d)
{
    // empty array
    int* arr = malloc(n * sizeof(int));
    
    int k = n-1;

    // initial range of numbers
    for(int i=0;i < n;++i){
        arr[i]=i+1;
    }

    
    // seed random number generator
    srand(time(NULL));
    int j;
    
    // mix up numbers
    for (int i = n-1; i >= 0; --i){
        j = rand() % (i +1);
        int m = arr[i];
        arr[i] = arr[j];
        arr[j] = m;
    }
    
    // output; printing array elements
    for (int i = 0; i <= k; i++) {
        printf("\nheap[%d] = %d", i, arr[i]);
    }
    
    // first value of the heap
    h->data[0] = arr[0];
    l = 0; // index of this value
    
    // insert elements one by one into heap
    for (int i = 1; i <= k; i++) {
        inserth(h, arr[i], d);
    }
    
    return *h;
    
}


int main()

{
    clock_t begin = clock();
    int ways;
    printf("\nChoose a value of n: ");
    scanf("%d", &size);
    
    // get value of d from user; determines the number of 'ways' for heap
    printf("Choose a value of d: ");
    scanf("%d", &ways);
                
    // create heap
    Heap iheap = create_heap(size, ways);

    Heap fheap = randominsert(&iheap, size, ways);
    
    printHeap(&fheap);
    
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("\n Time spent: %f s\n \n", time_spent);
    return 0;
}




