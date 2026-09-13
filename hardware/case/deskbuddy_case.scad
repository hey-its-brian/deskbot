// ---------------------------------------------------------------------------
//  Desk Buddy case - parametric, for a 0.96" SSD1306 module + ESP32-C3 Super
//  Mini. Two printed parts, four M2 self-tapping screws.
//
//  MEASURE YOUR PARTS FIRST. These modules vary by a millimetre or so between
//  batches; every dimension you might need to nudge is at the top of the file.
//
//  Printing (Bambu P1S, 0.2 mm layers, PLA or PETG):
//    bezel : window side DOWN on the plate, no supports needed.
//    shell : open side UP, no supports; the USB-C slot bridges fine.
//    Drop to 0.16 mm if you want the bezel edge crisper.
//
//  Set `part` below (or use -D on the command line) and export the STL:
//    openscad -o bezel.stl -D 'part="bezel"' deskbuddy_case.scad
//    openscad -o shell.stl -D 'part="shell"' deskbuddy_case.scad
// ---------------------------------------------------------------------------

part = "both";        // "bezel" | "shell" | "both" (both = preview only)

/* [ OLED module ] */
oled_pcb_w     = 27.4;   // PCB width
oled_pcb_h     = 27.9;   // PCB height
oled_pcb_t     = 1.3;    // PCB thickness
oled_glass_t   = 1.6;    // glass + adhesive standing proud of the PCB
oled_window_w  = 25.0;   // opening in the bezel (bigger than the active area
oled_window_h  = 15.0;   // on purpose - no alignment fiddling)
oled_window_dy = 4.0;    // window centre above the PCB centre; the active
                         // strip sits high on these boards

/* [ ESP32-C3 Super Mini ] */
mcu_w          = 18.2;
mcu_l          = 22.6;
mcu_t          = 4.2;    // board + tallest component
usb_w          = 9.6;    // USB-C cutout
usb_h          = 4.0;

/* [ Case ] */
case_w         = 46;
case_h         = 44;
shell_d        = 19;     // depth of the back shell
wall           = 2.2;
corner_r       = 4;
lean           = 14;     // degrees the face tilts back
fit            = 0.35;   // clearance around parts - raise if it's tight
screw_pilot    = 1.7;    // M2 self-tapping
screw_head     = 3.8;
post_d         = 5.4;
$fn            = 48;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------
module rrect(w, h, r) {
  offset(r = r) square([w - 2 * r, h - 2 * r], center = true);
}

module rbox(w, h, d, r) {
  linear_extrude(height = d) rrect(w, h, r);
}

// Screw posts sit just inside each corner.
post_x = case_w / 2 - corner_r - post_d / 2 + 0.6;
post_y = case_h / 2 - corner_r - post_d / 2 + 0.6;

module post_positions() {
  for (sx = [-1, 1], sy = [-1, 1]) translate([sx * post_x, sy * post_y, 0]) children();
}

// ---------------------------------------------------------------------------
//  Bezel - the face. The OLED drops into a pocket from behind.
// ---------------------------------------------------------------------------
bezel_t = wall + oled_glass_t + oled_pcb_t + 0.6;

