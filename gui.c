#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Global GTK widgets (defined here)
GtkWidget *g_main_window = NULL;
GtkWidget *g_log_textview = NULL;
GtkTextBuffer *g_log_buffer = NULL;
GtkWidget *g_page_fault_label = NULL;
GtkWidget *g_page_hit_label = NULL;
GtkWidget *g_frame_labels[FRAME_COUNT];
GtkWidget *g_page_size_entry = NULL; // This is now FRAME SIZE (still called page_size_entry for continuity)
GtkWidget *g_num_defined_pages_combo = NULL; // Renamed conceptually to processes
GtkWidget *g_replacement_algo_combo = NULL;
// Renamed conceptually to process_size_definition_entries/labels
GtkWidget *page_size_definition_entries[MAX_UNIQUE_PAGES_TO_DEFINE];
GtkWidget *g_page_size_definition_labels[MAX_UNIQUE_PAGES_TO_DEFINE];

GtkWidget *g_reference_string_entry = NULL;

// Buttons
GtkWidget *g_start_sim_button = NULL;
GtkWidget *g_next_step_button = NULL;
GtkWidget *g_run_sim_button = NULL;
GtkWidget *g_reset_sim_button = NULL;

// Global Simulator instance
Simulator sim_instance;

// --- Helper Functions ---
void gui_log_message(const char *format, ...) {
    if (!g_log_buffer) return;

    va_list args;
    va_start(args, format);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);

    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(g_log_buffer, &end_iter);
    gtk_text_buffer_insert(g_log_buffer, &end_iter, buffer, -1);

    GtkTextMark *mark = gtk_text_buffer_get_insert(g_log_buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(g_log_textview), mark, 0.0, FALSE, 0.0, 0.0);

    va_end(args);
}

void gui_update_counters(int page_faults, int page_hits) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Page Faults: %d", page_faults);
    gtk_label_set_text(GTK_LABEL(g_page_fault_label), buffer);

    snprintf(buffer, sizeof(buffer), "Page Hits: %d", page_hits);
    gtk_label_set_text(GTK_LABEL(g_page_hit_label), buffer);
}

// Updated to display Process ID and Size
void gui_update_memory_display(Simulator *sim, int highlight_frame, int highlight_type) {
    // Create a temporary array to correctly map logical frame contents to physical frame display order
    PageFrame display_frames[FRAME_COUNT];

    // Initialize display frames as empty
    for (int i = 0; i < FRAME_COUNT; i++) {
        display_frames[i].occupied = false;
        display_frames[i].frame = i; // Store actual frame number for display
        display_frames[i].process_id = -1;
        display_frames[i].process_kb_size = -1;
        display_frames[i].last_access_time = 0;
    }

    // Populate display frames from simulator's memory, mapping to physical frame numbers
    for (int i = 0; i < sim->memory_usage; i++) {
        int physical_frame_id = sim->memory[i].frame;
        if (physical_frame_id >= 0 && physical_frame_id < FRAME_COUNT) {
            display_frames[physical_frame_id] = sim->memory[i];
            display_frames[physical_frame_id].occupied = true;
        } else {
            fprintf(stderr, "Error: Page in sim->memory[%d] has invalid physical_frame_id %d\n", i, physical_frame_id);
        }
    }

    // Update GUI labels
    for (int i = 0; i < FRAME_COUNT; i++) {
        char frame_text[128]; // Increased size for more detailed info
        const char *bg_color = "white"; // Default background color

        if (!g_frame_labels[i]) {
            fprintf(stderr, "Error: g_frame_labels[%d] is NULL. GTK widget not initialized?\n", i);
            continue;
        }

        if (display_frames[i].occupied) {
            snprintf(frame_text, sizeof(frame_text), "Process ID: %d\nSize: %d KB\nFrame %d",
                     display_frames[i].process_id,
                     display_frames[i].process_kb_size,
                     display_frames[i].frame);

            // Apply highlight color based on highlight_type
            if (display_frames[i].frame == highlight_frame) {
                if (highlight_type == 1) bg_color = "#ADD8E6"; // Light Blue for hit
                else if (highlight_type == 2) bg_color = "#90EE90"; // Light Green for page-in
            }
        } else {
            snprintf(frame_text, sizeof(frame_text), "Empty\nFrame %d", i);
            if (i == highlight_frame && highlight_type == 3) bg_color = "#FFB6C1"; // Light Red for page-out (frame becomes empty after eviction)
        }

        // Set markup for background color
        char markup[256]; // Increased size for markup string
        snprintf(markup, sizeof(markup), "<span background=\"%s\">%s</span>", bg_color, frame_text);
        gtk_label_set_markup(GTK_LABEL(g_frame_labels[i]), markup);
    }
}

