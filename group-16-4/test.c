#include <stdio.h>

#define SIZE 6
#define ITERATIONS 300

float A[SIZE+2];
float B[SIZE+2];

void print_array(float arr[], int size) 
{
    for (int i = 0; i < size; i++) 
    {
        printf("%.4f ", arr[i]);
    }
    printf("\n");
}

void iterative_averaging() 
{
    A[SIZE+1] = 1;
    B[SIZE+1] = 1;
    print_array(A, SIZE+2);
    print_array(B, SIZE+2);
    
    int count=0;
    for (int iter = 0; iter < ITERATIONS; iter++) 
    {
        
        for (int j = 1; j <= SIZE; j++) 
        {
            B[j] = (A[j-1] + A[j+1]) / 2.0;
        }
        // Copy B into A
        for (int i = 0; i <= SIZE + 1; i++)
            A[i] = B[i];
    }



    printf("Final arrays after %d iterations:\n", ITERATIONS);
    print_array(A, SIZE+2);
    print_array(B, SIZE+2);
}

int main() 
{
    iterative_averaging();
    return 0;
}
