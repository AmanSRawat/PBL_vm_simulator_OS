#include "simulator.h"
#include "gui.h" // For logging and GUI updates
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

long global_access_time = 0;

static int is_process_in_memory(Simulator *sim, int process_id) {
    for (int i = 0; i < sim->memory_usage; i++) {
        if (sim->memory[i].occupied && sim->memory[i].process_id == process_id) {
            return i;
        }
    }
    return -1;
}

static int find_victim_frame_index(Simulator *sim, int current_ref_idx) {
    if (sim->memory_usage == 0) {
        return -1;
    }

    if (strcmp(sim->algorithm, "fifo") == 0) {
        return 0; 
    } else if (strcmp(sim->algorithm, "lru") == 0) {
        long min_time = -1;
        int lru_idx = -1;
        for (int i = 0; i < sim->memory_usage; i++) {
            if (sim->memory[i].occupied) {
                if (lru_idx == -1 || sim->memory[i].last_access_time < min_time) {
                    min_time = sim->memory[i].last_access_time;
                    lru_idx = i;
                }
            }
        }
        return lru_idx;
    } else if (strcmp(sim->algorithm, "optimal") == 0) {
        int optimal_idx = -1;
        int farthest_future_idx = -1;

        for (int i = 0; i < sim->memory_usage; i++) {
            if (!sim->memory[i].occupied) continue;

            int current_process_id = sim->memory[i].process_id;
            int next_use_idx = -1;

            for (int j = current_ref_idx + 1; j < sim->reference_string_len; j++) {
                if (sim->reference_string[j].process_id == current_process_id) {
                    next_use_idx = j;
                    break;
                }
            }

            if (next_use_idx == -1) {
                return i;
            }

            if (next_use_idx > farthest_future_idx) {
                farthest_future_idx = next_use_idx;
                optimal_idx = i;
            }
        }
        return optimal_idx;
    }
    return -1;
}

static void load_process(Simulator *sim, int process_id, int process_kb_size, int current_ref_idx) {
    char log_msg[128];
    int physical_frame_assigned = -1;

    if (sim->memory_usage < FRAME_COUNT) {
        physical_frame_assigned = sim->memory_usage; 

        sim->memory[sim->memory_usage].process_id = process_id;
        sim->memory[sim->memory_usage].process_kb_size = process_kb_size;
        sim->memory[sim->memory_usage].frame = physical_frame_assigned; 
        sim->memory[sim->memory_usage].last_access_time = global_access_time;
        sim->memory[sim->memory_usage].occupied = true;
        sim->memory_usage++;

        snprintf(log_msg, sizeof(log_msg), "Page-In: Process ID %d (Size %d KB) -> Frame %d (Memory not full)\n",
                 process_id, process_kb_size, physical_frame_assigned);
        gui_log_message(log_msg);

        sim->last_highlight_frame = physical_frame_assigned;
        sim->last_highlight_type = 2; 
    } else {
        int victim_mem_array_idx = find_victim_frame_index(sim, current_ref_idx);

        if (victim_mem_array_idx < 0 || victim_mem_array_idx >= sim->memory_usage || !sim->memory[victim_mem_array_idx].occupied) {
            fprintf(stderr, "Error: Invalid victim_mem_array_idx %d for memory_usage %d. Skipping page-in.\n", victim_mem_array_idx, sim->memory_usage);
            sim->last_highlight_frame = -1;
            sim->last_highlight_type = 0;
            return;
        }

        PageFrame victim_page = sim->memory[victim_mem_array_idx];
        physical_frame_assigned = victim_page.frame; 

        snprintf(log_msg, sizeof(log_msg), "Page-Out: Process ID %d (Size %d KB) from Frame %d (Algorithm: %s)\n",
                 victim_page.process_id, victim_page.process_kb_size, victim_page.frame, sim->algorithm);
        gui_log_message(log_msg);

        sim->last_highlight_frame = physical_frame_assigned;
        sim->last_highlight_type = 3;
        gui_update_memory_display(sim, sim->last_highlight_frame, sim->last_highlight_type);
        g_usleep(100000); 

        for (int i = victim_mem_array_idx; i < sim->memory_usage - 1; i++) {
            sim->memory[i] = sim->memory[i + 1];
        }
        sim->memory_usage--;

        sim->memory[sim->memory_usage].process_id = process_id;
        sim->memory[sim->memory_usage].process_kb_size = process_kb_size;
        sim->memory[sim->memory_usage].frame = physical_frame_assigned; 
        sim->memory[sim->memory_usage].last_access_time = global_access_time;
        sim->memory[sim->memory_usage].occupied = true;
        sim->memory_usage++;

        snprintf(log_msg, sizeof(log_msg), "Page-In:  Process ID %d (Size %d KB) -> Frame %d\n",
                 process_id, process_kb_size, physical_frame_assigned);
        gui_log_message(log_msg);

        sim->last_highlight_frame = physical_frame_assigned;
        sim->last_highlight_type = 2; 
    }
}


