/* SPDX-License-Identifier: GPL-2.0-or-later
 * Playlist Builder for Rockbox 4.0 / iPod Video.
 */
#include "plugin.h"
#include "playlist_model.h"

#if !defined(HAVE_TAGCACHE) || CONFIG_KEYPAD != IPOD_4G_PAD
#error Playlist Builder requires an iPod click wheel and the Rockbox database
#endif

/* Audio RAM holds a database snapshot; playback stops when a draft starts. */
struct song {
    char *path, *title, *artist, *album, *albumartist;
    long disc, track;
};
enum view { ROOT, ARTISTS, ARTIST_ALBUMS, ALBUMS, ALBUM_SONGS, SONGS, SEARCH, ORDER,
            ALBUM_ARTISTS, ALBUM_ARTIST_ALBUMS };
static const enum view root_views[] = { ARTISTS, ALBUM_ARTISTS, ALBUMS, SONGS, SEARCH };
enum key { K_NONE, K_UP, K_DOWN, K_OK, K_HOLD, K_LEFT, K_RIGHT, K_PLAY, K_BACK, K_USB };
static struct song *songs;
static int song_count, *rows, row_count, queue[PB_QUEUE_MAX], draft_count;
static char *arena, *arena_end;
static enum view screen;
static int artist_ref, album_ref;
static char query[128], heading[128];
static struct gui_synclist list;
static bool moving;

