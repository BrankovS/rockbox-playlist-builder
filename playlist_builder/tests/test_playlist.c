/* Exercises the production plugin logic with an in-memory device and disk. */
#include <assert.h>
#include "../playlist_builder.c"

static long tick;
static int button, physical, elapsed = 5;
static char output[4096], keyboard[128] = "Test", saved_path[260];
static size_t written;
static int fail_after = -1, close_error, rename_error, cancelled, collisions;
static int created, renamed, removed;
static bool missing_song;
static const int *script;
static int script_pos, script_length;
#define HOLD_TICK 2048 /* test-only: wait past the Centre hold threshold */
static struct song fixture[] = {
    {"/Music/alpha-2.flac", "Second", "Alpha", "Shared", "Alpha", 1, 2},
    {"/Music/beta.flac", "Other", "Beta", "Shared", "Beta", 1, 1},
    {"/Music/alpha-1.flac", "First", "Alpha", "Shared", "Alpha", 1, 1},
    {"/Music/alpha-disc2.flac", "Disc two", "Alpha", "Shared", "Alpha", 2, 1},
    {"/Music/comp-a.flac", "Guest A", "Alpha", "Mix", "Various Artists", 1, 2},
    {"/Music/comp-b.flac", "Guest B", "Beta", "Mix", "Various Artists", 1, 1},
    {"/Music/Bj\xc3\xb6rk.flac", "J\xc3\xb3ga", "Bj\xc3\xb6rk", "Homogenic", "Bj\xc3\xb6rk", 1, 1},
};
static int row_storage[32];
static char *contains(const char *a, const char *b) {
    for (; *a; ++a) if (!_strnicmp(a,b,strlen(b))) return (char *)a;
    return *b ? NULL : (char *)a;
}
static void noop(void) {}
static void splash(int n, const char *s) {(void)n;(void)s;}
static void splashf(int n, const char *s, ...) {(void)n;(void)s;}
static void set_title(struct gui_synclist *l,const char*s,int n){(void)l;(void)s;(void)n;}
static void set_count(struct gui_synclist *l,int n){l->count=n;}
static void select_item(struct gui_synclist *l,int n){l->selected=n;}
static long get_button(int n){(void)n;tick+=elapsed; int b=button;button=0;if(script){assert(script_pos<script_length);b=script[script_pos++];}if(b==HOLD_TICK){tick+=76;b=0;}if(b==BUTTON_SELECT)physical=b;if(b==(BUTTON_SELECT|BUTTON_REL))physical=0;return b;}
static int status(void){return physical;}
static long event(long b){return b;}
static int kbd(char *b,int n,unsigned short *k){(void)k; if(cancelled)return -1; snprintf(b,n,"%s",keyboard); if(collisions) {strcpy(keyboard,"Unique");} return 0;}
static bool exists(const char *p){if(!strncmp(p,"/Music/",7))return !missing_song; if(collisions && strstr(p,"Test.m3u8")){--collisions;return true;} return false;}
static bool directory(const char *p){(void)p;return true;}
static int make_dir(const char *p){(void)p;return 0;}
static int open_file(const char *p,int f,...){assert(strstr(p,".part"));assert(f&O_EXCL);++created;return 42;}
static ssize_t write_file(int f,const void *p,size_t n){assert(f==42); if(fail_after>=0 && (int)written>=fail_after)return -1; if(n>3)n=3;assert(written+n<sizeof(output));memcpy(output+written,p,n);written+=n;output[written]=0;return n;}
static int close_file(int f){assert(f==42);return close_error?-1:0;}
static int rename_file(const char*a,const char*b){(void)a;if(rename_error)return -1;++renamed;snprintf(saved_path,sizeof(saved_path),"%s",b);return 0;}
static int remove_file(const char*p){(void)p;++removed;return 0;}
static unsigned char memory[1024*1024];
static struct tagcache_stat dbstat={true,7};
static struct tagcache_stat *get_stat(void){return &dbstat;}
static void *get_memory(size_t *s){*s=sizeof(memory);return memory;}
static bool search_db(struct tagcache_search *t,int tag){assert(tag==tag_length);t->idx_id=-1;return true;}
static void finish_db(struct tagcache_search*t){(void)t;}
static bool next_db(struct tagcache_search*t,char*b,long n){if(++t->idx_id==7)return false;snprintf(b,n,"100");t->result=b;return true;}
static bool retrieve_db(struct tagcache_search*t,int id,int tag,char*b,long n){(void)t;const char*s=NULL;struct song*f=&fixture[id];switch(tag){case tag_filename:s=f->path;break;case tag_title:s=f->title;break;case tag_artist:s=f->artist;break;case tag_album:s=f->album;break;case tag_albumartist:s=f->albumartist;break;default:assert(0);}snprintf(b,n,"%s",s);return true;}
static long numeric_db(const struct tagcache_search*t,int tag){return tag==tag_discnumber?fixture[t->idx_id].disc:fixture[t->idx_id].track;}
static int menu_choice(const void*m,int*s,void*p,bool b){(void)m;(void)s;(void)p;(void)b;return 0;}
static void init_list(struct gui_synclist*l,const char*(*cb)(int,void*,char*,size_t),void*d,bool a,int b,void*p){(void)cb;(void)d;(void)a;(void)b;(void)p;l->selected=0;}
static void draw(struct gui_synclist*l){(void)l;}
static int get_selected(struct gui_synclist*l){return l->selected;}
/* Raw button reads leave action-layer wheel metadata stale. Model the zero
 * movement seen on the device rather than inventing action acceleration. */
