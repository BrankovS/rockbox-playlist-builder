# Playlist Builder — iPod Video / Rockbox 4.0

A native Rockbox plugin for selecting songs from your existing Rockbox database, arranging them, and saving a UTF-8 `.m3u8` playlist.

**Status: revision 3 — Album Artists and clearer removal controls.** The user confirmed the wheel-fix build works on their iPod. This revision adds Album Artists → Albums → Tracks, using track artist as a fallback when album artist is missing. The ordering screen now displays a hold-Centre removal hint and confirms removals. Automated tests cover compilation-album navigation, back navigation, missing album-artist tags, and single-removal-per-hold behavior. This revision still needs its device check.

## Install

1. Connect the iPod by USB.
2. Copy `playlist_builder.rock` from this package to `/.rockbox/rocks/apps/` on the iPod. Alternatively, extract the install ZIP at the root of the iPod; it contains the correct `.rockbox/rocks/apps/` folder.
3. Safely eject the iPod.
4. Open **Plugins → Applications → playlist_builder** and choose **New playlist**.

Use this build with **Rockbox 4.0 on iPod Video (5th/5.5th generation)**. Your originally 30 GB iPod with an iFlash Solo and 128 GB card uses this same target. Storage upgrades do not change the plugin target or add RAM.

No firmware or bootloader replacement is needed. To uninstall, delete that one `.rock` file. Saved playlists remain in `/Playlists/`.

## Controls

| Screen | Control | Action |
|---|---|---|
| Database | Wheel | Browse |
| Database | Centre, short press | Open category/album; add a song |
| Album row | Hold Centre for approximately 0.75 seconds | Add the entire album, once per hold |
| Database | Left or Menu | Go back |
| Database | Play | Go to ordering |
| Ordering | Wheel | Select a song |
| Ordering | Centre | Grab/drop the selected song |
| Ordering, grabbed | Left / Right | Move the song up / down |
| Ordering, grabbed | Wheel | Move the grabbed song |
| Ordering | Hold Centre for approximately 0.75 seconds | Remove the selected occurrence from the draft; the music file stays on disk |
| Ordering | Menu | Return to database and add more songs |
| Ordering | Play | Open the Rockbox keyboard, then save |

To remove a song before naming, press Play to open the ordering screen, highlight the song, and hold Centre until “Removed from playlist” appears. Release Centre before removing another song. Removing the last entry leaves an empty draft; Menu returns to the database to add more.

Adding songs or albums leaves you in the current database view. The title shows the number of queued tracks. Albums are added in disc-number, then track-number order. Repeated additions are allowed, so a song can appear more than once.

The final naming screen uses Rockbox's existing keyboard and its normal accept/cancel controls. Enter a name with or without `.m3u8`. Cancel returns to ordering with the draft intact. A successful save exits the app and shows the saved path, such as `/Playlists/Evening mix.m3u8`.

## Database and saving details

- Initialize/update the Rockbox database before launching. The app reads it through the firmware API; it does not parse the iTunes database or modify database files.
- Navigation is **Artists → Albums → Tracks**, **Album Artists → Albums → Tracks**, **Albums**, **Songs**, and **Search songs**. Search matches title, artist, and album. It uses the native Rockbox list widget, but does not embed the stock database browser or import `tagnavi` custom menu definitions. Other stock views such as Genres and Ratings are not included in this version.
- Album identity uses album title plus album artist, falling back to track artist if album artist is missing. This separates identically named albums by different artists. Compilation albums need a consistent album-artist tag to group correctly. Different releases with identical album and album-artist tags are grouped together.
- Starting a draft takes audio-buffer memory and stops playback. Library capacity depends on available RAM and metadata lengths, not the SD card capacity. The app reports insufficient memory instead of deliberately truncating the library. There is a 10,000-entry playlist limit; an album that will not fit is rejected as a whole.
- Files contain absolute on-device music paths in UTF-8. Existing playlists are never overwritten; choose another name if it is already used. Saving checks for missing files, handles short writes, and writes a temporary `.part` file before renaming to `.m3u8`. A save failure keeps the in-memory draft for another attempt.
- Drafts are held in memory. Discarding, USB exit, power loss, or leaving the plugin loses an unsaved draft. Following an interrupted save, a leftover `.part` file may need to be removed or a different playlist name chosen.

## Validation

The host tests compile and execute the actual `playlist_builder.c` with a fake device API and in-memory filesystem. Covered cases include category grouping, same-name albums, compilation albums, disc/track ordering, whole-album capacity checks, intentional duplicates, empty search results, reorder boundaries, short/long Centre behavior, keyboard cancellation, filename collisions, UTF-8 output, partial writes, write/close/rename failures, missing tracks, and the full workflow.

The `.rock` binary is little-endian ARMv4T / soft-float EABI, target ID 15, plugin API version 273. It uses Rockbox's original plugin startup, compiler-support routines, setjmp implementation, and linker script. Its linked symbols and header are checked by `verify_binary.py`.

The standalone build uses GNU Arm GCC 14.3 rather than Rockbox 4.0's recommended 4.9.4 toolchain. The target headers come from Rockbox 4.0. The complete workflow and button timing still need device confirmation. Downloading the official prebuilt release for a binary-to-binary ABI comparison returned an anti-bot page, so that comparison was not completed.

Suggested first device check: add two songs and one album, reorder them, save a playlist with a non-ASCII name, then open it through Rockbox's Playlist Catalogue. Verify the saved order and album contents. Test on a small playlist before a long session.

## Rebuild on Windows

Requirements: PowerShell, Git, Perl, a host GCC, Python 3, and an ARM EABI GCC with `arm7tdmi` support and matching binutils. The build installs nothing globally.

```powershell
git clone --branch v4.0-final --depth 1 https://github.com/Rockbox/rockbox.git rockbox
./playlist_builder/build.ps1 -Compiler 'C:/path/to/arm-none-eabi-gcc.exe'
gcc -std=gnu99 -Wall -Wextra -Werror -Iplaylist_builder/tests playlist_builder/tests/test_playlist.c -o playlist_builder/build/test_playlist.exe
./playlist_builder/build/test_playlist.exe
python playlist_builder/verify_binary.py playlist_builder/build/playlist_builder.rock
```

`-Rockbox` and `-HostCompiler` optionally override their paths. Output is `playlist_builder/build/playlist_builder.rock`. `autoconf.h` records the standalone configuration corresponding to normal target 22 (`ipodvideo`), including the firmware's compile-time `MEMORYSIZE=64`; Rockbox handles the original 30 GB model's smaller physical memory at runtime.

For Rockbox's standard build system instead: copy `playlist_builder.c` and `playlist_model.h` to `apps/plugins/`, add `playlist_builder.c` to `apps/plugins/SOURCES` inside an `#if defined(HAVE_TAGCACHE) && CONFIG_KEYPAD == IPOD_4G_PAD` guard, and add `playlist_builder,apps` to `apps/plugins/CATEGORIES`. Configure a normal iPod Video build from the 4.0 source and run `make`. Use the resulting `apps/plugins/playlist_builder.rock`.

Source: [Rockbox 4.0 release tag](https://github.com/Rockbox/rockbox/tree/v4.0-final), commit `e094c599` (clone this release into the repository-root `rockbox` folder before building). Plugin code is GPL-2.0-or-later; see `COPYING` and the upstream source notices. The `tests/plugin.h` stub is only for host tests and must never be used to compile a device binary.
