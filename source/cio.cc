/**
 * @file
 * @brief Platform-independent console IO functions.
**/

#include "AppHdr.h"

#include "cio.h"
#include "externs.h"
#include "options.h"
#include "libutil.h"
#include "macro.h"
#include "message.h"
#include "state.h"
#include "unicode.h"
#include "viewgeom.h"

#include <queue>

#ifdef UNIX
extern int unixcurses_get_vi_key(int keyin);

static keycode_type _numpad2vi(keycode_type key)
{
#ifndef USE_TILE
    key = unixcurses_get_vi_key(key);
#endif
    switch (key)
    {
    case CK_UP:    key = 'k'; break;
    case CK_DOWN:  key = 'j'; break;
    case CK_LEFT:  key = 'h'; break;
    case CK_RIGHT: key = 'l'; break;
    case -1001:    key = 'b'; break;
    case -1002:    key = 'j'; break;
    case -1003:    key = 'n'; break;
    case -1004:    key = 'h'; break;
    case -1005:    key = '.'; break;
    case -1006:    key = 'l'; break;
    case -1007:    key = 'y'; break;
    case -1008:    key = 'k'; break;
    case -1009:    key = 'u'; break;
    }
    if (key >= '1' && key <= '9')
    {
        const char *vikeys = "bjnh.lyku";
        return keycode_type(vikeys[key - '1']);
    }
    return (key);
}
#endif

int unmangle_direction_keys(int keyin, KeymapContext keymap,
                            bool fake_ctrl, bool fake_shift)
{
#ifdef UNIX
    // Kludging running and opening as two character sequences
    // for Unix systems.  This is an easy way out... all the
    // player has to do is find a termcap and numlock setting
    // that will get curses the numbers from the keypad.  This
    // will hopefully be easy.

    /* can we say yuck? -- haranp */
    if (fake_ctrl && keyin == '*')
    {
        keyin = getchm(keymap);
        // return control-key
        keyin = CONTROL(toupper(_numpad2vi(keyin)));
    }
    else if (fake_shift && keyin == '/')
    {
        keyin = getchm(keymap);
        // return shift-key
        keyin = toupper(_numpad2vi(keyin));
    }
#else
    // Old DOS keypad support
    if (keyin == 0)
    {
        /* FIXME haranp - hackiness */
        const char DOSidiocy[10]     = { "OPQKSMGHI" };
        const char DOSunidiocy[10]   = { "bjnh.lyku" };
        const int DOScontrolidiocy[9] =
        {
            117, 145, 118, 115, 76, 116, 119, 141, 132
        };
        keyin = getchm(keymap);
        for (int j = 0; j < 9; ++j)
        {
            if (keyin == DOSidiocy[j])
            {
                keyin = DOSunidiocy[j];
                break;
            }
            if (keyin == DOScontrolidiocy[j])
            {
                keyin = CONTROL(toupper(DOSunidiocy[j]));
                break;
            }
        }
    }
#endif

    // [dshaligram] More lovely keypad mangling.
    switch (keyin)
    {
#ifdef UNIX
    case '1': return 'b';
    case '2': return 'j';
    case '3': return 'n';
    case '4': return 'h';
    case '6': return 'l';
    case '7': return 'y';
    case '8': return 'k';
    case '9': return 'u';

 #ifndef USE_TILE
    default: return unixcurses_get_vi_key(keyin);
 #endif

#else
    case '1': return 'B';
    case '2': return 'J';
    case '3': return 'N';
    case '4': return 'H';
    case '6': return 'L';
    case '7': return 'Y';
    case '8': return 'K';
    case '9': return 'U';
#endif
    }

    return (keyin);
}

// Wrapper around cgotoxy that can draw a fake cursor for Unix terms where
// cursoring over darkgrey or black causes problems.
void cursorxy(int x, int y)
{
#if defined(USE_TILE)
    coord_def ep(x, y);
    coord_def gc = crawl_view.screen2grid(ep);
    tiles.place_cursor(CURSOR_MOUSE, gc);
#elif defined(UNIX)
    if (Options.use_fake_cursor)
        fakecursorxy(x, y);
    else
        cgotoxy(x, y, GOTO_CRT);
#else
    cgotoxy(x, y, GOTO_CRT);
#endif
}

