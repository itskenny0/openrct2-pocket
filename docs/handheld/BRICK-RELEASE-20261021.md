# Brick release: shared settings sheet and real-park captures

The previous scenery/landscape settings capture had full-width tabs crossing
between two separately framed panels. The shared tab bar now belongs to one
continuous sheet. Its two content columns have a divider below the tabs;
there are no panel edges or grass gaps running through the tab strip.
Other management pages retain their own existing frames.

Validated at native 512×384 using FriendlyUiTests (controller action dispatch,
group boundaries, group focus restoration, disabled controls, material-picker
return, native scenery height, track connections and technical-details return).
The corrected scenery settings PNG was opened and visually inspected.

AArch64 packaging, runtime closure and real-park capture results are recorded
below when completed. Physical Brick testing is performed by the player;
host captures are not device captures.
