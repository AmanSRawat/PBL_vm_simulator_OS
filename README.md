# Virtual Memory Manager Simulator

A comprehensive virtual memory management simulator with a graphical user interface built using GTK3 and C. This project demonstrates various page replacement algorithms and provides real-time visualization of memory management operations.

## Features

### Core Functionality
- **Multiple Page Replacement Algorithms**: FIFO, LRU, Optimal, and Second Chance
- **Real-time Visualization**: Interactive GUI showing physical memory frames and page tables
- **Multi-process Support**: Simulate multiple processes with separate page tables
- **Comprehensive Statistics**: Track page faults, hit ratios, and access patterns
- **Activity Logging**: Detailed log of all memory operations with timestamps

### GUI Components
- **Control Panel**: Algorithm selection, frame configuration, and page access controls
- **Physical Memory Display**: Visual representation of memory frames with color coding
- **Page Table Viewer**: Tabular view of all page table entries across processes
- **Statistics Panel**: Real-time metrics and performance indicators
- **Activity Log**: Chronological log of all simulator events

## Building and Installation

### Prerequisites

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential pkg-config libgtk-3-dev
```

#### CentOS/RHEL/Fedora:
```bash
# CentOS/RHEL 7:
sudo yum install gcc pkg-config gtk3-devel

# CentOS/RHEL 8+ or Fedora:
sudo dnf install gcc pkg-config gtk3-devel
```

### Building the Project

1. **Clone or download** the project files
2. **Navigate** to the project directory
3. **Build** using make:

```bash
# Build the project
make

# Or build and run immediately
make run

# For debug build
make debug

# For optimized release build
make release
```

### Alternative Manual Build
```bash
gcc -Wall -Wextra -std=c99 -g `pkg-config --cflags gtk+-3.0` \
    -c main.c vm_core.c vm_gui.c
gcc main.o vm_core.o vm_gui.o -o vm_simulator \
    `pkg-config --libs gtk+-3.0`
```

## Usage

### Starting the Simulator
```bash
./vm_simulator
```

### Using the Interface

1. **Select Algorithm**: Choose from FIFO, LRU, Optimal, or Second Chance
2. **Configure Frames**: Set the number of physical memory frames (1-50)
3. **Access Pages**: 
   - Enter Process ID (0-9)
   - Enter Page Number (0-99)
   - Click "Access Page" to simulate memory access
4. **Monitor Results**: Watch real-time updates in all display panels
5. **Reset**: Use "Reset Simulation" to clear all data and start over

### Understanding the Display

#### Physical Memory Panel
- **Green Boxes**: Occupied frames showing Process ID and Page Number
- **Gray Boxes**: Empty frames available for allocation
- **Frame Numbers**: Sequential numbering from 0 to n-1

#### Page Table Panel
- **PID**: Process identifier
- **Page**: Virtual page number
- **Frame**: Physical frame number (-1 if not loaded)
- **Valid**: Whether page is currently in memory
- **Dirty**: Whether page has been modified
- **Access Time**: Last access timestamp

#### Statistics Panel
- **Total Accesses**: Number of memory access requests
- **Page Faults**: Number of times page wasn't in memory
- **Hit Ratio**: Percentage of successful memory hits
- **Algorithm**: Currently selected replacement algorithm
- **Active Frames**: Number of occupied frames
- **Active Processes**: Number of processes with allocated pages
- **Clock Hand**: Current position for Second Chance algorithm
- **Global Time**: Internal simulation clock

## Page Replacement Algorithms

### FIFO (First In, First Out)
- Replaces the oldest page in memory
- Simple but may suffer from Belady's Anomaly
- Good for understanding basic concepts

### LRU (Least Recently Used)
- Replaces the page that hasn't been accessed for the longest time
- Better performance than FIFO in most cases
- More complex to implement efficiently

### Optimal
- Replaces the page that will be accessed furthest in the future
- Theoretical best performance (requires future knowledge)
- Used as benchmark for other algorithms

### Second Chance (Clock Algorithm)
- Enhanced FIFO with reference bits
- Gives pages a "second chance" before replacement
- Good balance between simplicity and performance

## File Structure

```
vm_simulator/
├── vm_simulator.h      # Header with all declarations
├── vm_core.c          # Core memory management logic
├── vm_gui.c           # GTK GUI implementation
├── main.c             # Main program entry point
├── Makefile           # Build configuration
└── README.md          # This documentation
```

## Technical Details

### Memory Layout
- **Maximum Pages**: 100 per process
- **Maximum Frames**: 50 physical frames
- **Maximum Processes**: 10 concurrent processes
- **Page Size**: 4KB (configurable constant)

### Data Structures
- **Page Table Entries**: Track frame mapping, validity, and access information
- **Physical Frames**: Store page data and metadata
- **Process Control**: Manage multiple processes and their page tables
- **Memory Manager**: Central coordinator for all operations

### Algorithm Implementation
- **FIFO**: Uses load time for ordering
- **LRU**: Tracks last access time for each page
- **Second Chance**: Implements circular queue with reference bits
- **Optimal**: Simplified implementation (falls back to LRU)

## Development and Debugging

### Debug Build
```bash
make debug
```

### Memory Leak Detection
```bash
make valgrind
```

### Adding New Features
1. **New Algorithm**: Add enum value and implement replacement function
2. **Enhanced GUI**: Modify GUI setup and update functions
3. **Additional Statistics**: Extend statistics structure and display

## Troubleshooting

### Common Issues

1. **GTK Development Files Missing**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libgtk-3-dev
   
   # CentOS/RHEL
   sudo yum install gtk3-devel
   ```

2. **Compilation Errors**
   - Ensure all source files are in the same directory
   - Check that pkg-config can find GTK3
   - Verify C compiler supports C99 standard

3. **Runtime Issues**
   - Check that X11 forwarding is enabled for remote connections
   - Ensure display environment is properly configured
   - Verify GTK3 runtime libraries are installed

### Performance Considerations
- The simulator is designed for educational purposes
- Large numbers of frames (>50) may slow down GUI updates
- Rapid page accesses may cause display lag

## Educational Value

This simulator is excellent for:
- **Operating Systems Courses**: Demonstrate virtual memory concepts
- **Algorithm Comparison**: Compare different replacement strategies
- **Performance Analysis**: Understand trade-offs between algorithms
- **Visual Learning**: See abstract concepts in action

## License and Contributing

This project is designed for educational use. Feel free to modify and extend the code for learning purposes. When contributing:
1. Maintain code clarity and comments
2. Follow existing naming conventions
3. Test all changes thoroughly
4. Update documentation as needed

## Future Enhancements

Potential improvements:
- **Working Set**: Implement working set-based algorithms
- **Demand Paging**: Add page loading simulation with delays
- **Thrashing Detection**: Monitor and display thrashing conditions
- **Batch Processing**: Load and execute memory access sequences
- **Performance Graphs**: Add real-time performance plotting
- **Save/Load**: Persist simulation states to files
