// ---------------------------------------------------------------------------
//  Desk Buddy case - parametric, for a 0.96" SSD1306 module + ESP32-C3 Super
//  Mini. Two printed parts, four M2 self-tapping screws.
//
//    bezel  - the face. The OLED drops into a pocket from behind.
//    shell  - a closed box behind it, 35 mm deep, the same outline as the
//             face, leaning back. The Super Mini sits on a shelf inside with
//             its USB-C port through the back wall. Touch pad tapes to the
//             inside of the top. Nothing is visible but the face and a cable.
//
//  MEASURE YOUR PARTS FIRST. These modules vary by a millimetre or so between
//  batches; every dimension you might need to nudge is at the top.
//
//  Printing (Bambu P1S, 0.2 mm layers, PLA or PETG, no supports):
//    part="bezel"  exports face-down, ready to print.
//    part="shell"  exports back-wall-down, open front up. The posts, the MCU
//                  shelf and the vents all print clean in that orientation.
//
//    openscad -o bezel.stl -D 'part="bezel"' deskbuddy_case.scad
//    openscad -o shell.stl -D 'part="shell"' deskbuddy_case.scad
//
//  part="both" is an assembly preview, standing on the desk as it will sit.
// ---------------------------------------------------------------------------

part = "both";        // "bezel" | "shell" | "both"
section = false;      // with "both": cut away the right half to see inside

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
mcu_w          = 18.2;   // across
mcu_l          = 22.6;   // USB-C end to antenna end
mcu_pcb_t      = 1.0;
usb_w          = 9.6;    // USB-C cutout in the back wall
usb_h          = 4.2;

/* [ Case ] */
case_w         = 46;     // the face; the shell is exactly this outline
case_h         = 44;
case_d         = 35;     // depth of the shell behind the face
wall           = 2.2;
corner_r       = 4;
lean           = 12;     // degrees the whole thing tilts back
fit            = 0.35;   // clearance around parts - raise if it's tight
screw_pilot    = 1.7;    // M2 self-tapping
screw_head     = 3.8;
post_d         = 5.4;
post_len       = 9;
button_d       = 0;      // 0 = none; 6.2 or 12.2 for a panel-mount button
                         // wired to GPIO9/GND, since BOOT is inside the box
$fn            = 48;

// ---------------------------------------------------------------------------
//  Helpers
//
//  "Body space": X across, Y depth (0 at the face, case_d at the back wall),
//  Z height (0 at the bottom). Everything is built axis-aligned in body
//  space; leaned() puts it on the desk.
// ---------------------------------------------------------------------------
module rrect(w, h, r) {
  offset(r = r) square([w - 2 * r, h - 2 * r], center = true);
}

// Extrude a front-view 2D shape (x across, y up) backwards along +Y.
module front_extrude(depth) {
  translate([0, depth, 0]) rotate([90, 0, 0]) linear_extrude(height = depth) children();
}

// Extrude a top-view 2D shape (x across, y depth) upwards along +Z.
module up_extrude(h) { linear_extrude(height = h) children(); }

// Extrude a back-view 2D shape through the back wall along +Y.
module back_cut(y0, depth) {
  translate([0, y0, 0]) rotate([-90, 0, 0]) linear_extrude(height = depth) children();
}

lift = case_d * sin(lean);              // how far the front-bottom edge rises
chin = lift * tan(lean);                // so the wedge continues the face plane

module leaned() {
  translate([0, 0, lift]) rotate([-lean, 0, 0]) children();
}
module unleaned() {
  rotate([lean, 0, 0]) translate([0, 0, -lift]) children();
}

// Screw posts, just inside each corner, merged 1 mm into the walls.
post_x = case_w / 2 - wall - post_d / 2 + 1.0;
post_z = case_h / 2 - wall - post_d / 2 + 1.0;   // offset from the centre line

module post_positions_xz() {   // 2D, centred, for the bezel
  for (sx = [-1, 1], sz = [-1, 1]) translate([sx * post_x, sz * post_z]) children();
}

// ---------------------------------------------------------------------------
//  Shell
// ---------------------------------------------------------------------------
shelf_z    = wall + 7;                        // top of the MCU shelf
shelf_len  = mcu_l + 2;
shelf_t    = 1.6;
rib        = 1.6;

module body_outer() {
  front_extrude(case_d) translate([0, case_h / 2]) rrect(case_w, case_h, corner_r);
}

// A thin plate on the desk under the leaned body; hull() fills the wedge.
module desk_slab() {
  translate([-case_w / 2, -chin, 0])
    cube([case_w, case_d * cos(lean) + chin, 0.01]);
}

