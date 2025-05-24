#include "simulator.h"

int main(int argc, char *argv[]) {
    // Initialize memory manager with default settings
    memory_manager_t *mm = init_memory_manager(16, FIFO);
    if (!mm) {
        fprintf(stderr, "Failed to initialize memory manager\n");
        return 1;
    }
    
    // Initialize GUI
    gui_t *gui = init_gui(mm);
    if (!gui) {
        fprintf(stderr, "Failed to initialize GUI\n");
        cleanup_memory_manager(mm);
        return 1;
    }
    
    // Show initial display
    update_gui_displays(gui);
    gtk_widget_show_all(gui->window);
    
    // Add some sample data for demonstration
    log_message(gui, "Welcome to Virtual Memory Simulator!\n");
    log_message(gui, "Instructions:\n");
    log_message(gui, "1. Select replacement algorithm\n");
    log_message(gui, "2. Set number of frames\n");
    log_message(gui, "3. Enter process ID and page number\n");
    log_message(gui, "4. Click 'Access Page' to simulate memory access\n");
    log_message(gui, "5. Watch the visualization update in real-time\n\n");
    
    // Run GTK main loop
    gtk_main();
    
    // Cleanup
    cleanup_memory_manager(mm);
    free(gui);
    
    return 0;
}