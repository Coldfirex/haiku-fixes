# touchpad-multifinger-liftoff

When a multi-finger contact drops to one finger with no button held, stock movement_maker can turn leftover scroll motion into pointer movement. This patch tracks the finger count and calls StartNewMovment() when it changes so the previous position and delta sums are reset. On lift from multi-finger to single finger with no button it also clears the button state.

Tested idea taken from the X270 gesture work. Not Gerrit yet.

Build the mouse add-on, drop it in ~/config/non-packaged/add-ons/input_server/devices/mouse, then kill input_server.
