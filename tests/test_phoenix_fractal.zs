import { print, println } from "core:io";
import { sin, cos, floor } from "core:math";

var screen_width  = 80.0;
var screen_height = 40.0;
var max_iter      = 64;

// Phoenix recurrence: z_new = z² + c + p·z_prev
// Classic phoenix lives near c=(0.5667,0), p=(-0.5,0)

var chars = [" ", "·", ".", ",", "-", "~", ":", ";", "=", "!", "*", "#", "$", "@"];

var b_arr = [];
for (i := 0; i < screen_width * screen_height; i++) {
    b_arr.push(" ");
}

println("Starting Phoenix fractal animation...");

for (frame := 0; frame < 600; frame++) {
    print("\e[H");

    // Animate c and p parameters in slow Lissajous paths
    local t    = frame * 0.018;
    local cx   =  0.5667 + 0.22 * cos(t * 0.7);
    local cy   =  0.18   * sin(t * 0.5);
    local px   = -0.5    + 0.10 * cos(t * 0.3);
    local py   =  0.06   * sin(t * 0.9);

    // ── render one frame ────────────────────────────────────────
    for (j := 0; j < screen_height; j++) {
        for (i := 0; i < screen_width; i++) {

            // Map pixel → complex plane  (aspect-corrected, ×2 wide)
            local zr = (i / screen_width  - 0.5) * 4.2;
            local zi = (j / screen_height - 0.5) * 2.6;

            local zr_prev = 0.0;
            local zi_prev = 0.0;
            local iter    = max_iter;

            for (n := 0; n < max_iter; n++) {
                // Phoenix step: z_new = z² + c + p·z_prev
                local zr_new = zr * zr - zi * zi + cx + px * zr_prev - py * zi_prev;
                local zi_new = 2.0 * zr * zi    + cy + px * zi_prev  + py * zr_prev;

                zr_prev = zr;
                zi_prev = zi;
                zr      = zr_new;
                zi      = zi_new;

                if (zr * zr + zi * zi > 4.0) {
                    iter = n;
                    break;
                }
            }

            local char_idx;
            if (iter == max_iter) {
                char_idx = 0;                                          // interior → space
            } else {
                char_idx = floor(iter * 13 / max_iter) + 1;
                if (char_idx > 13) { char_idx = 13; }
                if (char_idx < 1)  { char_idx = 1;  }
            }

            b_arr[j * screen_width + i] = chars[char_idx];
        }
    }

    // ── assemble and print frame ─────────────────────────────────
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

    // Slowly drift parameters even further between frames
    // (the cos/sin above already do this; nothing extra needed)
}