void gui_enable_controls(gboolean enable) {
    gtk_widget_set_sensitive(g_start_sim_button, enable);
    gtk_widget_set_sensitive(g_page_size_entry, enable);
    gtk_widget_set_sensitive(g_num_defined_pages_combo, enable);
    gtk_widget_set_sensitive(g_replacement_algo_combo, enable);
    gtk_widget_set_sensitive(g_reference_string_entry, enable); // Enable/disable new entry

    for (int i = 0; i < MAX_UNIQUE_PAGES_TO_DEFINE; i++) {
        if (g_page_size_definition_labels[i])
            gtk_widget_set_sensitive(g_page_size_definition_labels[i], enable);
        if (page_size_definition_entries[i])
            gtk_widget_set_sensitive(page_size_definition_entries[i], enable);
    }

    gtk_widget_set_sensitive(g_next_step_button, !enable);
    gtk_widget_set_sensitive(g_run_sim_button, !enable);
    gtk_widget_set_sensitive(g_reset_sim_button, !enable);
}

// --- Callback Functions ---

static void on_num_defined_pages_changed(GtkComboBoxText *combo, gpointer user_data) {
    Simulator *sim = (Simulator *)user_data;
    int new_count = atoi(gtk_combo_box_text_get_active_text(combo));

    if (new_count < 1 || new_count > MAX_UNIQUE_PAGES_TO_DEFINE) {
        gui_log_message("Error: Invalid number of defined processes.\n");
        return;
    }
    sim->defined_processes_count = new_count; // Updated field name

    // Show/hide process definition entries
    for (int i = 0; i < MAX_UNIQUE_PAGES_TO_DEFINE; i++) {
        gboolean visible = (i < sim->defined_processes_count);

        if (g_page_size_definition_labels[i]) {
            gtk_widget_set_visible(g_page_size_definition_labels[i], visible);
        }
        if (page_size_definition_entries[i]) {
            gtk_widget_set_visible(page_size_definition_entries[i], visible);
        }
    }
    gui_log_message("Number of defined processes changed to %d.\n", new_count);
    gui_log_message("Enter process access sequence (0-based Process IDs) in the 'Reference String' field.\n");
}


