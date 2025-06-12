#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <gtk/gtk.h>

#define MAX_PROCESSES 10
#define FRAME_COUNT 4

#define MAX_REFERENCES 500000 // Maximum total page accesses in a simulation run

typedef struct {
    int process_id;
    int page;
    int frame;
    long last_access_time; 
} PageFrame;

// Structure to hold a page reference for the Optimal algorithm
typedef struct {
    int pid;
    int page_num;
} PageReference;

typedef struct {
    int page_size;
    int process_count;
    int process_sizes[MAX_PROCESSES]; 
    char algorithm[10];
    PageFrame memory[FRAME_COUNT];
    int memory_usage;

    // For Optimal Algorithm:
    PageReference reference_string[MAX_REFERENCES];
    int reference_string_len;
} Simulator;

void simulator_init(Simulator *sim);
void simulator_set_algorithm(Simulator *sim, const char *algorithm);
void simulator_set_page_size(Simulator *sim, int page_size);
void simulator_set_process_count(Simulator *sim, int count);
void simulator_set_process_size(Simulator *sim, int index, int size);
void simulator_run(Simulator *sim, char *log_output);

#endif 