#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "simulator.h"

#define MAX_UNIQUE_PAGES_TO_DEFINE 10 
#define FRAME_COUNT 4

extern GtkWidget *g_main_window;
extern GtkWidget *g_log_textview;
extern GtkTextBuffer *g_log_buffer;
extern GtkWidget *g_page_fault_label;
extern GtkWidget *g_page_hit_label;
extern GtkWidget *g_frame_labels[FRAME_COUNT];
extern GtkWidget *g_page_size_entry; 
extern GtkWidget *g_num_defined_pages_combo;
extern GtkWidget *g_replacement_algo_combo;

extern GtkWidget *page_size_definition_entries[MAX_UNIQUE_PAGES_TO_DEFINE];
extern GtkWidget *g_page_size_definition_labels[MAX_UNIQUE_PAGES_TO_DEFINE];

extern GtkWidget *g_reference_string_entry; 

void create_main_window();
void gui_log_message(const char *format, ...);
void gui_update_counters(int page_faults, int page_hits);
void gui_update_memory_display(Simulator *sim, int highlight_frame, int highlight_type);
void gui_enable_controls(gboolean enable);

#endif 