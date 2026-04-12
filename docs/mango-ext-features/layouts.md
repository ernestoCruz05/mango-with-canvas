# New layouts in mango-ext

## Canvas

Mango-exts `canvas` layout is similar to window managers like hevel. It lets you move around floating windows on an infinite canvas (hence the name), move around your view along that canvas, pan between windows and zoom in and out.

### canvas-specific config options

|option|type|Description|
|:---|:---|:---|
|`canvas_tiling`|int| determines the position of newly spawned windows. see section `canvas_tiling options`|
|`canvas_tiling_gap`|int| determines the size of window gaps when using canvas_tiling|
|`canvas_pan_on_kill`|bool| wether or not to pan to another window when the current window is killed|
|`canvas_anchor_animate`|bool| when set to 1, smoothly animates the pan when jumping to an anchor point. Default: 0 (instant teleport)|

#### canvas_tiling options

|option|Description|
|:---|:---|
|0| spawns new windows in the center of the viewport|
|1| spawns new windows to the left of the focused window|
|2| spawns new windows to the right of the focused window|
|3| spawns new windows below the focused window|
|4| spawns new windows above the focused window|
|5| picks between 1 - 4 depending on the position of the mouse in relation to the focused window |

### canvas-specific dispatchers

|dispatcher|Description|
|:--- |:---|
|`canvas-overview-toggle`| toggles the canvas layouts' overview feature. This lets you see all of your windows at once and quickly reposition your view.|
|`toggleminimap`| toggles the minimap, which displays all of your windows along with a visual indicator of your current view.|
|`canvas_zoom_resize,<arg>`| zooms in/out, direction and strength are determined by <arg>|
|`canvas_fill_viewport`| resizes the currently focused window to make it fill up the entire current viewport|
|`canvas_centerview`| moves the viewport to center the currently focused window|
|`canvas_drag_pan`| enters canvas drag-pan mode (use with `mousebind` to bind to a mouse button)|
|`canvas_anchor_set,<slot>`| saves the current pan position to anchor slot `<slot>` (0–8). Anchors are per-tag.|
|`canvas_anchor_go,<slot>`| jumps to the pan position saved in anchor slot `<slot>` (0–8). No-op if the slot is unset.|

### Canvas anchor points

Anchor points let you bookmark up to 9 positions on the canvas per tag and jump back to them instantly (or with smooth animation).

**Setup example** (using numpad keys, NumLock off):

```ini
# Save anchors with SUPER+ALT + numpad
mousebind = ALT, BTN_MIDDLE, canvas_drag_pan

bind = SUPER+ALT, KP_Insert, canvas_anchor_set, 0
bind = SUPER+ALT, KP_End,    canvas_anchor_set, 1
bind = SUPER+ALT, KP_Down,   canvas_anchor_set, 2
bind = SUPER+ALT, KP_Next,   canvas_anchor_set, 3
bind = SUPER+ALT, KP_Left,   canvas_anchor_set, 4
bind = SUPER+ALT, KP_Begin,  canvas_anchor_set, 5
bind = SUPER+ALT, KP_Right,  canvas_anchor_set, 6
bind = SUPER+ALT, KP_Home,   canvas_anchor_set, 7
bind = SUPER+ALT, KP_Up,     canvas_anchor_set, 8

# Jump to anchors with SUPER + numpad
bind = SUPER, KP_Insert, canvas_anchor_go, 0
bind = SUPER, KP_End,    canvas_anchor_go, 1
bind = SUPER, KP_Down,   canvas_anchor_go, 2
bind = SUPER, KP_Next,   canvas_anchor_go, 3
bind = SUPER, KP_Left,   canvas_anchor_go, 4
bind = SUPER, KP_Begin,  canvas_anchor_go, 5
bind = SUPER, KP_Right,  canvas_anchor_go, 6
bind = SUPER, KP_Home,   canvas_anchor_go, 7
bind = SUPER, KP_Up,     canvas_anchor_go, 8

# Optional: smooth pan animation
canvas_anchor_animate = 1
```

Set anchors appear as color-coded dots on the minimap (red, orange, yellow, green, cyan, blue, purple, pink, white for slots 0–8).

> **Note:** Anchors are stored in memory only and reset when the compositor restarts. Zoom is not affected by anchors — only the pan position is saved and restored.


## Dwindle

The `dwindle` layout as known from WMs like bspwm and hyprland.Creating new windows splits the currently focused window either horizontally or vertically depending on its proportions.

