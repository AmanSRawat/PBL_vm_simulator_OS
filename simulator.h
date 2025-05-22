#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <gtk/gtk.h>

#define MAX_PROCESSES 10
#define FRAME_COUNT 4

typedef struct {
    int process_id;
    int page;
    int frame;
} PageFrame;

typedef struct {
    int page_size;
    int process_count;
    int process_sizes[MAX_PROCESSES];
    char algorithm[10];
    PageFrame memory[FRAME_COUNT];
    int memory_usage;
} Simulator;

void simulator_init(Simulator *sim);
void simulator_set_algorithm(Simulator *sim, const char *algorithm);
void simulator_set_page_size(Simulator *sim, int page_size);
void simulator_set_process_count(Simulator *sim, int count);
void simulator_set_process_size(Simulator *sim, int index, int size);
void simulator_run(Simulator *sim, char *log_output);

#endif // SIMULATOR_H