// cprintf that stops outputting when wrapped
// Conceptually very similar to wrapcprintf()
int nowrapcprintf(int wrapcol, const char *s, ...)
{
    char buf[1000]; // Hard max

    va_list args;
    va_start(args, s);
    // XXX: If snprintf isn't available, vsnprintf probably isn't, either.
    const int len = vsnprintf(buf, sizeof buf, s, args);
    va_end(args);

    // Sanity checking to prevent buffer overflows
    const int maxlen = std::min(std::max(wrapcol + 1 - wherex(), 0), len);

    // Force the string to terminate at maxlen
    buf[maxlen] = 0;

    cprintf("%s", buf);
    return std::min(len, maxlen);
}

// convenience wrapper (hah) for nowrapcprintf
// FIXME: should pass off to nowrapcprintf() instead of doing it manually
int nowrap_eol_cprintf(const char *s, ...)
{
    const int wrapcol = get_number_of_cols() - 1;
    char buf[1000]; // Hard max

    va_list args;
    va_start(args, s);
    // XXX: If snprintf isn't available, vsnprintf probably isn't, either.
    const int len = vsnprintf(buf, sizeof buf, s, args);
    va_end(args);

    // Sanity checking to prevent buffer overflows
    const int maxlen = std::min(std::max(wrapcol + 1 - wherex(), 0), len);

    // Force the string to terminate at maxlen
    buf[maxlen] = 0;

    cprintf("%s", buf);
    return std::min(len, maxlen);
}

// cprintf that knows how to wrap down lines (primitive, but what the heck)
void wrapcprintf(int wrapcol, const char *s, ...)
{
    va_list args;
    va_start(args, s);
    std::string buf = vmake_stringf(s, args);
    va_end(args);

    while (!buf.empty())
    {
        int x = wherex(), y = wherey();

        int avail = wrapcol - x + 1;
        if (avail > 0)
            cprintf("%s", wordwrap_line(buf, avail).c_str());
        if (!buf.empty())
            cgotoxy(1, y + 1);
    }
}

int cancelable_get_line(char *buf, int len, input_history *mh,
                        int (*keyproc)(int &ch))
{
    flush_prev_message();

    mouse_control mc(MOUSE_MODE_MORE);
    line_reader reader(buf, len, get_number_of_cols());
    reader.set_input_history(mh);
    reader.set_keyproc(keyproc);

    return reader.read_line();
}


/////////////////////////////////////////////////////////////
// input_history
//

input_history::input_history(size_t size)
    : history(), pos(), maxsize(size)
{
    if (maxsize < 2)
        maxsize = 2;

    pos = history.end();
}

void input_history::new_input(const std::string &s)
{
    history.remove(s);

    if (history.size() == maxsize)
        history.pop_front();

    history.push_back(s);

    // Force the iterator to the end (also revalidates it)
    go_end();
}

const std::string *input_history::prev()
{
    if (history.empty())
        return NULL;

    if (pos == history.begin())
        pos = history.end();

    return &*--pos;
}

const std::string *input_history::next()
{
    if (history.empty())
        return NULL;

    if (pos == history.end() || ++pos == history.end())
        pos = history.begin();

    return &*pos;
}

void input_history::go_end()
{
    pos = history.end();
}

void input_history::clear()
{
    history.clear();
    go_end();
}

/////////////////////////////////////////////////////////////////////////
// line_reader

line_reader::line_reader(char *buf, size_t sz, int wrap)
    : buffer(buf), bufsz(sz), history(NULL), region(GOTO_CRT),
      start(coord_def(0,0)), keyfn(NULL), wrapcol(wrap),
      cur(NULL), length(0), pos(-1)
{
}

line_reader::~line_reader()
{
}

std::string line_reader::get_text() const
{
    return (buffer);
}

void line_reader::set_input_history(input_history *i)
{
    history = i;
}

void line_reader::set_keyproc(keyproc fn)
{
    keyfn = fn;
}

