//Shawn Diaz
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define EMPTY_QUEUE INT_MIN

int num_procs = 0;

typedef struct Process{
  int pid;
  int event_type;
  int arrival_time;
  int burst_time;
  int remaining_time;
  int completion_time;
  bool is_running;
} Process;

typedef struct node {
  Process proc;
  struct node *next;
} node;

typedef struct {
  node *head;
  node *tail;
} queue;

node* new_node(Process p) {
  node* temp = malloc(sizeof(node));
  temp->proc = p;
  temp->next = NULL;
  return temp;
}

queue* init_queue() {
  queue* q = malloc(sizeof(queue));
  q->head = q->tail = NULL;
  return q;
}

void enqueue(queue *q, Process p) {
  //create new node
  node *temp = new_node(p);
  if(q->tail == NULL) {
    q->head = q->tail = temp;
    return;
  }
  q->tail->next = temp;
  q->tail = temp;
}

void dequeue(queue *q) {
  if(q->head == NULL)
    return;
  
  node* temp = q->head;
  q->head = q->head->next;

  if(q->head == NULL)
    q->tail = NULL;
  
  free(temp);
}

void sort_arrival_times(Process **procs) {
  int j, i, min;
  Process temp;

  for(i = 0; i < num_procs-1; i++) {
    min = i;
    for(j = i+1; j < num_procs; j++) {
      if((*procs)[j].arrival_time < (*procs)[min].arrival_time) {
        min = j;
      }
    }
    temp = (*procs)[min];
    (*procs)[min] = (*procs)[i];
    (*procs)[i] = temp;
  }
}

void get_processes(Process **procs, char* filename) {
  int i, min;
  char buff[255];
  FILE* fp = NULL;

  fp = fopen(filename, "r");
  if(fp == NULL) {
    printf("fopen() error.\n");
    exit(1);
  }

  char ch;
  while((ch=fgetc(fp)) != EOF) {
    if(ch == '\n')
      num_procs++;
  }
  rewind(fp);

  *procs = malloc(num_procs * sizeof(*procs));
  if(procs == NULL) {
    printf("malloc() error.\n");
    exit(1);
  }
  
  for(i = 0; i < num_procs; i++) {
    fgets(buff, sizeof(buff), fp);
    sscanf(buff, "%d %d %d %d", &(*procs)[i].pid, &(*procs)[i].event_type, 
                                &(*procs)[i].arrival_time, &(*procs)[i].burst_time);
    (*procs)[i].remaining_time = (*procs)[i].burst_time;
  }
  fclose(fp);
}

int main(int argc, char** argv)
{
  if(argc < 4 || argc > 4) {
    printf("args: simulator timequantum contextswitchtime filename\n");
    exit(1);
  }

  int i, j;
  int time_quantum = atoi(argv[1]);
  int context_switch_time = atoi(argv[2]);
  char* filename = argv[3];

  printf("time_quantum: %d, context_switch_time: %d, filename: %s\n", 
              time_quantum, context_switch_time, filename);

  Process *procs;

  get_processes(&procs, filename); 
  printf("number of processes: %d\n", num_procs);

  //sort processes by arrival_time
  sort_arrival_times(&procs);

  //calculate completion times
  /*for(i = 0; i < num_procs; i++) {
    procs[i].completion_time = procs[i].burst_time - procs[i].arrival_time; //TODO: doesnt work (LOOK AT GANTT CHART)
  }*/

  printf("----------------------------------------SORTED BY ARRIVAL TIME-----------------------------------------\n");
  for(i = 0; i < num_procs; i++) {
    printf("process_id: %d, event_type: %d, arrival_time: %d, burst_time: %d, remaining_time: %d, completion_time: %d\n", 
          procs[i].pid, procs[i].event_type, 
          procs[i].arrival_time, procs[i].burst_time, procs[i].remaining_time, procs[i].completion_time);
  }
  printf("--------------------------------------------------------------------------------------------------------\n");
  //place in queue by arrival time
  queue *wait_queue = init_queue();
  for(i = 0; i < num_procs; i++) {
    enqueue(wait_queue, procs[i]);
    printf("P%d \n", wait_queue->tail->proc.pid);
  }

  for(i = 0; i < num_procs; i++) {
    printf("P%d \n", wait_queue->head->proc.pid);
    dequeue(wait_queue);
  }
/*
  for(i = 0; i < num_procs; i++) {
    procs[i].is_running = false;
  }

  int cur_time = 0;
  int procs_remaining = num_procs;
  Process p = wait_queue->head->proc; //first process in queue

  //FCFS algorithm REMEMBER time quantum and context switch time
  for(;;) { 
    //check for arrivals every time_frame
    if(cur_time == p.arrival_time) {
      printf("Time %d P%d arrives\n", cur_time, p.pid);
      //procs[i].is_running = true; //just because process arrives, doesnt mean it starts running (WE NEED A QUEUE???????)
    }

    //if process has arrived and is next in queue
    if(p.arrival_time <= cur_time) {
      p.is_running = true;
    }
  

    //process running every time frame
    if(p.is_running) {
      p.remaining_time--;
    }
    
    //check for process remaining_time == 0 (finished)
    if(p.remaining_time == 0) {
      printf("dequeueing\n");
      dequeue(wait_queue);
      p = wait_queue->head->proc;
    }
    printf("next: %d", p.pid);

    cur_time++;
    if(cur_time == 112)
      break;
  }
*/
  free(procs); 
  exit(0); 
}
