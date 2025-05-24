#ifndef VM_SIMULATOR_H
#define VM_SIMULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <gtk/gtk.h>

// Constants
#define MAX_PAGES 100
#define MAX_FRAMES 50
#define MAX_PROCESSES 10
#define PAGE_SIZE 4096
#define INVALID_FRAME -1

// Page replacement algorithms
typedef enum {
    FIFO,
    LRU,
    OPTIMAL,
    SECOND_CHANCE
} replacement_algorithm_t;

// Page table entry structure
typedef struct {
    int frame_number;
    bool valid;
    bool dirty;
    bool referenced;
    int last_access_time;
    int load_time;
} page_table_entry_t;

// Frame structure
typedef struct {
    int page_number;
    int process_id;
    bool occupied;
    int load_time;
    int last_access_time;
    bool reference_bit;
} frame_t;

// Process structure
typedef struct {
    int process_id;
    int num_pages;
    page_table_entry_t page_table[MAX_PAGES];
    int page_faults;
    int total_accesses;
} process_t;

// Memory management structure
typedef struct {
    frame_t physical_memory[MAX_FRAMES];
    process_t processes[MAX_PROCESSES];
    int num_frames;
    int num_processes;
    int clock_hand;
    int global_time;
    replacement_algorithm_t algorithm;
    
    // Statistics
    int total_page_faults;
    int total_memory_accesses;
    double hit_ratio;
} memory_manager_t;

// GUI structure
typedef struct {
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *control_panel;
    GtkWidget *memory_display;
    GtkWidget *page_table_display;
    GtkWidget *stats_display;
    GtkWidget *log_display;
    
    // Control widgets
    GtkWidget *algo_combo;
    GtkWidget *frames_spin;
    GtkWidget *process_spin;
    GtkWidget *page_entry;
    GtkWidget *access_button;
    GtkWidget *reset_button;
    
    // Display widgets
    GtkWidget *memory_grid;
    GtkWidget *page_table_tree;
    GtkWidget *stats_labels[10];
    GtkWidget *log_textview;
    
    memory_manager_t *mm;
} gui_t;

// Function declarations
memory_manager_t* init_memory_manager(int num_frames, replacement_algorithm_t algo);
void cleanup_memory_manager(memory_manager_t *mm);
int access_page(memory_manager_t *mm, int process_id, int page_number, bool write);
int find_victim_page(memory_manager_t *mm, int process_id);
void update_statistics(memory_manager_t *mm);
void print_memory_state(memory_manager_t *mm);

// Page replacement algorithms
int fifo_replacement(memory_manager_t *mm);
int lru_replacement(memory_manager_t *mm);
int optimal_replacement(memory_manager_t *mm, int process_id, int page_number);
int second_chance_replacement(memory_manager_t *mm);

// GUI functions
gui_t* init_gui(memory_manager_t *mm);
void setup_main_window(gui_t *gui);
void setup_control_panel(gui_t *gui);
void setup_memory_display(gui_t *gui);
void setup_page_table_display(gui_t *gui);
void setup_stats_display(gui_t *gui);
void setup_log_display(gui_t *gui);
void update_gui_displays(gui_t *gui);

// Callback functions
void on_access_button_clicked(GtkWidget *widget, gpointer data);
void on_reset_button_clicked(GtkWidget *widget, gpointer data);
void on_algorithm_changed(GtkWidget *widget, gpointer data);
void on_frames_changed(GtkWidget *widget, gpointer data);

// Utility functions
void log_message(gui_t *gui, const char *format, ...);
const char* algorithm_to_string(replacement_algorithm_t algo);

#endif // VM_SIMULATOR_H