void simulator_init(Simulator *sim) {
    sim->frame_size = 0;
    sim->frame_count = FRAME_COUNT;
    sim->memory_usage = 0;
    sim->page_faults = 0;
    sim->page_hits = 0;
    sim->reference_string = NULL;
    sim->reference_string_len = 0;
    sim->current_ref_idx = 0;
    sim->last_highlight_frame = -1;
    sim->last_highlight_type = 0;
    strcpy(sim->algorithm, "fifo");

    sim->defined_processes_count = 0;

    for (int i = 0; i < FRAME_COUNT; i++) {
        sim->memory[i].process_id = -1; 
        sim->memory[i].process_kb_size = -1; 
        sim->memory[i].frame = i;
        sim->memory[i].last_access_time = 0;
        sim->memory[i].occupied = false;
    }
    global_access_time = 0;
    srand(time(NULL)); 
}

void simulator_configure_params(Simulator *sim, int frame_size_kb, const char *algo, int num_defined_processes) {
    sim->frame_size = frame_size_kb; 
    strcpy(sim->algorithm, algo);
    if (num_defined_processes < 1 || num_defined_processes > MAX_UNIQUE_PAGES_TO_DEFINE) {
        sim->defined_processes_count = 4; 
        gui_log_message("Warning: Invalid number of unique processes defined. Capped at 4.\n");
    } else {
        sim->defined_processes_count = num_defined_processes;
    }
}

void simulator_set_defined_process_size(Simulator *sim, int process_idx, int size_kb) {
    if (process_idx >= 0 && process_idx < sim->defined_processes_count) {
        sim->defined_process_kb_sizes[process_idx] = size_kb;
    } else {
        fprintf(stderr, "Error: Attempted to set size for invalid defined process index %d.\n", process_idx);
    }
}

bool simulator_prepare_run_from_string(Simulator *sim, const char *ref_string_input_str) {
    if (sim->reference_string != NULL) {
        free(sim->reference_string);
        sim->reference_string = NULL;
    }

    sim->page_faults = 0;
    sim->page_hits = 0;
    sim->current_ref_idx = 0;
    global_access_time = 0;

    for (int i = 0; i < FRAME_COUNT; i++) {
        sim->memory[i].process_id = -1;
        sim->memory[i].process_kb_size = -1;
        sim->memory[i].frame = i;
        sim->memory[i].last_access_time = 0;
        sim->memory[i].occupied = false;
    }
    sim->memory_usage = 0;
    sim->last_highlight_frame = -1;
    sim->last_highlight_type = 0;

    if (sim->defined_processes_count == 0) {
        gui_log_message("Error: No unique processes defined. Cannot generate reference string.\n");
        return false;
    }

    char *input_copy = strdup(ref_string_input_str); 
    if (input_copy == NULL) {
        gui_log_message("Error: Memory allocation failed for reference string parsing.\n");
        return false;
    }

    int count = 0;
    char *token = strtok(input_copy, ",");
    while (token != NULL) {
        count++;
        token = strtok(NULL, ",");
    }
    free(input_copy); 

    if (count == 0) {
        gui_log_message("Error: Reference string is empty or invalid format.\n");
        return false;
    }

    sim->reference_string_len = count;
    sim->reference_string = (ProcessReference*)malloc(sim->reference_string_len * sizeof(ProcessReference));
    if (sim->reference_string == NULL) {
        gui_log_message("Error: Failed to allocate memory for reference string.\n");
        sim->reference_string_len = 0;
        return false;
    }

    input_copy = strdup(ref_string_input_str); 
    if (input_copy == NULL) {
        gui_log_message("Error: Memory allocation failed for reference string parsing (second pass).\n");
        free(sim->reference_string);
        sim->reference_string = NULL;
        sim->reference_string_len = 0;
        return false;
    }

    int ref_idx = 0;
    token = strtok(input_copy, ",");
    while (token != NULL && ref_idx < sim->reference_string_len) {
        int process_idx_from_string = atoi(token); 

        if (process_idx_from_string < 0 || process_idx_from_string >= sim->defined_processes_count) {
            gui_log_message("Error: Invalid process ID '%d' found in reference string. Must be between 0 and %d.\n", process_idx_from_string, sim->defined_processes_count - 1);
            free(sim->reference_string);
            sim->reference_string = NULL;
            sim->reference_string_len = 0;
            free(input_copy);
            return false;
        }

        sim->reference_string[ref_idx].process_id = process_idx_from_string;
        sim->reference_string[ref_idx].process_kb_size = sim->defined_process_kb_sizes[process_idx_from_string];
        ref_idx++;
        token = strtok(NULL, ",");
    }
    free(input_copy); 

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Reference String Parsed. Total Accesses: %d\n", sim->reference_string_len);
    gui_log_message(log_msg);

    snprintf(log_msg, sizeof(log_msg),
             "Algorithm: %s\n"
             "Frame Size: %d KB\n"
             "Unique Processes Defined: %d\n",
             sim->algorithm, sim->frame_size, sim->defined_processes_count);
    gui_log_message(log_msg);

    for (int i = 0; i < sim->defined_processes_count; i++) {
        snprintf(log_msg, sizeof(log_msg), "Process %d (Size: %d KB)\n",
                 i, sim->defined_process_kb_sizes[i]);
        gui_log_message(log_msg);
    }
    snprintf(log_msg, sizeof(log_msg), "Input Reference String: %s\n", ref_string_input_str);
    gui_log_message(log_msg);


    return true;
}


