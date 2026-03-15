import { print, println, format } from "core:io";
import { sin, cos, floor } from "core:math";

var screen_width = 80;
var screen_height = 22;

var theta_spacing = 0.07;
var phi_spacing   = 0.02;

var R1 = 1;
var R2 = 2;
var K2 = 5;
var K1 = 30; // Keeps the doughnut perfectly sized for the terminal

var b_arr = [];
var z     = [];

for (i := 0; i < screen_width * screen_height; i++) {
    b_arr.push(" ");
    z.push(0.0);
}

var chars = [".", ",", "-", "~", ":", ";", "=", "!", "*", "#", "$", "@"];

var A = 0.0;
var B = 0.0;

println("Starting doughnut animation...");

// Animate 1884 frames (exactly 3 full, smooth rotations)
for (frame := 0; frame < 1884; frame++) {
    print("\e[H"); // Move cursor to top left without clearing screen
    
    // Clear buffers
    for (i := 0; i < screen_width * screen_height; i++) {
        b_arr[i] = " ";
        z[i] = 0.0;
    }
    
    local cosA = cos(A);
    local sinA = sin(A);
    local cosB = cos(B);
    local sinB = sin(B);
    
    for (theta := 0; theta < 6.28; theta += theta_spacing) {
        local costheta = cos(theta);
        local sintheta = sin(theta);
        
        local circlex = R2 + R1 * costheta;
        local circley = R1 * sintheta;
        
        for (phi := 0; phi < 6.28; phi += phi_spacing) {
            local cosphi = cos(phi);
            local sinphi = sin(phi);
            
            local x = circlex * (cosB * cosphi + sinA * sinB * sinphi) - circley * cosA * sinB;
            local y = circlex * (sinB * cosphi - sinA * cosB * sinphi) + circley * cosA * cosB;
            local z_coord = K2 + cosA * circlex * sinphi + circley * sinA;
            local ooz = 1 / z_coord;
            
            local xp = floor(40 + (K1 * ooz * x));
            local yp = floor(11 - (K1 / 2 * ooz * y));
            
            local L = cosphi * costheta * sinB - cosA * costheta * sinphi - sinA * sintheta + cosB * (cosA * sintheta - costheta * sinA * sinphi);
            if (L > 0) {
                if (xp >= 0 && xp < screen_width && yp >= 0 && yp < screen_height) {
                    local idx = floor(xp + yp * screen_width);
                    if (ooz > z[idx]) {
                        z[idx] = ooz;
                        local luminance_index = floor(L * 8);
                        if (luminance_index < 0) { luminance_index = 0; }
                        if (luminance_index > 11) { luminance_index = 11; }
                        b_arr[idx] = chars[luminance_index];
                    }
                }
            }
        }
    }
    
    // Assemble the frame into a single string to print all at once
    local full_frame = "";
    for (j := 0; j < screen_height; j++) {
        local row = "";
        for (i := 0; i < screen_width; i++) {
            row += b_arr[j * screen_width + i];
        }
        
        // Prevent terminal scrolling
        if (j < screen_height - 1) {
            full_frame += row + "\n";
        } else {
            full_frame += row;
        }
    }
    
    print(full_frame);
    
    // Smoother movement steps
    A += 0.02; 
    B += 0.01; 
}