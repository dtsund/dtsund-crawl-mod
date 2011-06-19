#include <limits.h>

#include "AppHdr.h"

#include "colour.h"
#include "format.h"
#include "libutil.h"
#include "showsymb.h"
#include "viewchar.h"

formatted_string::formatted_string(int init_colour)
    : ops()
{
    if (init_colour)
        textcolor(init_colour);
}

formatted_string::formatted_string(const std::string &s, int init_colour)
    : ops()
{
    if (init_colour)
        textcolor(init_colour);
    cprintf(s);
}

int formatted_string::get_colour(const std::string &tag)
{
    if (tag == "h")
        return (YELLOW);

    if (tag == "w")
        return (WHITE);

    const int colour = str_to_colour(tag);
    return (colour != -1? colour : LIGHTGREY);
}

// Display a formatted string without printing literal \n.
// This is important if the text is not supposed
// to clobber existing text to the right of the lines being displayed
// (some of the tutorial messages need this).
void display_tagged_block(const std::string &s)
{
    std::vector<formatted_string> lines;
    formatted_string::parse_string_to_multiple(s, lines);

    int x = wherex();
    int y = wherey();
    const unsigned int max_y = cgetsize(GOTO_CRT).y;
    const int size = std::min<unsigned int>(lines.size(), max_y - y + 1);
    for (int i = 0; i < size; ++i)
    {
        cgotoxy(x, y);
        lines[i].display();
        y++;
    }
}

formatted_string formatted_string::parse_string(
        const std::string &s,
        bool  eot_ends_format,
        bool (*process)(const std::string &tag),
        int main_colour)
{
    // main_colour will usually be LIGHTGREY (default).
    std::vector<int> colour_stack;
    colour_stack.push_back(main_colour);

    formatted_string fs;

    parse_string1(s, fs, colour_stack, process);
    if (eot_ends_format)
    {
        if (colour_stack.back() != colour_stack.front())
            fs.textcolor(colour_stack.front());
    }
    return fs;
}

// Parses a formatted string in much the same way as parse_string, but
// handles \n by creating a new formatted_string.
void formatted_string::parse_string_to_multiple(
    const std::string &s,
    std::vector<formatted_string> &out)
{
    std::vector<int> colour_stack;
    colour_stack.push_back(LIGHTGREY);

    std::vector<std::string> lines = split_string("\n", s, false, true);

    for (int i = 0, size = lines.size(); i < size; ++i)
    {
        out.push_back(formatted_string());
        formatted_string& fs = out.back();
        fs.textcolor(colour_stack.back());
        parse_string1(lines[i], fs, colour_stack, NULL);
        if (colour_stack.back() != colour_stack.front())
            fs.textcolor(colour_stack.front());
    }
}

// Helper for the other parse_ methods.
void formatted_string::parse_string1(
        const std::string &s,
        formatted_string &fs,
        std::vector<int> &colour_stack,
        bool (*process)(const std::string &tag))
{
    // FIXME: This is a lame mess, just good enough for the task on hand
    // (keyboard help).
    std::string::size_type tag    = std::string::npos;
    std::string::size_type length = s.length();

    std::string currs;
    bool masked = false;

    for (tag = 0; tag < length; ++tag)
    {
        bool revert_colour = false;
        std::string::size_type endpos = std::string::npos;

        // Break string up if it gets too big.
        if (currs.size() >= 999)
        {
            // Break the string at the end of a line, if possible, so
            // that none of the broken string ends up overwritten.
            std::string::size_type bound = currs.rfind("\n", 999);
            if (bound != endpos)
                bound++;
            else
                bound = 999;

            fs.cprintf(currs.substr(0, bound));
            if (currs.size() > bound)
                currs = currs.substr(bound);
            else
                currs.clear();
            tag--;
            continue;
        }

        if (s[tag] != '<' || tag >= length - 2)
        {
            if (!masked)
                currs += s[tag];
            continue;
        }

        // Is this a << escape?
        if (s[tag + 1] == '<')
        {
            if (!masked)
                currs += s[tag];
            tag++;
            continue;
        }

        endpos = s.find('>', tag + 1);
        // No closing >?
        if (endpos == std::string::npos)
        {
            if (!masked)
                currs += s[tag];
            continue;
        }

        std::string tagtext = s.substr(tag + 1, endpos - tag - 1);
        if (tagtext.empty() || tagtext == "/")
        {
            if (!masked)
                currs += s[tag];
            continue;
        }

        if (tagtext[0] == '/')
        {
            revert_colour = true;
            tagtext = tagtext.substr(1);
            tag++;
        }

        if (tagtext[0] == '?')
        {
            if (tagtext.length() == 1)
                masked = false;
            else if (process && !process(tagtext.substr(1)))
                masked = true;

            tag += tagtext.length() + 1;
            continue;
        }

        if (!currs.empty())
        {
            fs.cprintf(currs);
            currs.clear();
        }

        if (revert_colour)
        {
            colour_stack.pop_back();
            if (colour_stack.size() < 1)
                die("Stack underflow in string \"%s\"", s.c_str());
        }
        else
        {
            colour_stack.push_back(get_colour(tagtext));
        }

        // fs.cprintf("%d%d", colour_stack.size(), colour_stack.back());
        fs.textcolor(colour_stack.back());

        tag += tagtext.length() + 1;
    }
    if (currs.length())
        fs.cprintf(currs);
}

