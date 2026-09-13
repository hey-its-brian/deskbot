# Case

`deskbuddy_case.scad` is a two-part case: a **bezel** that the OLED drops into
and a **shell** that leans the face back ~14 degrees and holds the ESP32-C3.
They screw together with four M2x8 self-tapping screws.

It is parametric and **untested on a printer** - I do not have your modules in
front of me, and 0.96" OLED boards vary by a millimetre between batches. Put
calipers on yours and check these first:

| Variable | What to measure |
| --- | --- |
| `oled_pcb_w` / `oled_pcb_h` | the module PCB, not the glass |
| `oled_glass_t` | how far the glass stands proud of the PCB |
| `oled_window_dy` | the active strip sits high on the board - nudge until centred |
| `mcu_w` / `mcu_l` / `mcu_t` | the Super Mini, including the tallest component |
| `fit` | global clearance; raise to 0.5 if parts bind |

## Printing on the P1S

- 0.2 mm layers, PLA or PETG, 3 walls, 15% infill.
- **Bezel**: window side down on the plate. No supports.
- **Shell**: open side up. No supports - the USB-C slot bridges.
- Print the bezel first and test-fit the OLED before committing to the shell.

Export STLs:

```sh
openscad -o bezel.stl -D 'part="bezel"' deskbuddy_case.scad
openscad -o shell.stl -D 'part="shell"' deskbuddy_case.scad
```

Opening the file with `part = "both"` gives an assembly preview.

Black filament makes the bezel disappear around the display, which is most of
the EMO look. A dark grey or translucent smoke front looks good too.
