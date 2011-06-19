/**
 * @file
 * @brief Scorefile manipulation functions
**/


#ifndef HISCORES_H
#define HISCORES_H

class scorefile_entry;

void hiscores_new_entry(const scorefile_entry &se);

void logfile_new_entry(const scorefile_entry &se);

void hiscores_print_list(int display_count = -1, int format = SCORE_TERSE);
void hiscores_print_all(int display_count = -1, int format = SCORE_TERSE);

std::string hiscores_format_single(const scorefile_entry &se);
std::string hiscores_format_single_long(const scorefile_entry &se,
                                         bool verbose = false);

void mark_milestone(const std::string &type, const std::string &milestone,
                    bool report_origin_level = false, time_t t = 0);

#ifdef DGL_WHEREIS
std::string xlog_status_line();
#endif

std::string xlog_unescape(const std::string &);
std::string xlog_escape(const std::string &);

std::vector<std::string> xlog_split_fields(const std::string &s);

class xlog_fields
{
public:
    xlog_fields();
    xlog_fields(const std::string &line);

    void init(const std::string &line);
    std::string xlog_line() const;

    void add_field(const std::string &key,
                   const char *format, ...);

    std::string str_field(const std::string &) const;
    int int_field(const std::string &) const;
    long long_field(const std::string &) const;

private:
    void map_fields() const;

private:
    typedef std::vector< std::pair<std::string, std::string> > xl_fields;
    typedef std::map<std::string, std::string> xl_map;

    xl_fields fields;
    mutable xl_map fieldmap;
};

class scorefile_entry
{
private:
    std::string raw_line;

    std::string version;
    int         points;
    std::string name;
    species_type race;
    int         job;                // job_type + legacy values
    std::string race_class_name;    // overrides race & cls if non-empty.
    uint8_t     lvl;                // player level.
    skill_type  best_skill;         // best skill #
    uint8_t     best_skill_lvl;     // best skill level
    int         death_type;
    int         death_source;       // NON_MONSTER or monster type
    std::string death_source_name;  // overrides death_source
    std::string auxkilldata;        // weapon wielded, spell cast, etc
    std::string indirectkiller;     // the effect or real monster that summoned
    std::string killerpath;         // colon-separated intermediate killers
    uint8_t     dlvl;               // dungeon level (relative)
    short       absdepth;           // 1-based absolute depth
    level_area_type level_type;     // what kind of level died on..
    branch_type branch;             // dungeon branch
    std::string map;                // the vault (if any) the player is in
    std::string mapdesc;            // DESC: of the vault the player is in.
    int         final_hp;           // actual current HPs (probably <= 0)
    int         final_max_hp;       // net HPs after rot
    int         final_max_max_hp;   // gross HPs before rot
    int         damage;             // damage of final attack
    int         source_damage;      // total damage done by death_source
    int         turn_damage;        // total damage done last turn
    int         str;                // final str (useful for nickname)
    int         intel;              // final int
    int         dex;                // final dex (useful for nickname)
    god_type    god;                // god
    int         piety;              // piety
    int         penance;            // penance
    uint8_t     wiz_mode;           // character used wiz mode
    time_t      birth_time;         // start time of character
    time_t      death_time;         // end time of character
    time_t      real_time;          // real playing time in seconds
    int         num_turns;          // number of turns taken
    int         num_diff_runes;     // number of rune types in inventory
    int         num_runes;          // total number of runes in inventory
    int         kills;              // number of monsters killed
    std::string maxed_skills;       // comma-separated list of skills
                                    // at level 27
    std::string status_effects;     // comma-separated list of status effects
    int         gold;               // Remaining gold.
    int         gold_found;         // Gold found.
    int         gold_spent;         // Gold spent shopping.

    uint32_t    fruit_found_mask;   // Mask of fruits found.

    mutable std::auto_ptr<xlog_fields> fields;

public:
    scorefile_entry();
    scorefile_entry(int damage, int death_source, int death_type,
                    const char *aux, bool death_cause_only = false,
                    const char *death_source_name = NULL,
                    time_t death_time = 0);
    scorefile_entry(const scorefile_entry &se);

    scorefile_entry &operator = (const scorefile_entry &other);

    void init_death_cause(int damage, int death_source, int death_type,
                          const char *aux, const char *death_source_name);
    void init(time_t death_time = 0);
    void reset();

    enum death_desc_verbosity {
        DDV_TERSE,
        DDV_ONELINE,
        DDV_NORMAL,
        DDV_VERBOSE,
        DDV_LOGVERBOSE     // Semi-verbose for logging purposes
    };

    std::string raw_string() const;
    bool parse(const std::string &line);

    std::string hiscore_line(death_desc_verbosity verbosity) const;

    std::string character_description(death_desc_verbosity) const;
    // Full description of death: Killed by an xyz wielding foo
    std::string death_description(death_desc_verbosity) const;
    std::string death_place(death_desc_verbosity) const;
    std::string game_time(death_desc_verbosity) const;

    int    get_score() const      { return points; }
    int    get_death_type() const { return death_type; }
    time_t get_death_time() const { return death_time; }
    xlog_fields get_fields() const;

    void set_base_xlog_fields() const;
    std::string short_kill_message() const;
    std::string long_kill_message() const;

private:
    std::string single_cdesc() const;
    std::string strip_article_a(const std::string &s) const;
    std::string terse_missile_cause() const;
    std::string terse_missile_name() const;
    std::string terse_beam_cause() const;
    std::string terse_wild_magic() const;
    const char *damage_verb() const;
    std::string death_source_desc() const;
    std::string damage_string(bool terse = false) const;

    bool parse_scoreline(const std::string &line);

    void init_with_fields();
    void add_auxkill_field() const;
    void set_score_fields() const;
    void fixup_char_name();

    std::string make_oneline(const std::string &s) const;

    void init_from(const scorefile_entry &other);
};

#endif  // HISCORES_H
