import { print, println, format } from "core:io";

var width = 80;
var height = 24; 
var max_iterations = 100; // Increased to 100 for much finer detail

// Perfectly balanced bounds to fix terminal aspect ratio
var real_min = -1.6;
var real_max = 1.6;
var imag_min = -1.1;
var imag_max = 1.1;

var real_step = (real_max - real_min) / 80.0;
var imag_step = (imag_max - imag_min) / 24.0;

// A famously beautiful constant that creates a swirling dragon pattern
var julia_c_re = -0.8;
var julia_c_im = 0.156;

// Smooth ASCII density gradient from light to dark
var chars = [" ", ".", ":", "-", "=", "+", "*", "#", "%", "@"];

println("Calculating Perfected Julia Set...");

var y_coord = imag_max;

for (y := 0; y < height; y++) {
    local row = "";
    local x_coord = real_min;

    for (x := 0; x < width; x++) {
        local z_re = x_coord;
        local z_im = y_coord;
        local is_inside = 1;
        local n = 0;
        
        for (i := 0; i < max_iterations; i++) {
            local z_re2 = z_re * z_re;
            local z_im2 = z_im * z_im;
            
            if (z_re2 + z_im2 > 4.0) {
                is_inside = 0;
                break;
            }
            
            z_im = 2.0 * z_re * z_im + julia_c_im;
            z_re = z_re2 - z_im2 + julia_c_re;
            n++;
        }
        
        if (is_inside == 1) {
            // The absolute center of the fractal is left pure black
            row += " ";
        } else {
            // Map the escape time to our density gradient array
            local char_idx = n % 10;
            
            // Create a gorgeous blue/cyan/purple glowing aura
            // ANSI ranges 17 to 51 are deep blues to bright cyans
            local color = 17 + (n % 34); 
            
            row += format("\x1b[38;5;{}m{}\x1b[0m", color, chars[char_idx]);
        }
        
        x_coord += real_step;
    }
    
    println(row);
    y_coord -= imag_step;
}

println("Done!");