static void on_start_simulation_clicked(GtkButton *button, gpointer user_data) {
    Simulator *sim = (Simulator *)user_data;

    const char *frame_size_kb_str = gtk_entry_get_text(GTK_ENTRY(g_page_size_entry));
    int frame_size_kb = atoi(frame_size_kb_str);

    const char *num_defined_processes_str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(g_num_defined_pages_combo));
    int num_defined_processes = atoi(num_defined_processes_str);

    const char *algo = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(g_replacement_algo_combo));

    const char *ref_string_input = gtk_entry_get_text(GTK_ENTRY(g_reference_string_entry)); // Get the string

    // Input validations
    if (frame_size_kb <= 0) {
        gui_log_message("Error: Frame size (KB) must be a positive number.\n");
        return;
    }
    if (num_defined_processes <= 0) {
        gui_log_message("Error: Number of unique processes defined must be a positive number.\n");
        return;
    }
    if (!algo || strlen(algo) == 0) {
        gui_log_message("Error: Replacement algorithm not selected.\n");
        return;
    }
    if (!ref_string_input || strlen(ref_string_input) == 0) {
        gui_log_message("Error: Reference string cannot be empty. Please enter sequence of process IDs (e.g., 0,1,0,2).\n");
        return;
    }

    // Configure simulator basic parameters
    simulator_configure_params(sim, frame_size_kb, algo, num_defined_processes);

    // Set individual defined process sizes
    for (int i = 0; i < sim->defined_processes_count; i++) { // Use defined_processes_count
        const char *process_kb_str = gtk_entry_get_text(GTK_ENTRY(page_size_definition_entries[i]));
        int process_kb = atoi(process_kb_str);
        if (process_kb <= 0) {
            gui_log_message("Warning: Process %d size is zero or negative. Setting to 1 KB.\n", i);
            process_kb = 1;
        }
        if (process_kb > frame_size_kb) {
            gui_log_message("Error: Process %d size (%d KB) cannot be larger than Frame Size (%d KB). Aborting simulation.\n", i, process_kb, frame_size_kb);
            simulator_reset(sim);
            gui_update_memory_display(sim, -1, 0);
            gui_update_counters(sim->page_faults, sim->page_hits);
            gui_enable_controls(TRUE);
            return;
        }
        simulator_set_defined_process_size(sim, i, process_kb); // Updated function call
    }

    // Prepare reference string from the input string
    if (!simulator_prepare_run_from_string(sim, ref_string_input)) {
        gui_log_message("Error: Failed to parse reference string or invalid process ID found. Please check input.\n");
        simulator_reset(sim);
        gui_update_memory_display(sim, -1, 0);
        gui_update_counters(sim->page_faults, sim->page_hits);
        gui_enable_controls(TRUE);
        return;
    }

    gui_log_message("--- Simulation Ready ---\nClick 'Next Step' or 'Run Simulation'\n\n");

    gui_update_memory_display(sim, -1, 0);
    gui_update_counters(sim->page_faults, sim->page_hits);

    gui_enable_controls(FALSE); // Disable config controls
    gtk_widget_set_sensitive(g_next_step_button, TRUE);
    gtk_widget_set_sensitive(g_run_sim_button, TRUE);
    gtk_widget_set_sensitive(g_reset_sim_button, TRUE);
}

static void on_next_step_or_run(GtkButton *button, gpointer user_data) {
    Simulator *sim = (Simulator *)user_data;
    gboolean is_run_button = (button == GTK_BUTTON(g_run_sim_button));

    do {
        if (simulator_single_step(sim)) {
            gui_update_memory_display(sim, sim->last_highlight_frame, sim->last_highlight_type);
            gui_update_counters(sim->page_faults, sim->page_hits);

            if (is_run_button) {
                g_usleep(100000); // Small delay for visual effect
                while (gtk_events_pending()) {
                    gtk_main_iteration(); // Process GUI events
                }
            }
        } else {
            gui_log_message("--- Simulation Complete ---\n");
            gtk_widget_set_sensitive(g_next_step_button, FALSE);
            gtk_widget_set_sensitive(g_run_sim_button, FALSE);
            break; // Exit loop if simulation is complete
        }
    } while (is_run_button); // Continue if "Run" button clicked, or just one step for "Next Step"
}

static void on_reset_simulation_clicked(GtkButton *button, gpointer user_data) {
    Simulator *sim = (Simulator *)user_data;
    simulator_reset(sim);
    gui_log_message("\n--- Simulation Reset ---\n");
    gui_update_memory_display(sim, -1, 0); // Clear memory display
    gui_update_counters(sim->page_faults, sim->page_hits); // Reset counters in GUI
    gui_enable_controls(TRUE); // Enable config controls
    gtk_widget_set_sensitive(g_next_step_button, FALSE);
    gtk_widget_set_sensitive(g_run_sim_button, FALSE);
    gtk_widget_set_sensitive(g_reset_sim_button, FALSE);
}


// --- Main Window Creation ---

