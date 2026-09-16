# v0.1.0 — On-device playlist creation

First public release of Rockbox Playlist Builder for **iPod Video (5th/5.5th generation), Rockbox 4.0**.

- Browse Artists, Album Artists, Albums, Songs, and search results.
- Add songs with Centre or entire albums with a 0.75-second Centre hold.
- Reorder tracks and hold Centre to remove an entry before naming.
- Save UTF-8 `.m3u8` playlists using the native Rockbox keyboard.
- Includes the tested click-wheel navigation fix.

## Install

Copy the attached `playlist_builder.rock` to `/.rockbox/rocks/apps/`, safely eject, and open Plugins → Applications → playlist_builder.

Starting a draft stops playback. Unsaved drafts are held in memory. The binary is compiled against Rockbox 4.0; do not use it with other targets or a PC simulator.

## Validation

Automated workflow and regression tests pass, including Album Artists and deletion. Binary header validation passes (target 15, API 273). The earlier wheel-fix revision was tested on a real iPod; the newest Album Artists/removal changes still require device verification.

SHA-256: `e21bede0d9ac7745341a42916da8e7e74a40025e4b7f679dca573f0b8c597fd0`
