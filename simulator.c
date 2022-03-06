//Shawn Diaz
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

typedef struct Process{
  int pid;
  int event_type;
  int arrival_time;
  int burst_time;
  int remaining_time;
  int completion_time;
  int is_running;
} Process;

int num_procs = 0;

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

  int i = 0;
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
  for(i = 0; i < num_procs; i++) {
    procs[i].completion_time = procs[i].burst_time - procs[i].arrival_time; //TODO: doesnt work (LOOK AT GANTT CHART)
  }

  printf("-----------------------SORTED BY ARRIVAL TIME---------------------------------\n");
 
  for(i = 0; i < num_procs; i++) {
    printf("process_id: %d, event_type: %d, arrival_time: %d, burst_time: %d, remaining_time: %d, completion_time: %d\n", 
          procs[i].pid, procs[i].event_type, 
          procs[i].arrival_time, procs[i].burst_time, procs[i].remaining_time, procs[i].completion_time);
  }


  for(i = 0; i < num_procs; i++) {
    procs[i].is_running = 0;
  }

  int cur_time = 0;
  int procs_remaining = num_procs;

  //FCFS algorithm REMEMBER time quantum and context switch time
  while(1) { 
    //process arrives/finishes at this time frame
    for(i = 0; i < num_procs; i++) {    
      if(cur_time == procs[i].arrival_time) 
        printf("Time %d P%d arrives\n", cur_time, procs[i].pid);
      if(procs[i].remaining_time == 0)
        printf("Time %d P%d finishes\n", cur_time, procs[i].pid);

      //process runs (a process is always running unless there is a context switch)
      while(procs[i].remaining_time != 0 && cur_time % time_quantum != 0) {
        procs[i].remaining_time--;
        cur_time++;
      }
    }
    procs_remaining--;
    if(procs_remaining == 0) 
      break;
  }

  free(procs); 
  exit(0); 
}
