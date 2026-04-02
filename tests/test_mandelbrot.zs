import { print, println, format } from "core:io";

var width = 80;
var height = 22; 
var max_iterations = 50;

var real_min = -2.0;
var imag_max = 1.2;

// Hardcoded step factors to avoid parser issues
var real_factor = 0.0375;  // 3.0 / 80.0
var imag_factor = 0.10909; // 2.4 / 22.0

var chars = [".", ",", "-", "~", ":", ";", "=", "!", "*", "#", "$", "@"];

println("Calculating Colored Mandelbrot Set...");

var c_im = imag_max;

for (y := 0; y < height; y++) {
    local row = "";
    local c_re = real_min;

    for (x := 0; x < width; x++) {
        local z_re = 0.0;
        local z_im = 0.0;
        local is_inside = 1;
        local n = 0;
        
        for (i := 0; i < max_iterations; i++) {
            local z_re2 = z_re * z_re;
            local z_im2 = z_im * z_im;
            
            if (z_re2 + z_im2 > 4.0) {
                is_inside = 0;
                break;
            }
            
            z_im = 2.0 * z_re * z_im + c_im;
            z_re = z_re2 - z_im2 + c_re;
            n++;
        }
        
        if (is_inside == 1) {
            row += " ";
        } else {
            local char_idx = n % 12;
            // Generate a color code between 16 and 231
            local color = 16 + (n % 215); 
            
            // Append the ANSI escape sequence, the color, the character, and the reset code
            row += format("\x1b[38;5;{}m{}\x1b[0m", color, chars[char_idx]);
        }
        
        c_re += real_factor;
    }
    
    // Print the colored row immediately
    println(row);
    c_im -= imag_factor;
}

println("Done!");