#include "simulator.h"

memory_manager_t* init_memory_manager(int num_frames, replacement_algorithm_t algo) {
    memory_manager_t *mm = malloc(sizeof(memory_manager_t));
    if (!mm) return NULL;
    
    mm->num_frames = num_frames;
    mm->num_processes = 0;
    mm->clock_hand = 0;
    mm->global_time = 0;
    mm->algorithm = algo;
    mm->total_page_faults = 0;
    mm->total_memory_accesses = 0;
    mm->hit_ratio = 0.0;
    
    // Initialize physical memory frames
    for (int i = 0; i < MAX_FRAMES; i++) {
        mm->physical_memory[i].page_number = -1;
        mm->physical_memory[i].process_id = -1;
        mm->physical_memory[i].occupied = false;
        mm->physical_memory[i].load_time = 0;
        mm->physical_memory[i].last_access_time = 0;
        mm->physical_memory[i].reference_bit = false;
    }
    
    // Initialize processes
    for (int i = 0; i < MAX_PROCESSES; i++) {
        mm->processes[i].process_id = i;
        mm->processes[i].num_pages = 0;
        mm->processes[i].page_faults = 0;
        mm->processes[i].total_accesses = 0;
        
        for (int j = 0; j < MAX_PAGES; j++) {
            mm->processes[i].page_table[j].frame_number = INVALID_FRAME;
            mm->processes[i].page_table[j].valid = false;
            mm->processes[i].page_table[j].dirty = false;
            mm->processes[i].page_table[j].referenced = false;
            mm->processes[i].page_table[j].last_access_time = 0;
            mm->processes[i].page_table[j].load_time = 0;
        }
    }
    
    return mm;
}

void cleanup_memory_manager(memory_manager_t *mm) {
    if (mm) {
        free(mm);
    }
}

int access_page(memory_manager_t *mm, int process_id, int page_number, bool write) {
    if (process_id >= MAX_PROCESSES || page_number >= MAX_PAGES) {
        return -1;
    }
    
    mm->global_time++;
    mm->total_memory_accesses++;
    mm->processes[process_id].total_accesses++;
    
    // Ensure process is initialized
    if (mm->processes[process_id].num_pages <= page_number) {
        mm->processes[process_id].num_pages = page_number + 1;
        if (mm->num_processes <= process_id) {
            mm->num_processes = process_id + 1;
        }
    }
    
    page_table_entry_t *pte = &mm->processes[process_id].page_table[page_number];
    
    // Check if page is already in memory
    if (pte->valid) {
        // Page hit
        pte->last_access_time = mm->global_time;
        pte->referenced = true;
        if (write) pte->dirty = true;
        
        // Update frame information
        int frame = pte->frame_number;
        mm->physical_memory[frame].last_access_time = mm->global_time;
        mm->physical_memory[frame].reference_bit = true;
        
        update_statistics(mm);
        return frame;
    }
    
    // Page fault occurred
    mm->total_page_faults++;
    mm->processes[process_id].page_faults++;
    
    // Find free frame or victim frame
    int frame = find_victim_page(mm, process_id);
    if (frame == -1) return -1;
    
    // If frame was occupied, invalidate old page
    if (mm->physical_memory[frame].occupied) {
        int old_process = mm->physical_memory[frame].process_id;
        int old_page = mm->physical_memory[frame].page_number;
        if (old_process >= 0 && old_page >= 0) {
            mm->processes[old_process].page_table[old_page].valid = false;
            mm->processes[old_process].page_table[old_page].frame_number = INVALID_FRAME;
        }
    }
    
    // Load new page into frame
    mm->physical_memory[frame].page_number = page_number;
    mm->physical_memory[frame].process_id = process_id;
    mm->physical_memory[frame].occupied = true;
    mm->physical_memory[frame].load_time = mm->global_time;
    mm->physical_memory[frame].last_access_time = mm->global_time;
    mm->physical_memory[frame].reference_bit = true;
    
    // Update page table
    pte->frame_number = frame;
    pte->valid = true;
    pte->referenced = true;
    pte->last_access_time = mm->global_time;
    pte->load_time = mm->global_time;
    if (write) pte->dirty = true;
    
    update_statistics(mm);
    return frame;
}

