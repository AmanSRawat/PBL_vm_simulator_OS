#include "simulator.h"
#include <stdarg.h>

gui_t* init_gui(memory_manager_t *mm) {
    gui_t *gui = malloc(sizeof(gui_t));
    if (!gui) return NULL;
    
    gui->mm = mm;
    
    // Initialize GTK
    gtk_init(NULL, NULL);
    
    setup_main_window(gui);
    setup_control_panel(gui);
    setup_memory_display(gui);
    setup_page_table_display(gui);
    setup_stats_display(gui);
    setup_log_display(gui);
    
    return gui;
}

void setup_main_window(gui_t *gui) {
    gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gui->window), "Virtual Memory Manager Simulator");
    gtk_window_set_default_size(GTK_WINDOW(gui->window), 1200, 800);
    gtk_window_set_position(GTK_WINDOW(gui->window), GTK_WIN_POS_CENTER);
    
    g_signal_connect(gui->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Main container
    gui->main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);
    gtk_container_set_border_width(GTK_CONTAINER(gui->main_box), 10);
}

void setup_control_panel(gui_t *gui) {
    GtkWidget *control_frame, *control_grid;
    GtkWidget *algo_label, *frames_label, *process_label, *page_label;
    
    control_frame = gtk_frame_new("Control Panel");
    gtk_box_pack_start(GTK_BOX(gui->main_box), control_frame, FALSE, FALSE, 0);
    gtk_widget_set_size_request(control_frame, 250, -1);
    
    control_grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(control_frame), control_grid);
    gtk_container_set_border_width(GTK_CONTAINER(control_grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(control_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(control_grid), 10);
    
    // Algorithm selection
    algo_label = gtk_label_new("Algorithm:");
    gtk_grid_attach(GTK_GRID(control_grid), algo_label, 0, 0, 1, 1);
    
    gui->algo_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gui->algo_combo), "FIFO");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gui->algo_combo), "LRU");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gui->algo_combo), "Optimal");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gui->algo_combo), "Second Chance");
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui->algo_combo), 0);
    gtk_grid_attach(GTK_GRID(control_grid), gui->algo_combo, 1, 0, 1, 1);
    g_signal_connect(gui->algo_combo, "changed", G_CALLBACK(on_algorithm_changed), gui);
    
    // Number of frames
    frames_label = gtk_label_new("Frames:");
    gtk_grid_attach(GTK_GRID(control_grid), frames_label, 0, 1, 1, 1);
    
    gui->frames_spin = gtk_spin_button_new_with_range(1, MAX_FRAMES, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(gui->frames_spin), gui->mm->num_frames);
    gtk_grid_attach(GTK_GRID(control_grid), gui->frames_spin, 1, 1, 1, 1);
    g_signal_connect(gui->frames_spin, "value-changed", G_CALLBACK(on_frames_changed), gui);
    
    // Process ID
    process_label = gtk_label_new("Process ID:");
    gtk_grid_attach(GTK_GRID(control_grid), process_label, 0, 2, 1, 1);
    
    gui->process_spin = gtk_spin_button_new_with_range(0, MAX_PROCESSES-1, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(gui->process_spin), 0);
    gtk_grid_attach(GTK_GRID(control_grid), gui->process_spin, 1, 2, 1, 1);
    
    // Page number
    page_label = gtk_label_new("Page Number:");
    gtk_grid_attach(GTK_GRID(control_grid), page_label, 0, 3, 1, 1);
    
    gui->page_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(gui->page_entry), "0");
    gtk_grid_attach(GTK_GRID(control_grid), gui->page_entry, 1, 3, 1, 1);
    
    // Buttons
    gui->access_button = gtk_button_new_with_label("Access Page");
    gtk_grid_attach(GTK_GRID(control_grid), gui->access_button, 0, 4, 2, 1);
    g_signal_connect(gui->access_button, "clicked", G_CALLBACK(on_access_button_clicked), gui);
    
    gui->reset_button = gtk_button_new_with_label("Reset Simulation");
    gtk_grid_attach(GTK_GRID(control_grid), gui->reset_button, 0, 5, 2, 1);
    g_signal_connect(gui->reset_button, "clicked", G_CALLBACK(on_reset_button_clicked), gui);
}

