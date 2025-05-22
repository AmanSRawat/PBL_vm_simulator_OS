#include "simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void simulator_init(Simulator *sim) {
    sim->page_size = 4096;
    sim->process_count = 1;
    memset(sim->process_sizes, 0, sizeof(sim->process_sizes));
    strcpy(sim->algorithm, "lru");
    memset(sim->memory, 0, sizeof(sim->memory));
    sim->memory_usage = 0;
}

void simulator_set_algorithm(Simulator *sim, const char *algorithm) {
    strncpy(sim->algorithm, algorithm, sizeof(sim->algorithm) - 1);
    sim->algorithm[sizeof(sim->algorithm) - 1] = '\0';
}

void simulator_set_page_size(Simulator *sim, int page_size) {
    sim->page_size = page_size > 0 ? page_size : 4096;
}

void simulator_set_process_count(Simulator *sim, int count) {
    sim->process_count = (count > 0 && count <= MAX_PROCESSES) ? count : 1;
}

void simulator_set_process_size(Simulator *sim, int index, int size) {
    if (index >= 0 && index < MAX_PROCESSES)
        sim->process_sizes[index] = size;
}

static int is_page_in_memory(Simulator *sim, int pid, int page) {
    for (int i = 0; i < sim->memory_usage; i++) {
        if (sim->memory[i].process_id == pid && sim->memory[i].page == page)
            return sim->memory[i].frame;
    }
    return -1;
}

static int evict_page(Simulator *sim, char *log_output) {
    PageFrame victim = sim->memory[0];
    for (int i = 1; i < sim->memory_usage; i++) {
        sim->memory[i - 1] = sim->memory[i];
    }
    sim->memory_usage--;

    char msg[64];
    snprintf(msg, sizeof(msg), "Page-Out: Process %d Page %d from Frame %d\n",
             victim.process_id, victim.page, victim.frame);
    strcat(log_output, msg);
    return victim.frame;
}

static void load_page(Simulator *sim, int pid, int page, char *log_output) {
    int frame = (sim->memory_usage < FRAME_COUNT) ? sim->memory_usage : evict_page(sim, log_output);
    sim->memory[sim->memory_usage++] = (PageFrame){pid, page, frame};

    char msg[64];
    snprintf(msg, sizeof(msg), "Page-In:  Process %d Page %d -> Frame %d\n", pid, page, frame);
    strcat(log_output, msg);
}

void simulator_run(Simulator *sim, char *log_output) {
    sim->memory_usage = 0;
    for (int pid = 0; pid < sim->process_count; pid++) {
        int num_pages = (sim->process_sizes[pid] * 1024) / sim->page_size;
        for (int page = 0; page < num_pages; page++) {
            if (is_page_in_memory(sim, pid, page) == -1) {
                load_page(sim, pid, page, log_output);
            } else {
                char msg[64];
                snprintf(msg, sizeof(msg), "Access:   Process %d Page %d (In Memory)\n", pid, page);
                strcat(log_output, msg);
            }
        }
    }
}