import { print, println } from "core:io";
import { sin, cos, floor } from "core:math";

var screen_width = 80;
var screen_height = 22;

var b_arr = [];
var z     = [];

for (i := 0; i < screen_width * screen_height; i++) {
    b_arr.push(" ");
    z.push(0.0);
}

var A = 0.0;
var B = 0.0;
var C = 0.0;

var cube_width = 1.0;
var K1 = 40;
var increment_speed = 0.08;

println("Starting cube animation...");

for (frame := 0; frame < 1000; frame++) {
    print("\e[H");
    
    for (i := 0; i < screen_width * screen_height; i++) {
        b_arr[i] = " ";
        z[i] = 0.0;
    }
    
    local cosA = cos(A);
    local sinA = sin(A);
    local cosB = cos(B);
    local sinB = sin(B);
    local cosC = cos(C);
    local sinC = sin(C);
    
    // Function to calculate and plot points
    // Replaced inline for simplicity
    
    for (cube_x := -cube_width; cube_x <= cube_width; cube_x += increment_speed) {
        for (cube_y := -cube_width; cube_y <= cube_width; cube_y += increment_speed) {
            // We'll sample 3 faces here since the other 3 will be hidden or we can just sample all 6 and use Z-buffer.
            // Face 1: z = cube_width
            // Face 2: z = -cube_width
            // Face 3: y = cube_width
            // Face 4: y = -cube_width
            // Face 5: x = cube_width
            // Face 6: x = -cube_width
            
            // To emulate functions, we'll use a small loop for the 6 faces
            for (face := 0; face < 6; face++) {
                local x = 0.0;
                local y = 0.0;
                local z_c = 0.0;
                local ch = "@";
                
                if (face == 0) { x = cube_x; y = cube_y; z_c = cube_width; ch = "@"; }
                if (face == 1) { x = cube_x; y = cube_y; z_c = -cube_width; ch = "$"; }
                if (face == 2) { x = cube_x; y = cube_width; z_c = cube_y; ch = "~"; }
                if (face == 3) { x = cube_x; y = -cube_width; z_c = cube_y; ch = "#"; }
                if (face == 4) { x = cube_width; y = cube_x; z_c = cube_y; ch = ";"; }
                if (face == 5) { x = -cube_width; y = cube_x; z_c = cube_y; ch = "+"; }

                local x1 = x * cosB * cosC - y * cosB * sinC + z_c * sinB;
                local y1 = x * (sinA * sinB * cosC + cosA * sinC) + y * (-sinA * sinB * sinC + cosA * cosC) - z_c * sinA * cosB;
                local z1 = x * (-cosA * sinB * cosC + sinA * sinC) + y * (cosA * sinB * sinC + sinA * cosC) + z_c * cosA * cosB;
                
                local z_coord = z1 + 5.0; // Distance K2
                local ooz = 1.0 / z_coord;
                
                local xp = floor(40 + (K1 * ooz * x1 * 1.5)); // Mildly stretch X to fix terminal font ratio
                local yp = floor(11 + (K1 / 2 * ooz * y1)); // Map Y to K1/2 to fix the top and bottom being cut off
                
                if (xp >= 0 && xp < screen_width && yp >= 0 && yp < screen_height) {
                    local idx = floor(xp + yp * screen_width);
                    if (idx >= 0 && idx < screen_width * screen_height) {
                        if (ooz > z[idx]) {
                            z[idx] = ooz;
                            b_arr[idx] = ch;
                        }
                    }
                }
            }
        }
    }
    
    local full_frame = "";
    for (j := 0; j < screen_height; j++) {
        local row = "";
        for (i := 0; i < screen_width; i++) {
            row += b_arr[j * screen_width + i];
        }
        if (j < screen_height - 1) {
            full_frame += row + "\n";
        } else {
            full_frame += row;
        }
    }
    
    print(full_frame);
    
    A += 0.05;
    B += 0.05;
    C += 0.01;
}