void create_main_window() {
    simulator_init(&sim_instance);

    g_main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_main_window), "Virtual Memory Simulator (Process ID Identity)");
    gtk_window_set_default_size(GTK_WINDOW(g_main_window), 800, 700); // Slightly taller for more inputs
    gtk_container_set_border_width(GTK_CONTAINER(g_main_window), 10);
    g_signal_connect(g_main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(g_main_window), main_vbox);

    // Configuration Grid
    GtkWidget *config_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(config_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(config_grid), 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), config_grid, FALSE, FALSE, 0);

    // Row 0: Frame Size
    GtkWidget *frame_size_label = gtk_label_new("Frame Size (KB):");
    gtk_grid_attach(GTK_GRID(config_grid), frame_size_label, 0, 0, 1, 1);
    g_page_size_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(g_page_size_entry), "4"); // Default Frame Size
    gtk_grid_attach(GTK_GRID(config_grid), g_page_size_entry, 1, 0, 1, 1);

    // Row 1: Number of Defined Processes (formerly Unique Pages)
    GtkWidget *num_defined_pages_label = gtk_label_new("Number of Unique Processes to Define:");
    gtk_grid_attach(GTK_GRID(config_grid), num_defined_pages_label, 0, 1, 1, 1);
    g_num_defined_pages_combo = gtk_combo_box_text_new();
    for (int i = 1; i <= MAX_UNIQUE_PAGES_TO_DEFINE; i++) {
        char num_str[4];
        snprintf(num_str, sizeof(num_str), "%d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_num_defined_pages_combo), num_str);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(g_num_defined_pages_combo), 3); // Default to 4 unique processes (indices 0-3)
    g_signal_connect(g_num_defined_pages_combo, "changed", G_CALLBACK(on_num_defined_pages_changed), &sim_instance);
    gtk_grid_attach(GTK_GRID(config_grid), g_num_defined_pages_combo, 1, 1, 1, 1);

    // Dynamic Process Size Definition Entries (Starting from row 2, not 3)
    for (int i = 0; i < MAX_UNIQUE_PAGES_TO_DEFINE; i++) {
        char label_text[64];
        snprintf(label_text, sizeof(label_text), "Process %d Size (KB):", i); // Use 0-based index for Process ID
        g_page_size_definition_labels[i] = gtk_label_new(label_text);
        gtk_grid_attach(GTK_GRID(config_grid), g_page_size_definition_labels[i], 0, 2 + i, 1, 1); // Row 2 + i

        page_size_definition_entries[i] = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(page_size_definition_entries[i]), "10"); // Default value 10KB
        gtk_grid_attach(GTK_GRID(config_grid), page_size_definition_entries[i], 1, 2 + i, 1, 1); // Row 2 + i

        gtk_widget_set_visible(g_page_size_definition_labels[i], FALSE);
        gtk_widget_set_visible(page_size_definition_entries[i], FALSE);
    }
    // Manually trigger initial visibility for default 4 unique processes
    on_num_defined_pages_changed(GTK_COMBO_BOX_TEXT(g_num_defined_pages_combo), &sim_instance);


    // NEW: Reference String Input Field (Place it after process definitions)
    int next_row = 2 + MAX_UNIQUE_PAGES_TO_DEFINE; // Calculate row after last process definition
    GtkWidget *ref_string_label = gtk_label_new("Reference String (comma-separated Process IDs, e.g., 0,1,0,2):");
    gtk_grid_attach(GTK_GRID(config_grid), ref_string_label, 0, next_row, 1, 1);
    g_reference_string_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(g_reference_string_entry), "0,1,0,2,0,1,3,3"); // Example reference string
    gtk_grid_attach(GTK_GRID(config_grid), g_reference_string_entry, 1, next_row, 1, 1);

    // Row for Algorithm selection
    GtkWidget *algo_label = gtk_label_new("Replacement Algorithm:");
    gtk_grid_attach(GTK_GRID(config_grid), algo_label, 0, next_row + 1, 1, 1);
    g_replacement_algo_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_replacement_algo_combo), "fifo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_replacement_algo_combo), "lru");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_replacement_algo_combo), "optimal");
    gtk_combo_box_set_active(GTK_COMBO_BOX(g_replacement_algo_combo), 0); // Default to FIFO
    gtk_grid_attach(GTK_GRID(config_grid), g_replacement_algo_combo, 1, next_row + 1, 1, 1);


    // Start Simulation Button
    g_start_sim_button = gtk_button_new_with_label("Start Simulation");
    g_signal_connect(g_start_sim_button, "clicked", G_CALLBACK(on_start_simulation_clicked), &sim_instance);
    gtk_box_pack_start(GTK_BOX(main_vbox), g_start_sim_button, FALSE, FALSE, 5);


    // Simulation Controls
    GtkWidget *sim_controls_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), sim_controls_hbox, FALSE, FALSE, 0);

    g_next_step_button = gtk_button_new_with_label("Next Step");
    g_signal_connect(g_next_step_button, "clicked", G_CALLBACK(on_next_step_or_run), &sim_instance);
    gtk_box_pack_start(GTK_BOX(sim_controls_hbox), g_next_step_button, TRUE, TRUE, 0);

    g_run_sim_button = gtk_button_new_with_label("Run Simulation");
    g_signal_connect(g_run_sim_button, "clicked", G_CALLBACK(on_next_step_or_run), &sim_instance);
    gtk_box_pack_start(GTK_BOX(sim_controls_hbox), g_run_sim_button, TRUE, TRUE, 0);

    g_reset_sim_button = gtk_button_new_with_label("Reset Simulation");
    g_signal_connect(g_reset_sim_button, "clicked", G_CALLBACK(on_reset_simulation_clicked), &sim_instance);
    gtk_box_pack_start(GTK_BOX(sim_controls_hbox), g_reset_sim_button, TRUE, TRUE, 0);

    // Initially disable step/run/reset buttons
    gtk_widget_set_sensitive(g_next_step_button, FALSE);
    gtk_widget_set_sensitive(g_run_sim_button, FALSE);
    gtk_widget_set_sensitive(g_reset_sim_button, FALSE);


    // Memory Display Grid (for frames)
    GtkWidget *memory_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(memory_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(memory_grid), 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), memory_grid, FALSE, FALSE, 10);

    GtkWidget *memory_label = gtk_label_new("Physical Memory Frames:");
    gtk_box_pack_start(GTK_BOX(main_vbox), memory_label, FALSE, FALSE, 5);


    for (int i = 0; i < FRAME_COUNT; i++) {
        g_frame_labels[i] = gtk_label_new("Empty\nFrame X");
        // Initial markup for empty frame
        gtk_label_set_markup(GTK_LABEL(g_frame_labels[i]), "<span background=\"white\">Empty\nFrame X</span>");
        gtk_widget_set_halign(g_frame_labels[i], GTK_ALIGN_CENTER);
        gtk_widget_set_valign(g_frame_labels[i], GTK_ALIGN_CENTER);
        gtk_widget_set_size_request(g_frame_labels[i], 120, 60); // Adjust size if needed
        gtk_grid_attach(GTK_GRID(memory_grid), g_frame_labels[i], i, 0, 1, 1);
    }
    gui_update_memory_display(&sim_instance, -1, 0);


    // Counters (Page Faults, Page Hits)
    GtkWidget *counters_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_pack_start(GTK_BOX(main_vbox), counters_hbox, FALSE, FALSE, 10);

    g_page_fault_label = gtk_label_new("Page Faults: 0");
    gtk_box_pack_start(GTK_BOX(counters_hbox), g_page_fault_label, TRUE, TRUE, 0);

    g_page_hit_label = gtk_label_new("Page Hits: 0");
    gtk_box_pack_start(GTK_BOX(counters_hbox), g_page_hit_label, TRUE, TRUE, 0);


    // Log View
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 10);

    g_log_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(g_log_textview), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(g_log_textview), FALSE);
    g_log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(g_log_textview));
    gtk_container_add(GTK_CONTAINER(scrolled_window), g_log_textview);

    gui_log_message("Welcome to Virtual Memory Simulator!\nConfigure parameters and click 'Start Simulation'.\n");


    gtk_widget_show_all(g_main_window);
}