void line_reader::cursorto(int ncx)
{
    int x = (start.x + ncx - 1) % wrapcol + 1;
    int y = start.y + (start.x + ncx - 1) / wrapcol;
    int diff = y - cgetsize(region).y;
    if (diff > 0)
    {
        // There's no space left in the region, so we scroll it.
        // XXX: cscroll only implemented for GOTO_MSG.
        // XXX: wrapcprintf works in GOTO_SCREEN; in particular
        //      it wraps to the screen's first column, so this
        //      won't work for regions that don't start at the
        //      left edge.
        cscroll(diff, region);
        start.y -= diff;
        y -= diff;
        cgotoxy(start.x, start.y, region);
        wrapcprintf(wrapcol, "%s", buffer);
    }
    cgotoxy(x, y, region);
}

int line_reader::read_line(bool clear_previous)
{
    if (bufsz <= 0)
        return (false);

    cursor_control con(true);

    if (clear_previous)
        *buffer = 0;

    region = get_cursor_region();
    start = cgetpos(region);

    length = strlen(buffer);
    int width = strwidth(buffer);

    // Remember the previous cursor position, if valid.
    if (pos < 0 || pos > width)
        pos = width;

    cur = buffer;
    int cpos = 0;
    while (*cur && cpos < pos)
    {
        ucs_t c;
        int s = utf8towc(&c, cur);
        cur += s;
        cpos += wcwidth(c);
    }

    if (length)
        wrapcprintf(wrapcol, "%s", buffer);

    if (pos != width)
        cursorto(pos);

    if (history)
        history->go_end();

    while (true)
    {
        int ch = getchm(getch_ck);

#if defined(USE_UNIX_SIGNALS) && defined(SIGHUP_SAVE) && defined(USE_CURSES)
        // Don't return a partial string if a HUP signal interrupted things
        if (crawl_state.seen_hups)
        {
            buffer[0] = '\0';
            return (0);
        }
#endif

        if (keyfn)
        {
            int whattodo = (*keyfn)(ch);
            if (whattodo == 0)
            {
                buffer[length] = 0;
                if (history && length)
                    history->new_input(buffer);
                return (0);
            }
            else if (whattodo == -1)
            {
                buffer[length] = 0;
                return (ch);
            }
        }

        int ret = process_key(ch);
        if (ret != -1)
            return (ret);
    }
}

void line_reader::backspace()
{
    if (!pos)
        return;

    char *np = prev_glyph(cur, buffer);
    ASSERT(np);
    ucs_t ch;
    utf8towc(&ch, np);
    buffer[length] = 0;
    length -= cur - np;
    char *c = cur;
    cur = np;
    while (*c)
        *np++ = *c++;
    calc_pos();

    cursorto(pos);
    buffer[length] = 0;
    wrapcprintf(wrapcol, "%s ", cur);
    cursorto(pos);
}

bool line_reader::is_wordchar(ucs_t c)
{
    return iswalnum(c) || c == '_' || c == '-';
}

void line_reader::kill_to_begin()
{
    if (!pos || cur == buffer)
        return;

    int rest = length - (cur - buffer);
    buffer[length] = 0;
    cursorto(0);
    wrapcprintf(wrapcol, "%s%*s", cur, pos, "");
    memmove(buffer, cur, rest);
    buffer[length = rest] = 0;;
    pos = 0;
    cur = buffer;
    cursorto(pos);
}

void line_reader::killword()
{
    if (cur == buffer)
        return;

    bool foundwc = false;
    char *word = cur;
    int ew = 0;
    while (1)
    {
        char *np = prev_glyph(word, buffer);
        if (!np)
            break;

        ucs_t c;
        utf8towc(&c, np);
        if (is_wordchar(c))
            foundwc = true;
        else if (foundwc)
            break;

        word = np;
        ew += wcwidth(c);
    }
    memmove(word, cur, strlen(cur) + 1);
    length -= cur - word;
    cur = word;
    calc_pos();

    cursorto(0);
    wrapcprintf(wrapcol, "%s%*s", buffer, ew);
    cursorto(pos);
}

void line_reader::calc_pos()
{
    int p = 0;
    const char *cp = buffer;
    ucs_t c;
    int s;
    while (cp < cur && (s = utf8towc(&c, cp)))
    {
        // FIXME: this won't handle a CJK character wrapping prematurely
        // (if there's only one space left)
        cp += s;
        p += wcwidth(c);
    }
    pos = p;
}

