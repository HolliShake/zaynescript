import { print, println } from "core:io";
import { sin, cos, floor } from "core:math";

var screen_width  = 80;
var screen_height = 22;

var b_arr = [];
var zbuf  = [];

for (i := 0; i < screen_width * screen_height; i++) {
    b_arr.push(" ");
    zbuf.push(0.0);
}

var A = 0.0;
var B = 0.0;
var C = 0.0;

var cube_width      = 1.0;
var K1              = 40.0;
var increment_speed = 0.05;

// Use an integer step count so loop bounds never drift due to float accumulation.
// (0.08, 0.05, etc. are not exactly representable; repeated += would skip the
// last strip at some angles and cause the cube to look "torn".)
var steps = floor(2.0 * cube_width / increment_speed) + 1;

println("Starting cube animation...");

for (frame := 0; frame < 1000; frame++) {
    print("\e[H");

    for (i := 0; i < screen_width * screen_height; i++) {
        b_arr[i] = " ";
        zbuf[i]  = 0.0;
    }

    local cosA = cos(A);
    local sinA = sin(A);
    local cosB = cos(B);
    local sinB = sin(B);
    local cosC = cos(C);
    local sinC = sin(C);

    for (xi := 0; xi < steps; xi++) {
        local cube_x = -cube_width + xi * increment_speed;

        for (yi := 0; yi < steps; yi++) {
            local cube_y = -cube_width + yi * increment_speed;

            for (face := 0; face < 6; face++) {
                local x  = 0.0;
                local y  = 0.0;
                local zc = 0.0;
                local ch = "@";

                if (face == 0) { x = cube_x;      y = cube_y;      zc =  cube_width; ch = "@"; }
                if (face == 1) { x = cube_x;      y = cube_y;      zc = -cube_width; ch = "$"; }
                if (face == 2) { x = cube_x;      y =  cube_width; zc = cube_y;      ch = "~"; }
                if (face == 3) { x = cube_x;      y = -cube_width; zc = cube_y;      ch = "#"; }
                if (face == 4) { x =  cube_width; y = cube_x;      zc = cube_y;      ch = ";"; }
                if (face == 5) { x = -cube_width; y = cube_x;      zc = cube_y;      ch = "+"; }

                local x1 = x * cosB * cosC - y * cosB * sinC + zc * sinB;
                local y1 = x * (sinA * sinB * cosC + cosA * sinC) + y * (-sinA * sinB * sinC + cosA * cosC) - zc * sinA * cosB;
                local z1 = x * (-cosA * sinB * cosC + sinA * sinC) + y * (cosA * sinB * sinC + sinA * cosC) + zc * cosA * cosB;

                local z_coord = z1 + 5.0;
                if (z_coord <= 0.0) { x = x; } // near-plane guard: skip degenerate points
                if (z_coord > 0.0) {
                    local ooz = 1.0 / z_coord;

                    local xp = floor(40.0 + (K1 * ooz * x1 * 1.5));
                    local yp = floor(11.0 + (K1 / 2.0 * ooz * y1));

                    if (xp >= 0 && xp < screen_width && yp >= 0 && yp < screen_height) {
                        local idx = floor(xp + yp * screen_width);
                        if (idx >= 0 && idx < screen_width * screen_height) {
                            if (ooz > zbuf[idx]) {
                                zbuf[idx]  = ooz;
                                b_arr[idx] = ch;
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

    A += 0.05;
    B += 0.05;
    C += 0.01;
}