module bezel() {
  difference() {
    union() {
      rbox(case_w, case_h, wall, corner_r);
      // pocket walls
      translate([0, 0, wall])
        difference() {
          rbox(case_w, case_h, bezel_t - wall, corner_r);
          translate([0, 0, -0.1])
            rbox(case_w - 2 * wall, case_h - 2 * wall, bezel_t - wall + 0.2, corner_r - 0.8);
        }
      // ledge the PCB rests against, so the glass presses on the bezel
      translate([0, 0, wall])
        difference() {
          rbox(oled_pcb_w + 2 * fit + 4, oled_pcb_h + 2 * fit + 4, oled_glass_t + 0.4, 1);
          translate([0, 0, -0.1])
            cube([oled_pcb_w + 2 * fit, oled_pcb_h + 2 * fit, oled_glass_t + 0.6], center = false);
        }
      translate([0, 0, wall]) post_positions() cylinder(d = post_d, h = bezel_t - wall);
    }

    // viewing window
    translate([0, oled_window_dy, -0.1])
      linear_extrude(height = wall + 0.2)
        offset(r = 1) square([oled_window_w - 2, oled_window_h - 2], center = true);

    // pocket for the glass, then the PCB behind it
    translate([0, 0, wall - 0.01])
      cube([oled_pcb_w + 2 * fit, oled_pcb_h + 2 * fit, oled_glass_t + oled_pcb_t + 0.8],
           center = true);
    translate([0, 0, wall + (oled_glass_t + oled_pcb_t + 0.8) / 2 - 0.01])
      cube([oled_pcb_w + 2 * fit, oled_pcb_h + 2 * fit, oled_glass_t + oled_pcb_t + 0.8],
           center = true);

    // screw clearance + countersink from the front
    translate([0, 0, -0.1]) post_positions() cylinder(d = screw_pilot + 0.9, h = bezel_t + 0.2);
    translate([0, 0, -0.1]) post_positions() cylinder(d = screw_head, h = 1.4);

    // notch for the 4-pin header/wires to leave the pocket
    translate([0, -case_h / 2 + wall / 2, wall + 1])
      cube([14, wall * 2, oled_glass_t + oled_pcb_t + 2], center = true);
  }
}

// ---------------------------------------------------------------------------
//  Shell - the body. Leans back, holds the MCU, has a flat foot.
// ---------------------------------------------------------------------------
module shell_raw() {
  difference() {
    rbox(case_w, case_h, shell_d, corner_r);

    // cavity
    translate([0, 0, -0.1])
      rbox(case_w - 2 * wall, case_h - 2 * wall, shell_d - wall + 0.1, corner_r - 0.8);

    // USB-C slot, low on the back face so the cable runs down behind
    translate([0, -case_h / 2 + 8, shell_d - wall - usb_h / 2 - 0.6])
      rotate([90, 0, 0])
        linear_extrude(height = wall * 3, center = true)
          offset(r = 1) square([usb_w - 2, usb_h - 2], center = true);

    // vents
    for (i = [-2 : 2])
      translate([i * 6, case_h / 2 - wall / 2, shell_d / 2])
        cube([2.4, wall * 3, shell_d * 0.55], center = true);
  }

  // screw posts
  translate([0, 0, wall - 0.01]) post_positions()
    difference() {
      cylinder(d = post_d, h = shell_d - wall);
      translate([0, 0, -0.1]) cylinder(d = screw_pilot, h = shell_d - wall + 0.2);
    }

  // a simple cradle for the MCU, mounted on the back wall
  translate([0, 4, shell_d - wall - mcu_t / 2 - 1])
    difference() {
      cube([mcu_w + 2 * fit + 4, mcu_l + 2 * fit + 4, mcu_t + 2], center = true);
      cube([mcu_w + 2 * fit, mcu_l + 2 * fit, mcu_t + 4], center = true);
      // slot so the board slides in from the USB end
      translate([0, -mcu_l / 2 - 2, 0]) cube([usb_w + 2, 6, mcu_t + 4], center = true);
    }
}

// Tilt it back, then slice off everything below the table to get a flat foot.
module shell() {
  difference() {
    rotate([lean, 0, 0]) translate([0, 0, -shell_d]) shell_raw();
    translate([0, 0, -60]) cube([200, 200, 120], center = true);
  }
}

// ---------------------------------------------------------------------------
//  Output
// ---------------------------------------------------------------------------
if (part == "bezel") {
  bezel();
} else if (part == "shell") {
  shell();
} else {
  // assembly preview
  shell();
  rotate([lean, 0, 0]) translate([0, 0, 1]) rotate([180, 0, 0]) bezel();
}
