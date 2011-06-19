#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#ifdef USE_TILE

#include "externs.h"
#include "glwrapper.h"
#include "tilereg.h"
#include "tilesdl.h"
#include "tiletex.h"

enum wm_endianness
{
    WM_BIG_ENDIAN,
    WM_LIL_ENDIAN,
};

enum wm_event_type
{
    WM_NOEVENT = 0,
    WM_ACTIVEEVENT,
    WM_KEYDOWN,
    WM_KEYUP,
    WM_MOUSEMOTION,
    WM_MOUSEBUTTONUP,
    WM_MOUSEBUTTONDOWN,
    WM_QUIT,
    WM_CUSTOMEVENT,
    WM_RESIZE,
    WM_EXPOSE,
    WM_NUMEVENTS = 15
};

struct wm_keysym
{
    unsigned char scancode;
    int sym;
    unsigned char key_mod;
    unsigned int unicode;
};

struct wm_active_event
{
    unsigned char type;
    unsigned char gain;
    unsigned char state;
};

struct wm_keyboard_event
{
    unsigned char type;
    unsigned char state;
    wm_keysym keysym;
};

struct wm_resize_event
{
    unsigned char type;
    int w, h;
};

struct wm_expose_event
{
    unsigned char type;
};

struct wm_quit_event
{
    unsigned char type;
};

struct wm_custom_event
{
    unsigned char type;
    int code;
    void *data1;
    void *data2;
};

// Basically a generic SDL_Event
struct wm_event
{
    unsigned char type;
    wm_active_event active;
    wm_keyboard_event key;
    MouseEvent mouse_event;
    wm_resize_event resize;
    wm_expose_event expose;
    wm_quit_event quit;
    wm_custom_event custom;
};

// custom timer callback function
typedef unsigned int (*wm_timer_callback)(unsigned int interval);

class WindowManager
{
public:
    // To silence pre 4.3 g++ compiler warnings
    virtual ~WindowManager() {};

    // Static Alloc/deallocators
    // Note: Write this function in each implementation-specific file,
    // e.g. windowmanager-sdl.cc has its own WindowManager::create().
    static void create();
    static void shutdown();

    // Class functions
    virtual int init(coord_def *m_windowsz) = 0;

    // Environment state functions
    virtual void set_window_title(const char *title) = 0;
    virtual bool set_window_icon(const char* icon_name) = 0;
    virtual key_mod get_mod_state() const = 0;
    virtual void set_mod_state(key_mod mod) = 0;
    virtual int byte_order() = 0;

    // System time functions
    virtual void set_timer(unsigned int interval,
                           wm_timer_callback callback) = 0;
    virtual unsigned int get_ticks() const = 0;
    virtual void delay(unsigned int ms) = 0;

    // Event functions
    virtual int raise_custom_event() = 0;
    virtual int wait_event(wm_event *event) = 0;
    virtual unsigned int get_event_count(wm_event_type type) = 0;

    // Display functions
    virtual void resize(coord_def &m_windowsz) = 0;
    virtual void swap_buffers() = 0;
    virtual int screen_width() const = 0;
    virtual int screen_height() const = 0;
    virtual int desktop_width() const = 0;
    virtual int desktop_height() const = 0;

    // Texture loading
    virtual bool load_texture(GenericTexture *tex, const char *filename,
                              MipMapOptions mip_opt, unsigned int &orig_width,
                              unsigned int &orig_height,
                              tex_proc_func proc = NULL,
                              bool force_power_of_two = true) = 0;

};

// Main interface for UI functions
extern WindowManager *wm;

#endif //USE_TILE
#endif //include guard