static bool list_button(struct gui_synclist*l,int*a){(void)l;(void)a;return true;}
static struct plugin_api api = {
    .strlen=strlen,.memcpy=memcpy,.memmove=memmove,.strchr=strchr,.strcmp=strcmp,
    .strcasecmp=_stricmp,.strcasestr=contains,.snprintf=snprintf,.qsort=qsort,
    .button_get_w_tmo=get_button,.button_status=status,.current_tick=&tick,.default_event_handler=event,
    .gui_synclist_set_title=set_title,.gui_synclist_set_nb_items=set_count,.gui_synclist_select_item=select_item,
    .lcd_scroll_stop=noop,.splash=splash,.splashf=splashf,.yield=noop,.kbd_input=kbd,
    .file_exists=exists,.dir_exists=directory,.mkdir=make_dir,.open=open_file,.write=write_file,
    .close=close_file,.rename=rename_file,.remove=remove_file
};
const struct plugin_api *rb=&api;
static void reset_disk(void){written=created=renamed=removed=close_error=rename_error=cancelled=collisions=0;fail_after=-1;missing_song=false;output[0]=0;strcpy(keyboard,"Test");}
static void test_browser(void){
    songs=fixture;song_count=7;rows=row_storage;draft_count=0;
    build_view(ARTISTS,0);assert(row_count==3);
    build_view(ALBUM_ARTISTS,0);assert(row_count==4);
    char label[128];assert(!strcmp(row_name(3,NULL,label,sizeof(label)),"Various Artists"));
    artist_ref=4;build_view(ALBUM_ARTIST_ALBUMS,0);assert(row_count==1&&same_album(rows[0],4));
    artist_ref=0;build_view(ALBUM_ARTIST_ALBUMS,0);assert(row_count==1&&same_album(rows[0],0));
    build_view(ALBUMS,0);assert(row_count==4);
    assert(!same_album(0,1));assert(same_album(4,5));
    artist_ref=0;build_view(ARTIST_ALBUMS,0);assert(row_count==2);
    album_ref=0;build_view(ALBUM_SONGS,0);assert(row_count==3);assert(rows[0]==2&&rows[1]==0&&rows[2]==3);
    strcpy(query,"j\xc3\xb3ga");build_view(SEARCH,0);assert(row_count==1&&rows[0]==6);
    strcpy(query,"nonexistent");build_view(SEARCH,0);assert(row_count==0&&list.count==1);
    add_album(0);assert(draft_count==3&&queue[0]==2&&queue[1]==0&&queue[2]==3);
    add_album(4);assert(draft_count==5&&queue[3]==5&&queue[4]==4);
    add_album(0);assert(draft_count==8); /* intentional duplicates */
    draft_count=PB_QUEUE_MAX-1;add_album(0);assert(draft_count==PB_QUEUE_MAX-1);
    draft_count=0;
}
static void test_order(void){
    int a[]={4,9,2};assert(pb_move(a,3,0,-1)==0&&a[0]==4);
    assert(pb_move(a,3,0,1)==1&&a[0]==9&&a[1]==4);
    assert(pb_move(a,3,1,1)==2&&a[1]==2&&a[2]==4);
    assert(pb_move(a,3,2,1)==2);assert(pb_move(a,0,0,1)==0);
    draft_count=4;queue[0]=0;queue[1]=1;queue[2]=0;queue[3]=2;
    build_view(ORDER,1);moving=true;remove_selected(1);
    assert(draft_count==3&&queue[0]==0&&queue[1]==0&&queue[2]==2&&!moving&&list.selected==1);
    remove_selected(0);assert(draft_count==2&&queue[0]==0&&queue[1]==2);
    remove_selected(1);assert(draft_count==1&&queue[0]==0&&list.selected==0);
    remove_selected(0);assert(draft_count==0&&row_count==0&&list.selected==0);
    remove_selected(0);assert(draft_count==0);
}
static void test_keys(void){
    button=physical=BUTTON_SELECT;elapsed=5;assert(read_key()==K_NONE);
    physical=0;button=BUTTON_SELECT|BUTTON_REL;assert(read_key()==K_OK);
    button=physical=BUTTON_SELECT;assert(read_key()==K_NONE);
    elapsed=76;assert(read_key()==K_HOLD);assert(read_key()==K_NONE);
    button=BUTTON_SELECT|BUTTON_REL;physical=0;assert(read_key()==K_NONE);
    elapsed=5;button=BUTTON_PLAY|BUTTON_REL;assert(read_key()==K_PLAY);
    button=BUTTON_SCROLL_FWD;assert(read_key()==K_DOWN);
    button=BUTTON_SCROLL_FWD|BUTTON_REPEAT;assert(read_key()==K_DOWN);
    button=BUTTON_SCROLL_BACK;assert(read_key()==K_UP);
    button=BUTTON_SCROLL_BACK|BUTTON_REPEAT;assert(read_key()==K_UP);
    button=SYS_USB_CONNECTED;assert(read_key()==K_USB);
}
static void test_save(void){
    draft_count=2;queue[0]=6;queue[1]=2;
    reset_disk();assert(save_playlist());assert(created==1&&renamed==1&&!removed);
    assert(!strcmp(output,"#EXTM3U\n/Music/Bj\xc3\xb6rk.flac\n/Music/alpha-1.flac\n"));
    assert(!strcmp(saved_path,"/Playlists/Test.m3u8"));
    reset_disk();strcpy(keyboard,"Night.M3U8");assert(save_playlist());assert(!strcmp(saved_path,"/Playlists/Night.m3u8"));
    reset_disk();fail_after=10;assert(!save_playlist());assert(!renamed&&removed==1&&draft_count==2);
    reset_disk();close_error=1;assert(!save_playlist());assert(!renamed&&removed==1);
    reset_disk();rename_error=1;assert(!save_playlist());assert(!renamed&&removed==1);
    reset_disk();missing_song=true;assert(!save_playlist());assert(!renamed&&removed==1);
    reset_disk();cancelled=1;assert(!save_playlist());assert(!created&&draft_count==2);
    reset_disk();collisions=1;assert(save_playlist());assert(!strcmp(saved_path,"/Playlists/Unique.m3u8"));
    reset_disk();draft_count=0;assert(!save_playlist());assert(!created);
    assert(pb_valid_name("Bj\xc3\xb6rk mix"));assert(!pb_valid_name("../bad"));
    assert(!pb_valid_name("a/b"));assert(!pb_valid_name("bad\nname"));assert(!pb_valid_name(""));
    assert(!pb_valid_name(" trailing "));assert(!pb_valid_name("bad:"));
}
static void test_workflow(void){
    static const int events[]={BUTTON_SCROLL_FWD,BUTTON_SCROLL_FWD,BUTTON_SCROLL_FWD,BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,
        BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,BUTTON_SCROLL_FWD,BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,
        BUTTON_PLAY|BUTTON_REL,BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,BUTTON_RIGHT,
        BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,BUTTON_PLAY|BUTTON_REL};
    api.tagcache_get_stat=get_stat;api.plugin_get_audio_buffer=get_memory;api.tagcache_search=search_db;
    api.tagcache_search_finish=finish_db;api.tagcache_get_next=next_db;api.tagcache_retrieve=retrieve_db;
    api.tagcache_get_numeric=numeric_db;api.do_menu=menu_choice;api.gui_synclist_init=init_list;
    api.gui_synclist_draw=draw;api.gui_synclist_get_sel_pos=get_selected;api.gui_synclist_do_button=list_button;
    reset_disk();script=events;script_pos=0;script_length=sizeof(events)/sizeof(*events);elapsed=5;
    assert(plugin_start(NULL)==PLUGIN_OK);script=NULL;
    assert(script_pos==(int)(sizeof(events)/sizeof(*events)));
    assert(draft_count==2&&renamed==1);
    assert(!strcmp(output,"#EXTM3U\n/Music/alpha-1.flac\n/Music/alpha-disc2.flac\n"));
    /* Reproduce the reported first-artist trap. Repeated and reverse wheel
     * events must navigate to Beta, then its Shared album and Other track. */
    static const int artist_events[]={BUTTON_SCROLL_BACK,BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,
        BUTTON_SCROLL_FWD|BUTTON_REPEAT,BUTTON_SCROLL_BACK|BUTTON_REPEAT,BUTTON_SCROLL_FWD,
        BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,BUTTON_SCROLL_FWD,
        BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,
        BUTTON_PLAY|BUTTON_REL,BUTTON_PLAY|BUTTON_REL};
    reset_disk();script=artist_events;script_pos=0;script_length=sizeof(artist_events)/sizeof(*artist_events);
    assert(plugin_start(NULL)==PLUGIN_OK);script=NULL;
    assert(script_pos==script_length&&draft_count==1&&renamed==1);
    assert(!strcmp(output,"#EXTM3U\n/Music/beta.flac\n"));
    static const int album_artist_events[]={BUTTON_SCROLL_FWD,BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,
        BUTTON_SCROLL_FWD,BUTTON_SCROLL_FWD,BUTTON_SCROLL_FWD,BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,
        BUTTON_MENU|BUTTON_REL, /* back to Album Artists, not track Artists */
        BUTTON_SCROLL_FWD,BUTTON_SCROLL_FWD,BUTTON_SCROLL_FWD,BUTTON_SELECT,BUTTON_SELECT|BUTTON_REL,
        BUTTON_SELECT,HOLD_TICK,BUTTON_SELECT|BUTTON_REL, /* add compilation album */
        BUTTON_PLAY|BUTTON_REL,
        BUTTON_SELECT,HOLD_TICK,HOLD_TICK,BUTTON_SELECT|BUTTON_REPEAT,BUTTON_SELECT|BUTTON_REL,
        BUTTON_PLAY|BUTTON_REL};
    reset_disk();script=album_artist_events;script_pos=0;script_length=sizeof(album_artist_events)/sizeof(*album_artist_events);
    assert(plugin_start(NULL)==PLUGIN_OK);script=NULL;
    assert(script_pos==script_length&&draft_count==1&&renamed==1&&!moving);
    assert(!strcmp(output,"#EXTM3U\n/Music/comp-a.flac\n"));
    char *saved_artist=fixture[6].albumartist;
    fixture[6].albumartist=UNTAGGED;assert(load_database()==100);
    assert(!strcmp(songs[6].albumartist,fixture[6].artist));
    fixture[6].albumartist=saved_artist;
    dbstat.ready=false;assert(load_database()==PLUGIN_ERROR);dbstat.ready=true;
}
int main(void){test_browser();test_order();test_keys();test_save();test_workflow();puts("PASS: browsing, album identity/order/capacity, queue moves, timed buttons, UTF-8/partial writes, save failures, names, complete new/browse/add/reorder/name/save workflow");return 0;}
