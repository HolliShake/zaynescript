import { print, println } from "core:io";
import { sin, cos, floor } from "core:math";

var screen_width  = 80;
var screen_height = 22;

var theta_spacing = 0.07;
var phi_spacing   = 0.02;

var R1 = 1.0;
var R2 = 2.0;
var K2 = 5.0;
var K1 = 30.0;

var b_arr = [];
var zbuf  = [];

for (i := 0; i < screen_width * screen_height; i++) {
    b_arr.push(" ");
    zbuf.push(0.0);
}

var chars = [".", ",", "-", "~", ":", ";", "=", "!", "*", "#", "$", "@"];

var A = 0.0;
var B = 0.0;

// Precompute integer step counts so loops never drift from float accumulation.
// (theta_spacing/phi_spacing are not exactly representable in IEEE 754;
//  repeated += would skip the last ring/band and cause a visible seam.)
var steps_theta = floor(6.28318 / theta_spacing) + 1;
var steps_phi   = floor(6.28318 / phi_spacing)   + 1;

println("Starting doughnut animation...");

for (frame := 0; frame < 1884; frame++) {
    print("\e[H");

    for (i := 0; i < screen_width * screen_height; i++) {
        b_arr[i] = " ";
        zbuf[i]  = 0.0;
    }

    local cosA = cos(A);
    local sinA = sin(A);
    local cosB = cos(B);
    local sinB = sin(B);

    for (ti := 0; ti < steps_theta; ti++) {
        local theta    = ti * theta_spacing;
        local costheta = cos(theta);
        local sintheta = sin(theta);

        local circlex = R2 + R1 * costheta;
        local circley = R1 * sintheta;

        for (pi := 0; pi < steps_phi; pi++) {
            local phi    = pi * phi_spacing;
            local cosphi = cos(phi);
            local sinphi = sin(phi);

            local x       = circlex * (cosB * cosphi + sinA * sinB * sinphi) - circley * cosA * sinB;
            local y       = circlex * (sinB * cosphi - sinA * cosB * sinphi) + circley * cosA * cosB;
            local z_coord = K2 + cosA * circlex * sinphi + circley * sinA;

            if (z_coord > 0.0) {
                local ooz = 1.0 / z_coord;

                local xp = floor(40.0 + (K1 * ooz * x));
                local yp = floor(11.0 - (K1 / 2.0 * ooz * y));

                local L = cosphi * costheta * sinB
                        - cosA * costheta * sinphi
                        - sinA * sintheta
                        + cosB * (cosA * sintheta - costheta * sinA * sinphi);

                if (L > 0.0) {
                    if (xp >= 0 && xp < screen_width && yp >= 0 && yp < screen_height) {
                        local idx = floor(xp + yp * screen_width);
                        if (idx >= 0 && idx < screen_width * screen_height) {
                            if (ooz > zbuf[idx]) {
                                zbuf[idx] = ooz;

                                local luminance_index = floor(L * 8.0);
                                if (luminance_index < 0)  { luminance_index = 0;  }
                                if (luminance_index > 11) { luminance_index = 11; }
                                b_arr[idx] = chars[luminance_index];
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

    A += 0.02;
    B += 0.01;
}
