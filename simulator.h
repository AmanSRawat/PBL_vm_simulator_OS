// simulator.h

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <gtk/gtk.h>
#include <stdbool.h>

#define MAX_UNIQUE_PAGES_TO_DEFINE 10 
#define FRAME_COUNT 4                

typedef struct {
    int process_id;        
    int process_kb_size;   
    int frame;             
    long last_access_time;  
    bool occupied;          
} PageFrame;

typedef struct {
    int process_id;         
    int process_kb_size;    
} ProcessReference;


typedef struct {
    int frame_size;                   
    int frame_count;                 

    PageFrame memory[FRAME_COUNT];    
    int memory_usage;                
    char algorithm[16];               

    int page_faults;                  
    int page_hits;                    
   
    ProcessReference *reference_string; 
    int reference_string_len;         
    int current_ref_idx;              

    
    int defined_processes_count; 
    int defined_process_kb_sizes[MAX_UNIQUE_PAGES_TO_DEFINE];

    int last_highlight_frame;        
    int last_highlight_type;          

} Simulator;

extern long global_access_time;

void simulator_init(Simulator *sim);
void simulator_configure_params(Simulator *sim, int frame_size_kb, const char *algo, int num_defined_processes);
void simulator_set_defined_process_size(Simulator *sim, int process_idx, int size_kb);
bool simulator_prepare_run_from_string(Simulator *sim, const char *ref_string_input_str);
void simulator_reset(Simulator *sim);
bool simulator_single_step(Simulator *sim);

#endif 