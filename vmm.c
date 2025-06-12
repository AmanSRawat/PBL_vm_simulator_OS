#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 10
#define FRAME_COUNT 4
#define MAX_PAGES 256

int page_size = 4096;
int process_count = 1;
int process_sizes[MAX_PROCESSES];
char algorithm[10] = "lru";

GtkWidget *process_dropdown, *page_size_entry, *algorithm_combo;
GtkWidget *process_size_entries[MAX_PROCESSES];
GtkWidget *output_view;

typedef struct {
    int process_id;
    int page;
    int frame;
} PageFrame;

PageFrame memory[FRAME_COUNT];
int memory_usage = 0;

int get_page(int addr) { return (addr & 0xFF00) >> 8; }
int get_offset(int addr) { return addr & 0xFF; }

int is_page_in_memory(int pid, int page) {
    for (int i = 0; i < memory_usage; i++) {
        if (memory[i].process_id == pid && memory[i].page == page)
            return memory[i].frame;
    }
    return -1;
}

int evict_page(char *log_output) {
    PageFrame victim = memory[0];
    for (int i = 1; i < memory_usage; i++) {
        memory[i - 1] = memory[i];
    }
    memory_usage--;

    char msg[64];
    snprintf(msg, sizeof(msg), "Page-Out: Process %d Page %d from Frame %d\n",
             victim.process_id, victim.page, victim.frame);
    strcat(log_output, msg);
    return victim.frame;
}

void load_page(int pid, int page, char *log_output) {
    int frame;
    if (memory_usage < FRAME_COUNT) {
        frame = memory_usage;
    } else {
        frame = evict_page(log_output);
    }

    memory[memory_usage++] = (PageFrame){pid, page, frame};

    char msg[64];
    snprintf(msg, sizeof(msg), "Page-In:  Process %d Page %d -> Frame %d\n",
             pid, page, frame);
    strcat(log_output, msg);
}

void simulate(char *log_output) {
    memory_usage = 0;

    for (int pid = 0; pid < process_count; pid++) {
        int num_pages = (process_sizes[pid] * 1024) / page_size;
        for (int page = 0; page < num_pages; page++) {
            if (is_page_in_memory(pid, page) == -1) {
                load_page(pid, page, log_output);
            } else {
                char msg[64];
                snprintf(msg, sizeof(msg), "Access:   Process %d Page %d (In Memory)\n", pid, page);
                strcat(log_output, msg);
            }
        }
    }
}

void on_algorithm_changed(GtkComboBoxText *combo, gpointer user_data) {
    strcpy(algorithm, gtk_combo_box_text_get_active_text(combo));
}

void on_process_count_changed(GtkComboBoxText *combo, gpointer user_data) {
    process_count = atoi(gtk_combo_box_text_get_active_text(combo));
    for (int i = 0; i < MAX_PROCESSES; i++) {
        gtk_widget_set_visible(process_size_entries[i], i < process_count);
    }
}

void on_start_simulation(GtkButton *button, gpointer user_data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));
    gtk_text_buffer_set_text(buffer, "", -1);

    const char *page_size_str = gtk_entry_get_text(GTK_ENTRY(page_size_entry));
    page_size = atoi(page_size_str);
    if (page_size <= 0) page_size = 4096;

    for (int i = 0; i < process_count; i++) {
        const char *size_text = gtk_entry_get_text(GTK_ENTRY(process_size_entries[i]));
        process_sizes[i] = atoi(size_text);
    }

    char result[4096] = "";
    snprintf(result, sizeof(result),
             "Algorithm: %s\nPage Size: %d KB\nProcesses: %d\n",
             algorithm, page_size / 1024, process_count);
    for (int i = 0; i < process_count; i++) {
        char temp[64];
        snprintf(temp, sizeof(temp), "Process %d Size: %d KB\n", i + 1, process_sizes[i]);
        strcat(result, temp);
    }

    strcat(result, "\n--- Simulation Start ---\n");
    simulate(result);
    strcat(result, "--- Simulation End ---\n");

    gtk_text_buffer_set_text(buffer, result, -1);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Virtual Memory Simulator");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    GtkWidget *input_frame = gtk_frame_new("Input Parameters");
    GtkWidget *input_grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(input_frame), input_grid);

    GtkWidget *proc_label = gtk_label_new("Number of Processes:");
    process_dropdown = gtk_combo_box_text_new();
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        char num[4];
        sprintf(num, "%d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(process_dropdown), num);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(process_dropdown), 0);
    g_signal_connect(process_dropdown, "changed", G_CALLBACK(on_process_count_changed), NULL);

    GtkWidget *page_size_label = gtk_label_new("Page Size (Bytes):");
    page_size_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(page_size_entry), "4096");

    GtkWidget *algo_label = gtk_label_new("Replacement Algorithm:");
    algorithm_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "lru");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "fifo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "optimal");
    gtk_combo_box_set_active(GTK_COMBO_BOX(algorithm_combo), 0);
    g_signal_connect(algorithm_combo, "changed", G_CALLBACK(on_algorithm_changed), NULL);

    gtk_grid_attach(GTK_GRID(input_grid), proc_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(input_grid), process_dropdown, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(input_grid), page_size_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(input_grid), page_size_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(input_grid), algo_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(input_grid), algorithm_combo, 1, 2, 1, 1);

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

    GtkWidget *sim_frame = gtk_frame_new("Simulation Output");
    output_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), output_view);
    gtk_container_add(GTK_CONTAINER(sim_frame), scroll);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(main_box), sim_frame, TRUE, TRUE, 6);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *start_btn = gtk_button_new_with_label("Start Simulation");
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_simulation), NULL);
    gtk_box_pack_end(GTK_BOX(btn_box), start_btn, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(main_box), btn_box, FALSE, FALSE, 6);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}