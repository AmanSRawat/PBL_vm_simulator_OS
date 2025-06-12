#include "simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static long global_access_time = 0; // Used by LRU to track access order

// Initializes the simulator with default settings
void simulator_init(Simulator *sim) {
    sim->page_size = 4096;
    sim->process_count = 1;
    memset(sim->process_sizes, 0, sizeof(sim->process_sizes));
    strcpy(sim->algorithm, "lru");
    memset(sim->memory, 0, sizeof(sim->memory));
    sim->memory_usage = 0;
    sim->reference_string_len = 0;
    global_access_time = 0;
}

// Sets the page replacement algorithm
void simulator_set_algorithm(Simulator *sim, const char *algorithm) {
    strncpy(sim->algorithm, algorithm, sizeof(sim->algorithm) - 1);`
    sim->algorithm[sizeof(sim->algorithm) - 1] = '\0';
}

// Sets the page size (default fallback is 4096)
void simulator_set_page_size(Simulator *sim, int page_size) {
    sim->page_size = page_size > 0 ? page_size : 4096;
}

// Sets the number of processes
void simulator_set_process_count(Simulator *sim, int count) {
    sim->process_count = (count > 0 && count <= MAX_PROCESSES) ? count : 1;
}

// Sets the memory size required by a specific process
void simulator_set_process_size(Simulator *sim, int index, int size) {
    if (index >= 0 && index < MAX_PROCESSES)
        sim->process_sizes[index] = size;
}

// Checks if a page is in memory; updates access time if LRU
static int is_page_in_memory(Simulator *sim, int pid, int page) {
    for (int i = 0; i < sim->memory_usage; i++) {
        if (sim->memory[i].process_id == pid && sim->memory[i].page == page) {
            if (strcmp(sim->algorithm, "lru") == 0) {
                sim->memory[i].last_access_time = global_access_time;
            }
            return sim->memory[i].frame;
        }
    }
    return -1;
}

// Applies selected page replacement algorithm and evicts a page
static int evict_page(Simulator *sim, char *log_output, int current_ref_idx) {
    int victim_frame_idx = -1;
    char msg[100];

    if (strcmp(sim->algorithm, "fifo") == 0) {
        // FIFO: Evict the first page loaded
        victim_frame_idx = 0;

    } else if (strcmp(sim->algorithm, "lru") == 0) {
        // LRU: Evict the page with the smallest access time
        long min_access_time = LONG_MAX;
        for (int i = 0; i < sim->memory_usage; i++) {
            if (sim->memory[i].last_access_time < min_access_time) {
                min_access_time = sim->memory[i].last_access_time;
                victim_frame_idx = i;
            }
        }

    } else if (strcmp(sim->algorithm, "optimal") == 0) {
        // Optimal: Evict the page with the farthest next use or never used again
        int farthest_future_use = -1;
        int page_to_evict_mem_idx = -1;

        for (int i = 0; i < sim->memory_usage; i++) {
            int pid = sim->memory[i].process_id;
            int page = sim->memory[i].page;
            int next_use_idx = INT_MAX;

            for (int k = current_ref_idx + 1; k < sim->reference_string_len; k++) {
                if (sim->reference_string[k].pid == pid &&
                    sim->reference_string[k].page_num == page) {
                    next_use_idx = k;
                    break;
                }
            }

            if (next_use_idx == INT_MAX) {
                page_to_evict_mem_idx = i;
                break;
            } else if (next_use_idx > farthest_future_use) {
                farthest_future_use = next_use_idx;
                page_to_evict_mem_idx = i;
            }
        }
        victim_frame_idx = page_to_evict_mem_idx;
    } else {
        // Fallback: default to FIFO
        victim_frame_idx = 0;
    }

    PageFrame victim = sim->memory[victim_frame_idx];
    snprintf(msg, sizeof(msg), "Page-Out: Process %d Page %d from Frame %d (Algorithm: %s)\n",
             victim.process_id, victim.page, victim.frame, sim->algorithm);
    strcat(log_output, msg);

    int freed_frame = victim.frame;

    // Remove victim from memory and shift remaining frames
    for (int i = victim_frame_idx; i < sim->memory_usage - 1; i++) {
        sim->memory[i] = sim->memory[i + 1];
    }
    sim->memory_usage--;

    return freed_frame;
}

// Loads a page into memory; triggers eviction if memory full
static void load_page(Simulator *sim, int pid, int page, char *log_output, int current_ref_idx) {
    int frame;
    char msg[100];

    if (sim->memory_usage < FRAME_COUNT) {
        frame = sim->memory_usage;
    } else {
        // Perform eviction using selected page replacement algorithm
        frame = evict_page(sim, log_output, current_ref_idx);
    }

    sim->memory[sim->memory_usage].process_id = pid;
    sim->memory[sim->memory_usage].page = page;
    sim->memory[sim->memory_usage].frame = frame;
    sim->memory[sim->memory_usage].last_access_time = global_access_time;

    sim->memory_usage++;

    snprintf(msg, sizeof(msg), "Page-In:  Process %d Page %d -> Frame %d\n", pid, page, frame);
    strcat(log_output, msg);
}

// Executes the simulation of memory accesses with page replacement
void simulator_run(Simulator *sim, char *log_output) {
    sim->memory_usage = 0;
    memset(sim->memory, 0, sizeof(sim->memory));
    global_access_time = 0;
    sim->reference_string_len = 0;

    // Generate reference string based on process sizes
    for (int pid = 0; pid < sim->process_count; pid++) {
        int num_pages = (sim->process_sizes[pid] * 1024) / sim->page_size;
        for (int page = 0; page < num_pages; page++) {
            if (sim->reference_string_len < MAX_REFERENCES) {
                sim->reference_string[sim->reference_string_len].pid = pid;
                sim->reference_string[sim->reference_string_len].page_num = page;
                sim->reference_string_len++;
            } else {
                strcat(log_output, "Warning: Reference string exceeded MAX_REFERENCES. Simulation truncated.\n");
                goto end_reference_string_generation;
            }
        }
    }

end_reference_string_generation:
    strcat(log_output, "Reference String Generated. Total Accesses: ");
    char ref_len_str[16];
    sprintf(ref_len_str, "%d\n\n", sim->reference_string_len);
    strcat(log_output, ref_len_str);

    // Simulate page accesses and apply page replacement logic
    for (int i = 0; i < sim->reference_string_len; i++) {
        global_access_time++;
        int current_pid = sim->reference_string[i].pid;
        int current_page = sim->reference_string[i].page_num;

        char current_access_msg[64];
        snprintf(current_access_msg, sizeof(current_access_msg), "Accessing P%d, Page %d (Time %ld)\n",
                 current_pid, current_page, global_access_time);
        strcat(log_output, current_access_msg);

        if (is_page_in_memory(sim, current_pid, current_page) == -1) {
            // Page fault → trigger page load and possible eviction
            load_page(sim, current_pid, current_page, log_output, i);
        } else {
            // Page hit: already in memory
            char msg[64];
            snprintf(msg, sizeof(msg), "Access:   Process %d Page %d (In Memory)\n", current_pid, current_page);
            strcat(log_output, msg);
        }

        // Print memory state after each access
        strcat(log_output, "Current Memory State: [");
        for (int j = 0; j < FRAME_COUNT; j++) {
            if (j < sim->memory_usage) {
                char frame_info[32];
                snprintf(frame_info, sizeof(frame_info), "F%d:P%d.P%d(T%ld)", 
                         sim->memory[j].frame, sim->memory[j].process_id, 
                         sim->memory[j].page, sim->memory[j].last_access_time);
                strcat(log_output, frame_info);
            } else {
                strcat(log_output, "Empty");
            }
            if (j < FRAME_COUNT - 1) strcat(log_output, ", ");
        }
        strcat(log_output, "]\n\n");
    }
}