static void *alloc_bytes(size_t size)
{
    void *result;
    size = (size + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
    if (size > (size_t)(arena_end - arena)) return NULL;
    result = arena; arena += size;
    return result;
}

static char *copy_string(const char *value)
{
    size_t n = rb->strlen(value) + 1;
    char *p = alloc_bytes(n);
    if (p) rb->memcpy(p, value, n);
    return p;
}

/* Timed independently of the firmware's button-repeat delay. */
static enum key read_key(void)
{
    static bool held, fired;
    static long started;
    int b = rb->button_get_w_tmo(HZ / 20);
    if (rb->default_event_handler(b) == SYS_USB_CONNECTED) return K_USB;
    if (b == BUTTON_SELECT) {
        held = true; fired = false; started = *rb->current_tick;
    }
    if (held && !fired && TIME_AFTER(*rb->current_tick, started + 3 * HZ / 4)) {
        fired = true;
        if (b & BUTTON_REL) held = false;
        return K_HOLD;
    }
    if (b == (BUTTON_SELECT | BUTTON_REL)) {
        bool tap = held && !fired;
        held = false;
        return tap ? K_OK : K_NONE;
    }
    if (held && !(rb->button_status() & BUTTON_SELECT)) held = false;
    switch (b) {
    case BUTTON_SCROLL_BACK:
    case BUTTON_SCROLL_BACK | BUTTON_REPEAT: return K_UP;
    case BUTTON_SCROLL_FWD:
    case BUTTON_SCROLL_FWD | BUTTON_REPEAT: return K_DOWN;
    case BUTTON_LEFT:
    case BUTTON_LEFT | BUTTON_REPEAT: return K_LEFT;
    case BUTTON_RIGHT:
    case BUTTON_RIGHT | BUTTON_REPEAT: return K_RIGHT;
    case BUTTON_PLAY | BUTTON_REL: return K_PLAY;
    case BUTTON_MENU | BUTTON_REL: return K_BACK;
    default: return K_NONE;
    }
}

static bool same_album(int a, int b)
{
    return !rb->strcmp(songs[a].album, songs[b].album) &&
           !rb->strcmp(songs[a].albumartist, songs[b].albumartist);
}

static int compare_rows(const void *va, const void *vb)
{
    int a = *(const int *)va, b = *(const int *)vb, c = 0;
    if (screen == ARTISTS) c = rb->strcmp(songs[a].artist, songs[b].artist);
    else if (screen == ALBUM_ARTISTS) c = rb->strcmp(songs[a].albumartist, songs[b].albumartist);
    else if (screen == ALBUMS || screen == ARTIST_ALBUMS || screen == ALBUM_ARTIST_ALBUMS) {
        c = rb->strcmp(songs[a].album, songs[b].album);
        if (!c) c = rb->strcmp(songs[a].albumartist, songs[b].albumartist);
    } else if (screen == ALBUM_SONGS) {
        if (songs[a].disc != songs[b].disc) return songs[a].disc < songs[b].disc ? -1 : 1;
        if (songs[a].track != songs[b].track) return songs[a].track < songs[b].track ? -1 : 1;
    }
    if (!c) c = rb->strcasecmp(songs[a].title, songs[b].title);
    return c ? c : rb->strcmp(songs[a].path, songs[b].path);
}

static const char *row_name(int index, void *data, char *buf, size_t size)
{
    static const char *root[] = { "Artists", "Album Artists", "Albums", "Songs", "Search songs" };
    struct song *s;
    (void)data;
    if (screen == ROOT) return root[index];
    if (!row_count) return screen == ORDER ? "(Empty - Menu: add songs)" : "(No songs found)";
    s = &songs[screen == ORDER ? queue[index] : rows[index]];
    if (screen == ARTISTS) return s->artist;
    if (screen == ALBUM_ARTISTS) return s->albumartist;
    if (screen == ALBUMS || screen == ARTIST_ALBUMS || screen == ALBUM_ARTIST_ALBUMS)
        rb->snprintf(buf, size, "%s - %s", s->album, s->albumartist);
    else if (screen == ORDER)
        rb->snprintf(buf, size, "%d. %s - %s", index + 1, s->title, s->artist);
    else rb->snprintf(buf, size, "%s - %s", s->title, s->artist);
    return buf;
}

static void title(void)
{
    const char *names[] = { "Database", "Artists", "Albums", "Albums", "Tracks", "Songs", "Search", "Order",
                           "Album Artists", "Albums" };
    rb->snprintf(heading, sizeof(heading), "%s (%d) | %s", names[screen], draft_count,
                 moving ? "< > move; OK drop" : screen == ORDER ? "Hold OK: remove" : "Play: next");
    rb->gui_synclist_set_title(&list, heading, Icon_NOICON);
}

static void build_view(enum view next, int selection)
{
    screen = next; row_count = 0; moving = false;
    if (next == ROOT) row_count = sizeof(root_views) / sizeof(*root_views);
    else if (next == ORDER) row_count = draft_count;
    else {
        for (int i = 0; i < song_count; ++i) {
            if (next == ARTIST_ALBUMS && rb->strcmp(songs[i].artist, songs[artist_ref].artist)) continue;
            if (next == ALBUM_ARTIST_ALBUMS && rb->strcmp(songs[i].albumartist, songs[artist_ref].albumartist)) continue;
            if (next == ALBUM_SONGS && !same_album(i, album_ref)) continue;
            if (next == SEARCH && !rb->strcasestr(songs[i].title, query) &&
                !rb->strcasestr(songs[i].artist, query) && !rb->strcasestr(songs[i].album, query)) continue;
            rows[row_count++] = i;
        }
        rb->qsort(rows, row_count, sizeof(*rows), compare_rows);
        if (next == ARTISTS || next == ALBUM_ARTISTS || next == ALBUMS || next == ARTIST_ALBUMS || next == ALBUM_ARTIST_ALBUMS) {
            int n = 0;
            for (int i = 0; i < row_count; ++i)
                if (!n || (next == ARTISTS ? rb->strcmp(songs[rows[i]].artist, songs[rows[n-1]].artist) != 0
                           : next == ALBUM_ARTISTS ? rb->strcmp(songs[rows[i]].albumartist, songs[rows[n-1]].albumartist) != 0
                                          : !same_album(rows[i], rows[n-1]))) rows[n++] = rows[i];
            row_count = n;
        }
    }
    rb->lcd_scroll_stop();
    rb->gui_synclist_set_nb_items(&list, row_count ? row_count : 1);
    if (selection >= row_count) selection = row_count ? row_count - 1 : 0;
    rb->gui_synclist_select_item(&list, selection);
    title();
}

/* Remove only the selected occurrence, never the music file itself. */
static void remove_selected(int pos)
{
    if (pos < 0 || pos >= draft_count) return;
    rb->memmove(queue + pos, queue + pos + 1, (draft_count - pos - 1) * sizeof(*queue));
    --draft_count;
    build_view(ORDER, pos);
    rb->splash(HZ / 2, "Removed from playlist");
}

/* A numeric search walks live master-index entries, skipping deleted records.
 * Searching unfiltered filenames on disk can still return deleted entries. */
static int load_database(void)
{
    struct tagcache_search tcs;
    char buf[TAGCACHE_BUFSZ];
    size_t size;
    int capacity = rb->tagcache_get_stat()->total_entries;
    if (!rb->tagcache_get_stat()->ready || capacity <= 0) {
        rb->splash(2 * HZ, "Initialize the Rockbox database first"); return PLUGIN_ERROR;
    }
    arena = rb->plugin_get_audio_buffer(&size);
    if (!arena) return PLUGIN_ERROR;
    arena_end = arena + size;
    if ((size_t)capacity > size / (sizeof(*songs) + sizeof(*rows))) goto full;
    songs = alloc_bytes((size_t)capacity * sizeof(*songs));
    rows = alloc_bytes((size_t)capacity * sizeof(*rows));
    if (!songs || !rows) goto full;
    song_count = 0;
    if (!rb->tagcache_search(&tcs, tag_length)) {
        rb->tagcache_search_finish(&tcs);
        rb->splash(2 * HZ, "Cannot open database"); return PLUGIN_ERROR;
    }
    while (rb->tagcache_get_next(&tcs, buf, sizeof(buf))) {
        struct song *s;
        int id = tcs.idx_id;
        if (song_count >= capacity) goto load_full;
        s = &songs[song_count];
        if (!rb->tagcache_retrieve(&tcs, id, tag_filename, buf, sizeof(buf))) goto bad_db;
        s->path = copy_string(buf);
        if (!s->path) goto load_full;
        /* Failed retrieval is a load failure, never silently save wrong paths. */
#define GET_TAG(field, tag) \
        if (!rb->tagcache_retrieve(&tcs, id, tag, buf, sizeof(buf))) goto bad_db; \
        s->field = copy_string(buf); if (!s->field) goto load_full
        GET_TAG(title, tag_title);
        GET_TAG(artist, tag_artist);
        GET_TAG(album, tag_album);
        GET_TAG(albumartist, tag_albumartist);
#undef GET_TAG
        if (!s->albumartist[0] || !rb->strcmp(s->albumartist, UNTAGGED)) s->albumartist = s->artist;
        if (!s->title[0] || !rb->strcmp(s->title, UNTAGGED)) s->title = s->path;
        s->disc = rb->tagcache_get_numeric(&tcs, tag_discnumber);
        s->track = rb->tagcache_get_numeric(&tcs, tag_tracknumber);
        ++song_count;
        if ((song_count % 64) == 0) {
            int b = rb->button_get(false);
            if (rb->default_event_handler(b) == SYS_USB_CONNECTED) {
                rb->tagcache_search_finish(&tcs); return PLUGIN_USB_CONNECTED;
            }
            if (b == BUTTON_MENU) { rb->tagcache_search_finish(&tcs); return PLUGIN_OK; }
            rb->splashf(0, "Loading database: %d", song_count);
            rb->yield();
        }
    }
    rb->tagcache_search_finish(&tcs);
    if (!song_count) { rb->splash(2 * HZ, "Database is empty"); return PLUGIN_ERROR; }
    return 100; /* loaded */
bad_db:
    rb->tagcache_search_finish(&tcs);
    rb->splash(2 * HZ, "Database read failed"); return PLUGIN_ERROR;
load_full:
    rb->tagcache_search_finish(&tcs);
full:
    rb->splash(2 * HZ, "Library exceeds available memory"); return PLUGIN_ERROR;
}

static void add_album(int ref)
{
    int needed = 0, begin = draft_count;
    for (int i = 0; i < song_count; ++i) if (same_album(i, ref)) ++needed;
    if (needed > PB_QUEUE_MAX - draft_count) {
        rb->splash(2 * HZ, "Album will not fit (10,000 limit)"); return;
    }
    for (int i = 0; i < song_count; ++i) if (same_album(i, ref)) queue[draft_count++] = i;
    enum view old = screen;
    screen = ALBUM_SONGS;
    rb->qsort(queue + begin, needed, sizeof(*queue), compare_rows);
    screen = old;
    rb->splashf(HZ / 2, "Added %d songs", needed);
}

static bool write_all(int fd, const char *s)
{
    size_t left = rb->strlen(s);
    while (left) {
        ssize_t n = rb->write(fd, s, left);
        if (n <= 0) return false;
        left -= n; s += n;
    }
    return true;
}

static bool save_playlist(void)
{
    char name[128] = "", path[MAX_PATH], temp[MAX_PATH];
    int fd;
    if (!draft_count) { rb->splash(HZ, "Add some songs first"); return false; }
    for (;;) {
        if (rb->kbd_input(name, sizeof(name), NULL) < 0) return false;
        if (!pb_valid_name(name)) { rb->splash(2 * HZ, "Invalid filename; try again"); continue; }
        int len = rb->strlen(name);
        if (len > 5 && !rb->strcasecmp(name + len - 5, ".m3u8")) name[len - 5] = '\0';
        if (!pb_valid_name(name)) continue;
        rb->snprintf(path, sizeof(path), "/Playlists/%s.m3u8", name);
        rb->snprintf(temp, sizeof(temp), "/Playlists/%s.m3u8.part", name);
        if (rb->file_exists(path) || rb->file_exists(temp)) {
            rb->splash(2 * HZ, "Name already exists; choose another"); continue;
        }
        break;
    }
    if (!rb->dir_exists("/Playlists") && rb->mkdir("/Playlists") < 0) goto failed;
    fd = rb->open(temp, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0) goto failed;
    bool ok = write_all(fd, "#EXTM3U\n");
    for (int i = 0; ok && i < draft_count; ++i) {
        const char *p = songs[queue[i]].path;
        if (p[0] != '/' || rb->strchr(p, '\n') || rb->strchr(p, '\r') || !rb->file_exists(p)) {
            ok = false; break;
        }
        ok = write_all(fd, p) && write_all(fd, "\n");
        rb->yield();
    }
    if (rb->close(fd) < 0) ok = false;
    if (!ok || rb->file_exists(path) || rb->rename(temp, path) < 0) {
        rb->remove(temp); goto failed;
    }
    rb->splashf(3 * HZ, "Saved %s", path);
    return true;
failed:
    rb->splash(3 * HZ, "Save failed; draft kept. Check disk / music files");
    return false;
}

enum plugin_status plugin_start(const void *parameter)
{
    MENUITEM_STRINGLIST(menu, "Playlist Builder", NULL,
                        "New playlist (stops playback)", "Help", "Exit");
    int selected = 0, result;
    (void)parameter;
    for (;;) {
        result = rb->do_menu(&menu, &selected, NULL, false);
        if (result == MENU_ATTACHED_USB) return PLUGIN_USB_CONNECTED;
        if (result != 0 && result != 1) return PLUGIN_OK;
        if (!result) break;
        rb->splash(6 * HZ, "Wheel: browse. OK: add/open. Hold OK: album. Play: order/save. Menu: back. Order: OK grab, Left/Right move, Hold OK delete.");
    }
    result = load_database();
    if (result != 100) return result;
    draft_count = 0;
    rb->gui_synclist_init(&list, row_name, NULL, false, 1, NULL);
    build_view(ROOT, 0);
    enum view album_parent = ALBUMS;
    for (;;) {
        title();
        rb->gui_synclist_draw(&list);
        enum key key = read_key();
        int pos = rb->gui_synclist_get_sel_pos(&list);
        if (key == K_USB) { result = PLUGIN_USB_CONNECTED; break; }
        if (key == K_UP || key == K_DOWN) {
            if (screen == ORDER && moving && row_count) {
                pos = pb_move(queue, draft_count, pos, key == K_UP ? -1 : 1);
                rb->gui_synclist_select_item(&list, pos);
            } else {
                /* We read raw button events, not get_action(). On iPods,
                 * gui_synclist_do_button() derives its step from cached
                 * get_action_data(), which is stale (often zero) here.
                 * Select directly so wheel events always advance a row. */
                int next = pos + (key == K_UP ? -1 : 1);
                if (next >= 0 && next < row_count)
                    rb->gui_synclist_select_item(&list, next);
            }
        } else if (key == K_PLAY) {
            if (!draft_count) rb->splash(HZ, "Add some songs first");
            else if (screen != ORDER) build_view(ORDER, 0);
            else if (save_playlist()) { result = PLUGIN_OK; break; }
        } else if (screen == ORDER) {
            if (key == K_BACK) build_view(ROOT, 0);
            else if (row_count && key == K_OK) moving = !moving;
            else if (row_count && moving && (key == K_LEFT || key == K_RIGHT)) {
                pos = pb_move(queue, draft_count, pos, key == K_LEFT ? -1 : 1);
                rb->gui_synclist_select_item(&list, pos);
            } else if (row_count && key == K_HOLD) {
                remove_selected(pos);
            }
        } else if (key == K_BACK || key == K_LEFT) {
            if (screen == ROOT) {
                if (draft_count) {
                    MENUITEM_STRINGLIST(exit_menu, "Unsaved playlist", NULL, "Keep editing", "Discard and exit");
                    int choice = rb->do_menu(&exit_menu, NULL, NULL, false);
                    if (choice == MENU_ATTACHED_USB) { result = PLUGIN_USB_CONNECTED; break; }
                    if (choice != 1) continue;
                }
                result = PLUGIN_OK; break;
            }
            build_view(screen == ALBUM_SONGS ? album_parent : screen == ARTIST_ALBUMS ? ARTISTS :
                       screen == ALBUM_ARTIST_ALBUMS ? ALBUM_ARTISTS : ROOT, 0);
        } else if (row_count && (key == K_OK || key == K_HOLD || key == K_RIGHT)) {
            if (screen == ROOT) {
                if (key == K_HOLD) continue;
                if (root_views[pos] == SEARCH) {
                    if (rb->kbd_input(query, sizeof(query), NULL) < 0) continue;
                    build_view(SEARCH, 0);
                } else build_view(root_views[pos], 0);
            } else if (screen == ARTISTS || screen == ALBUM_ARTISTS) {
                if (key == K_HOLD) continue;
                artist_ref = rows[pos];
                build_view(screen == ALBUM_ARTISTS ? ALBUM_ARTIST_ALBUMS : ARTIST_ALBUMS, 0);
            } else if (screen == ALBUMS || screen == ARTIST_ALBUMS || screen == ALBUM_ARTIST_ALBUMS) {
                if (key == K_HOLD) add_album(rows[pos]);
                else { album_parent = screen; album_ref = rows[pos]; build_view(ALBUM_SONGS, 0); }
            } else if (key == K_OK) {
                if (draft_count == PB_QUEUE_MAX) rb->splash(HZ, "Playlist full (10,000)");
                else { queue[draft_count++] = rows[pos]; rb->splash(HZ / 4, "Added"); }
            }
        }
    }
    rb->lcd_scroll_stop();
    return result;
}