int find_victim_page(memory_manager_t *mm, int process_id) {
    // First, try to find a free frame
    for (int i = 0; i < mm->num_frames; i++) {
        if (!mm->physical_memory[i].occupied) {
            return i;
        }
    }
    
    // No free frames, use replacement algorithm
    switch (mm->algorithm) {
        case FIFO:
            return fifo_replacement(mm);
        case LRU:
            return lru_replacement(mm);
        case OPTIMAL:
            return optimal_replacement(mm, process_id, -1); // Simplified for demo
        case SECOND_CHANCE:
            return second_chance_replacement(mm);
        default:
            return fifo_replacement(mm);
    }
}

int fifo_replacement(memory_manager_t *mm) {
    int oldest_time = mm->global_time + 1;
    int victim_frame = 0;
    
    for (int i = 0; i < mm->num_frames; i++) {
        if (mm->physical_memory[i].occupied && 
            mm->physical_memory[i].load_time < oldest_time) {
            oldest_time = mm->physical_memory[i].load_time;
            victim_frame = i;
        }
    }
    
    return victim_frame;
}

int lru_replacement(memory_manager_t *mm) {
    int oldest_time = mm->global_time + 1;
    int victim_frame = 0;
    
    for (int i = 0; i < mm->num_frames; i++) {
        if (mm->physical_memory[i].occupied && 
            mm->physical_memory[i].last_access_time < oldest_time) {
            oldest_time = mm->physical_memory[i].last_access_time;
            victim_frame = i;
        }
    }
    
    return victim_frame;
}

int optimal_replacement(memory_manager_t *mm, int process_id, int page_number) {
    // Simplified optimal - in practice, would need future reference string
    return lru_replacement(mm); // Fallback to LRU
}

int second_chance_replacement(memory_manager_t *mm) {
    while (true) {
        int current_frame = mm->clock_hand;
        
        if (!mm->physical_memory[current_frame].occupied) {
            return current_frame;
        }
        
        if (!mm->physical_memory[current_frame].reference_bit) {
            mm->clock_hand = (mm->clock_hand + 1) % mm->num_frames;
            return current_frame;
        }
        
        // Give second chance
        mm->physical_memory[current_frame].reference_bit = false;
        mm->clock_hand = (mm->clock_hand + 1) % mm->num_frames;
    }
}

void update_statistics(memory_manager_t *mm) {
    if (mm->total_memory_accesses > 0) {
        int hits = mm->total_memory_accesses - mm->total_page_faults;
        mm->hit_ratio = (double)hits / mm->total_memory_accesses * 100.0;
    }
}

void print_memory_state(memory_manager_t *mm) {
    printf("\n=== Memory State ===\n");
    printf("Total Accesses: %d\n", mm->total_memory_accesses);
    printf("Total Page Faults: %d\n", mm->total_page_faults);
    printf("Hit Ratio: %.2f%%\n", mm->hit_ratio);
    
    printf("\nPhysical Memory:\n");
    for (int i = 0; i < mm->num_frames; i++) {
        frame_t *frame = &mm->physical_memory[i];
        if (frame->occupied) {
            printf("Frame %d: P%d-Page%d (Load: %d, Access: %d)\n",
                   i, frame->process_id, frame->page_number,
                   frame->load_time, frame->last_access_time);
        } else {
            printf("Frame %d: Empty\n", i);
        }
    }
}

const char* algorithm_to_string(replacement_algorithm_t algo) {
    switch (algo) {
        case FIFO: return "FIFO";
        case LRU: return "LRU";
        case OPTIMAL: return "Optimal";
        case SECOND_CHANCE: return "Second Chance";
        default: return "Unknown";
    }
}