//Shawn Diaz
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

enum status {
  QUEUED, RUNNING, IO, FIN
};

typedef struct process{
  int pid;
  int event_type;
  int arrival_time;
  int burst_time;

  int burst_remaining;
  int quantum_remaining;

  int start_time;
  int wait_time;
  int end_time;

  int status;
} process_t;

typedef struct node {
  process_t *proc;
  struct node *next;
} node_t;


process_t **procs;
process_t *running = NULL;
node_t *proc_queue;

int num_procs = 0;
int fin_procs = 0;

int time_quantum = 0;
int context_switch_time = 0;

int cur_time = 0;
int cpu_idle = 0;
int cpu_busy = 0;

void sort_arrival_times(process_t *(**procs)) {
  int j, i, min;
  process_t *temp;

  for(i = 0; i < num_procs-1; i++) {
    min = i;
    for(j = i+1; j < num_procs; j++) {
      if((*procs)[j]->arrival_time < (*procs)[min]->arrival_time) {
        min = j;
      }
    }
    temp = (*procs)[min];
    (*procs)[min] = (*procs)[i];
    (*procs)[i] = temp;
  }
}

void free_procs() {
  int i;
  for(i = 0; i < num_procs; i++)
    free(procs[i]);
  
  if(proc_queue)
    free(proc_queue);
}

void enqueue(node_t **head, process_t *p) {
  node_t *new_node = malloc(sizeof(node_t));
  new_node->proc = p;
  new_node->next = NULL;
  
  if(*head == NULL) {
    *head = new_node;
    return;
  }

  node_t *cur_node = *head;

  while(cur_node->next)
    cur_node = cur_node->next;

  new_node->next = cur_node->next;
  cur_node->next = new_node;
}

process_t *dequeue(node_t **head) {
  node_t *cur_node = *head;

  if(!cur_node) {
    return NULL;
  }

  process_t *p = cur_node->proc;
  *head = (*head)->next;
  free(cur_node);

  return p;
}

int check_arrived(node_t **head) {
  node_t *cur_node = *head;
  if(!cur_node)
    return 0;

  if(cur_node->proc->arrival_time < cur_time)
    return 1;
}

void print_next_frame() {
  if(running) {
    printf("Time %d P%d runs\n", cur_time, running->pid);
  }

}

void get_processes(char* filename) {
  int i;  
  char ch;
  char buff[255];
  FILE* fp = NULL;

  fp = fopen(filename, "r");
  if(fp == NULL) {
    printf("fopen() error.\n");
    exit(1);
  }

  while((ch=fgetc(fp)) != EOF) {
    if(ch == '\n')
      num_procs++;
  }
  rewind(fp);

  procs = calloc(1, num_procs * sizeof(process_t));
  if(procs == NULL) {
    printf("malloc() error.\n");
    exit(1);
  }
  //(*procs)[1].pid = 1;

  //printf("\n\n\nHERE: procs[1]->pid = %d\n", (*procs)[1]->pid);

  for(i = 0; i < num_procs; i++) {
    fgets(buff, sizeof(buff), fp); 
    process_t *proc = calloc(1, sizeof(process_t));

    sscanf(buff, "%d %d %d %d", 
            &proc->pid,
            &proc->event_type,
            &proc->arrival_time,
            &proc->burst_time);

    procs[i] = proc;
    
    //enqueue(&proc_queue, proc);
    //num_procs++;
  }

  //sort by arrival time
  sort_arrival_times(&procs);

  //place in queue
  /*node_t *cur_proc = proc_queue;
  
  i = 0;
  while(cur_proc) {
    procs[i] = cur_proc->proc;
    cur_proc = cur_proc->next;
    i++;
  }*/
  for(i = 0; i < num_procs; i++) {
    enqueue(&proc_queue, procs[i]);
  }

  fclose(fp);
}

process_t *serve_next_process() {
  process_t *p = NULL;
  int arrived = check_arrived(&proc_queue);

  if(arrived) {
    if((p = dequeue(&proc_queue))) {
      if(cur_time >= p->arrival_time) {
        printf("SERVED NEXT PROCESS: P%d\n", p->pid);
        p->status = RUNNING;
        if(!p->start_time) {
          p->start_time = cur_time;
        }

        if(p->burst_remaining <= 0) {
          p->burst_remaining = p->burst_time;
        }

        p->quantum_remaining = time_quantum;
      }
      
    }
    return p; //set to running
  }
}

void sim() {

  for(;;) {

    if(running) {
      //printf("Time %d P%d runs\n", cur_time, running->pid);
      running->burst_remaining--;
      running->quantum_remaining--;

      //CPU burst done, serve new process
      if(running->burst_remaining == 0) {
        printf("P%d finishes at time %d\n", running->pid, cur_time);
        running->status = FIN;
        running->end_time = cur_time;
        fin_procs++;
        running = serve_next_process();
      }
      //quantum expired, put at back of queue, serve new process
      else if(running->quantum_remaining == 0) {
        printf("QUANTUM EXPIRED\n");
        enqueue(&proc_queue, running);
        running = serve_next_process();
      }
      cpu_busy++;
    }
    //idle time, try serving process in queue
    else {
      if(!(running = serve_next_process())) {
        //printf("SERVING idle\n");
        cpu_idle++;
      }
    }

    //processes not running
    int i;
    for(i = 0; i < num_procs; i++) {
      process_t *p =  procs[i];

      if(p->arrival_time == cur_time) {
        printf("Time %d P%d arrives\n", cur_time, procs[i]->pid);
      }

      if(p->status == QUEUED){
        p->wait_time++;
      }
    }

    if(fin_procs < num_procs) {
      //printf("finished_processes: %d\n", fin_procs);
      print_next_frame();
    }
    else break;

    cur_time++;
  }

  free_procs();
}

int main(int argc, char** argv)
{
  if(argc != 4 ) {
    printf("./simulator <timequantum> <contextswitchtime> <filename>\n");
    exit(1);
  }

  time_quantum = atoi(argv[1]);
  context_switch_time = atoi(argv[2]);
  char* filename = argv[3];

  printf("time_quantum: %d, context_switch_time: %d, filename: %s\n", 
              time_quantum, context_switch_time, filename);

  get_processes(filename); 
  printf("number of processes: %d\n", num_procs);
  printf("----------------------------------------------\n");

  sort_arrival_times(&procs);

  int i;
  for(i = 0; i < num_procs; i++)
    printf("pid: %d, arrival_time: %d\n", procs[i]->pid, procs[i]->arrival_time);
  printf("----------------------------------------------\n");
  sim();

  free(procs); 
  exit(0); 
}