void simulator_reset(Simulator *sim) {
    sim->page_faults = 0;
    sim->page_hits = 0;
    sim->current_ref_idx = 0;
    global_access_time = 0;

    for (int i = 0; i < FRAME_COUNT; i++) {
        sim->memory[i].process_id = -1; 
        sim->memory[i].process_kb_size = -1; 
        sim->memory[i].frame = i;
        sim->memory[i].last_access_time = 0;
        sim->memory[i].occupied = false;
    }
    sim->memory_usage = 0;
    sim->last_highlight_frame = -1;
    sim->last_highlight_type = 0;

    if (sim->reference_string != NULL) {
        free(sim->reference_string);
        sim->reference_string = NULL;
    }
    sim->reference_string_len = 0;

    sim->frame_size = 4; 
    sim->defined_processes_count = 4; 
    strcpy(sim->algorithm, "fifo");

    for (int i = 0; i < MAX_UNIQUE_PAGES_TO_DEFINE; i++) {
        sim->defined_process_kb_sizes[i] = 10; 
    }
}

bool simulator_single_step(Simulator *sim) {
    if (sim->current_ref_idx >= sim->reference_string_len) {
        return false; 
    }

    global_access_time++; 

    ProcessReference current_ref = sim->reference_string[sim->current_ref_idx];
    int requested_process_id = current_ref.process_id;
    int requested_process_kb_size = current_ref.process_kb_size;

    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Step %d: Accessing Process ID %d (Size %d KB) (Time %ld)\n",
             sim->current_ref_idx + 1, requested_process_id, requested_process_kb_size, global_access_time);
    gui_log_message(log_msg);

    int memory_idx = is_process_in_memory(sim, requested_process_id);

    if (memory_idx != -1) {
        sim->page_hits++;
        sim->last_highlight_frame = sim->memory[memory_idx].frame;
        sim->last_highlight_type = 1; 

        if (strcmp(sim->algorithm, "lru") == 0) {
            sim->memory[memory_idx].last_access_time = global_access_time;
            PageFrame hit_page = sim->memory[memory_idx];
            for (int i = memory_idx; i < sim->memory_usage - 1; i++) {
                sim->memory[i] = sim->memory[i + 1];
            }
            sim->memory[sim->memory_usage - 1] = hit_page;
        }
        gui_log_message("Page Hit: Process ID %d (Size %d KB) in Frame %d\n", requested_process_id, requested_process_kb_size, sim->memory[memory_idx].frame);
    } else {
        sim->page_faults++;
        load_process(sim, requested_process_id, requested_process_kb_size, sim->current_ref_idx);
    }

    snprintf(log_msg, sizeof(log_msg), "Current Memory State: [");
    char frame_status[64]; 
    for (int i = 0; i < FRAME_COUNT; i++) {
        int found_in_mem_list_idx = -1;
        for (int j = 0; j < sim->memory_usage; j++) {
            if (sim->memory[j].occupied && sim->memory[j].frame == i) {
                found_in_mem_list_idx = j;
                break;
            }
        }

        if (found_in_mem_list_idx != -1) {
            snprintf(frame_status, sizeof(frame_status), "F%d:PID %d(S%dKB|T%ld)",
                     sim->memory[found_in_mem_list_idx].frame,
                     sim->memory[found_in_mem_list_idx].process_id,
                     sim->memory[found_in_mem_list_idx].process_kb_size,
                     sim->memory[found_in_mem_list_idx].last_access_time);
        } else {
            snprintf(frame_status, sizeof(frame_status), "F%d:Empty", i);
        }
        strcat(log_msg, frame_status);
        if (i < FRAME_COUNT - 1) {
            strcat(log_msg, ", ");
        }
    }
    strcat(log_msg, "]\n\n");
    gui_log_message(log_msg);

    gui_update_counters(sim->page_faults, sim->page_hits);

    sim->current_ref_idx++;
    return true; 
}