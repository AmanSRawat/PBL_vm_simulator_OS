// simulator.h

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <gtk/gtk.h>
#include <stdbool.h>

#define MAX_UNIQUE_PAGES_TO_DEFINE 10 // Max number of different processes user can define
#define FRAME_COUNT 4                 // Number of physical memory frames

// Structure to represent a process (or its page) loaded into a physical frame
typedef struct {
    int process_id;         // Unique ID of the process loaded into this frame
    int process_kb_size;    // The size of this process in KB
    int frame;              // Physical frame number it's currently in
    long last_access_time;  // For LRU, or just insertion time for FIFO
    bool occupied;          // Flag to indicate if this frame is truly occupied
} PageFrame;

// Structure for a process reference in the reference string
typedef struct {
    int process_id;         // The unique ID of the process being referenced
    int process_kb_size;    // The size of the process being referenced (cached for convenience)
} ProcessReference;

// Main simulation state structure
typedef struct {
    int frame_size;                   // Size of a physical frame in KB
    int frame_count;                  // Total number of physical frames (FRAME_COUNT)

    PageFrame memory[FRAME_COUNT];    // Represents physical memory frames
    int memory_usage;                 // Current number of occupied frames (count of PageFrame structs where occupied is true)

    char algorithm[16];               // "fifo", "lru", "optimal"

    int page_faults;                  // Counter for page faults
    int page_hits;                    // Counter for page hits

    // For simulation execution
    ProcessReference *reference_string; // Array of process IDs and their sizes to access
    int reference_string_len;         // Total length of the reference string
    int current_ref_idx;              // Current index in the reference string

    // For GUI setup and initial definition of available processes
    int defined_processes_count; // Number of unique processes user has defined
    // Array to store the KB sizes of defined processes, indexed by process_id
    int defined_process_kb_sizes[MAX_UNIQUE_PAGES_TO_DEFINE];

    // For GUI highlighting
    int last_highlight_frame;         // Frame that was last hit/in/out
    int last_highlight_type;          // 0:none, 1:hit, 2:page-in, 3:page-out

} Simulator;

// Global variable for simulation time
extern long global_access_time;

// Function prototypes
void simulator_init(Simulator *sim);
void simulator_configure_params(Simulator *sim, int frame_size_kb, const char *algo, int num_defined_processes);
// Renamed from simulator_set_defined_page_size
void simulator_set_defined_process_size(Simulator *sim, int process_idx, int size_kb);
// Function to parse reference string from input
bool simulator_prepare_run_from_string(Simulator *sim, const char *ref_string_input_str);
void simulator_reset(Simulator *sim);
bool simulator_single_step(Simulator *sim);

#endif // SIMULATOR_H