void setup_memory_display(gui_t *gui) {
    GtkWidget *memory_frame, *memory_scroll;
    
    memory_frame = gtk_frame_new("Physical Memory");
    gtk_box_pack_start(GTK_BOX(gui->main_box), memory_frame, TRUE, TRUE, 0);
    
    memory_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(memory_frame), memory_scroll);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(memory_scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    gui->memory_grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(memory_scroll), gui->memory_grid);
    gtk_container_set_border_width(GTK_CONTAINER(gui->memory_grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(gui->memory_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(gui->memory_grid), 5);
}

void setup_page_table_display(gui_t *gui) {
    GtkWidget *page_frame, *page_scroll;
    GtkListStore *store;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    
    page_frame = gtk_frame_new("Page Tables");
    gtk_box_pack_start(GTK_BOX(gui->main_box), page_frame, TRUE, TRUE, 0);
    
    page_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(page_frame), page_scroll);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page_scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    // Create tree view model
    store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, 
                              G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_INT);
    
    gui->page_table_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(page_scroll), gui->page_table_tree);
    
    // Add columns
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui->page_table_tree), column);
    
    column = gtk_tree_view_column_new_with_attributes("Page", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui->page_table_tree), column);
    
    column = gtk_tree_view_column_new_with_attributes("Frame", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui->page_table_tree), column);
    
    renderer = gtk_cell_renderer_toggle_new();
    column = gtk_tree_view_column_new_with_attributes("Valid", renderer, "active", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui->page_table_tree), column);
    
    column = gtk_tree_view_column_new_with_attributes("Dirty", renderer, "active", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui->page_table_tree), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Access Time", renderer, "text", 5, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui->page_table_tree), column);
    
    g_object_unref(store);
}

void setup_stats_display(gui_t *gui) {
    GtkWidget *stats_frame, *stats_grid;
    
    stats_frame = gtk_frame_new("Statistics");
    gtk_box_pack_start(GTK_BOX(gui->main_box), stats_frame, FALSE, FALSE, 0);
    gtk_widget_set_size_request(stats_frame, 200, -1);
    
    stats_grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(stats_frame), stats_grid);
    gtk_container_set_border_width(GTK_CONTAINER(stats_grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(stats_grid), 5);
    
    // Create labels for statistics
    const char *stat_labels[] = {
        "Total Accesses:", "Page Faults:", "Hit Ratio:", "Algorithm:",
        "Active Frames:", "Active Processes:", "Clock Hand:", "Global Time:"
    };
    
    for (int i = 0; i < 8; i++) {
        GtkWidget *label = gtk_label_new(stat_labels[i]);
        gtk_grid_attach(GTK_GRID(stats_grid), label, 0, i, 1, 1);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        
        gui->stats_labels[i] = gtk_label_new("0");
        gtk_grid_attach(GTK_GRID(stats_grid), gui->stats_labels[i], 1, i, 1, 1);
        gtk_widget_set_halign(gui->stats_labels[i], GTK_ALIGN_END);
    }
}

void setup_log_display(gui_t *gui) {
    GtkWidget *log_frame, *log_scroll;
    GtkTextBuffer *buffer;
    
    log_frame = gtk_frame_new("Activity Log");
    gtk_box_pack_start(GTK_BOX(gui->main_box), log_frame, TRUE, TRUE, 0);
    
    log_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(log_frame), log_scroll);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(log_scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    gui->log_textview = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(log_scroll), gui->log_textview);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gui->log_textview), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(gui->log_textview), FALSE);
    
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gui->log_textview));
    gtk_text_buffer_set_text(buffer, "Virtual Memory Simulator Started\n", -1);
}

