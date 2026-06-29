#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <pthread.h>
#include <chrono>
#include <vector>

using namespace std;


struct ThreadData1
{
  int low;
  int high;
  function<void(int)> lambda;

};

struct ThreadData2
{
  int low1,high1;
  int low2,high2;
  function <void(int,int)> lambda;
};


void* runTask1(void* arg)
{
    ThreadData1* data_p=(ThreadData1*)arg;

    int start = data_p->low;
    int end = data_p->high;

    for (int i=start;i<end;i++)
    {
        data_p->lambda(i);
    }

    return nullptr;
}


void* runTask2(void* arg)
{
  ThreadData2* data_p=(ThreadData2*)arg;

  int rowStart=data_p->low1;
  int rowEnd=data_p->high1;
  int colStart=data_p->low2;
  int colEnd=data_p->high2;

  for(int i=rowStart;i<rowEnd;i++)
  {
      for(int j=colStart;j<colEnd;j++)
      {
        data_p->lambda(i, j);
      }
  }
  return nullptr;

}





static void parallel_for(int low, int high, function<void(int)> &&lambda, int numThreads)
{
  if (numThreads<=0)
  {
    numThreads=1;
  }

  if (low>=high)
  {
    return;
  }

  auto starting=chrono::high_resolution_clock::now();

  int total=high-low;

  vector<pthread_t>threads(numThreads);
  vector<ThreadData1>data(numThreads);

  int base=(total/numThreads);
  int extra=(total%numThreads);


  int current=low;
  int count=0;

  while (count<numThreads)
  {
    int begin = current;
    int len = base;

    if (count<extra)
    {
      len++;
    }

    int x=begin+len;
    current=x;

    data[count].low=begin;
    data[count].high=x;
    data[count].lambda=lambda;

    pthread_create(&threads[count],nullptr,runTask1,&data[count]);
    count++;

  }


  for(int i=0;i<numThreads;i++)
  {
    pthread_join(threads[i],nullptr);
  }

  auto finish=chrono::high_resolution_clock::now();
  double secs=chrono::duration<double>(finish-starting).count();

  printf("Execution time (1D)- %f seconds\n",secs);

}

static void parallel_for(int low1, int high1, int low2, int high2,function<void(int,int)> &&lambda, int numThreads)
{

  if (numThreads<=0)
  {
    numThreads=1;
  }
  if (low1>=high1)
  {
    return;
  }
  if (low2>=high2)
  {
    return;
  }

  auto starting=chrono::high_resolution_clock::now();

  int total=high1-low1;

  vector<pthread_t> threads(numThreads);
  vector<ThreadData2> data(numThreads);

  int base=(total/numThreads);
  int extra=(total % numThreads);

  int current=low1;
  int count=0;

  while (count<numThreads)
  {
    int begin=current;
    int len=base;

    if (count<extra)
    {
      len++;
    }

    int x=begin+len;
    current=x;
    
    data[count].low1=begin;
    data[count].high1=x;
    data[count].low2=low2;
    data[count].high2=high2;
    data[count].lambda=lambda;

    pthread_create(&threads[count],nullptr,runTask2,&data[count]);
    count++;

  }

  for (int i=0;i<numThreads;i++)
  {
    pthread_join(threads[i],nullptr);
  }

  auto finish=chrono::high_resolution_clock::now();
  double secs=chrono::duration<double>(finish-starting).count();

  printf("Execution time (2D)- %f seconds\n", secs);

}




int user_main(int argc, char **argv);

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> && lambda) {
  lambda();
}

int main(int argc, char **argv) {
  /* 
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be 
   * explicity captured if they are used inside lambda.
   */
  int x=5,y=1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/[/*by value*/ x, /*by reference*/ &y](void) {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    std::cout<<"====== Welcome to Assignment-"<<y<<" of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);
 
  auto /*name*/ lambda2 = [/*nothing captured*/]() {
    std::cout<<"====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main


