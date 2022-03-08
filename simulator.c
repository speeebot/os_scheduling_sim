//Shawn Diaz
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

enum status {
  QUEUED, RUNNING, IO, FINISHED
};

enum event {
  ARRIVES, RUNS, FINISHES, END
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
  int arrived;
} process_t;

typedef struct node {
  process_t *proc;
  struct node *next;
} node_t;

typedef struct report {
  char* msg;
  int t;
} report_t;


process_t **procs;
process_t *running = NULL;
node_t *proc_queue;
report_t **reports;

int num_reports;

int num_procs = 0;
int finished_procs = 0;

int time_quantum = 0;
int context_switch_time = 0;
int context_switch_count = 0;

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

void free_mem() {
  int i;
  for(i = 0; i < num_procs; i++)
    free(procs[i]);
  
  if(proc_queue)
    free(proc_queue);
  
  for(i = 0; i < num_reports; i++)
    free(reports[i]);

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

int has_arrived(node_t **head) {
  int ret;

  if(*head == NULL) {
    return 0;
  }
  //if process arrives later than the current time (0)
  if((*head)->proc->arrival_time > cur_time) {
    ret = 0; 
  }
  //process has arrived already (1)
  else {
    ret = 1; 
  }
  return ret;
}

void summary() {
  double summed_turnaround_time = 0.0;
  double summed_wait_time = 0.0;
  printf("\n---------------SUMMARY-------------------\n");
  int i;
  for (i = 0; i < num_procs; i++) {
    summed_turnaround_time += (double)(procs[i]->end_time 
                              - procs[i]->arrival_time);
                                
    summed_wait_time += (double)(procs[i]->wait_time);
            
    
    printf("P%d: AT=%d ST=%d ET=%d TT=%.2f WT=%d\n", 
          procs[i]->pid, 
          procs[i]->arrival_time, 
          procs[i]->start_time, 
          procs[i]->end_time, 
          (double)(procs[i]->end_time - procs[i]->arrival_time), 
          procs[i]->wait_time);
  }

  printf("\naverage turnaround time: %.2f\n", summed_turnaround_time / (double)num_procs);
  printf("average wait time: %.2f\n",  summed_wait_time / (double)num_procs);
  printf("cpu busy time: %d\n", cpu_busy);
  printf("cpu idle time: %d\n", cpu_idle);
  printf("context switch count %d\n", context_switch_count);
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
    printf("calloc() error.\n");
    exit(1);
  }

  for(i = 0; i < num_procs; i++) {
    process_t *proc = calloc(1, sizeof(process_t));
    if(proc == NULL) {
      printf("calloc() error.\n");
      exit(1);
    }

    fgets(buff, sizeof(buff), fp); 
    sscanf(buff, "%d %d %d %d", 
            &proc->pid,
            &proc->event_type,
            &proc->arrival_time,
            &proc->burst_time);

    procs[i] = proc;
  }

  //sort by arrival time
  sort_arrival_times(&procs);

  fclose(fp);
}

process_t *serve_next_process() {
  process_t *p = NULL;
  if((p = dequeue(&proc_queue))) {
    p->status = RUNNING;
    if(!p->start_time) {
      p->start_time = cur_time;
    }
    if(p->burst_remaining <= 0) {
      p->burst_remaining = p->burst_time;
    }
    p->quantum_remaining = time_quantum;
  }

  return p; //set to running
}

