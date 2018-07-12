#include <cstdio>
#include <pthread.h>
#include <cstdlib>
#include "ConcurrentList.h"

using namespace std;

bool is_correct(ConcurrentList & list, size_t s) {
  if(list.size() != s)
    return false;

  size_t count = 0;

  ConcurrentList::node_t* head = list.getHead();

  while(1) {
    head = list.getNext(head);
    if(!head) break;

    if(head->status == ConcurrentList::OBSOLETE)
      continue;

    count++;
    printf("%d\n", head->elem);
  }

  return count == s;
}

void* thread_main(void* args) {
  long* args_ary = (long*) args;

  long tid = (long)args_ary[0];
  ConcurrentList* list = (ConcurrentList*) args_ary[1];

  //printf("Thread %ld start.\n", tid);

  for(int i = 0; i < 100; i++) {
    list->insert(tid * 1000000 + i);
  }
  
  /* ConcurrentList::node_t* head = list->getHead();
   * for(int i = 0; i < 10; i++) {
   *   list->erase(head);
   * } */

  //printf("Thread %ld end.\n", tid);

  delete[] args_ary;
  pthread_exit((void *) 0);
}

#define N_THREAD 100
pthread_t threads[N_THREAD];

void test(ConcurrentList &list) {
  for(int i = 0; i < 10; i++) {
    list.insert(i);
  }

  ConcurrentList::node_t *head = list.getHead();

  head = list.getNext(head);
  head = list.getNext(head);

  list.erase(head);
  head = list.getNext(head);
  list.erase(head);
  head = list.getNext(head);

  printf("\n%ld\n", list.size());
  printf("%s\n", is_correct(list, 10-2) ? "Correct\n" : "Incorrect\n");

  exit(0);
}

int main(void)
{
  ConcurrentList list;
  // test(list);

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

  // Check consistency
  assert(list.size() == N_THREAD * 100);
  printf("%ld\n", list.size());
  printf("%s\n", is_correct(list, list.size()) ? "Correct\n" : "Incorrect\n");

  return 0;
}