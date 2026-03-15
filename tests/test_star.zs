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

// Camera and projection settings
var K1 = 40; 
var K2 = 10.0; 
var increment_speed = 0.04; // Triangle sampling density (optimized for speed)

// Star parameters
var R_out = 1.8; // Radius of the 5 outer points
var R_in = 0.7;  // Radius of the 5 inner "crevices"
var H = 0.5;     // Height of the top and bottom peaks

var lum_chars = [".", ",", "-", "~", ":", ";", "=", "!", "*", "#", "$", "@"];

println("Starting 3D Star animation...");

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
    
    // Precompute rotation matrix for this frame
    local m11 = cosB * cosC;
    local m12 = -cosB * sinC;
    local m13 = sinB;
    
    local m21 = sinA * sinB * cosC + cosA * sinC;
    local m22 = -sinA * sinB * sinC + cosA * cosC;
    local m23 = -sinA * cosB;
    
    local m31 = -cosA * sinB * cosC + sinA * sinC;
    local m32 = cosA * sinB * sinC + sinA * cosC;
    local m33 = cosA * cosB;
    
    // Construct the 10 segments of the 5-pointed star
    for (seg := 0; seg < 10; seg++) {
        local angle1 = seg * 3.14159265 / 5.0;
        local angle2 = (seg + 1.0) * 3.14159265 / 5.0;
        
        local r1 = R_in; if (seg % 2 == 0) { r1 = R_out; }
        local r2 = R_out; if (seg % 2 == 0) { r2 = R_in; }
        
        local px1 = r1 * cos(angle1);
        local py1 = r1 * sin(angle1);
        local px2 = r2 * cos(angle2);
        local py2 = r2 * sin(angle2);
        
        // Render top (half=0) and bottom (half=1) of the star
        for (half := 0; half < 2; half++) {
            local pz_apex = H;
            if (half == 1) { pz_apex = -H; }
            
            // Calculate the surface normal for shading
            local nx = (py1 * -pz_apex) - (-pz_apex * py2);
            local ny = (-pz_apex * px2) - (px1 * -pz_apex);
            local nz = (px1 * py2) - (py1 * px2);
            
            // Flip the normal vector for the bottom half to ensure it points outwards
            if (half == 1) {
                nx = -nx;
                ny = -ny;
                nz = -nz;
            }
            
            // Rotate the normal vector ONCE per face!
            local nx1 = nx * m11 + ny * m12 + nz * m13;
            local ny1 = nx * m21 + ny * m22 + nz * m23;
            local nz1 = nx * m31 + ny * m32 + nz * m33;

            // Light source pointing up and back
            local L = ny1 - nz1; 
            local lum_idx = floor(L * 4.0);
            
            // Only process visible, lit triangles
            if (lum_idx > 0) {
                if (lum_idx > 11) { lum_idx = 11; }
                
                // Sample the triangle surface using u, v parameters
                for (u := 0.0; u <= 1.0; u += increment_speed) {
                    for (v := 0.0; v <= 1.0 - u; v += increment_speed) {
                        local w = 1.0 - u - v;
                        
                        local x = u * px1 + v * px2;
                        local y = u * py1 + v * py2;
                        local z_c = w * pz_apex;
                        
                        local x1 = x * m11 + y * m12 + z_c * m13;
                        local y1 = x * m21 + y * m22 + z_c * m23;
                        local z1 = x * m31 + y * m32 + z_c * m33;
                        
                        local z_coord = z1 + K2;
                        local ooz = 1.0 / z_coord;
                        
                        local xp = floor(screen_width / 2 + (K1 * ooz * x1 * 2.2));
                        local yp = floor(screen_height / 2 + (K1 * ooz * y1 * 0.5)); // Fix aspect ratio cutoffs
                        
                        if (xp >= 0 && xp < screen_width && yp >= 0 && yp < screen_height) {
                            local idx = floor(xp + yp * screen_width);
                            if (idx >= 0 && idx < screen_width * screen_height) {
                                if (ooz > z[idx]) {
                                    z[idx] = ooz;
                                    b_arr[idx] = lum_chars[lum_idx];
                                }
                            }
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
    
    // Rotate the star
    A += 0.03;
    B += 0.05;
    C += 0.02;
}