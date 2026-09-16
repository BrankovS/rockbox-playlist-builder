/* Host-only fake device API. Production builds use Rockbox's actual plugin.h. */
#ifndef TEST_PLUGIN_H
#define TEST_PLUGIN_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#define HAVE_TAGCACHE 1
#define IPOD_4G_PAD 1
#define CONFIG_KEYPAD IPOD_4G_PAD
#define HZ 100
#define MAX_PATH 260
#define TAGCACHE_BUFSZ 552
#define UNTAGGED "<Untagged>"
#define BUTTON_SELECT 1
#define BUTTON_LEFT 2
#define BUTTON_RIGHT 4
#define BUTTON_PLAY 8
#define BUTTON_MENU 16
#define BUTTON_SCROLL_BACK 32
#define BUTTON_SCROLL_FWD 64
#define BUTTON_REPEAT 256
#define BUTTON_REL 512
#define SYS_USB_CONNECTED 1024
#define MENU_ATTACHED_USB -2
#define ACTION_STD_PREV 1
#define ACTION_STD_NEXT 2
#define Icon_NOICON -1
#define TIME_AFTER(a,b) ((long)(b)-(long)(a)<0)
#define MENUITEM_STRINGLIST(name, ...) int name = 0
enum plugin_status { PLUGIN_OK = 0, PLUGIN_USB_CONNECTED = 256, PLUGIN_ERROR = -1 };
enum {tag_filename, tag_title, tag_artist, tag_album, tag_albumartist, tag_discnumber, tag_tracknumber, tag_length};
struct tagcache_search { int idx_id; char *result; };
struct tagcache_stat { bool ready; int total_entries; };
struct gui_synclist { int count, selected; };
struct plugin_api {
    size_t (*strlen)(const char *);
    void *(*memcpy)(void *, const void *, size_t);
    void *(*memmove)(void *, const void *, size_t);
    char *(*strchr)(const char *, int);
    int (*strcmp)(const char *, const char *);
    int (*strcasecmp)(const char *, const char *);
    char *(*strcasestr)(const char *, const char *);
    int (*snprintf)(char *, size_t, const char *, ...);
    void (*qsort)(void *, size_t, size_t, int (*)(const void *, const void *));
    long (*button_get_w_tmo)(int);
    int (*button_get)(bool);
    int (*button_status)(void);
    long *current_tick;
    long (*default_event_handler)(long);
    void (*gui_synclist_set_title)(struct gui_synclist *, const char *, int);
    void (*gui_synclist_set_nb_items)(struct gui_synclist *, int);
    void (*gui_synclist_select_item)(struct gui_synclist *, int);
    void (*gui_synclist_init)(struct gui_synclist *, const char *(*)(int, void *, char *, size_t), void *, bool, int, void *);
    void (*gui_synclist_draw)(struct gui_synclist *);
    int (*gui_synclist_get_sel_pos)(struct gui_synclist *);
    bool (*gui_synclist_do_button)(struct gui_synclist *, int *);
    void (*lcd_scroll_stop)(void);
    struct tagcache_stat *(*tagcache_get_stat)(void);
    bool (*tagcache_search)(struct tagcache_search *, int);
    void (*tagcache_search_finish)(struct tagcache_search *);
    bool (*tagcache_get_next)(struct tagcache_search *, char *, long);
    bool (*tagcache_retrieve)(struct tagcache_search *, int, int, char *, long);
    long (*tagcache_get_numeric)(const struct tagcache_search *, int);
    void *(*plugin_get_audio_buffer)(size_t *);
    void (*splash)(int, const char *);
    void (*splashf)(int, const char *, ...);
    void (*yield)(void);
    int (*kbd_input)(char *, int, unsigned short *);
    bool (*file_exists)(const char *);
    bool (*dir_exists)(const char *);
    int (*mkdir)(const char *);
    int (*open)(const char *, int, ...);
    ssize_t (*write)(int, const void *, size_t);
    int (*close)(int);
    int (*rename)(const char *, const char *);
    int (*remove)(const char *);
    int (*do_menu)(const void *, int *, void *, bool);
};
extern const struct plugin_api *rb;
#endif
