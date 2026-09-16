# Rockbox Playlist Builder

Create playlists directly on an iPod Video running **Rockbox 4.0**. Browse the music database, add songs or whole albums, arrange the order, and save a UTF-8 `.m3u8` playlist without starting each selected song.

## Features

- Browse **Artists**, **Album Artists**, **Albums**, **Songs**, or search by title, artist, and album.
- Press Centre to add a song while staying in the database browser.
- Hold Centre on an album for approximately **0.75 seconds** to add the whole album in disc/track order.
- Press Play to review the playlist, reorder songs, or remove individual entries.
- Name the playlist with Rockbox's keyboard and save it under `/Playlists/`.
- Keep existing playlist files intact: duplicate names prompt for another name.

## Install

Download **playlist_builder.rock** from this repository's Releases page.

1. Connect your iPod over USB.
2. Copy the file into `/.rockbox/rocks/apps/` on the iPod.
3. Safely eject and open **Plugins → Applications → playlist_builder**.
4. Choose **New playlist**.

No firmware or bootloader replacement is required. To update, replace the same `.rock` file. To uninstall, remove it; your saved playlists remain.

**Target:** iPod Video / 5th and 5.5th generation, Rockbox 4.0. Flash-storage upgrades use the same target. This binary is not for iPod Classic 6th/7th generation or a PC simulator.

## Controls

| Screen | Button | Action |
|---|---|---|
| Database | Wheel | Browse |
| Database | Centre | Open a category/album or add a song |
| Album row | Hold Centre | Add the complete album |
| Database | Left / Menu | Back |
| Database | Play | Review and reorder |
| Ordering | Wheel | Select a song |
| Ordering | Centre | Grab / drop a song |
| Ordering, grabbed | Left / Right or wheel | Move the song up / down |
| Ordering | Hold Centre | Remove one playlist entry; keep the music file |
| Ordering | Menu | Return to the database to add more |
| Ordering | Play | Name and save the playlist |

Release Centre between removals. Cancelling the keyboard returns to ordering with the draft intact.

## Notes

Starting a draft stops playback and uses audio-buffer RAM for the database snapshot. Drafts are held in memory and are lost on exit or power loss. Playlists support up to 10,000 entries; library capacity depends on available RAM and metadata lengths.

Album Artists falls back to track Artist when the album-artist tag is missing. Albums are grouped by album title plus album artist; compilation albums need consistent tags. The plugin uses its own database-backed browser and does not import custom `tagnavi` menus.

The wheel-navigation fix has been tested successfully on an iPod Video. Album Artists and the latest removal feedback pass automated tests and still need device verification. The ARM binary uses Rockbox 4.0's headers with GCC 14.3, rather than the release's recommended GCC 4.9.4.

## Build and test

Source and build scripts are in [`playlist_builder/`](playlist_builder/). See the [detailed build instructions](playlist_builder/README.md).

From the repository root on Windows, with Git, Perl, Python 3, host GCC, and an ARM EABI compiler installed:

```powershell
git clone --branch v4.0-final --depth 1 https://github.com/Rockbox/rockbox.git rockbox
./playlist_builder/build.ps1 -Compiler 'C:/path/to/arm-none-eabi-gcc.exe'
gcc -std=gnu99 -Wall -Wextra -Werror -Iplaylist_builder/tests playlist_builder/tests/test_playlist.c -o playlist_builder/build/test_playlist.exe
./playlist_builder/build/test_playlist.exe
python playlist_builder/verify_binary.py playlist_builder/build/playlist_builder.rock
```

Output: `playlist_builder/build/playlist_builder.rock`. The upstream Rockbox checkout and generated build files are excluded from this repository.

Tests cover wheel navigation, album-artist and compilation grouping, adding albums, reordering, one removal per hold, filename validation, UTF-8 output, save failures, and the complete creation workflow. They use a fake device API; they are not a hardware emulator.

## License

GPL-2.0-or-later. See [LICENSE](LICENSE). Rockbox code linked into the binary retains its upstream copyright and license notices. Upstream source: [Rockbox v4.0-final](https://github.com/Rockbox/rockbox/tree/v4.0-final).