void update_gui_displays(gui_t *gui) {
    // Update memory display
    GList *children = gtk_container_get_children(GTK_CONTAINER(gui->memory_grid));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    // Add memory frame displays
    for (int i = 0; i < gui->mm->num_frames; i++) {
        GtkWidget *frame_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        GtkWidget *frame_label = gtk_label_new(g_strdup_printf("Frame %d", i));
        GtkWidget *content_label;
        
        if (gui->mm->physical_memory[i].occupied) {
            content_label = gtk_label_new(g_strdup_printf("P%d:Pg%d",
                gui->mm->physical_memory[i].process_id,
                gui->mm->physical_memory[i].page_number));
            gtk_widget_override_background_color(frame_box, GTK_STATE_NORMAL,
                &(GdkRGBA){0.7, 0.9, 0.7, 1.0}); // Light green for occupied
        } else {
            content_label = gtk_label_new("Empty");
            gtk_widget_override_background_color(frame_box, GTK_STATE_NORMAL,
                &(GdkRGBA){0.9, 0.9, 0.9, 1.0}); // Light gray for empty
        }
        
        gtk_box_pack_start(GTK_BOX(frame_box), frame_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(frame_box), content_label, FALSE, FALSE, 0);
        
        gtk_grid_attach(GTK_GRID(gui->memory_grid), frame_box, i % 8, i / 8, 1, 1);
        gtk_widget_set_size_request(frame_box, 80, 60);
    }
    
    // Update page table display
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(gui->page_table_tree));
    gtk_list_store_clear(GTK_LIST_STORE(model));
    
    for (int pid = 0; pid < gui->mm->num_processes; pid++) {
        process_t *proc = &gui->mm->processes[pid];
        for (int page = 0; page < proc->num_pages; page++) {
            page_table_entry_t *pte = &proc->page_table[page];
            if (pte->valid || pte->frame_number != INVALID_FRAME) {
                GtkTreeIter iter;
                gtk_list_store_append(GTK_LIST_STORE(model), &iter);
                gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                    0, pid,
                    1, page,
                    2, pte->valid ? pte->frame_number : -1,
                    3, pte->valid,
                    4, pte->dirty,
                    5, pte->last_access_time,
                    -1);
            }
        }
    }
    
    // Update statistics
    char temp[64];
    
    sprintf(temp, "%d", gui->mm->total_memory_accesses);
    gtk_label_set_text(GTK_LABEL(gui->stats_labels[0]), temp);
    
    sprintf(temp, "%d", gui->mm->total_page_faults);
    gtk_label_set_text(GTK_LABEL(gui->stats_labels[1]), temp);
    
    sprintf(temp, "%.2f%%", gui->mm->hit_ratio);
    gtk_label_set_text(GTK_LABEL(gui->stats_labels[2]), temp);
    
    gtk_label_set_text(GTK_LABEL(gui->stats_labels[3]), algorithm_to_string(gui->mm->algorithm));
    
    int active_frames = 0;
    for (int i = 0; i < gui->mm->num_frames; i++) {
        if (gui->mm->physical_memory[i].occupied) active_frames++;
    }
    sprintf(temp, "%d/%d", active_frames, gui->mm->num_frames);
    gtk_label_set_text(GTK_LABEL(gui->stats_labels[4]), temp);
    
    sprintf(temp, "%d", gui->mm->num_processes);
    gtk_label_set_text(GTK_LABEL(gui->stats_labels[5]), temp);
    
    sprintf(temp, "%d", gui->mm->clock_hand);
    gtk_label_set_text(GTK_LABEL(gui->stats_labels[6]), temp);
    
    sprintf(temp, "%d", gui->mm->global_time);
    gtk_label_set_text(GTK_LABEL(gui->stats_labels[7]), temp);
    
    gtk_widget_show_all(gui->window);
}

void on_access_button_clicked(GtkWidget *widget, gpointer data) {
    gui_t *gui = (gui_t*)data;
    
    int process_id = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gui->process_spin));
    const char *page_text = gtk_entry_get_text(GTK_ENTRY(gui->page_entry));
    int page_number = atoi(page_text);
    
    if (page_number < 0 || page_number >= MAX_PAGES) {
        log_message(gui, "Error: Invalid page number %d (must be 0-%d)\n", page_number, MAX_PAGES-1);
        return;
    }
    
    log_message(gui, "Accessing Process %d, Page %d\n", process_id, page_number);
    
    int frame = access_page(gui->mm, process_id, page_number, false);
    
    if (frame >= 0) {
        log_message(gui, "Page loaded into Frame %d\n", frame);
    } else {
        log_message(gui, "Error: Failed to access page\n");
    }
    
    update_gui_displays(gui);
}

void on_reset_button_clicked(GtkWidget *widget, gpointer data) {
    gui_t *gui = (gui_t*)data;
    
    int num_frames = gui->mm->num_frames;
    replacement_algorithm_t algo = gui->mm->algorithm;
    
    cleanup_memory_manager(gui->mm);
    gui->mm = init_memory_manager(num_frames, algo);
    
    log_message(gui, "Simulation reset\n");
    update_gui_displays(gui);
}

void on_algorithm_changed(GtkWidget *widget, gpointer data) {
    gui_t *gui = (gui_t*)data;
    int active = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    
    gui->mm->algorithm = (replacement_algorithm_t)active;
    log_message(gui, "Algorithm changed to %s\n", algorithm_to_string(gui->mm->algorithm));
    update_gui_displays(gui);
}

void on_frames_changed(GtkWidget *widget, gpointer data) {
    gui_t *gui = (gui_t*)data;
    int new_frames = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
    
    if (new_frames != gui->mm->num_frames) {
        gui->mm->num_frames = new_frames;
        log_message(gui, "Number of frames changed to %d\n", new_frames);
        update_gui_displays(gui);
    }
}

void log_message(gui_t *gui, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gui->log_textview));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(text_buffer, &end);
    
    // Add timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "[%H:%M:%S] ", tm_info);
    
    gtk_text_buffer_insert(text_buffer, &end, timestamp, -1);
    gtk_text_buffer_insert(text_buffer, &end, buffer, -1);
    
    // Auto-scroll to bottom
    GtkTextMark *mark = gtk_text_buffer_get_insert(text_buffer);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(gui->log_textview), mark);
    
    va_end(args);
}