import { print, println, format } from "core:io";

var width = 80;
var height = 22; 
var iterations = 200; // How long we let the population settle
var plot_points = 50;  // How many points we plot after it settles

// r parameter range (from stability into chaos)
var r_min = 3.4;
var r_max = 4.0;
var r_step = (r_max - r_min) / 80.0;

println("Generating Logistic Map Bifurcation Diagram...");

// We iterate through r (X-axis, columns) and determine where x (Y-axis, rows) lands.
// This requires generating the whole grid in memory before printing because 
// we are plotting columns, not lines.

// Initialize a 2D text grid with spaces
var grid = []; 
for (h := 0; h < height; h++) {
    local line_array = [];
    for (w := 0; w < width; w++) {
        line_array.push(" "); 
    }
    // Push the entire array object, do not spread it!
    grid.push(line_array); 
}

var current_r = r_min;

for (col := 0; col < width; col++) {
    local x = 0.5; // Starting population
    
    // 1. Let the system settle down (ignore transients)
    for (i := 0; i < iterations; i++) {
        x = current_r * x * (1.0 - x);
    }
    
    // 2. The system has settled into either a stable value, a cycle, or chaos.
    // Plot the next few points to see where they land.
    for (p := 0; p < plot_points; p++) {
        x = current_r * x * (1.0 - x);
        
        // Map population x (0.0 to 1.0) to row index (height-1 to 0)
        // Note: Terminal rows are top-to-bottom, so 0 is top.
        // We want x=1 at top, x=0 at bottom.
        local row_f = (1.0 - x) * height;
        local row = row_f; // Assuming automatic integer cast. If not, floor it.
        
        // Clamp bounds just in case
        if (row >= 0 && row < height) {
            // Update grid in-memory. If array mutation is hard,
            // this is a good stress-test for your interpreter.
            local target_row = grid[row];
            target_row[col] = "*";
        }
    }
    
    current_r += r_step;
}

// 3. Print the finalized, colored grid line by line
for (y_print := 0; y_print < height; y_print++) {
    local final_row_str = "";
    local grid_row = grid[y_print];
    
    for (x_print := 0; x_print < width; x_print++) {
        local char = grid_row[x_print];
        if (char == "*") {
            // Apply a nice gradient based on row
            local color = 30 + (y_print * 2); 
            final_row_str += format("\x1b[38;5;{}m*\x1b[0m", color);
        } else {
            final_row_str += " ";
        }
    }
    println(final_row_str);
}

println("Done!");