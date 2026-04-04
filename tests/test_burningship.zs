import { print, println, format } from "core:io";
import { abs } from "core:math";

var width = 80;
var height = 24; 
var max_iterations = 64; 

// Perfected bounds to frame the whole ship while respecting terminal aspect ratio
var real_min = -2.25;
var real_max = 1.05;
var imag_min = -2.0;
var imag_max = 0.8;

var real_step = (real_max - real_min) / 80.0;
var imag_step = (imag_max - imag_min) / 24.0;

// Curated ANSI fire palette: Dark Red (52) up to Bright White (231)
var fire_colors = [52, 88, 124, 160, 196, 202, 208, 214, 220, 226, 231];
var chars = [".", ":", "-", "=", "+", "*", "#", "%", "@"];

println("Igniting the Burning Ship...");

// Notice we start at imag_min instead of imag_max to flip the ship upright!
var y_coord = imag_min;

for (y := 0; y < height; y++) {
    local row = "";
    local x_coord = real_min;

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
            
            local abs_z_re = abs(z_re);
            local abs_z_im = abs(z_im);

            z_im = 2.0 * abs_z_re * abs_z_im + y_coord;
            z_re = z_re2 - z_im2 + x_coord;
            n++;
        }
        
        if (is_inside == 1) {
            // The unescaped values form the dark hull and masts of the ship
            row += " "; 
        } else {
            // Map the iterations to our fiery color palette and characters
            // We slow down the color progression slightly (n / 2) to widen the flames
            local color_idx = (n / 2) % 11; 
            local char_idx = n % 9;
            local color = fire_colors[color_idx];
            
            row += format("\x1b[38;5;{}m{}\x1b[0m", color, chars[char_idx]);
        }
        
        x_coord += real_step;
    }
    
    println(row);
    
    // We ADD imag_step instead of subtracting, completing the vertical flip
    y_coord += imag_step; 
}

println("Done!");