module cavity() {
  translate([0, -1, 0])
    front_extrude(case_d - wall + 1)
      translate([0, case_h / 2]) rrect(case_w - 2 * wall, case_h - 2 * wall, corner_r - 1);
}

module back_features() {
  // USB-C, level with the MCU sitting on its shelf.
  translate([0, 0, shelf_z + mcu_pcb_t + usb_h / 2 - 0.4])
    back_cut(case_d - wall - 0.5, wall + 1) rrect(usb_w, usb_h, 1.5);
  // Vents: five on the back wall, three along the top near the back.
  for (i = [-2 : 2])
    translate([i * 6, 0, case_h * 0.62])
      back_cut(case_d - wall - 0.5, wall + 1) rrect(2.4, 14, 1.1);
  for (i = [-1 : 1])
    translate([i * 6, case_d - 9, case_h - wall - 0.5])
      up_extrude(wall + 1) rrect(2.4, 10, 1.1);
}

module posts() {
  for (sx = [-1, 1], sz = [-1, 1])
    translate([sx * post_x, 0, case_h / 2 + sz * post_z])
      rotate([-90, 0, 0])
        difference() {
          cylinder(d = post_d, h = post_len);
          translate([0, 0, -0.5]) cylinder(d = screw_pilot, h = post_len + 1);
        }
}

// A shelf off the back wall. The Super Mini lies on it, USB end against
// the wall, held sideways by two ribs and forwards by a lip.
module mcu_shelf() {
  w = mcu_w + 2 * fit;
  y0 = case_d - wall - shelf_len;
  translate([-w / 2 - rib, y0, shelf_z - shelf_t])
    cube([w + 2 * rib, shelf_len + 0.5, shelf_t]);             // +0.5 fuses into the wall
  for (sx = [-1, 1])
    translate([sx * (w / 2 + rib / 2) - rib / 2, y0, shelf_z - 0.01])
      cube([rib, shelf_len + 0.5, 3]);
  translate([-w / 2, y0, shelf_z - 0.01]) cube([w, 1.2, 1.5]);  // lip
}

module shell_body() {   // in body space
  difference() {
    hull() {
      body_outer();
      unleaned() desk_slab();
    }
    cavity();
    back_features();
  }
  posts();
  mcu_shelf();
}

module shell()       { leaned() shell_body(); }                                   // on the desk
module shell_print() { translate([0, 0, case_d]) rotate([-90, 0, 0]) shell_body(); }  // back wall down

// ---------------------------------------------------------------------------
//  Bezel - built flat, face at z=0, back at z=bezel_t; XY is the front view.
// ---------------------------------------------------------------------------
bezel_t = wall + oled_glass_t + oled_pcb_t + 0.6;

module bezel() {
  difference() {
    up_extrude(bezel_t) rrect(case_w, case_h, corner_r);

    // viewing window
    translate([0, oled_window_dy, -0.1]) up_extrude(wall + 0.2) rrect(oled_window_w, oled_window_h, 1);

    // pocket from the back: the glass presses on the front wall around the
    // window, the PCB sits behind it, the pin header carries on into the
    // shell. Pins-at-the-top is the usual orientation; if you mount it the
    // other way up, set DB_FLIP_DISPLAY in config.h.
    translate([0, 0, wall]) up_extrude(bezel_t)
      square([oled_pcb_w + 2 * fit, oled_pcb_h + 2 * fit], center = true);

    // screws from the front, countersunk
    translate([0, 0, -0.1]) post_positions_xz() cylinder(d = screw_pilot + 0.9, h = bezel_t + 0.2);
    translate([0, 0, -0.1]) post_positions_xz() cylinder(d = screw_head, h = 1.4);

    // optional front button, bottom-right, clear of the pocket
    if (button_d > 0)
      translate([case_w / 2 - wall - button_d / 2 - 1.5, -case_h / 2 + wall + button_d / 2 + 1.5, -0.1])
        cylinder(d = button_d, h = bezel_t + 0.2);
  }
}

module bezel_print() { translate([0, 0, bezel_t]) rotate([180, 0, 0]) bezel(); }  // face down

// ---------------------------------------------------------------------------
//  Output
// ---------------------------------------------------------------------------
if (part == "bezel") {
  bezel_print();
} else if (part == "shell") {
  shell_print();
} else {
  difference() {
    union() {
      shell();
      // bezel on the front of the shell: bezel z -> body -y, bezel y -> body z
      leaned() translate([0, 0, case_h / 2]) rotate([90, 0, 0]) bezel();
    }
    if (section) translate([0, -50, -1]) cube([100, 200, 200]);
  }
}