void report_event(int event, process_t **p) {
  int display_time = cur_time+(context_switch_time*context_switch_count);
  context_switch_count++;
  num_reports++;


  if(realloc(reports, num_reports * sizeof(report_t)) == NULL) {
    printf("realloc() error.\n");
    exit(1);
  }

  report_t *report = calloc(1, sizeof(report_t));
  if(report == NULL) {
    printf("clloc() error.\n");
    exit(1);
  }

  if(event == ARRIVES) {
    size_t buff_size = snprintf(NULL, 0, "Time %d P%d arrives", cur_time, (*p)->pid) + 1;

    reports[num_reports-1]->msg = malloc(buff_size * sizeof(char*));
    if(report->msg == NULL) {
      printf("malloc() error.\n");
      exit(1);
    }
    sprintf(reports[num_reports-1]->msg, "Time %d P%d arrives", cur_time, (*p)->pid);
    printf("%s\n", report->msg);
    reports[num_reports-1]->t = cur_time;
  }
  
  if(event == RUNS) {
    size_t buff_size = snprintf(NULL, 0, "Time %d P%d runs", display_time, (*p)->pid) + 1;

    report->msg = malloc(buff_size);
    if(report->msg == NULL) {
      printf("malloc() error.\n");
      exit(1);
    }
    sprintf(report->msg, "Time %d P%d runs", display_time, (*p)->pid);
    printf("%s\n", report->msg);
    report->t = cur_time;
  }

  if(event == FINISHES) {
    size_t buff_size = snprintf(NULL, 0, "Time %d P%d finishes", display_time, (*p)->pid) + 1;

    report->msg = malloc(buff_size);
    if(report->msg == NULL) {
      printf("malloc() error.\n");
      exit(1);
    }
    sprintf(report->msg, "Time %d P%d finishes", display_time, (*p)->pid);
    printf("%s\n", report->msg);
    report->t = cur_time;
  }
}

int qsort_compare(const void *a, const void *b) {
  report_t *report_a = (report_t *)a;
  report_t *report_b = (report_t *)b;

  return (report_a->t - report_b->t);
}

void print_report() {

  //qsort(reports, num_procs, sizeof(report_t), qsort_compare);

  int i;
  for(i = 0; i < num_reports; i++) {
    printf("%d\n\n\n\n", reports[i]->t);
  }
}

void sim() {

  for(;;) {
/*--------------------------------processes not running-------------------------------------*/
    int i;
    for(i = 0; i < num_procs; i++) {
      process_t *p =  procs[i];

      //process arrives, but doesn't run
      if(p->arrival_time == cur_time) {
        enqueue(&proc_queue, procs[i]);
        p->arrived = true;
        p->status = QUEUED;

        report_event(ARRIVES, &p);
        if(running != NULL) {
          report_event(RUNS, &running);
        }
      }
    }

    if(running) {
      running->burst_remaining--;
      running->quantum_remaining--;
/*--------------------------------CPU burst done, serve new process------------------------------*/
//TODO: what if CPU burst is done but next process in queue isnt arrived yet
      if(running->burst_remaining <= 0) {
        report_event(FINISHES, &running);
        running->status = FINISHED;
        running->end_time = cur_time + (context_switch_count*context_switch_time);
        finished_procs++;
        
  /*set finished process, now grab next process in queue*/
          running = serve_next_process(); //set running if arrived
          if(running != NULL) {
            report_event(RUNS, &running);
          }
      }
/*------------------quantum expired, put at back of queue, serve new process--------------------*/
      else if(running->quantum_remaining <= 0) {
        enqueue(&proc_queue, running); //place at back of queue
        running->status = QUEUED;
  /*set queued process, now grab next process in queue if its arrived*/
        running = serve_next_process(); //set process to running
        if(running != NULL) {
          report_event(RUNS, &running);
        }
      }
      cpu_busy++;
    }
/*------------idle time (no processes running), try serving process in queue-------------------*/
    else {
      if(!has_arrived(&proc_queue)) {
        cpu_idle++;
      }
      running = serve_next_process();
      if(running != NULL) {
        report_event(RUNS, &running);
      }
    }

    //if process has arrived and is queued
    for(i = 0; i < num_procs; i++) {
      process_t *p =  procs[i];
      if(p->status == QUEUED && p->arrived){ 
        p->wait_time++;
      }
    }

    if(finished_procs == num_procs) {
      //report_event(END, &running);
      break;
    }
    else
      cur_time++; //increment time frame by one
  }

  print_report();
  summary();

  free_mem();
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
  printf("\n-------------SORTED ARRIVAL TIMES-------------\n");
  int i;
  for(i = 0; i < num_procs; i++)
    printf("pid: %d, arrival_time: %d, burst_time: %d\n", 
            procs[i]->pid, procs[i]->arrival_time, procs[i]->burst_time);
  printf("----------------------------------------------\n\n");

  sim();

  free(procs); 
  exit(0); 
}
