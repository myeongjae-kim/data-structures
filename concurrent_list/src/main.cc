#include <cstdio>
#include <pthread.h>
#include <cstdlib>
#include <vector>
#include "ConcurrentList.h"

using namespace std;

bool is_correct(ConcurrentList & list, size_t s) {
  if(list.size() != s)
    return false;

  size_t count = 0;
  size_t actual_size = 0;

  ConcurrentList::node_t* head = list.getHead();

  while(1) {
    head = list.getNext(head);
    if(!head) break;

    actual_size++;
    if(head->status == ConcurrentList::OBSOLETE)
      continue;

    count++;
    // printf("%d\n", head->elem);
  }
  printf("Actual nodes: %ld\n", actual_size);

  return count == s;
}

void* thread_main(void* args) {
  long* args_ary = (long*) args;

  long tid = (long)args_ary[0];
  ConcurrentList* list = (ConcurrentList*) args_ary[1];

  std::vector<ConcurrentList::node_t*> nodes;

  //printf("Thread %ld start.\n", tid);

  for(int i = 0; i < 100; i++) {
    nodes.push_back(list->push_back(tid * 1000000 + i));
  }
  
  //printf("Thread %ld end.\n", tid);
  
  int cnt = 0;
  for(auto it = nodes.rbegin();
      cnt < 10;
      // it != nodes.rend();
      cnt++, it++) {
    list->erase(*it);
  }

  delete[] args_ary;
  pthread_exit((void *) 0);
}

#define N_THREAD 500
pthread_t threads[N_THREAD];

int main(void)
{
  ConcurrentList list;

  int tid = 1;

  for(int i = 0; i < N_THREAD; i++) {
    long* args = new long[2];
    args[0] = tid++;
    args[1] = (long)&list;

    pthread_create(&threads[i], NULL, thread_main, (void *)args);
  }

  for(int i = 0; i < N_THREAD; i++) {
    pthread_join(threads[i], NULL);
  }

  list.next_pointer_update();

  // Check consistency
  printf("Active nodes: %ld\n", list.size());
  printf("%s\n", is_correct(list, list.size()) ? "Correct\n" : "Incorrect\n");

  return 0;
}
