#include "gui.h"
#include "simulator.h"

static GtkWidget *process_dropdown, *page_size_entry, *algorithm_combo;
static GtkWidget *process_size_entries[MAX_PROCESSES];
static GtkWidget *output_view;

static void on_algorithm_changed(GtkComboBoxText *combo, gpointer user_data) {
    Simulator *sim = (Simulator *)user_data;
    simulator_set_algorithm(sim, gtk_combo_box_text_get_active_text(combo));
}

static void on_process_count_changed(GtkComboBoxText *combo, gpointer user_data) {
    Simulator *sim = (Simulator *)user_data;
    simulator_set_process_count(sim, atoi(gtk_combo_box_text_get_active_text(combo)));
    for (int i = 0; i < MAX_PROCESSES; i++)
        gtk_widget_set_visible(process_size_entries[i], i < sim->process_count);
}

static void on_start_simulation(GtkButton *button, gpointer user_data) {
    Simulator *sim = (Simulator *)user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));
    gtk_text_buffer_set_text(buffer, "", -1);

    simulator_set_page_size(sim, atoi(gtk_entry_get_text(GTK_ENTRY(page_size_entry))));
    for (int i = 0; i < sim->process_count; i++)
        simulator_set_process_size(sim, i, atoi(gtk_entry_get_text(GTK_ENTRY(process_size_entries[i]))));

    char result[4096];
    snprintf(result, sizeof(result), "Algorithm: %s\nPage Size: %d KB\nProcesses: %d\n",
             sim->algorithm, sim->page_size / 1024, sim->process_count);
    for (int i = 0; i < sim->process_count; i++) {
        char temp[64];
        snprintf(temp, sizeof(temp), "Process %d Size: %d KB\n", i + 1, sim->process_sizes[i]);
        strcat(result, temp);
    }

    strcat(result, "\n--- Simulation Start ---\n");
    simulator_run(sim, result);
    strcat(result, "--- Simulation End ---\n");
    gtk_text_buffer_set_text(buffer, result, -1);
}

void create_main_window(void) {
    Simulator *sim = malloc(sizeof(Simulator));
    simulator_init(sim);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Virtual Memory Simulator");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    GtkWidget *input_frame = gtk_frame_new("Input Parameters");
    GtkWidget *input_grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(input_frame), input_grid);

    // Process Count Dropdown
    GtkWidget *proc_label = gtk_label_new("Number of Processes:");
    process_dropdown = gtk_combo_box_text_new();
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        char num[4];
        sprintf(num, "%d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(process_dropdown), num);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(process_dropdown), 0);
    g_signal_connect(process_dropdown, "changed", G_CALLBACK(on_process_count_changed), sim);

    // Page Size Entry
    GtkWidget *page_size_label = gtk_label_new("Page Size (Bytes):");
    page_size_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(page_size_entry), "4096");

    // Algorithm Combo
    GtkWidget *algo_label = gtk_label_new("Replacement Algorithm:");
    algorithm_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "lru");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "fifo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "optimal");
    gtk_combo_box_set_active(GTK_COMBO_BOX(algorithm_combo), 0);
    g_signal_connect(algorithm_combo, "changed", G_CALLBACK(on_algorithm_changed), sim);

    // Attach widgets to grid
    gtk_grid_attach(GTK_GRID(input_grid), proc_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(input_grid), process_dropdown, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(input_grid), page_size_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(input_grid), page_size_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(input_grid), algo_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(input_grid), algorithm_combo, 1, 2, 1, 1);

    // Process size entries
    for (int i = 0; i < MAX_PROCESSES; i++) {
        char label[32];
        sprintf(label, "Process %d Size (KB):", i + 1);
        GtkWidget *entry_label = gtk_label_new(label);
        GtkWidget *entry = gtk_entry_new();
        process_size_entries[i] = entry;
        gtk_grid_attach(GTK_GRID(input_grid), entry_label, 0, 3 + i, 1, 1);
        gtk_grid_attach(GTK_GRID(input_grid), entry, 1, 3 + i, 1, 1);
        gtk_widget_set_visible(entry_label, i == 0);
        gtk_widget_set_visible(entry, i == 0);
    }

    gtk_box_pack_start(GTK_BOX(main_box), input_frame, FALSE, FALSE, 6);

    // Simulation Output
    GtkWidget *sim_frame = gtk_frame_new("Simulation Output");
    output_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), output_view);
    gtk_container_add(GTK_CONTAINER(sim_frame), scroll);
    gtk_box_pack_start(GTK_BOX(main_box), sim_frame, TRUE, TRUE, 6);

    // Start Button
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *start_btn = gtk_button_new_with_label("Start Simulation");
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_simulation), sim);
    gtk_box_pack_end(GTK_BOX(btn_box), start_btn, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(main_box), btn_box, FALSE, FALSE, 6);

    gtk_widget_show_all(window);
}