int line_reader::process_key(int ch)
{
    switch (ch)
    {
    CASE_ESCAPE
        return (CK_ESCAPE);
    case CK_UP:
    case CK_DOWN:
    {
        if (!history)
            break;

        const std::string *text = (ch == CK_UP) ? history->prev()
                                                : history->next();

        if (text)
        {
            int olen = strwidth(buffer);
            length = text->length();
            if (length >= static_cast<int>(bufsz))
                length = bufsz - 1;
            memcpy(buffer, text->c_str(), length);
            buffer[length] = 0;
            cur = buffer + length;
            calc_pos();
            cursorto(0);

            int clear = pos < olen ? olen - pos : 0;
            wrapcprintf(wrapcol, "%s%*s", buffer, clear, "");

            cursorto(pos);
        }
        break;
    }
    case CK_ENTER:
        buffer[length] = 0;
        if (history && length)
            history->new_input(buffer);
        return (0);

    case CONTROL('K'):
    {
        // Kill to end of line.
        if (*cur)
        {
            int erase = strwidth(cur);
            length = cur - buffer;
            *cur = 0;
            wrapcprintf(wrapcol, "%*s", erase, "");
            cursorto(pos);
        }
        break;
    }
    case CK_DELETE:
        if (*cur)
        {
            char *np = next_glyph(cur);
            ASSERT(np);
            char *c = cur;
            while (*np)
                *c++ = *np++;
            length = np - buffer;

            cursorto(pos);
            buffer[length] = 0;
            wrapcprintf(wrapcol, "%s ", cur);
            cursorto(pos);
        }
        break;

    case CK_BKSP:
        backspace();
        break;

    case CONTROL('W'):
        killword();
        break;

    case CONTROL('U'):
        kill_to_begin();
        break;

    case CK_LEFT:
        if (char *np = prev_glyph(cur, buffer))
        {
            cur = np;
            calc_pos();
            cursorto(pos);
        }
        break;
    case CK_RIGHT:
        if (char *np = next_glyph(cur))
        {
            cur = np;
            calc_pos();
            cursorto(pos);
        }
        break;
    case CK_HOME:
    case CONTROL('A'):
        pos = 0;
        cur = buffer;
        cursorto(pos);
        break;
    case CK_END:
    case CONTROL('E'):
        cur = buffer + length;
        calc_pos();
        cursorto(pos);
        break;
    case CK_MOUSE_CLICK:
        return (-1);
    default:
        if (wcwidth(ch) >= 0 && length + wclen(ch) < static_cast<int>(bufsz))
        {
            int w = wcwidth(ch);
            int len = wclen(ch);
            if (*cur)
            {
                char *c = buffer + length - 1;
                while (c >= cur)
                {
                    c[len] = *c;
                    c--;
                }
            }
            wctoutf8(cur, ch);
            cur += len;
            length += len;
            buffer[length] = 0;
            pos += w;
            if (!w)
            {
                cursorto(0);
                wrapcprintf(wrapcol, "%s", buffer);
            }
            else
            {
                putwch(ch);
                if (*cur)
                    wrapcprintf(wrapcol, "%s", cur);
            }
            cursorto(pos);
        }
        break;
    }

    return (-1);
}

/////////////////////////////////////////////////////////////////////////////
// Of mice and other mice.

static std::queue<c_mouse_event> mouse_events;

coord_def get_mouse_pos()
{
    // lib$(OS) has to maintain mousep. This function is just the messenger.
    return (crawl_view.mousep);
}

c_mouse_event get_mouse_event()
{
    if (mouse_events.empty())
        return c_mouse_event();

    c_mouse_event ce = mouse_events.front();
    mouse_events.pop();
    return (ce);
}

void new_mouse_event(const c_mouse_event &ce)
{
    mouse_events.push(ce);
}

static void _flush_mouse_events()
{
    while (!mouse_events.empty())
        mouse_events.pop();
}

void c_input_reset(bool enable_mouse, bool flush)
{
    crawl_state.mouse_enabled = (enable_mouse && Options.mouse_input);
    set_mouse_enabled(crawl_state.mouse_enabled);

    if (flush)
        _flush_mouse_events();
}