formatted_string::operator std::string() const
{
    std::string s;
    for (unsigned i = 0, size = ops.size(); i < size; ++i)
    {
        if (ops[i] == FSOP_TEXT)
            s += ops[i].text;
    }
    return (s);
}

void replace_all_in_string(std::string& s, const std::string& search,
                           const std::string& replace)
{
    std::string::size_type pos = 0;
    while ((pos = s.find(search, pos)) != std::string::npos)
    {
        s.replace(pos, search.size(), replace);
        pos += replace.size();
    }
}

std::string formatted_string::html_dump() const
{
    std::string s;
    for (unsigned i = 0; i < ops.size(); ++i)
    {
        std::string tmp;
        switch (ops[i].type)
        {
        case FSOP_TEXT:
            tmp = ops[i].text;
            // (very) crude HTMLification
            replace_all_in_string(tmp, "&", "&amp;");
            replace_all_in_string(tmp, " ", "&nbsp;");
            replace_all_in_string(tmp, "<", "&lt;");
            replace_all_in_string(tmp, ">", "&gt;");
            replace_all_in_string(tmp, "\n", "<br>");
            s += tmp;
            break;
        case FSOP_COLOUR:
            s += "<font color=";
            s += colour_to_str(ops[i].x);
            s += ">";
            break;
        }
    }
    return s;
}

bool formatted_string::operator < (const formatted_string &other) const
{
    return std::string(*this) < std::string(other);
}

const formatted_string &
formatted_string::operator += (const formatted_string &other)
{
    ops.insert(ops.end(), other.ops.begin(), other.ops.end());
    return (*this);
}

std::string::size_type formatted_string::length() const
{
    // Just add up the individual string lengths.
    std::string::size_type len = 0;
    for (unsigned i = 0, size = ops.size(); i < size; ++i)
        if (ops[i] == FSOP_TEXT)
            len += ops[i].text.length();
    return (len);
}

inline void cap(int &i, int max)
{
    if (i < 0 && -i <= max)
        i += max;
    if (i >= max)
        i = max - 1;
    if (i < 0)
        i = 0;
}

char &formatted_string::operator [] (size_t idx)
{
    size_t rel_idx = idx;
    int size = ops.size();
    for (int i = 0; i < size; ++i)
    {
        if (ops[i] != FSOP_TEXT)
            continue;

        size_t len = ops[i].text.length();
        if (rel_idx >= len)
            rel_idx -= len;
        else
            return ops[i].text[rel_idx];
    }
    die("Invalid index");
}


std::string formatted_string::tostring(int s, int e) const
{
    std::string st;

    int size = ops.size();
    cap(s, size);
    cap(e, size);

    for (int i = s; i <= e && i < size; ++i)
    {
        if (ops[i] == FSOP_TEXT)
            st += ops[i].text;
    }
    return st;
}

std::string formatted_string::to_colour_string() const
{
    std::string st;
    const int size = ops.size();
    for (int i = 0; i < size; ++i)
    {
        if (ops[i] == FSOP_TEXT)
        {
            // gotta double up those '<' chars ...
            size_t start = st.size();
            st += ops[i].text;

            while (true)
            {
                const size_t left_angle = st.find('<', start);
                if (left_angle == std::string::npos)
                    break;

                st.insert(left_angle, "<");
                start = left_angle + 2;
            }
        }
        else if (ops[i] == FSOP_COLOUR)
        {
            st += "<";
            st += colour_to_str(ops[i].x);
            st += ">";
        }
    }

    return st;
}

void formatted_string::display(int s, int e) const
{
    int size = ops.size();
    if (!size)
        return;

    cap(s, size);
    cap(e, size);

    for (int i = s; i <= e && i < size; ++i)
        ops[i].display();
}

int formatted_string::find_last_colour() const
{
    if (!ops.empty())
    {
        for (int i = ops.size() - 1; i >= 0; --i)
            if (ops[i].type == FSOP_COLOUR)
                return (ops[i].x);
    }
    return (LIGHTGREY);
}

