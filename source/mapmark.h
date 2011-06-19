/**
 * @file
 * @brief Level markers (annotations).
**/

#ifndef __MAPMARK_H__
#define __MAPMARK_H__

#include "dgnevent.h"
#include "clua.h"
#include "dlua.h"
#include <map>
#include <string>
#include <vector>
#include <memory>

//////////////////////////////////////////////////////////////////////////
// Map markers

class reader;
class writer;

void remove_markers_and_listeners_at(coord_def p);

bool marker_vetoes_operation(const char *op);
bool feature_marker_at(const coord_def &pos, dungeon_feature_type feat);
coord_def find_marker_position_by_prop(const std::string &prop,
                                       const std::string &expected = "");
std::vector<coord_def> find_marker_positions_by_prop(
    const std::string &prop,
    const std::string &expected = "",
    unsigned maxresults = 0);
std::vector<map_marker*> find_markers_by_prop(
    const std::string &prop,
    const std::string &expected = "",
    unsigned maxresults = 0);

class map_marker
{
public:
    map_marker(map_marker_type type, const coord_def &pos);
    virtual ~map_marker();

    map_marker_type get_type() const { return type; }

    virtual map_marker *clone() const = 0;
    virtual void activate(bool verbose = true);
    virtual void write(writer &) const;
    virtual void read(reader &);
    virtual std::string debug_describe() const = 0;
    virtual std::string property(const std::string &pname) const;

    static map_marker *read_marker(reader &);
    static map_marker *parse_marker(const std::string &text,
                                    const std::string &ctx = "")
        throw (std::string);

public:
    coord_def pos;

protected:
    map_marker_type type;

    typedef map_marker *(*marker_reader)(reader &, map_marker_type);
    typedef map_marker *(*marker_parser)(const std::string &,
                                         const std::string &);
    static marker_reader readers[NUM_MAP_MARKER_TYPES];
    static marker_parser parsers[NUM_MAP_MARKER_TYPES];
};

class map_feature_marker : public map_marker
{
public:
    map_feature_marker(const coord_def &pos = coord_def(0, 0),
                       dungeon_feature_type feat = DNGN_UNSEEN);
    map_feature_marker(const map_feature_marker &other);
    void write(writer &) const;
    void read(reader &);
    std::string debug_describe() const;
    map_marker *clone() const;
    static map_marker *read(reader &, map_marker_type);
    static map_marker *parse(const std::string &s, const std::string &)
        throw (std::string);

public:
    dungeon_feature_type feat;
};

class map_corruption_marker : public map_marker
{
public:
    map_corruption_marker(const coord_def &pos = coord_def(0, 0),
                          int dur = 0);

    void write(writer &) const;
    void read(reader &);
    map_marker *clone() const;
    std::string debug_describe() const;

    static map_marker *read(reader &, map_marker_type);

public:
    int duration;
};

class map_tomb_marker : public map_marker
{
public:
    map_tomb_marker(const coord_def& pos = coord_def(0, 0),
                    int dur = 0, int src = 0, int targ = 0);

    void write(writer &) const;
    void read(reader &);
    map_marker *clone() const;
    std::string debug_describe() const;

    static map_marker *read(reader &, map_marker_type);

public:
    int duration, source, target;
};

class map_malign_gateway_marker : public map_marker
{
public:
    map_malign_gateway_marker (const coord_def& pos = coord_def(0, 0),
                    int dur = 0, bool ip = false, std::string caster = "",
                    beh_type bh = BEH_HOSTILE, god_type gd = GOD_NO_GOD,
                    int pow = 0);

    void write (writer &) const;
    void read (reader &);
    map_marker *clone() const;
    std::string debug_describe() const;

    static map_marker *read(reader &, map_marker_type);

public:
    int duration;
    bool is_player;
    bool monster_summoned;
    std::string summoner_string;
    beh_type behaviour;
    god_type god;
    int power;
};

// A marker powered by phoenixes!
class map_phoenix_marker : public map_marker
{
public:
    map_phoenix_marker (const coord_def& pos = coord_def(0, 0),
                    int tst = 0, int tso = 0, beh_type bh = BEH_HOSTILE,
                    god_type gd = GOD_NO_GOD, coord_def cp = coord_def(-1, -1)
                    );

    void write (writer &) const;
    void read (reader &);
    map_marker *clone() const;
    std::string debug_describe() const;

    static map_marker *read(reader &, map_marker_type);

public:
    int turn_start;
    int turn_stop;
    beh_type behaviour;
    god_type god;
    coord_def& corpse_pos;
};

// A marker powered by Lua.
class map_lua_marker : public map_marker, public dgn_event_listener
{
public:
    map_lua_marker();
    map_lua_marker(const lua_datum &function);
    map_lua_marker(const std::string &s, const std::string &ctx,
                   bool mapdef_marker = true);
    ~map_lua_marker();

    void activate(bool verbose);

    void write(writer &) const;
    void read(reader &);
    map_marker *clone() const;
    std::string debug_describe() const;
    std::string property(const std::string &pname) const;

    bool notify_dgn_event(const dgn_event &e);

    static map_marker *read(reader &, map_marker_type);
    static map_marker *parse(const std::string &s, const std::string &)
        throw (std::string);

    std::string debug_to_string() const;
private:
    bool initialised;
    std::auto_ptr<lua_datum> marker_table;

private:
    void check_register_table();
    bool get_table() const;
    void push_fn_args(const char *fn) const;
    bool callfn(const char *fn, bool warn_err = false, int args = -1) const;
    std::string call_str_fn(const char *fn) const;
};

class map_wiz_props_marker : public map_marker
{
public:
    map_wiz_props_marker(const coord_def &pos = coord_def(0, 0));
    map_wiz_props_marker(const map_wiz_props_marker &other);
    void write(writer &) const;
    void read(reader &);
    std::string debug_describe() const;
    std::string property(const std::string &pname) const;
    std::string set_property(const std::string &key, const std::string &val);
    map_marker *clone() const;
    static map_marker *read(reader &, map_marker_type);
    static map_marker *parse(const std::string &s, const std::string &)
        throw (std::string);

public:
    std::map<std::string, std::string> properties;
};

#endif
