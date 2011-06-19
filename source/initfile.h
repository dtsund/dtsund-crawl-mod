/**
 * @file
 * @brief Simple reading of init file.
**/


#ifndef INITFILE_H
#define INITFILE_H

#include <string>
#include <cstdio>

#include "enum.h"
#include "unicode.h"

enum drop_mode_type
{
    DM_SINGLE,
    DM_MULTI,
};

int str_to_summon_type(const std::string &str);
std::string gametype_to_str(game_type type);

std::string read_init_file(bool runscript = false);

struct newgame_def;
newgame_def read_startup_prefs();

void read_options(const std::string &s, bool runscript = false,
                  bool clear_aliases = false);

void parse_option_line(const std::string &line, bool runscript = false);

void apply_ascii_display(bool ascii);

void get_system_environment(void);

struct system_environment
{
public:
    std::string crawl_name;
    std::string crawl_rc;
    std::string crawl_dir;

    std::vector<std::string> rcdirs;   // Directories to search for includes.

    std::string morgue_dir;
    std::string macro_dir;
    std::string crawl_base;        // Directory from argv[0], may be used to
                                   // locate datafiles.
    std::string crawl_exe;         // File from argv[0].
    std::string home;

#ifdef DGL_SIMPLE_MESSAGING
    std::string messagefile;       // File containing messages from other users.
    bool have_messages;            // There are messages waiting to be read.
    unsigned  message_check_tick;
#endif

    std::string scorefile;
    std::vector<std::string> cmd_args;

    int map_gen_iters;

    std::vector<std::string> extra_opts_first;
    std::vector<std::string> extra_opts_last;

public:
    void add_rcdir(const std::string &dir);
};

extern system_environment SysEnv;

bool parse_args(int argc, char **argv, bool rc_only);

struct newgame_def;
void write_newgame_options_file(const newgame_def& prefs);

void save_player_name(void);

std::string channel_to_str(int ch);

int str_to_channel(const std::string &);

class StringLineInput : public LineInput
{
public:
    StringLineInput(const std::string &s) : str(s), pos(0) { }

    bool eof()
    {
        return pos >= str.length();
    }

    std::string get_line()
    {
        if (eof())
            return "";
        std::string::size_type newl = str.find("\n", pos);
        if (newl == std::string::npos)
            newl = str.length();
        std::string line = str.substr(pos, newl - pos);
        pos = newl + 1;
        return line;
    }
private:
    const std::string &str;
    std::string::size_type pos;
};

#endif