formatted_string formatted_string::substr(size_t start, size_t substr_length) const
{
    const unsigned int NONE = UINT_MAX; // from limits.h
    unsigned int last_FSOP_COLOUR = NONE;

    // Find the first string to copy
    unsigned int i;
    for (i = 0; i < ops.size(); ++i)
    {
        const fs_op& op = ops[i];
        if (op.type == FSOP_COLOUR)
            last_FSOP_COLOUR = i;
        else if (op.type == FSOP_TEXT)
        {
            if (op.text.length() > start)
                break;
            else
                start -= op.text.length();
        }
    }

    if (i == ops.size())
        return formatted_string();

    formatted_string result;
    // set up the state
    if (last_FSOP_COLOUR != NONE)
        result.ops.push_back(ops[last_FSOP_COLOUR]);

    // Copy the text
    for (; i<ops.size(); i++)
    {
        const fs_op& op = ops[i];
        if (op.type == FSOP_TEXT)
        {
            result.ops.push_back(op);
            std::string& new_string = result.ops[result.ops.size()-1].text;
            if (start > 0 || op.text.length() > substr_length)
                new_string = new_string.substr(start, substr_length);

            substr_length -= new_string.length();
            if (substr_length == 0)
                break;
        }
        else
        {
            result.ops.push_back(op);
        }
    }

    return result;
}

void formatted_string::add_glyph(glyph g)
{
    const int last_col = find_last_colour();
    this->textcolor(g.col);
    this->cprintf("%s", stringize_glyph(g.ch).c_str());
    this->textcolor(last_col);
}

void formatted_string::textcolor(int color)
{
    if (!ops.empty() && ops[ ops.size() - 1 ].type == FSOP_COLOUR)
        ops.erase(ops.end() - 1);

    ops.push_back(color);
}

void formatted_string::clear()
{
    ops.clear();
}

bool formatted_string::empty()
{
    return (ops.empty());
}

void formatted_string::cprintf(const char *s, ...)
{
    va_list args;
    va_start(args, s);
    cprintf(vmake_stringf(s, args));
    va_end(args);
}

void formatted_string::cprintf(const std::string &s)
{
    ops.push_back(s);
}

void formatted_string::fs_op::display() const
{
    switch (type)
    {
    case FSOP_COLOUR:
        ::textattr(x);
        break;
    case FSOP_TEXT:
        ::cprintf("%s", text.c_str());
        break;
    }
}

void formatted_string::swap(formatted_string& other)
{
    ops.swap(other.ops);
}

void formatted_string::all_caps()
{
    for (unsigned int i = 0; i < ops.size(); i++)
        if (ops[i].type == FSOP_TEXT)
            uppercase(ops[i].text);
}

int count_linebreaks(const formatted_string& fs)
{
    std::string::size_type where = 0;
    const std::string s = fs;
    int count = 0;
    while (1)
    {
        where = s.find("\n", where);
        if (where == std::string::npos)
            break;
        else
        {
            ++count;
            ++where;
        }
    }
    return count;
}

// Return the actual (string) offset of character #loc to be printed,
// i.e. ignoring tags. So for instance, if s == "<tag>ab</tag>", then
// _find_string_location(s, 2) == 6.
static int _find_string_location(const std::string& s, int loc)
{
    int real_offset = 0;
    bool in_tag = false;
    int last_taglen = 0;
    int offset = 0;
    for (std::string::const_iterator ci = s.begin();
         ci != s.end() && real_offset < loc;
         ++ci, ++offset)
    {
        if (in_tag)
        {
            if (*ci == '<' && last_taglen == 1)
            {
                ++real_offset;
                in_tag = false;
            }
            else if (*ci == '>')
            {
                in_tag = false;
            }
            else
            {
                ++last_taglen;
            }
        }
        else if (*ci == '<')
        {
            in_tag = true;
            last_taglen = 1;
        }
        else
        {
            ++real_offset;
        }
    }
    return (offset);
}

// Return the substring of s from character start to character end,
// where tags count as length 0.
std::string tagged_string_substr(const std::string& s, int start, int end)
{
    return (s.substr(_find_string_location(s, start),
                     _find_string_location(s, end)));
}

static int _tagged_string_printable_length(const std::string& s)
{
    int len = 0;
    bool in_tag = false;
    int last_taglen = 0;
    for (std::string::const_iterator ci = s.begin(); ci != s.end(); ++ci)
    {
        if (in_tag)
        {
            if (*ci == '<' && last_taglen == 1) // "<<" sequence
            {
                in_tag = false;  // this is an escape for '<'
                ++len;           // len wasn't incremented before
            }
            else if (*ci == '>') // tag close, still nothing printed
            {
                in_tag = false;
            }
            else                 // tag continues
            {
                ++last_taglen;
            }
        }
        else if (*ci == '<')     // tag starts
        {
            in_tag = true;
            last_taglen = 1;
        }
        else                     // normal, printable character
        {
            ++len;
        }
    }
    return (len);
}


// Count the length of the tags in the string.
int tagged_string_tag_length(const std::string& s)
{
    return s.size() - _tagged_string_printable_length(s);
}
