/**
 * @file
 * @brief Misc monster related functions.
**/

// $pellbinder: (c) D.G.S.E 1998
// some routines snatched from former monsstat.cc

#include "AppHdr.h"

#include "mon-util.h"

#include "act-iter.h"
#include "artefact.h"
#include "beam.h"
#include "colour.h"
#include "coordit.h"
#include "database.h"
#include "debug.h"
#include "dgn-overview.h"
#include "directn.h"
#include "env.h"
#include "fight.h"
#include "food.h"
#include "fprop.h"
#include "ghost.h"
#include "goditem.h"
#include "itemname.h"
#include "libutil.h"
#include "mapmark.h"
#include "mislead.h"
#include "mon-abil.h"
#include "mon-behv.h"
#include "mon-clone.h"
#include "mon-death.h"
#include "mon-iter.h"
#include "mon-place.h"
#include "coord.h"
#include "mon-stuff.h"
#include "options.h"
#include "random.h"
#include "religion.h"
#include "species.h"
#include "spl-util.h"
#include "state.h"
#include "stuff.h"
#include "terrain.h"
#include "traps.h"
#include "unicode.h"
#include "viewchar.h"

//jmf: moved from inside function
static FixedVector < int, NUM_MONSTERS > mon_entry;

mon_display monster_symbols[NUM_MONSTERS];

static bool initialised_randmons = false;
static std::vector<monster_type> monsters_by_habitat[NUM_HABITATS];

#include "mon-mst.h"

struct mon_spellbook
{
    mon_spellbook_type type;
    spell_type spells[NUM_MONSTER_SPELL_SLOTS];
};

static mon_spellbook mspell_list[] =
{
#include "mon-spll.h"
};

#include "mon-data.h"

#define MONDATASIZE ARRAYSZ(mondata)

static int _mons_exp_mod(int mclass);

// Macro that saves some typing, nothing more.
#define smc get_monster_data(mc)

/* ******************** BEGIN PUBLIC FUNCTIONS ******************** */

habitat_type grid2habitat(dungeon_feature_type grid)
{
    if (feat_is_watery(grid))
        return (HT_WATER);

    if (feat_is_rock(grid) && !feat_is_permarock(grid))
        return (HT_ROCK);

    switch (grid)
    {
    case DNGN_LAVA:
        return (HT_LAVA);
    case DNGN_FLOOR:
    default:
        return (HT_LAND);
    }
}

dungeon_feature_type habitat2grid(habitat_type ht)
{
    switch (ht)
    {
    case HT_WATER:
        return (DNGN_DEEP_WATER);
    case HT_LAVA:
        return (DNGN_LAVA);
    case HT_ROCK:
        return (DNGN_ROCK_WALL);
    case HT_LAND:
    case HT_AMPHIBIOUS:
    default:
        return (DNGN_FLOOR);
    }
}

static void _initialise_randmons()
{
    for (int i = 0; i < NUM_HABITATS; ++i)
    {
        const dungeon_feature_type grid = habitat2grid(habitat_type(i));

        for (int m = 0; m < NUM_MONSTERS; ++m)
        {
            monster_type mt = static_cast<monster_type>(m);
            if (invalid_monster_type(mt))
                continue;

            if (monster_habitable_grid(mt, grid))
                monsters_by_habitat[i].push_back(mt);
        }
    }
    initialised_randmons = true;
}

monster_type random_monster_at_grid(const coord_def& p)
{
    return (random_monster_at_grid(grd(p)));
}

monster_type random_monster_at_grid(dungeon_feature_type grid)
{
    if (!initialised_randmons)
        _initialise_randmons();

    const habitat_type ht = grid2habitat(grid);
    const std::vector<monster_type> &valid_mons = monsters_by_habitat[ht];

    ASSERT(!valid_mons.empty());
    return (valid_mons.empty() ? MONS_PROGRAM_BUG
                               : valid_mons[ random2(valid_mons.size()) ]);
}

typedef std::map<std::string, unsigned> mon_name_map;
static mon_name_map Mon_Name_Cache;

void init_mon_name_cache()
{
    if (!Mon_Name_Cache.empty())
        return;

    for (unsigned i = 0; i < sizeof(mondata) / sizeof(*mondata); ++i)
    {
        std::string name = mondata[i].name;
        lowercase(name);

        const int          mtype = mondata[i].mc;
        const monster_type mon   = monster_type(mtype);

        // Deal sensibly with duplicate entries; refuse or allow the
        // insert, depending on which should take precedence.  Mostly we
        // don't care, except looking up "rakshasa" and getting _FAKE
        // breaks ?/M rakshasa.
        if (Mon_Name_Cache.find(name) != Mon_Name_Cache.end())
        {
            if (mon == MONS_RAKSHASA_FAKE
                || mon == MONS_ARMOUR_MIMIC
                || mon == MONS_SCROLL_MIMIC
                || mon == MONS_POTION_MIMIC
                || mon == MONS_DOOR_MIMIC
                || mon == MONS_PORTAL_MIMIC
                || mon == MONS_SHOP_MIMIC
                || mon == MONS_STAIR_MIMIC
                || mon == MONS_FOUNTAIN_MIMIC
                || mon == MONS_MARA_FAKE)
            {
                // Keep previous entry.
                continue;
            }
            else
                die("Un-handled duplicate monster name: %s", name.c_str());
        }

        Mon_Name_Cache[name] = mon;
    }
}

monster_type get_monster_by_name(std::string name, bool exact)
{
    lowercase(name);

    if (exact)
    {
        mon_name_map::iterator i = Mon_Name_Cache.find(name);

        if (i != Mon_Name_Cache.end())
            return static_cast<monster_type>(i->second);

        return MONS_PROGRAM_BUG;
    }

    monster_type mon = MONS_PROGRAM_BUG;
    for (unsigned i = 0; i < sizeof(mondata) / sizeof(*mondata); ++i)
    {
        std::string candidate = mondata[i].name;
        lowercase(candidate);

        const int mtype = mondata[i].mc;

        const std::string::size_type match = candidate.find(name);
        if (match == std::string::npos)
            continue;

        mon = monster_type(mtype);
        // We prefer prefixes over partial matches.
        if (match == 0)
            break;
    }
    return (mon);
}

void init_monsters()
{
    // First, fill static array with dummy values. {dlb}
    mon_entry.init(-1);

    // Next, fill static array with location of entry in mondata[]. {dlb}:
    for (unsigned int i = 0; i < MONDATASIZE; ++i)
        mon_entry[mondata[i].mc] = i;

    // Finally, monsters yet with dummy entries point to TTTSNB(tm). {dlb}:
    for (unsigned int i = 0; i < NUM_MONSTERS; ++i)
        if (mon_entry[i] == -1)
            mon_entry[i] = mon_entry[MONS_PROGRAM_BUG];

    init_monster_symbols();
}

void init_monster_symbols()
{
    std::map<unsigned, monster_type> base_mons;
    std::map<unsigned, monster_type>::iterator it;
    for (int i = 0; i < NUM_MONSTERS; ++i)
    {
        mon_display &md = monster_symbols[i];
        const monsterentry *me = get_monster_data(i);
        if (me)
        {
            md.glyph  = me->showchar;
            md.colour = me->colour;
            it = base_mons.find(md.glyph);
            if (it == base_mons.end() || it->first == MONS_PROGRAM_BUG)
                base_mons[md.glyph] = static_cast<monster_type>(i);
            md.detected = base_mons[md.glyph];
        }
    }

    for (int i = 0, size = Options.mon_glyph_overrides.size();
         i < size; ++i)
    {
        const mon_display &md = Options.mon_glyph_overrides[i];
        if (md.type == MONS_PROGRAM_BUG)
            continue;

        if (md.glyph)
            monster_symbols[md.type].glyph = get_glyph_override(md.glyph);
        if (md.colour)
            monster_symbols[md.type].colour = md.colour;
    }

    monster_symbols[MONS_GOLD_MIMIC].glyph   = dchar_glyph(DCHAR_ITEM_GOLD);
    monster_symbols[MONS_WEAPON_MIMIC].glyph = dchar_glyph(DCHAR_ITEM_WEAPON);
    monster_symbols[MONS_ARMOUR_MIMIC].glyph = dchar_glyph(DCHAR_ITEM_ARMOUR);
    monster_symbols[MONS_SCROLL_MIMIC].glyph = dchar_glyph(DCHAR_ITEM_SCROLL);
    monster_symbols[MONS_POTION_MIMIC].glyph = dchar_glyph(DCHAR_ITEM_POTION);

    // Validate all glyphs, even those which didn't come from an override.
    for (int i = 0; i < NUM_MONSTERS; ++i)
        if (wcwidth(monster_symbols[i].glyph) != 1)
            monster_symbols[i].glyph = mons_base_char(i);
}

static bool _get_kraken_head(monster& mon)
{
    if (!valid_kraken_connection(&mon))
        return (false);

    // For kraken tentacle segments, find the associated tentacle.
    if (mon.type == MONS_KRAKEN_TENTACLE_SEGMENT)
    {
        if (invalid_monster_index(mon.number))
            return (false);

        mon = menv[mon.number];
    }

    // For kraken tentacles, find the associated head.
    if (mon.type == MONS_KRAKEN_TENTACLE)
    {
        if (invalid_monster_index(mon.number))
            return (false);

        mon = menv[mon.number];
    }

    return (true);
}

const mon_resist_def &get_mons_class_resists(int mc)
{
    const monsterentry *me = get_monster_data(mc);
    return (me ? me->resists : get_monster_data(MONS_PROGRAM_BUG)->resists);
}

mon_resist_def serpent_of_hell_resists(int flavour)
{
    mon_resist_def res;

    switch (flavour)
    {
    case BRANCH_GEHENNA:
        res.hellfire = 1;
        res.fire = 3;
        break;

    case BRANCH_COCYTUS:
        res.cold = 3;
        break;

    case BRANCH_TARTARUS:
        res.rotting = 1;
        break;
    }

    return res;
}

static mon_resist_def serpent_of_hell_resists(const monster* mon)
{
    int flavour = mon->props["serpent_of_hell_flavour"].get_int();
    return serpent_of_hell_resists(flavour);
}

mon_resist_def get_mons_resists(const monster* mon)
{
    monster newmon = *mon;

    _get_kraken_head(newmon);

    mon_resist_def resists;

    if (mons_is_ghost_demon(newmon.type))
        resists = newmon.ghost->resists;
    else
        resists = mon_resist_def();

    resists |= get_mons_class_resists(newmon.type);

    // Undead get one level of poison resistance.  Don't just add it; if
    // they're undead due to the MF_FAKE_UNDEAD flag, they might have at
    // least one level already, in which case they shouldn't get more.
    if (newmon.holiness() == MH_UNDEAD)
        resists.poison = std::max(static_cast<int>(resists.poison), 1);

    if (mons_genus(newmon.type) == MONS_DRACONIAN
            && newmon.type != MONS_DRACONIAN
        || newmon.type == MONS_TIAMAT)
    {
        monster_type draco_species = draco_subspecies(&newmon);
        if (draco_species != newmon.type)
            resists |= get_mons_class_resists(draco_species);
    }

    if (newmon.type == MONS_SERPENT_OF_HELL)
        resists |= serpent_of_hell_resists(&newmon);

    return (resists);
}

monster* monster_at(const coord_def &pos)
{
    if (!in_bounds(pos))
        return NULL;

    const int mindex = mgrd(pos);
    if (mindex == NON_MONSTER)
        return NULL;

    ASSERT(mindex <= MAX_MONSTERS);
    return (&menv[mindex]);
}

int mons_piety(const monster* mon)
{
    if (mon->god == GOD_NO_GOD)
        return (0);

    // We're open to fine-tuning.
    return (mon->hit_dice * 14);
}

bool mons_class_flag(int mc, uint64_t bf)
{
    const monsterentry *me = get_monster_data(mc);
    return (me ? (me->bitfields & bf) != 0 : false);
}

int scan_mon_inv_randarts(const monster* mon,
                          artefact_prop_type ra_prop)
{
    int ret = 0;

    if (mons_itemuse(mon) >= MONUSE_STARTING_EQUIPMENT)
    {
        const int weapon = mon->inv[MSLOT_WEAPON];
        const int second = mon->inv[MSLOT_ALT_WEAPON]; // Two-headed ogres, etc.
        const int armour = mon->inv[MSLOT_ARMOUR];
        const int shield = mon->inv[MSLOT_SHIELD];

        if (weapon != NON_ITEM && mitm[weapon].base_type == OBJ_WEAPONS
            && is_artefact(mitm[weapon]))
        {
            ret += artefact_wpn_property(mitm[weapon], ra_prop);
        }

        if (second != NON_ITEM && mitm[second].base_type == OBJ_WEAPONS
            && is_artefact(mitm[second]))
        {
            ret += artefact_wpn_property(mitm[second], ra_prop);
        }

        if (armour != NON_ITEM && mitm[armour].base_type == OBJ_ARMOUR
            && is_artefact(mitm[armour]))
        {
            ret += artefact_wpn_property(mitm[armour], ra_prop);
        }

        if (shield != NON_ITEM && mitm[shield].base_type == OBJ_ARMOUR
            && is_artefact(mitm[shield]))
        {
            ret += artefact_wpn_property(mitm[shield], ra_prop);
        }
    }

    return (ret);
}

static int _scan_mon_inv_items(const monster* mon,
                               bool (*item_type)(const item_def&))
{
    int ret = 0;

    if (mons_itemuse(mon) >= MONUSE_STARTING_EQUIPMENT)
    {
        const int weapon = mon->inv[MSLOT_WEAPON];
        const int second = mon->inv[MSLOT_ALT_WEAPON]; // Two-headed ogres, etc.
        const int misc   = mon->inv[MSLOT_MISCELLANY];
        const int potion = mon->inv[MSLOT_POTION];
        const int wand   = mon->inv[MSLOT_WAND];
        const int scroll = mon->inv[MSLOT_SCROLL];

        if (weapon != NON_ITEM && mitm[weapon].base_type == OBJ_WEAPONS
            && item_type(mitm[weapon]))
        {
            ret++;
        }

        if (second != NON_ITEM && mitm[second].base_type == OBJ_WEAPONS
            && item_type(mitm[second]))
        {
            ret++;
        }

        if (misc != NON_ITEM && mitm[misc].base_type == OBJ_MISCELLANY
            && item_type(mitm[misc]))
        {
            ret++;
        }

        if (potion != NON_ITEM && mitm[potion].base_type == OBJ_POTIONS
            && item_type(mitm[potion]))
        {
            ret++;
        }

        if (wand != NON_ITEM && mitm[misc].base_type == OBJ_WANDS
            && item_type(mitm[wand]))
        {
            ret++;
        }

        if (scroll != NON_ITEM && mitm[scroll].base_type == OBJ_SCROLLS
            && item_type(mitm[scroll]))
        {
            ret++;
        }
    }

    return (ret);
}

static bool _mons_has_undrinkable_potion(const monster* mon)
{
    if (mons_itemuse(mon) >= MONUSE_STARTING_EQUIPMENT)
    {
        const int potion = mon->inv[MSLOT_POTION];

        if (potion != NON_ITEM && mitm[potion].base_type == OBJ_POTIONS)
        {
            const potion_type ptype =
                static_cast<potion_type>(mitm[potion].sub_type);

            if (!mon->can_drink_potion(ptype))
                return (true);
        }
    }

    return (false);
}

int mons_unusable_items(const monster* mon)
{
    int ret = 0;

    if (mon->is_holy())
        ret += _scan_mon_inv_items(mon, is_evil_item) > 0;
    else if (mon->undead_or_demonic())
    {
        ret += _scan_mon_inv_items(mon, is_holy_item) > 0;

        if (mon->holiness() == MH_UNDEAD && _mons_has_undrinkable_potion(mon))
            ret++;
    }

    return (ret);
}

mon_holy_type mons_class_holiness(int mc)
{
    ASSERT(smc);
    return (smc->holiness);
}

bool mons_class_is_confusable(int mc)
{
    ASSERT(smc);
    return (smc->resist_magic < MAG_IMMUNE
            && mons_class_holiness(mc) != MH_NONLIVING
            && mons_class_holiness(mc) != MH_PLANT);
}

bool mons_class_is_slowable(int mc)
{
    ASSERT(smc);
    return (smc->resist_magic < MAG_IMMUNE);
}

bool mons_class_is_stationary(int mc)
{
    return (mons_class_flag(mc, M_STATIONARY));
}

bool mons_is_stationary(const monster* mon)
{
    return (mons_class_is_stationary(mon->type)
            || mons_is_feat_mimic(mon->type) && mons_is_unknown_mimic(mon)
            || mon->has_ench(ENCH_WITHDRAWN));
}

// Monsters that are worthless obstacles: not to
// be attacked by default, but may be cut down to
// get to target even if coaligned.
bool mons_class_is_firewood(int mc)
{
    return (mons_class_is_stationary(mc)
            && mons_class_flag(mc, M_NO_EXP_GAIN)
            && mc != MONS_KRAKEN_TENTACLE
            && mc != MONS_KRAKEN_TENTACLE_SEGMENT);
}

bool mons_is_firewood(const monster* mon)
{
    return (mons_class_is_firewood(mon->type));
}

// "body" in a purely grammatical sense.
bool mons_has_body(const monster* mon)
{
    if (mon->type == MONS_FLYING_SKULL
        || mon->type == MONS_CURSE_SKULL
        || mon->type == MONS_CURSE_TOE
        || mon->type == MONS_DANCING_WEAPON)
    {
        return (false);
    }

    switch (mons_base_char(mon->type))
    {
    case 'P':
    case 'v':
    case 'G':
    case '*':
    case '%':
    case 'J':
        return (false);
    }

    return (true);
}

bool mons_has_flesh(const monster* mon)
{
    if (mon->is_skeletal() || mon->is_insubstantial())
        return false;

    // Dictionary says:
    // 1. (12) flesh -- (the soft tissue of the body of a vertebrate:
    //    mainly muscle tissue and fat)
    // 3. pulp, flesh -- (a soft moist part of a fruit)
    // yet I exclude sense 3 anyway but include arthropods and molluscs.
    return (mon->holiness() != MH_PLANT
            && mon->holiness() != MH_NONLIVING
            && mons_base_char(mon->type) != 'G'  // eyes
            && mons_base_char(mon->type) != 'J'  // jellies
            && mons_base_char(mon->type) != '%');// cobs (plant!)
}

// Difference in speed between monster and the player for Cheibriados'
// purposes. This is the speed difference disregarding the player's
// slow status.
int cheibriados_monster_player_speed_delta(const monster* mon)
{
    // Ignore the Slow effect.
    unwind_var<int> ignore_slow(you.duration[DUR_SLOW], 0);
    const int pspeed = 1000 / (player_movement_speed(true) * player_speed());
#ifdef DEBUG_DIAGNOSTICS
    mprf(MSGCH_DIAGNOSTICS, "Your delay: %d, your speed: %d, mon speed: %d",
        player_movement_speed(), pspeed, mon->speed);
#endif
    return (mon->speed - pspeed);
}

bool cheibriados_thinks_mons_is_fast(const monster* mon)
{
    return (cheibriados_monster_player_speed_delta(mon) > 0);
}

bool mons_is_projectile(int mc)
{
    return (mc == MONS_ORB_OF_DESTRUCTION);
}

// Conjuration or Hexes.  Summoning and Necromancy make the monster a creature
// at least in some degree, golems have a chem granting them that.
bool mons_is_object(int mc)
{
    return mons_is_projectile(mc)
           || mc == MONS_BALL_LIGHTNING
           || mc == MONS_FIRE_VORTEX
           || mc == MONS_SPATIAL_VORTEX
           || mc == MONS_TWISTER
           // unloading seeds helps the species
           || mc == MONS_GIANT_SPORE
           || mc == MONS_DANCING_WEAPON;
}

bool mons_has_blood(int mc)
{
    // XXX: Jory is hacked in here so that he can
    // explode in a cloud of blood when he dies (this
    // is historically and thematically good).
    return (mons_class_flag(mc, M_COLD_BLOOD)
            || mons_class_flag(mc, M_WARM_BLOOD)
            || mc == MONS_JORY);
}

bool mons_is_sensed(int mc)
{
    return mc == MONS_SENSED
           || mc == MONS_SENSED_FRIENDLY
           || mc == MONS_SENSED_TRIVIAL
           || mc == MONS_SENSED_EASY
           || mc == MONS_SENSED_TOUGH
           || mc == MONS_SENSED_NASTY;
}

bool mons_behaviour_perceptible(const monster* mon)
{
    return (!mons_class_flag(mon->type, M_NO_EXP_GAIN)
            && !mons_is_mimic(mon->type)
            && !mons_is_statue(mon->type)
            && mon->type != MONS_OKLOB_PLANT
            && mon->type != MONS_BALLISTOMYCETE
            && mon->type != MONS_OKLOB_SAPLING
            && mon->type != MONS_BURNING_BUSH);
}

// Returns true for monsters that obviously (to the player) feel
// "thematically at home" in a branch.  Currently used for native
// monsters recognising traps and patrolling branch entrances.
bool mons_is_native_in_branch(const monster* mons,
                              const branch_type branch)
{
    switch (branch)
    {
    case BRANCH_ELVEN_HALLS:
        return (mons_genus(mons->type) == MONS_ELF);

    case BRANCH_ORCISH_MINES:
        return (mons_genus(mons->type) == MONS_ORC);

    case BRANCH_DWARVEN_HALL:
        return (mons_genus(mons->type) == MONS_DWARF);

    case BRANCH_SHOALS:
        return (mons_species(mons->type) == MONS_CYCLOPS
                || mons_species(mons->type) == MONS_MERFOLK
                || mons_genus(mons->type) == MONS_MERMAID
                || mons->type == MONS_HARPY);

    case BRANCH_SLIME_PITS:
        return (mons_genus(mons->type) == MONS_JELLY);

    case BRANCH_SNAKE_PIT:
        return (mons_genus(mons->type) == MONS_NAGA
                || mons_genus(mons->type) == MONS_SNAKE);

    case BRANCH_HALL_OF_ZOT:
        return (mons_genus(mons->type) == MONS_DRACONIAN
                || mons->type == MONS_ORB_GUARDIAN
                || mons->type == MONS_ORB_OF_FIRE
                || mons->type == MONS_DEATH_COB);

    case BRANCH_CRYPT:
        return (mons->holiness() == MH_UNDEAD);

    case BRANCH_TOMB:
        return (mons_genus(mons->type) == MONS_MUMMY);

    case BRANCH_HIVE:
        return (mons->type == MONS_KILLER_BEE_LARVA
                || mons->type == MONS_KILLER_BEE
                || mons->type == MONS_QUEEN_BEE);

    case BRANCH_HALL_OF_BLADES:
        return (mons->type == MONS_DANCING_WEAPON);

    default:
        return (false);
    }
}

bool mons_is_poisoner(const monster* mon)
{
    if (chunk_is_poisonous(mons_corpse_effect(mon->type)))
        return (true);

    if (mon->has_attack_flavour(AF_POISON)
        || mon->has_attack_flavour(AF_POISON_NASTY)
        || mon->has_attack_flavour(AF_POISON_MEDIUM)
        || mon->has_attack_flavour(AF_POISON_STRONG)
        || mon->has_attack_flavour(AF_POISON_STR)
        || mon->has_attack_flavour(AF_POISON_INT)
        || mon->has_attack_flavour(AF_POISON_DEX)
        || mon->has_attack_flavour(AF_POISON_STAT))
    {
        return (true);
    }

    return (false);
}

// Monsters considered as "slime" for Jiyva.
bool mons_class_is_slime(int mc)
{
    return (mons_genus(mc) == MONS_JELLY
            || mons_genus(mc) == MONS_GIANT_EYEBALL
            || mons_genus(mc) == MONS_GIANT_ORANGE_BRAIN);
}

bool mons_is_slime(const monster* mon)
{
    return (mons_class_is_slime(mon->type));
}

bool herd_monster_class(int mc)
{
    return (mons_class_flag(mc, M_HERD));
}

bool herd_monster(const monster * mon)
{
    return (herd_monster_class(mon->type));
}

// Plant or fungus really
bool mons_class_is_plant(int mc)
{
    return (mons_genus(mc) == MONS_PLANT
            || mons_genus(mc) == MONS_BUSH
            || mons_genus(mc) == MONS_FUNGUS);
}

bool mons_is_plant(const monster* mon)
{
    return (mons_class_is_plant(mon->type));
}

bool mons_eats_items(const monster* mon)
{
    return (mons_itemeat(mon) == MONEAT_ITEMS
            || mon->has_ench(ENCH_EAT_ITEMS));
}

bool mons_eats_corpses(const monster* mon)
{
    return (mons_itemeat(mon) == MONEAT_CORPSES);
}

bool mons_eats_food(const monster* mon)
{
    return (mons_itemeat(mon) == MONEAT_FOOD);
}

bool invalid_monster(const monster* mon)
{
    return (!mon || invalid_monster_type(mon->type));
}

bool invalid_monster_type(monster_type mt)
{
    return (mt < 0 || mt >= NUM_MONSTERS
            || mon_entry[mt] == mon_entry[MONS_PROGRAM_BUG]);
}

bool invalid_monster_index(int i)
{
    return (i < 0 || i >= MAX_MONSTERS);
}

bool mons_is_statue(int mc, bool allow_disintegrate)
{
    if (mc == MONS_ORANGE_STATUE || mc == MONS_SILVER_STATUE)
        return (true);

    if (!allow_disintegrate)
        return (mc == MONS_ICE_STATUE || mc == MONS_ROXANNE);

    return (false);
}

bool mons_is_mimic(int mc)
{
    return (mons_is_item_mimic(mc) || mons_is_feat_mimic(mc));
}

bool mons_is_item_mimic(int mc)
{
    return (mons_genus(mc) == MONS_GOLD_MIMIC);
}

bool mons_is_feat_mimic(int mc)
{
    return (mons_genus(mc) == MONS_DOOR_MIMIC);
}

void discover_mimic(monster* mimic)
{
    if (mons_is_known_mimic(mimic))
        return;

    mimic->flags |= MF_KNOWN_MIMIC;
    if (mons_is_feat_mimic(mimic->type))
    {
        unnotice_feature(level_pos(level_id::current(), mimic->pos()));
        if (mimic->type == MONS_SHOP_MIMIC)
            StashTrack.remove_shop(mimic->pos());
    }
}

bool mons_is_demon(int mc)
{
    const char show_char = mons_base_char(mc);

    // Not every demonic monster is a demon (hell hog, hell hound, etc.)
    if (mons_class_holiness(mc) == MH_DEMONIC
        && (isadigit(show_char) || show_char == '&'))
    {
        return (true);
    }

    return (false);
}

bool mons_is_draconian(int mc)
{
    return (mc >= MONS_FIRST_DRACONIAN && mc <= MONS_LAST_DRACONIAN);
}

// Conjured (as opposed to summoned) monsters are actually here, eventhough
// they're typically volatile (like, made of real fire).  As such, they
// should be immune to Abjuration or Recall.  Also, they count as things
// rather than beings.
bool mons_is_conjured(int mc)
{
    return mons_is_projectile(mc)
           || mc == MONS_FIRE_VORTEX
           || mc == MONS_SPATIAL_VORTEX
           || mc == MONS_BALL_LIGHTNING;
}

// Returns true if the given monster's foe is also a monster.
bool mons_foe_is_mons(const monster* mons)
{
    const actor *foe = mons->get_foe();
    return (foe && foe->atype() == ACT_MONSTER);
}

int mons_weight(int mc)
{
    ASSERT(smc);
    return (smc->weight);
}

corpse_effect_type mons_corpse_effect(int mc)
{
    ASSERT(smc);
    return (smc->corpse_thingy);
}

monster_type mons_species(int mc)
{
    const monsterentry *me = get_monster_data(mc);
    return (me ? me->species : MONS_PROGRAM_BUG);
}

monster_type mons_genus(int mc)
{
    if (mc == RANDOM_DRACONIAN || mc == RANDOM_BASE_DRACONIAN
        || mc == RANDOM_NONBASE_DRACONIAN
        || (mc == MONS_PLAYER_ILLUSION && player_genus(GENPC_DRACONIAN)))
    {
        return (MONS_DRACONIAN);
    }

    if (mc == MONS_NO_MONSTER)
        return (MONS_NO_MONSTER);

    ASSERT(smc);
    return (smc->genus);
}

monster_type mons_detected_base(monster_type mc)
{
    return (monster_symbols[mc].detected);
}

monster_type draco_subspecies(const monster* mon)
{
    ASSERT(mons_genus(mon->type) == MONS_DRACONIAN);

    if (mon->type == MONS_PLAYER_ILLUSION)
        return (player_species_to_mons_species(mon->ghost->species));

    monster_type retval = mons_species(mon->type);

    if (retval == MONS_DRACONIAN && mon->type != MONS_DRACONIAN)
        retval = static_cast<monster_type>(mon->base_monster);

    return (retval);
}

int get_shout_noise_level(const shout_type shout)
{
    switch (shout)
    {
    case S_SILENT:
        return 0;
    case S_HISS:
    case S_VERY_SOFT:
        return 4;
    case S_SOFT:
        return 6;
    case S_GURGLE:
    case S_LOUD:
        return 10;
    case S_SHOUT2:
    case S_ROAR:
    case S_VERY_LOUD:
        return 12;

    default:
        return 8;
    }
}

// Only beasts and chaos spawns use S_RANDOM for noise type.
// Pandemonium lords can also get here, but this is mostly used for the
// "says" verb used for insults.
static bool _shout_fits_monster(int type, int shout)
{
    if (shout == NUM_SHOUTS || shout >= NUM_LOUDNESS || shout == S_SILENT)
        return (false);

    // Chaos spawns can do anything but demon taunts, since they're
    // not coherent enough to actually say words.
    if (type == MONS_CHAOS_SPAWN)
        return (shout != S_DEMON_TAUNT);

    // For Pandemonium lords, almost everything is fair game.  It's only
    // used for the shouting verb ("say", "bellow", "roar", etc.) anyway.
    if (type != MONS_BEAST)
        return (shout != S_BUZZ && shout != S_WHINE && shout != S_CROAK);

    switch (shout)
    {
    // 2-headed ogres, bees or mosquitos never fit.
    case S_SHOUT2:
    case S_BUZZ:
    case S_WHINE:
    // The beast cannot speak.
    case S_DEMON_TAUNT:
        return (false);
    default:
        return (true);
    }
}

// If demon_shout is true, we're trying to find a random verb and
// loudness for a Pandemonium lord trying to shout.
shout_type mons_shouts(int mc, bool demon_shout)
{
    ASSERT(smc);
    shout_type u = smc->shouts;

    // Pandemonium lords use this to get the noises.
    if (u == S_RANDOM || demon_shout && u == S_DEMON_TAUNT)
    {
        const int max_shout = (u == S_RANDOM ? NUM_SHOUTS : NUM_LOUDNESS);
        do
            u = static_cast<shout_type>(random2(max_shout));
        while (!_shout_fits_monster(mc, u));
    }

    return (u);
}

bool mons_is_ghost_demon(int mc)
{
    return (mc == MONS_UGLY_THING
            || mc == MONS_VERY_UGLY_THING
            || mc == MONS_PLAYER_GHOST
            || mc == MONS_PLAYER_ILLUSION
            || mc == MONS_DANCING_WEAPON
            || mc == MONS_PANDEMONIUM_DEMON
            || mc == MONS_LABORATORY_RAT);
}

bool mons_is_pghost(int mc)
{
    return (mc == MONS_PLAYER_GHOST || mc == MONS_PLAYER_ILLUSION);
}

bool mons_is_unique(int mc)
{
    return (mons_class_flag(mc, M_UNIQUE));
}

bool mons_sense_invis(const monster* mon)
{
    return (mons_class_flag(mon->type, M_SENSE_INVIS));
}

wchar_t mons_char(int mc)
{
    return monster_symbols[mc].glyph;
}

char mons_base_char(int mc)
{
    const monsterentry *me = get_monster_data(mc);
    return (me ? me->showchar : 0);
}

mon_itemuse_type mons_class_itemuse(int mc)
{
    ASSERT(smc);
    return (smc->gmon_use);
}

mon_itemuse_type mons_itemuse(const monster* mon)
{
    if (mons_enslaved_soul(mon))
        return (mons_class_itemuse(mons_zombie_base(mon)));

    return (mons_class_itemuse(mon->type));
}

mon_itemeat_type mons_class_itemeat(int mc)
{
    ASSERT(smc);
    return (smc->gmon_eat);
}

mon_itemeat_type mons_itemeat(const monster* mon)
{
    if (mons_enslaved_soul(mon))
        return (mons_class_itemeat(mons_zombie_base(mon)));

    if (mon->has_ench(ENCH_EAT_ITEMS))
        return (MONEAT_ITEMS);

    return (mons_class_itemeat(mon->type));
}

int mons_class_colour(int mc)
{
    ASSERT(smc);
    return (monster_symbols[mc].colour);
}

int mons_colour(const monster* mon)
{
    return (mon->colour);
}

bool mons_class_can_regenerate(int mc)
{
    return (!mons_class_flag(mc, M_NO_REGEN));
}

bool mons_can_regenerate(const monster* mon)
{
    monster newmon = *mon;

    _get_kraken_head(newmon);

    if (testbits(newmon.flags, MF_NO_REGEN))
        return (false);

    return (mons_class_can_regenerate(newmon.type));
}

bool mons_class_can_display_wounds(int mc)
{
    return (!monster_descriptor(mc, MDSC_NOMSG_WOUNDS));
}

bool mons_can_display_wounds(const monster* mon)
{
    monster newmon = *mon;

    _get_kraken_head(newmon);

    return (mons_class_can_display_wounds(newmon.type));
}

// Size based on zombie class.
zombie_size_type zombie_class_size(monster_type cs)
{
    switch (cs)
    {
        case MONS_ZOMBIE_SMALL:
        case MONS_SIMULACRUM_SMALL:
        case MONS_SKELETON_SMALL:
            return (Z_SMALL);
        case MONS_ZOMBIE_LARGE:
        case MONS_SIMULACRUM_LARGE:
        case MONS_SKELETON_LARGE:
            return (Z_BIG);
        case MONS_SPECTRAL_THING:
            return (Z_NOZOMBIE);
        default:
            die("invalid zombie type");
    }
}

int mons_zombie_size(int mc)
{
    ASSERT(smc);
    return (smc->zombie_size);
}

monster_type mons_zombie_base(const monster* mon)
{
    return (mon->base_monster);
}

bool mons_class_is_zombified(int mc)
{
    return (mc == MONS_ZOMBIE_SMALL || mc == MONS_ZOMBIE_LARGE
            || mc == MONS_SKELETON_SMALL || mc == MONS_SKELETON_LARGE
            || mc == MONS_SIMULACRUM_SMALL || mc == MONS_SIMULACRUM_LARGE
            || mc == MONS_SPECTRAL_THING);
}

bool mons_is_zombified(const monster* mon)
{
    return (mons_class_is_zombified(mon->type));
}

monster_type mons_base_type(const monster* mon)
{
    return (mons_is_zombified(mon) ? mon->base_monster : mon->type);
}

bool mons_class_can_leave_corpse(monster_type mc)
{
    return (mons_weight(mc) > 0);
}

bool mons_class_can_be_zombified(int mc)
{
    monster_type ms = mons_species(mc);
    return (!invalid_monster_type(ms)
            && mons_zombie_size(ms) != Z_NOZOMBIE
            && mons_weight(ms) > 0);
}

bool mons_can_be_zombified(const monster* mon)
{
    return (mons_class_can_be_zombified(mon->type)
            && !mon->is_summoned()
            && !mons_enslaved_body_and_soul(mon));
}

bool mons_class_can_use_stairs(int mc)
{
    return ((!mons_class_is_zombified(mc) || mc == MONS_SPECTRAL_THING)
            && !mons_is_tentacle(mc)
            && mc != MONS_SILENT_SPECTRE
            && mc != MONS_PLAYER_GHOST
            && mc != MONS_GERYON
            && mc != MONS_ROYAL_JELLY);
}

bool mons_can_use_stairs(const monster* mon)
{
    return (mons_class_can_use_stairs(mon->type));
}

bool mons_enslaved_body_and_soul(const monster* mon)
{
    return (mon->has_ench(ENCH_SOUL_RIPE));
}

bool mons_enslaved_soul(const monster* mon)
{
    return (testbits(mon->flags, MF_ENSLAVED_SOUL));
}

bool name_zombie(monster* mon, int mc, const std::string mon_name)
{
    mon->mname = mon_name;

    // Special case for Blork the orc: shorten his name to "Blork" to
    // avoid mentions of "Blork the orc the orc zombie".
    if (mc == MONS_BLORK_THE_ORC)
        mon->mname = "Blork";
    // Also for the Lernaean hydra: treat Lernaean as an adjective to
    // avoid mentions of "the Lernaean hydra the X-headed hydra zombie".
    else if (mc == MONS_LERNAEAN_HYDRA)
    {
        mon->mname = "Lernaean";
        mon->flags |= MF_NAME_ADJECTIVE;
    }

    if (starts_with(mon->mname, "shaped "))
        mon->flags |= MF_NAME_SUFFIX;

    return (true);
}

bool name_zombie(monster* mon, const monster* orig)
{
    if (!mons_is_unique(orig->type) && orig->mname.empty())
        return (false);

    std::string name;

    if (!orig->mname.empty())
        name = orig->mname;
    else
        name = mons_type_name(orig->type, DESC_PLAIN);

    return (name_zombie(mon, orig->type, name));
}

static int _downscale_zombie_damage(int damage)
{
    // These are cumulative, of course: {dlb}
    if (damage > 1)
        damage--;
    if (damage > 4)
        damage--;
    if (damage > 11)
        damage--;
    if (damage > 14)
        damage--;

    return (damage);
}

static mon_attack_def _downscale_zombie_attack(const monster* mons,
                                               mon_attack_def attk)
{
    switch (attk.type)
    {
    case AT_STING: case AT_SPORE:
    case AT_TOUCH: case AT_ENGULF:
        attk.type = AT_HIT;
        break;
    default:
        break;
    }

    if (mons->type == MONS_SIMULACRUM_LARGE
        || mons->type == MONS_SIMULACRUM_SMALL)
    {
        attk.flavour = AF_COLD;
    }
    else if (mons->type == MONS_SPECTRAL_THING && coinflip())
        attk.flavour = AF_DRAIN_XP;
    else if (attk.flavour != AF_REACH)
        attk.flavour = AF_PLAIN;

    attk.damage = _downscale_zombie_damage(attk.damage);

    return (attk);
}

mon_attack_def mons_attack_spec(const monster* mon, int attk_number)
{
    int mc = mon->type;

    if (mc == MONS_KRAKEN_TENTACLE
        && !invalid_monster_index(mon->number))
    {
        // Use the zombie, etc info from the kraken if it's alive.
        const monster* body_kraken = &menv[mon->number];
        if (body_kraken->alive())
            mon = body_kraken;
    }

    const bool zombified = mons_is_zombified(mon);

    if (attk_number < 0 || attk_number > 3 || mon->has_hydra_multi_attack())
        attk_number = 0;

    if (mons_is_ghost_demon(mc))
    {
        if (attk_number == 0)
        {
            return (mon_attack_def::attk(mon->ghost->damage,
                                         mon->ghost->att_type,
                                         mon->ghost->att_flav));
        }

        return (mon_attack_def::attk(0, AT_NONE));
    }

    if (zombified && mc != MONS_KRAKEN_TENTACLE)
        mc = mons_zombie_base(mon);

    ASSERT(smc);
    mon_attack_def attk = smc->attack[attk_number];

    if (attk.type == AT_RANDOM)
        attk.type = static_cast<mon_attack_type>(random_range(AT_HIT,
                                                              AT_GORE));

    if (attk.flavour == AF_KLOWN)
    {
        mon_attack_flavour flavours[] =
            {AF_POISON_NASTY, AF_ROT, AF_DRAIN_XP, AF_FIRE, AF_COLD, AF_BLINK};

        attk.flavour = RANDOM_ELEMENT(flavours);
    }

    if (attk.flavour == AF_POISON_STAT)
    {
        mon_attack_flavour flavours[] =
            {AF_POISON_STR, AF_POISON_INT, AF_POISON_DEX};

        attk.flavour = RANDOM_ELEMENT(flavours);
    }

    if (attk.flavour == AF_DRAIN_STAT)
    {
        mon_attack_flavour flavours[] =
            {AF_DRAIN_STR, AF_DRAIN_INT, AF_DRAIN_DEX};

        attk.flavour = RANDOM_ELEMENT(flavours);
    }

    // Slime creature attacks are multiplied by the number merged.
    if (mon->type == MONS_SLIME_CREATURE && mon->number > 1)
        attk.damage *= mon->number;

    return (zombified ? _downscale_zombie_attack(mon, attk) : attk);
}

int mons_damage(int mc, int rt)
{
    if (rt < 0 || rt > 3)
        rt = 0;
    ASSERT(smc);
    return (smc->attack[rt].damage);
}

std::string resist_margin_phrase(int margin)
{
    ASSERT(margin > 0);

    return (margin >= 40) ? " easily resists." :
           (margin >= 24) ? " resists." :
           (margin >= 12) ? " resists with some effort."
                          : " struggles to resist.";
}

bool mons_immune_magic(const monster* mon)
{
    return (get_monster_data(mon->type)->resist_magic == MAG_IMMUNE);
}

std::string mons_resist_string(const monster* mon, int res_margin)
{
    if (mons_immune_magic(mon))
        return " is unaffected";
    else
        return resist_margin_phrase(res_margin);
}

bool mons_skeleton(int mc)
{
    return (!mons_class_flag(mc, M_NO_SKELETON));
}

flight_type mons_class_flies(int mc)
{
    const monsterentry *me = get_monster_data(mc);
    return (me ? me->fly : FL_NONE);
}

flight_type mons_flies(const monster* mon, bool temp)
{
    flight_type ret;
    // For dancing weapons, this function can get called before their
    // ghost_demon is created, so check for a NULL ghost. -cao
    if (mons_is_ghost_demon(mon->type) && mon->ghost.get())
        ret = mon->ghost->fly;
    else
        ret = mons_class_flies(mons_base_type(mon));

    // Handle the case where the zombified base monster can't fly, but
    // the zombified monster can (e.g. spectral things).
    if (ret == FL_NONE && mons_is_zombified(mon))
        ret = mons_class_flies(mon->type);

    if (temp && ret == FL_NONE
        && scan_mon_inv_randarts(mon, ARTP_LEVITATE) > 0)
    {
        ret = FL_LEVITATE;
    }

    if (temp && ret == FL_NONE && mon->has_ench(ENCH_LEVITATION))
        ret = FL_LEVITATE;

    return (ret);
}

bool mons_class_flattens_trees(int mc)
{
    return (mc == MONS_LERNAEAN_HYDRA);
}

bool mons_flattens_trees(const monster* mon)
{
    return (mons_class_flattens_trees(mons_base_type(mon)));
}

int mons_class_res_wind(int mc)
{
    // Lightning goes well with storms.
    if (mc == MONS_AIR_ELEMENTAL || mc == MONS_BALL_LIGHTNING
        || mc == MONS_TWISTER)
    {
        return 1;
    }

    // Flyers are not immune due to buffeting -- and for airstrike, even
    // specially vulnerable.
    // Smoky humanoids may have problems staying together.
    // Insubstantial wisps are a toss-up between being immune and immediately
    // fatally dispersing.
    return 0;
}

bool mons_class_wall_shielded(int mc)
{
    return (mons_class_habitat(mc) == HT_ROCK);
}

bool mons_wall_shielded(const monster* mon)
{
    return (mons_class_wall_shielded(mons_base_type(mon)));
}

// This nice routine we keep in exactly the way it was.
int hit_points(int hit_dice, int min_hp, int rand_hp)
{
    int hrolled = 0;

    for (int hroll = 0; hroll < hit_dice; ++hroll)
    {
        hrolled += random2(1 + rand_hp);
        hrolled += min_hp;
    }

    return (hrolled);
}

// This function returns the standard number of hit dice for a type of
// monster, not a particular monster's current hit dice. - bwr
int mons_class_hit_dice(int mc)
{
    const monsterentry *me = get_monster_data(mc);
    return (me ? me->hpdice[0] : 0);
}

int mons_difficulty(int mc)
{
    // Currently, difficulty is defined as "average hp".  Leaks too much info?
    const monsterentry* me = get_monster_data(mc);

    if (!me)
        return (0);

    // [ds] XXX: Use monster experience value as a better indicator of diff.?
    return (me->hpdice[0] * (me->hpdice[1] + (me->hpdice[2]/2))
            + me->hpdice[3]);
}

int exper_value(const monster* mon)
{
    long x_val = 0;

    // These four are the original arguments.
    const int mc          = mon->type;
    const int hd          = mon->hit_dice;
    int maxhp             = mon->max_hit_points;

    // A berserking monster is much harder, but the xp value shouldn't depend
    // on whether it was berserk at the moment of death.
    if (mon->has_ench(ENCH_BERSERK))
        maxhp = (maxhp * 2 + 1) / 3;

    // Hacks to make merged slime creatures not worth so much exp.  We
    // will calculate the experience we would get for 1 blob, and then
    // just multiply it so that exp is linear with blobs merged. -cao
    if (mon->type == MONS_SLIME_CREATURE && mon->number > 1)
        maxhp /= mon->number;

    // These are some values we care about.
    const int speed       = mons_base_speed(mon);
    const int modifier    = _mons_exp_mod(mc);
    const int item_usage  = mons_itemuse(mon);

    // XXX: Shapeshifters can qualify here, even though they can't cast.
    const bool spellcaster = mon->can_use_spells();

    // Early out for no XP monsters.
    if (mons_class_flag(mc, M_NO_EXP_GAIN))
        return (0);

    // The beta26 statues have non-spell-like abilities that the experience
    // code can't see, so inflate their XP a bit.  Ice statues and Roxanne
    // get plenty of XP for their spells.
    if (mc == MONS_ORANGE_STATUE || mc == MONS_SILVER_STATUE)
        return (hd * 15);

    x_val = (16 + maxhp) * (hd * hd) / 10;

    // Let's calculate a simple difficulty modifier. - bwr
    int diff = 0;

    // Let's look for big spells.
    if (spellcaster)
    {
        const monster_spells &hspell_pass = mon->spells;

        for (int i = 0; i < 6; ++i)
        {
            switch (hspell_pass[i])
            {
            case SPELL_PARALYSE:
            case SPELL_SMITING:
            case SPELL_HAUNT:
            case SPELL_HELLFIRE_BURST:
            case SPELL_HELLFIRE:
            case SPELL_SYMBOL_OF_TORMENT:
            case SPELL_ICE_STORM:
            case SPELL_FIRE_STORM:
                diff += 25;
                break;

            case SPELL_LIGHTNING_BOLT:
            case SPELL_BOLT_OF_DRAINING:
            case SPELL_VENOM_BOLT:
            case SPELL_STICKY_FLAME_RANGE:
            case SPELL_DISINTEGRATE:
            case SPELL_SUMMON_GREATER_DEMON:
            case SPELL_BANISHMENT:
            case SPELL_LEHUDIBS_CRYSTAL_SPEAR:
            case SPELL_IRON_SHOT:
            case SPELL_TELEPORT_SELF:
            case SPELL_TELEPORT_OTHER:
            case SPELL_PORKALATOR:
                diff += 10;
                break;

            default:
                break;
            }
        }
    }

    // Let's look at regeneration.
    if (monster_descriptor(mc, MDSC_REGENERATES))
        diff += 15;

    // Monsters at normal or fast speed with big melee damage.
    if (speed >= 10)
    {
        int max_melee = 0;
        for (int i = 0; i < 4; ++i)
            max_melee += mons_damage(mc, i);

        if (max_melee > 30)
            diff += (max_melee / ((speed == 10) ? 2 : 1));
    }

    // Monsters who can use equipment (even if only the equipment
    // they are given) can be considerably enhanced because of
    // the way weapons work for monsters. - bwr
    if (item_usage >= MONUSE_STARTING_EQUIPMENT)
        diff += 30;

    // Set a reasonable range on the difficulty modifier...
    // Currently 70% - 200%. - bwr
    if (diff > 100)
        diff = 100;
    else if (diff < -30)
        diff = -30;

    // Apply difficulty.
    x_val *= (100 + diff);
    x_val /= 100;

    // Basic speed modification.
    if (speed > 0)
    {
        x_val *= speed;
        x_val /= 10;
    }

    // Slow monsters without spells and items often have big HD which
    // cause the experience value to be overly large... this tries
    // to reduce the inappropriate amount of XP that results. - bwr
    if (speed < 10 && !spellcaster && item_usage < MONUSE_STARTING_EQUIPMENT)
        x_val /= 2;

    // Apply the modifier in the monster's definition.
    if (modifier > 0)
    {
        x_val *= modifier;
        x_val /= 10;
    }

    // Slime creature exp hack part 2: Scale exp back up by the number
    // of blobs merged. -cao
    if (mon->type == MONS_SLIME_CREATURE && mon->number > 1)
        x_val *= mon->number;

    // Reductions for big values. - bwr
    if (x_val > 100)
        x_val = 100 + ((x_val - 100) * 3) / 4;
    if (x_val > 1000)
        x_val = 1000 + (x_val - 1000) / 2;

    // Guarantee the value is within limits.
    if (x_val <= 0)
        x_val = 1;
    else if (x_val > 15000)
        x_val = 15000;

    return (x_val);
}

monster_type random_draconian_monster_species()
{
    const int num_drac = MONS_PALE_DRACONIAN - MONS_BLACK_DRACONIAN + 1;
    return static_cast<monster_type>(MONS_BLACK_DRACONIAN + random2(num_drac));
}

// Note: For consistent behavior in player_will_anger_monster(), all
// spellbooks a given monster can get here should produce the same
// return values in the following:
//
//     is_holy_spell()
//
//     (is_unholy_spell() || is_evil_spell())
//
//     (is_unclean_spell() || is_chaotic_spell())
//
// FIXME: This is not true for one set of spellbooks; MST_WIZARD_IV
// contains the unholy and chaotic Banishment spell, but the other
// MST_WIZARD-type spellbooks contain no unholy, evil, unclean or
// chaotic spells.
static bool _get_spellbook_list(mon_spellbook_type book[6],
                                monster* mon)
{
    bool retval = true;
    int mon_type = mon->type;

    book[0] = MST_NO_SPELLS;
    book[1] = MST_NO_SPELLS;
    book[2] = MST_NO_SPELLS;
    book[3] = MST_NO_SPELLS;
    book[4] = MST_NO_SPELLS;
    book[5] = MST_NO_SPELLS;

    switch (mon_type)
    {
    case MONS_DEEP_ELF_CONJURER:
        book[0] = MST_DEEP_ELF_CONJURER_I;
        book[1] = MST_DEEP_ELF_CONJURER_II;
        break;

    case MONS_HELL_KNIGHT:
        book[0] = MST_HELL_KNIGHT_I;
        book[1] = MST_HELL_KNIGHT_II;
        break;

    case MONS_LICH:
    case MONS_ANCIENT_LICH:
        book[0] = MST_LICH_I;
        book[1] = MST_LICH_II;
        book[2] = MST_LICH_III;
        book[3] = MST_LICH_IV;
        break;

    case MONS_NECROMANCER:
        book[0] = MST_NECROMANCER_I;
        book[1] = MST_NECROMANCER_II;
        break;

    case MONS_DEEP_DWARF_NECROMANCER:
        book[0] = MST_DEEP_DWARF_NECROMANCER;
        book[1] = MST_JESSICA;
        break;

    case MONS_UNBORN_DEEP_DWARF:
        book[0] = MST_UNBORN_DEEP_DWARF;
        book[1] = MST_NECROMANCER_I;
        break;

    case MONS_ORC_WIZARD:
    case MONS_DEEP_ELF_SOLDIER:
    case MONS_DEEP_ELF_FIGHTER:
    case MONS_DEEP_ELF_KNIGHT:
        book[0] = MST_ORC_WIZARD_I;
        book[1] = MST_ORC_WIZARD_II;
        book[2] = MST_ORC_WIZARD_III;
        break;

    case MONS_WIZARD:
    case MONS_OGRE_MAGE:
    case MONS_EROLCHA:
    case MONS_DEEP_ELF_MAGE:
        book[0] = MST_WIZARD_I;
        book[1] = MST_WIZARD_II;
        book[2] = MST_WIZARD_III;
        book[3] = MST_WIZARD_IV;
        book[4] = MST_WIZARD_V;
        break;

    case MONS_DRACONIAN_KNIGHT:
        book[0] = MST_DEEP_ELF_CONJURER_I;
        book[1] = MST_DEEP_ELF_CONJURER_II;
        book[2] = MST_HELL_KNIGHT_I;
        book[3] = MST_HELL_KNIGHT_II;
        book[4] = MST_NECROMANCER_I;
        book[5] = MST_NECROMANCER_II;
        break;

    case MONS_SERPENT_OF_HELL:
        switch (mon->props["serpent_of_hell_flavour"].get_int())
        {
        case BRANCH_GEHENNA:
            book[0] = MST_SERPENT_OF_HELL_GEHENNA;
            break;
        case BRANCH_COCYTUS:
            book[0] = MST_SERPENT_OF_HELL_COCYTUS;
            break;
        case BRANCH_DIS:
            book[0] = MST_SERPENT_OF_HELL_DIS;
            break;
        case BRANCH_TARTARUS:
            book[0] = MST_SERPENT_OF_HELL_TARTARUS;
            break;
        default:
            book[0] = MST_SERPENT_OF_HELL_GEHENNA;
            break;
        }
        break;

    default:
        retval = false;
        break;
    }

    return (retval);
}

void _mons_load_spells(monster* mon, mon_spellbook_type book)
{
    if (book == MST_GHOST)
        return mon->load_ghost_spells();

    mon->spells.init(SPELL_NO_SPELL);
    if (book == MST_NO_SPELLS)
        return;

    dprf("%s: loading spellbook #%d", mon->name(DESC_PLAIN, true).c_str(),
         static_cast<int>(book));

    for (unsigned int i = 0; i < ARRAYSZ(mspell_list); ++i)
    {
        if (mspell_list[i].type == book)
        {
            for (int j = 0; j < NUM_MONSTER_SPELL_SLOTS; ++j)
                mon->spells[j] = mspell_list[i].spells[j];
            break;
        }
    }
}

static void _get_spells(mon_spellbook_type book, monster* mon)
{
    if (book == MST_NO_SPELLS && mons_class_flag(mon->type, M_SPELLCASTER))
    {
        mon_spellbook_type multi_book[6];
        if (_get_spellbook_list(multi_book, mon))
        {
            do
                book = multi_book[random2(6)];
            while (book == MST_NO_SPELLS);
        }
    }

    _mons_load_spells(mon, book);

    // (Dumb) special casing to give ogre mages Haste Other. -cao
    if (mon->type == MONS_OGRE_MAGE)
        mon->spells[0] = SPELL_HASTE_OTHER;
}

// Never hand out DARKGREY as a monster colour, even if it is randomly
// chosen.
uint8_t random_monster_colour()
{
    uint8_t col = DARKGREY;
    while (col == DARKGREY)
        col = random_colour();

    return (col);
}

// Abominations.
uint8_t random_large_abomination_colour()
{
    uint8_t col;
    // Restricted colours:
    //  MAGENTA = orb guardian
    //  GREEN = tentacled monstrosity
    //  RED, LIGHTRED, BROWN = used for twisted resurrection
    do
        col = random_monster_colour();
    while (col == MAGENTA || col == GREEN || col == RED || col == LIGHTRED
           || col == BROWN);

    return (col);
}

uint8_t random_small_abomination_colour()
{
    uint8_t col;
    // Restricted colours:
    //  MAGENTA = unseen horror
    //  RED, LIGHTRED, BROWN = used for twisted resurrection
    do
        col = random_monster_colour();
    while (col == MAGENTA || col == RED || col == LIGHTRED || col == BROWN);

    return (col);
}

static int _serpent_of_hell_color(const monster* mon)
{
    switch (mon->props["serpent_of_hell_flavour"].get_int())
    {
    case BRANCH_GEHENNA:
        return ETC_FIRE;
        break;
    case BRANCH_COCYTUS:
        return ETC_ICE;
        break;
    case BRANCH_DIS:
        return ETC_IRON;
        break;
    case BRANCH_TARTARUS:
        return ETC_NECRO;
        break;
    default:
        return ETC_FIRE;
    }
}

// Generate a shiny, new and unscarred monster.
void define_monster(monster* mons)
{
    int mcls                  = mons->type;
    int monnumber             = mons->number;
    monster_type monbase      = mons->base_monster;
    const monsterentry *m     = get_monster_data(mcls);
    int col                   = mons_class_colour(mcls);
    int hd                    = mons_class_hit_dice(mcls);
    int speed                 = mons_real_base_speed(mcls);
    int hp, hp_max, ac, ev;

    mons->mname.clear();

    // misc
    ac = m->AC;
    ev = m->ev;

    mons->god = GOD_NO_GOD;

    switch (mcls)
    {
    case MONS_ABOMINATION_SMALL:
        hd = 4 + random2(4);
        ac = 3 + random2(7);
        ev = 7 + random2(6);
        col = random_small_abomination_colour();
        break;

    case MONS_ZOMBIE_SMALL:
        hd = (coinflip() ? 1 : 2);
        break;

    case MONS_ABOMINATION_LARGE:
        hd = 8 + random2(4);
        ac = 5 + random2avg(9, 2);
        ev = 3 + random2(5);
        col = random_large_abomination_colour();
        break;

    case MONS_ZOMBIE_LARGE:
        hd = 3 + random2(5);
        break;

    case MONS_BEAST:
        hd = 4 + random2(4);
        ac = 2 + random2(5);
        ev = 7 + random2(5);
        break;

    case MONS_MANTICORE:
        // Manticores start off with 8 to 16 spike volleys.
        monnumber = 8 + random2(9);
        break;

    case MONS_SLIME_CREATURE:
        // Slime creatures start off as only single un-merged blobs.
        monnumber = 1;
        break;

    case MONS_HYDRA:
        // Hydras start off with 4 to 8 heads.
        monnumber = random_range(4, 8);
        break;

    case MONS_LERNAEAN_HYDRA:
        // The Lernaean hydra starts off with 27 heads.
        monnumber = 27;
        break;

    case MONS_GILA_MONSTER:
        if (col != BLACK) // May be overwritten by the mon_glyph option.
            break;

        col = element_colour(ETC_GILA);
        break;

    case MONS_KRAKEN:
        if (col != BLACK) // May be overwritten by the mon_glyph option.
            break;

        col = element_colour(ETC_KRAKEN);
        break;

    case MONS_DRACONIAN_CALLER:
    case MONS_DRACONIAN_MONK:
    case MONS_DRACONIAN_ZEALOT:
    case MONS_DRACONIAN_SHIFTER:
    case MONS_DRACONIAN_ANNIHILATOR:
    case MONS_DRACONIAN_SCORCHER:
    case MONS_DRACONIAN_KNIGHT:
    {
        // Professional draconians still have a base draconian type.
        // White draconians will never be draconian scorchers, but
        // apart from that, anything goes.
        do
            monbase = random_draconian_monster_species();
        while (drac_colour_incompatible(mcls, monbase));
        break;
    }

    case MONS_TIAMAT:
        // Initialise to a random draconian type.
        draconian_change_colour(mons);
        monbase = mons->base_monster;
        col = mons->colour;
        break;

    case MONS_DRACONIAN:
    case MONS_ELF:
    case MONS_HUMAN:
        // These are supposed to only be created by polymorph.
        hd += random2(10) - 4;
        ac += random2(5) - 2;
        ev += random2(5) - 2;
        break;

    case MONS_SERPENT_OF_HELL:
    {
        int &flavour = mons->props["serpent_of_hell_flavour"].get_int();
        if (!flavour)
            flavour = player_in_hell() ? you.where_are_you : BRANCH_GEHENNA;
        col = _serpent_of_hell_color(mons);
        break;
    }

    default:
        if (mons_is_item_mimic(mcls))
            col = get_mimic_colour(mons);
        break;
    }

    if (col == BLACK) // but never give out darkgrey to monsters
        col = random_monster_colour();

    // Some calculations.
    hp     = hit_points(hd, m->hpdice[1], m->hpdice[2]);
    hp    += m->hpdice[3];
    hp_max = hp;

    // So let it be written, so let it be done.
    mons->hit_dice        = hd;
    mons->hit_points      = hp;
    mons->max_hit_points  = hp_max;
    mons->ac              = ac;
    mons->ev              = ev;
    mons->speed           = speed;
    mons->speed_increment = 70;

    if (mons->base_monster == MONS_NO_MONSTER)
        mons->base_monster = monbase;

    if (mons->number == 0)
        mons->number = monnumber;

    mons->flags      = 0L;
    mons->experience = 0L;
    mons->colour     = col;

    mons->bind_melee_flags();

    _get_spells((mon_spellbook_type)m->sec, mons);
    mons->bind_spell_flags();

    // Reset monster enchantments.
    mons->enchantments.clear();
    mons->ench_countdown = 0;

    // NOTE: For player ghosts and (very) ugly things this just ensures
    // that the monster instance is valid and won't crash when used,
    // though the (very) ugly thing generated should actually work.  The
    // player ghost and (very) ugly thing code is currently only used
    // for generating a monster for MonsterMenuEntry in
    // _find_description() in command.cc.
    switch (mcls)
    {
    case MONS_PANDEMONIUM_DEMON:
    {
        ghost_demon ghost;
        ghost.init_random_demon();
        mons->set_ghost(ghost);
        mons->pandemon_init();
        mons->bind_melee_flags();
        mons->bind_spell_flags();
        break;
    }

    case MONS_PLAYER_GHOST:
    case MONS_PLAYER_ILLUSION:
    {
        ghost_demon ghost;
        ghost.init_player_ghost();
        mons->set_ghost(ghost);
        mons->ghost_init(!mons->props.exists("fake"));
        mons->bind_melee_flags();
        mons->bind_spell_flags();
        if (mons->ghost->species == SP_DEEP_DWARF)
            mons->flags |= MF_NO_REGEN;
        break;
    }

    case MONS_UGLY_THING:
    case MONS_VERY_UGLY_THING:
    {
        ghost_demon ghost;
        ghost.init_ugly_thing(mcls == MONS_VERY_UGLY_THING);
        mons->set_ghost(ghost, false);
        mons->uglything_init();
        break;
    }

    case MONS_LABORATORY_RAT:
    {
        ghost_demon ghost;
        ghost.init_labrat();
        mons->set_ghost(ghost, false);
        mons->labrat_init();
    }
    default:
        break;
    }
}

static const char *ugly_colour_names[] = {
    "red", "brown", "green", "cyan", "purple", "white"
};

std::string ugly_thing_colour_name(uint8_t colour)
{
    int colour_offset = ugly_thing_colour_offset(colour);

    if (colour_offset == -1)
        return ("buggy");

    return (ugly_colour_names[colour_offset]);
}

std::string ugly_thing_colour_name(const monster* mon)
{
    if (mon->type == MONS_UGLY_THING || mon->type == MONS_VERY_UGLY_THING)
        return (ugly_thing_colour_name(mon->colour));
    else
        return ("buggy");
}

static const uint8_t ugly_colour_values[] = {
    RED, BROWN, GREEN, CYAN, MAGENTA, LIGHTGREY
};

uint8_t ugly_thing_random_colour()
{
    return (RANDOM_ELEMENT(ugly_colour_values));
}

int str_to_ugly_thing_colour(const std::string &s)
{
    COMPILE_CHECK(ARRAYSZ(ugly_colour_values) == ARRAYSZ(ugly_colour_names),
                  ugly_thing_colour_size_check);
    for (int i = 0, size = ARRAYSZ(ugly_colour_values); i < size; ++i)
        if (s == ugly_colour_names[i])
            return ugly_colour_values[i];
    return (BLACK);
}

int ugly_thing_colour_offset(const uint8_t colour)
{
    for (unsigned i = 0; i < ARRAYSZ(ugly_colour_values); ++i)
    {
        if (make_low_colour(colour) == ugly_colour_values[i])
            return (i);
    }

    return (-1);
}

static const char *drac_colour_names[] = {
    "black", "mottled", "yellow", "green", "purple", "red", "white", "grey", "pale"
};

std::string draconian_colour_name(monster_type mon_type)
{
    COMPILE_CHECK(ARRAYSZ(drac_colour_names) ==
                  MONS_PALE_DRACONIAN - MONS_DRACONIAN, c1);

    if (mon_type < MONS_BLACK_DRACONIAN || mon_type > MONS_PALE_DRACONIAN)
        return ("buggy");

    return (drac_colour_names[mon_type - MONS_BLACK_DRACONIAN]);
}

monster_type draconian_colour_by_name(const std::string &name)
{
    COMPILE_CHECK(ARRAYSZ(drac_colour_names)
                  == (MONS_PALE_DRACONIAN - MONS_DRACONIAN), c1);

    for (unsigned i = 0; i < ARRAYSZ(drac_colour_names); ++i)
    {
        if (name == drac_colour_names[i])
            return (static_cast<monster_type>(i + MONS_BLACK_DRACONIAN));
    }

    return (MONS_PROGRAM_BUG);
}

std::string mons_type_name(int type, description_level_type desc)
{
    std::string result;

    if (!mons_is_unique(type))
    {
        switch (desc)
        {
        case DESC_CAP_THE:   result = "The "; break;
        case DESC_NOCAP_THE: result = "the "; break;
        case DESC_CAP_A:     result = "A ";   break;
        case DESC_NOCAP_A:   result = "a ";   break;
        case DESC_PLAIN: default:             break;
        }
    }

    switch (type)
    {
    case RANDOM_MONSTER:
        result += "random monster";
        return (result);
    case RANDOM_DRACONIAN:
        result += "random draconian";
        return (result);
    case RANDOM_BASE_DRACONIAN:
        result += "random base draconian";
        return (result);
    case RANDOM_NONBASE_DRACONIAN:
        result += "random nonbase draconian";
        return (result);
    case WANDERING_MONSTER:
        result += "wandering monster";
        return (result);
    }

    const monsterentry *me = get_monster_data(type);
    ASSERT(me != NULL);
    if (me == NULL)
    {
        result += make_stringf("invalid type %d", type);
        return (result);
    }

    result += me->name;

    // Vowel fix: Change 'a orc' to 'an orc'..
    if (result.length() >= 3
        && (result[0] == 'a' || result[0] == 'A')
        && result[1] == ' '
        && is_vowel(result[2])
        // XXX: Hack
        && !starts_with(&result[2], "one-"))
    {
        result.insert(1, "n");
    }

    return (result);
}

static std::string _get_proper_monster_name(const monster* mon)
{
    const monsterentry *me = mon->find_monsterentry();
    if (!me)
        return ("");

    std::string name = getRandNameString(me->name, " name");
    if (!name.empty())
        return (name);

    name = getRandNameString(get_monster_data(mons_genus(mon->type))->name,
                             " name");

    if (!name.empty())
        return (name);

    name = getRandNameString("generic_monster_name");
    return (name);
}

// Names a previously unnamed monster.
bool give_monster_proper_name(monster* mon, bool orcs_only)
{
    // Already has a unique name.
    if (mon->is_named())
        return (false);

    // Since this is called from the various divine blessing routines,
    // don't bless non-orcs, and normally don't bless plain orcs, either.
    if (orcs_only)
    {
        if (mons_genus(mon->type) != MONS_ORC
            || mon->type == MONS_ORC && !one_chance_in(8))
        {
            return (false);
        }
    }

    mon->mname = _get_proper_monster_name(mon);
    return (mon->is_named());
}

// See mons_init for initialization of mon_entry array.
monsterentry *get_monster_data(int mc)
{
    if (mc >= 0 && mc < NUM_MONSTERS)
        return (&mondata[mon_entry[mc]]);
    else
        return (NULL);
}

static int _mons_exp_mod(int mc)
{
    ASSERT(smc);
    return (smc->exp_mod);
}

int mons_class_base_speed(int mc)
{
    ASSERT(smc);
    return (smc->speed);
}

int mons_real_base_speed(int mc)
{
    ASSERT(smc);
    int speed = smc->speed;

    switch (mc)
    {
    case MONS_ABOMINATION_SMALL:
        speed = 7 + random2avg(9, 2);
        break;
    case MONS_ABOMINATION_LARGE:
        speed = 6 + random2avg(7, 2);
        break;
    case MONS_BEAST:
        speed = 10 + random2(8);
        break;
    }

    return (speed);
}

int mons_class_zombie_base_speed(int zombie_base_mc)
{
    return (std::max(3, mons_class_base_speed(zombie_base_mc) - 2));
}

int mons_base_speed(const monster* mon)
{
    if (mons_enslaved_soul(mon))
        return (mons_class_base_speed(mons_zombie_base(mon)));

    return (mons_is_zombified(mon) ? mons_class_zombie_base_speed(mons_zombie_base(mon))
                                   : mons_class_base_speed(mon->type));
}

mon_intel_type mons_class_intel(int mc)
{
    ASSERT(smc);
    return (smc->intel);
}

mon_intel_type mons_intel(const monster* mon)
{
    monster newmon = *mon;

    _get_kraken_head(newmon);

    if (mons_enslaved_soul(mon))
        return (mons_class_intel(mons_zombie_base(mon)));

    return (mons_class_intel(newmon.type));
}

habitat_type mons_class_habitat(int mc, bool real_amphibious)
{
    const monsterentry *me = get_monster_data(mc);
    habitat_type ht = (me ? me->habitat
                          : get_monster_data(MONS_PROGRAM_BUG)->habitat);
    if (!real_amphibious)
    {
        // XXX: No class equivalent of monster::body_size(PSIZE_BODY)!
        size_type st = (me ? me->size
                           : get_monster_data(MONS_PROGRAM_BUG)->size);
        if (ht == HT_LAND && st >= SIZE_GIANT || mc == MONS_GREY_DRACONIAN)
            ht = HT_AMPHIBIOUS;
    }
    return (ht);
}

habitat_type mons_habitat(const monster* mon, bool real_amphibious)
{
    return (mons_class_habitat(mons_base_type(mon), real_amphibious));
}

habitat_type mons_class_primary_habitat(int mc)
{
    habitat_type ht = mons_class_habitat(mc);
    if (ht == HT_AMPHIBIOUS)
        ht = HT_LAND;
    return (ht);
}

habitat_type mons_primary_habitat(const monster* mon)
{
    return (mons_class_primary_habitat(mons_base_type(mon)));
}

habitat_type mons_class_secondary_habitat(int mc)
{
    habitat_type ht = mons_class_habitat(mc);
    if (ht == HT_AMPHIBIOUS)
        ht = HT_WATER;
    else if (ht == HT_ROCK)
        ht = HT_LAND;
    return (ht);
}

habitat_type mons_secondary_habitat(const monster* mon)
{
    return (mons_class_secondary_habitat(mons_base_type(mon)));
}

bool intelligent_ally(const monster* mon)
{
    return (mon->attitude == ATT_FRIENDLY && mons_intel(mon) >= I_NORMAL);
}

int mons_power(int mc)
{
    // For now, just return monster hit dice.
    ASSERT(smc);
    return (mons_class_hit_dice(mc));
}

bool mons_aligned(const actor *m1, const actor *m2)
{
    mon_attitude_type fr1, fr2;
    const monster* mon1, *mon2;

    if (!m1 || !m2)
        return (true);

    if (m1->atype() == ACT_PLAYER)
        fr1 = ATT_FRIENDLY;
    else
    {
        mon1 = static_cast<const monster* >(m1);
        if (mons_is_projectile(mon1->type))
            return (true);
        fr1 = mons_attitude(mon1);
    }

    if (m2->atype() == ACT_PLAYER)
        fr2 = ATT_FRIENDLY;
    else
    {
        mon2 = static_cast<const monster* >(m2);
        if (mons_is_projectile(mon2->type))
            return (true);
        fr2 = mons_attitude(mon2);
    }

    return (mons_atts_aligned(fr1, fr2));
}

bool mons_atts_aligned(mon_attitude_type fr1, mon_attitude_type fr2)
{
    if (mons_att_wont_attack(fr1) && mons_att_wont_attack(fr2))
        return (true);

    return (fr1 == fr2);
}

bool mons_class_wields_two_weapons(int mc)
{
    return (mons_class_flag(mc, M_TWO_WEAPONS));
}

bool mons_wields_two_weapons(const monster* mon)
{
    if (testbits(mon->flags, MF_TWO_WEAPONS))
        return (true);

    return (mons_class_wields_two_weapons(mons_base_type(mon)));
}

bool mons_self_destructs(const monster* m)
{
    return (m->type == MONS_GIANT_SPORE
            || m->type == MONS_BALL_LIGHTNING
            || m->type == MONS_ORB_OF_DESTRUCTION);
}

int mons_base_damage_brand(const monster* m)
{
    if (mons_is_ghost_demon(m->type))
        return (m->ghost->brand);

    return (SPWPN_NORMAL);
}

bool mons_att_wont_attack(mon_attitude_type fr)
{
    return (fr == ATT_FRIENDLY || fr == ATT_GOOD_NEUTRAL
            || fr == ATT_STRICT_NEUTRAL);
}

mon_attitude_type mons_attitude(const monster* m)
{
    if (m->friendly())
        return ATT_FRIENDLY;
    else if (m->good_neutral())
        return ATT_GOOD_NEUTRAL;
    else if (m->strict_neutral())
        return ATT_STRICT_NEUTRAL;
    else if (m->neutral())
        return ATT_NEUTRAL;
    else
        return ATT_HOSTILE;
}

bool mons_is_confused(const monster* m, bool class_too)
{
    return ((m->has_ench(ENCH_CONFUSION) || m->has_ench(ENCH_MAD))
            && (class_too || !mons_class_flag(m->type, M_CONFUSED)));
}

bool mons_is_wandering(const monster* m)
{
    return (m->behaviour == BEH_WANDER);
}

bool mons_is_seeking(const monster* m)
{
    return (m->behaviour == BEH_SEEK);
}

bool mons_is_fleeing(const monster* m)
{
    return (m->behaviour == BEH_FLEE
            || mons_class_flag(m->type, M_FLEEING));
}

bool mons_is_panicking(const monster* m)
{
    return (m->behaviour == BEH_PANIC);
}

bool mons_is_cornered(const monster* m)
{
    return (m->behaviour == BEH_CORNERED);
}

bool mons_is_lurking(const monster* m)
{
    return (m->behaviour == BEH_LURK);
}

bool mons_is_influenced_by_sanctuary(const monster* m)
{
    return (!m->wont_attack() && !mons_is_stationary(m));
}

bool mons_is_fleeing_sanctuary(const monster* m)
{
    return (mons_is_influenced_by_sanctuary(m)
            && in_bounds(env.sanctuary_pos)
            && (m->flags & MF_FLEEING_FROM_SANCTUARY));
}

bool mons_is_batty(const monster* m)
{
    return mons_class_flag(m->type, M_BATTY);
}

bool mons_was_seen(const monster* m)
{
    return testbits(m->flags, MF_SEEN);
}

bool mons_is_known_mimic(const monster* m)
{
    return mons_is_mimic(m->type) && testbits(m->flags, MF_KNOWN_MIMIC);
}

bool mons_is_unknown_mimic(const monster* m)
{
    return mons_is_mimic(m->type) && !mons_is_known_mimic(m);
}

bool mons_looks_stabbable(const monster* m)
{
    const unchivalric_attack_type uat = is_unchivalric_attack(&you, m);
    return (mons_behaviour_perceptible(m)
            && !m->friendly()
            && (uat == UCAT_PARALYSED || uat == UCAT_SLEEPING));
}

bool mons_looks_distracted(const monster* m)
{
    const unchivalric_attack_type uat = is_unchivalric_attack(&you, m);
    return (mons_behaviour_perceptible(m)
            && !m->friendly()
            && uat != UCAT_NO_ATTACK
            && uat != UCAT_PARALYSED
            && uat != UCAT_SLEEPING);
}

void mons_start_fleeing_from_sanctuary(monster* mons)
{
    mons->flags |= MF_FLEEING_FROM_SANCTUARY;
    mons->target = env.sanctuary_pos;
    behaviour_event(mons, ME_SCARE, MHITNOT, env.sanctuary_pos);
}

void mons_stop_fleeing_from_sanctuary(monster* mons)
{
    const bool had_flag = (mons->flags & MF_FLEEING_FROM_SANCTUARY);
    mons->flags &= (~MF_FLEEING_FROM_SANCTUARY);
    if (had_flag)
        behaviour_event(mons, ME_EVAL, MHITYOU);
}

void mons_pacify(monster* mon, mon_attitude_type att)
{
    // If the _real_ (non-charmed) attitude is already that or better,
    // don't degrade it.  This can happen, for example, with a high-power
    // Crusade card or the Ring of Charms on Pikel's slaves who would then
    // go down from friendly to good_neutral when you kill Pikel.
    if (mon->attitude >= att)
        return;

    // Make the monster permanently neutral.
    mon->attitude = att;
    mon->flags |= MF_WAS_NEUTRAL;

    if (!testbits(mon->flags, MF_GOT_HALF_XP)
        && !mon->is_summoned()
        && !testbits(mon->flags, MF_NO_REWARD))
    {
        // Give the player half of the monster's XP.
        unsigned int exp_gain = 0, avail_gain = 0;
        gain_exp((exper_value(mon) + 1) / 2, &exp_gain, &avail_gain);
        mon->flags |= MF_GOT_HALF_XP;
    }

    // Cancel fleeing and such.
    mon->behaviour = BEH_WANDER;

    // Make the monster begin leaving the level.
    behaviour_event(mon, ME_EVAL);

    if (mons_is_pikel(mon))
        pikel_band_neutralise();
    if (mons_is_elven_twin(mon))
        elven_twins_pacify(mon);
    if (mons_is_kirke(mon))
        hogs_to_humans();

    mons_att_changed(mon);
}

static bool _mons_should_fire_beneficial(bolt &beam)
{
    // Should monster haste other be able to target the player?
    // Saying no for now. -cao
    if (beam.target == you.pos())
        return false;

    // Assuming all beneficial beams are enchantments if a foe is in
    // the path the beam will definitely hit them so we shouldn't fire
    // in that case.
    if (beam.friend_info.count == 0
        || beam.foe_info.count != 0)
    {
        return (false);
    }

    // Should beneficial monster enchantment beams be allowed in a
    // sanctuary? -cao
    if (is_sanctuary(you.pos()) || is_sanctuary(beam.source))
        return (false);

    return (true);
}

static bool _beneficial_beam_flavour(beam_type flavour)
{
    switch (flavour)
    {
    case BEAM_HASTE:
    case BEAM_HEALING:
    case BEAM_INVISIBILITY:
        return (true);

    default:
        return (false);
    }
}

bool mons_should_fire(struct bolt &beam)
{
#ifdef DEBUG_DIAGNOSTICS
    mprf(MSGCH_DIAGNOSTICS,
         "tracer: foes %d (pow: %d), friends %d (pow: %d), "
         "foe_ratio: %d, smart: %s",
         beam.foe_info.count, beam.foe_info.power,
         beam.friend_info.count, beam.friend_info.power,
         beam.foe_ratio, beam.smart_monster ? "yes" : "no");
#endif

    // Use different evaluation criteria if the beam is a beneficial
    // enchantment (haste other).
    if (_beneficial_beam_flavour(beam.flavour))
        return (_mons_should_fire_beneficial(beam));

    // Friendly monsters shouldn't be targeting you: this will happen
    // often because the default behaviour for charmed monsters is to
    // have you as a target.  While foe_ratio will handle this, we
    // don't want a situation where a friendly dragon breathes through
    // you to hit other creatures... it should target the other
    // creatures, and coincidentally hit you.
    //
    // FIXME: this can cause problems with reflection, bounces, etc.
    // It would be better to have the monster fire logic never reach
    // this point for friendlies.
    if (!invalid_monster_index(beam.beam_source))
    {
        monster& m = menv[beam.beam_source];
        if (m.alive() && m.friendly() && beam.target == you.pos())
            return (false);
    }

    // Use of foeRatio:
    // The higher this number, the more monsters will _avoid_ collateral
    // damage to their friends.
    // Setting this to zero will in fact have all monsters ignore their
    // friends when considering collateral damage.

    // Quick check - did we in fact get any foes?
    if (beam.foe_info.count == 0)
        return (false);

    if (is_sanctuary(you.pos()) || is_sanctuary(beam.source))
        return (false);

    // If we hit no friends, fire away.
    if (beam.friend_info.count == 0)
        return (true);

    // Only fire if they do acceptably low collateral damage.
    return (beam.foe_info.power >=
            div_round_up(beam.foe_ratio *
                         (beam.foe_info.power + beam.friend_info.power),
                         100));
}

// Returns true if the spell is something you wouldn't want done if
// you had a friendly target...  only returns a meaningful value for
// non-beam spells.
bool ms_direct_nasty(spell_type monspell)
{
    return (spell_needs_foe(monspell)
            && !spell_typematch(monspell, SPTYP_SUMMONING));
}

// Spells a monster may want to cast if fleeing from the player, and
// the player is not in sight.
bool ms_useful_fleeing_out_of_sight(const monster* mon, spell_type monspell)
{
    if (monspell == SPELL_NO_SPELL || ms_waste_of_time(mon, monspell))
        return (false);

    switch (monspell)
    {
    case SPELL_HASTE:
    case SPELL_SWIFTNESS:
    case SPELL_INVISIBILITY:
    case SPELL_MINOR_HEALING:
    case SPELL_MAJOR_HEALING:
    case SPELL_ANIMATE_DEAD:
        return (true);

    default:
        if (spell_typematch(monspell, SPTYP_SUMMONING) && one_chance_in(4))
            return (true);
        break;
    }

    return (false);
}

bool ms_low_hitpoint_cast(const monster* mon, spell_type monspell)
{
    bool targ_adj      = false;
    bool targ_sanct    = false;
    bool targ_friendly = false;
    bool targ_undead   = false;

    if (mon->foe == MHITYOU || mon->foe == MHITNOT)
    {
        if (adjacent(you.pos(), mon->pos()))
            targ_adj = true;
        if (is_sanctuary(you.pos()))
            targ_sanct = true;
        if (you.undead_or_demonic())
            targ_undead = true;
    }
    else
    {
        if (adjacent(menv[mon->foe].pos(), mon->pos()))
            targ_adj = true;
        if (is_sanctuary(menv[mon->foe].pos()))
            targ_sanct = true;
        if (menv[mon->foe].undead_or_demonic())
            targ_undead = true;
    }

    targ_friendly = (mon->foe == MHITYOU
                     ? mon->wont_attack()
                     : mons_aligned(mon, mon->get_foe()));

    switch (monspell)
    {
    case SPELL_TELEPORT_OTHER:
        return !targ_sanct && !targ_friendly;
    case SPELL_TELEPORT_SELF:
        // Don't cast again if already about to teleport.
        return !mon->has_ench(ENCH_TP);
    case SPELL_MINOR_HEALING:
    case SPELL_MAJOR_HEALING:
        return true;
    case SPELL_VAMPIRIC_DRAINING:
        return !targ_sanct && targ_adj && !targ_friendly && !targ_undead;
    case SPELL_BLINK_AWAY:
    case SPELL_BLINK_RANGE:
        return !targ_friendly;
    case SPELL_BLINK_OTHER:
        return !targ_sanct && targ_adj && !targ_friendly;
    case SPELL_BLINK:
        return targ_adj;
    case SPELL_TOMB_OF_DOROKLOHE:
        return true;
    case SPELL_NO_SPELL:
        return false;
    case SPELL_INK_CLOUD:
        if (mon->type == MONS_KRAKEN)
            return true;
    default:
        return !targ_adj && spell_typematch(monspell, SPTYP_SUMMONING);
    }
}

// Spells for a quick get-away.
// Currently only used to get out of a net.
bool ms_quick_get_away(const monster* mon /*unused*/, spell_type monspell)
{
    switch (monspell)
    {
    case SPELL_TELEPORT_SELF:
        // Don't cast again if already about to teleport.
        if (mon->has_ench(ENCH_TP))
            return (false);
        // intentional fall-through
    case SPELL_BLINK:
        return (true);
    default:
        return (false);
    }
}

// Checks if the foe *appears* to be immune to negative energy.  We
// can't just use foe->res_negative_energy(), because that'll mean
// monsters will just "know" whether a player is fully life-protected.
static bool _foe_should_res_negative_energy(const actor* foe)
{
    const mon_holy_type holiness = foe->holiness();

    if (foe->atype() == ACT_PLAYER)
    {
        // Non-bloodless vampires do not appear immune.
        if (holiness == MH_UNDEAD
            && you.is_undead == US_SEMI_UNDEAD
            && you.hunger_state > HS_STARVING)
        {
            return (false);
        }

        // Demonspawn do not appear immune.
        if (holiness == MH_DEMONIC)
            return (false);
    }

    return (holiness != MH_NATURAL);
}

// Checks to see if a particular spell is worth casting in the first place.
bool ms_waste_of_time(const monster* mon, spell_type monspell)
{
    bool ret = false;
    actor *foe = mon->get_foe();

    // Keep friendly summoners from spamming summons constantly.
    if (mon->friendly()
        && (!foe || foe == &you)
        && spell_typematch(monspell, SPTYP_SUMMONING))
    {
        return (true);
    }

    if (!mon->wont_attack())
    {
        if (spell_harms_area(monspell) && env.sanctuary_time > 0)
            return (true);

        if (spell_harms_target(monspell) && is_sanctuary(mon->target))
            return (true);
    }

    // Eventually, we'll probably want to be able to have monsters
    // learn which of their elemental bolts were resisted and have those
    // handled here as well. - bwr
    switch (monspell)
    {
    case SPELL_CALL_TIDE:
        return (!player_in_branch(BRANCH_SHOALS)
                || mon->has_ench(ENCH_TIDE)
                || !foe
                || (grd(mon->pos()) == DNGN_DEEP_WATER
                    && grd(foe->pos()) == DNGN_DEEP_WATER));

    case SPELL_BRAIN_FEED:
        ret = (foe != &you);
        break;

    case SPELL_BOLT_OF_DRAINING:
    case SPELL_AGONY:
    case SPELL_SYMBOL_OF_TORMENT:
        ret = (!foe || _foe_should_res_negative_energy(foe));
        break;

    case SPELL_MIASMA:
        ret = (!foe || foe->res_rotting());
        break;

    case SPELL_DISPEL_UNDEAD:
        // [ds] How is dispel undead intended to interact with vampires?
        ret = (!foe || foe->holiness() != MH_UNDEAD);
        break;

    case SPELL_CORONA:
        ret = (!foe || foe->backlit() || foe->glows_naturally());
        break;

    case SPELL_BERSERKER_RAGE:
        if (!mon->needs_berserk(false))
            ret = true;
        break;

    case SPELL_HASTE:
        if (mon->has_ench(ENCH_HASTE))
            ret = true;
        break;

    case SPELL_MIGHT:
        if (mon->has_ench(ENCH_MIGHT))
            ret = true;
        break;

    case SPELL_SWIFTNESS:
        if (mon->has_ench(ENCH_SWIFT))
            ret = true;
        break;

    case SPELL_REGENERATION:
        if (mon->has_ench(ENCH_REGENERATION))
            ret = true;
        break;

    case SPELL_MIRROR_DAMAGE:
        if (mon->has_ench(ENCH_MIRROR_DAMAGE))
            ret = true;
        break;

    case SPELL_STONESKIN:
        if (mon->has_ench(ENCH_STONESKIN))
            ret = true;
        break;

    case SPELL_INVISIBILITY:
        if (mon->has_ench(ENCH_INVIS)
            || mon->glows_naturally()
            || mon->friendly() && !you.can_see_invisible(false))
        {
            ret = true;
        }
        break;

    case SPELL_MINOR_HEALING:
    case SPELL_MAJOR_HEALING:
        if (mon->hit_points > mon->max_hit_points / 2)
            ret = true;
        break;

    case SPELL_TELEPORT_SELF:
        // Monsters aren't smart enough to know when to cancel teleport.
        if (mon->has_ench(ENCH_TP))
            ret = true;
        break;

    case SPELL_TELEPORT_OTHER:
        // Monsters aren't smart enough to know when to cancel teleport.
        if (mon->foe == MHITYOU)
        {
            ret = you.duration[DUR_TELEPORT];
            break;
        }
        else if (mon->foe != MHITNOT)
        {
            ret = (menv[mon->foe].has_ench(ENCH_TP));
            break;
        }
        // intentional fall-through

    case SPELL_SLOW:
    case SPELL_CONFUSE:
    case SPELL_PAIN:
    case SPELL_BANISHMENT:
    case SPELL_DISINTEGRATE:
    case SPELL_PARALYSE:
    case SPELL_SLEEP:
    case SPELL_HIBERNATION:
    {
        if (monspell == SPELL_HIBERNATION && (!foe || foe->asleep()))
        {
            ret = true;
            break;
        }

        // Occasionally we don't estimate... just fire and see.
        if (one_chance_in(5))
        {
            ret = false;
            break;
        }

        // Only intelligent monsters estimate.
        int intel = mons_intel(mon);
        if (intel < I_NORMAL)
        {
            ret = false;
            break;
        }

        // We'll estimate the target's resistance to magic, by first getting
        // the actual value and then randomising it.
        int est_magic_resist = (mon->foe == MHITNOT) ? 10000 : 0;

        if (mon->foe != MHITNOT)
        {
            if (mon->foe == MHITYOU)
                est_magic_resist = you.res_magic();
            else
                est_magic_resist = menv[mon->foe].res_magic();

            // now randomise (normal intels less accurate than high):
            if (intel == I_NORMAL)
                est_magic_resist += random2(80) - 40;
            else
                est_magic_resist += random2(30) - 15;
        }

        int power = 12 * mon->hit_dice * (monspell == SPELL_PAIN ? 2 : 1);
        power = stepdown_value(power, 30, 40, 100, 120);

        // Determine the amount of chance allowed by the benefit from
        // the spell.  The estimated difficulty is the probability
        // of rolling over 100 + diff on 2d100. -- bwr
        int diff = (monspell == SPELL_PAIN
                    || monspell == SPELL_SLOW
                    || monspell == SPELL_CONFUSE) ? 0 : 50;

        if (est_magic_resist - power > diff)
            ret = true;
        break;
    }

    case SPELL_MISLEAD:
        if (you.duration[DUR_MISLED] > 10 && one_chance_in(3))
            ret = true;

        break;

    case SPELL_SUMMON_ILLUSION:
        if (!foe || !actor_is_illusion_cloneable(foe))
            ret = true;
        break;

    case SPELL_FAKE_MARA_SUMMON:
        if (count_mara_fakes() == 2)
            ret = true;

        break;

    case SPELL_AWAKEN_FOREST:
        if (mon->has_ench(ENCH_AWAKEN_FOREST)
            || env.forest_awoken_until > you.elapsed_time
            || !forest_near_enemy(mon))
        {
            ret = true;
        }
        break;

    case SPELL_NO_SPELL:
        ret = true;
        break;

    default:
        break;
    }

    return (ret);
}

static bool _ms_los_spell(spell_type monspell)
{
    // True, the tentacles _are_ summoned, but they are restricted to
    // water just like the kraken is, so it makes more sense not to
    // count them here.
    if (monspell == SPELL_KRAKEN_TENTACLES)
        return (false);

    if (monspell == SPELL_SMITING
        || monspell == SPELL_AIRSTRIKE
        || monspell == SPELL_HAUNT
        || monspell == SPELL_MISLEAD
        || monspell == SPELL_SUMMON_SPECTRAL_ORCS
        || spell_typematch(monspell, SPTYP_SUMMONING))
    {
        return (true);
    }

    return (false);
}


static bool _ms_ranged_spell(spell_type monspell, bool attack_only = false,
                             bool ench_too = true)
{
    // Check for Smiting specially, so it's not filtered along
    // with the summon spells.
    if (attack_only
        && (monspell == SPELL_SMITING || monspell == SPELL_AIRSTRIKE
            || monspell == SPELL_MISLEAD))
    {
        return (true);
    }

    // These spells are ranged, but aren't direct attack spells.
    if (_ms_los_spell(monspell))
        return (!attack_only);

    switch (monspell)
    {
    case SPELL_NO_SPELL:
    case SPELL_CANTRIP:
    case SPELL_HASTE:
    case SPELL_MIGHT:
    case SPELL_MINOR_HEALING:
    case SPELL_MAJOR_HEALING:
    case SPELL_TELEPORT_SELF:
    case SPELL_INVISIBILITY:
    case SPELL_BLINK:
    case SPELL_BLINK_CLOSE:
    case SPELL_BLINK_RANGE:
    case SPELL_BERSERKER_RAGE:
    case SPELL_SWIFTNESS:
        return (false);

    // The animation spells don't work through transparent walls and
    // thus are listed here instead of above.
    case SPELL_ANIMATE_DEAD:
    case SPELL_ANIMATE_SKELETON:
        return (!attack_only);

    case SPELL_CONFUSE:
    case SPELL_SLOW:
    case SPELL_PARALYSE:
    case SPELL_SLEEP:
    case SPELL_TELEPORT_OTHER:
    case SPELL_BLINK_OTHER_CLOSE:
    case SPELL_BLINK_AWAY:
    case SPELL_BLINK_OTHER:
        return (ench_too);

    default:
        // All conjurations count as ranged spells.
        return (true);
    }
}

// Returns true if the monster has an ability that only needs LOS to
// affect the target.
bool mons_has_los_ability(monster_type mon_type)
{
    // These eyes only need LOS, as well.  (The other eyes use spells.)
    if (mon_type == MONS_GIANT_EYEBALL
        || mon_type == MONS_EYE_OF_DRAINING
        || mon_type == MONS_GOLDEN_EYE)
    {
        return (true);
    }

    // Although not using spells, these are exceedingly dangerous.
    if (mon_type == MONS_SILVER_STATUE || mon_type == MONS_ORANGE_STATUE)
        return (true);

    // Beholding just needs LOS.
    if (mons_genus(mon_type) == MONS_MERMAID)
        return (true);

    return (false);
}

bool mons_has_los_attack(const monster* mon)
{
    // Monsters may have spell like abilities.
    if (mons_has_los_ability(mon->type))
        return (true);

    if (mon->can_use_spells())
    {
        for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
            if (_ms_los_spell(mon->spells[i]))
                return (true);
    }

    return (false);
}

bool mons_has_ranged_spell(const monster* mon, bool attack_only,
                           bool ench_too)
{
    // Monsters may have spell-like abilities.
    if (mons_has_los_ability(mon->type))
        return (true);

    if (mon->can_use_spells())
    {
        for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
            if (_ms_ranged_spell(mon->spells[i], attack_only, ench_too))
                return (true);
    }

    return (false);
}

bool mons_has_ranged_ability(const monster* mon)
{
    // [ds] FIXME: Get rid of special abilities and remove this.
    if (mon->type == MONS_ELECTRIC_EEL
        || mon->type == MONS_LAVA_SNAKE
        || mon->type == MONS_OKLOB_PLANT)
    {
        return (true);
    }

    return (false);
}

bool mons_has_ranged_attack(const monster* mon)
{
    // Ugh.
    monster* mnc = const_cast<monster* >(mon);
    const item_def *weapon  = mnc->launcher();
    const item_def *primary = mnc->mslot_item(MSLOT_WEAPON);
    const item_def *missile = mnc->missiles();

    if (!missile && weapon != primary && primary
        && get_weapon_brand(*primary) == SPWPN_RETURNING)
    {
        return (true);
    }

    if (!missile)
        return (false);

    return (is_launched(mnc, weapon, *missile));
}


// Use of variant:
// 0 : She is tap dancing.
// 1 : It seems she is tap dancing. (lower case pronoun)
// 2 : Her sword explodes!          (upper case possessive)
// 3 : It sticks to her sword!      (lower case possessive)
// ... as needed

const char *mons_pronoun(monster_type mon_type, pronoun_type variant,
                         bool visible)
{
    gender_type gender = GENDER_NEUTER;

    if (mons_genus(mon_type) == MONS_MERMAID
        || mon_type == MONS_HARPY
        || mon_type == MONS_SPHINX)
    {
        gender = GENDER_FEMALE;
    }
    // Mara's fakes aren't a unique, but should still be classified
    // as male.
    else if (mon_type == MONS_MARA_FAKE)
        gender = GENDER_MALE;
    else if (mons_is_unique(mon_type) && !mons_is_pghost(mon_type))
    {
        switch (mon_type)
        {
        case MONS_TERPSICHORE:
        case MONS_JESSICA:
        case MONS_PSYCHE:
        case MONS_JOSEPHINE:
        case MONS_AGNES:
        case MONS_MAUD:
        case MONS_LOUISE:
        case MONS_FRANCES:
        case MONS_MARGERY:
        case MONS_EROLCHA:
        case MONS_ERICA:
        case MONS_TIAMAT:
        case MONS_ERESHKIGAL:
        case MONS_ROXANNE:
        case MONS_SONJA:
        case MONS_ILSUIW:
        case MONS_NERGALLE:
        case MONS_KIRKE:
        case MONS_DUVESSA:
        case MONS_THE_ENCHANTRESS:
        case MONS_NELLIE:
            gender = GENDER_FEMALE;
            break;
        case MONS_ROYAL_JELLY:
        case MONS_LERNAEAN_HYDRA:
        case MONS_IRON_GIANT:
        case MONS_SERPENT_OF_HELL:
            gender = GENDER_NEUTER;
            break;
        default:
            gender = GENDER_MALE;
            break;
        }
    }

    if (!visible)
        gender = GENDER_NEUTER;

    switch (variant)
    {
        case PRONOUN_CAP:
            return ((gender == GENDER_NEUTER) ? "It" :
                    (gender == GENDER_MALE)   ? "He" : "She");

        case PRONOUN_NOCAP:
            return ((gender == GENDER_NEUTER) ? "it" :
                    (gender == GENDER_MALE)   ? "he" : "she");

        case PRONOUN_CAP_POSSESSIVE:
            return ((gender == GENDER_NEUTER) ? "Its" :
                    (gender == GENDER_MALE)   ? "His" : "Her");

        case PRONOUN_NOCAP_POSSESSIVE:
            return ((gender == GENDER_NEUTER) ? "its" :
                    (gender == GENDER_MALE)   ? "his" : "her");

        case PRONOUN_REFLEXIVE:  // Awkward at start of sentence, always lower.
            return ((gender == GENDER_NEUTER) ? "itself"  :
                    (gender == GENDER_MALE)   ? "himself" : "herself");

        case PRONOUN_OBJECTIVE:  // Awkward at start of sentence, always lower.
            return ((gender == GENDER_NEUTER) ? "it"  :
                    (gender == GENDER_MALE)   ? "him" : "her");
     }

    return ("");
}

// Checks if the monster can use smiting/torment to attack without
// unimpeded LOS to the player.
static bool _mons_has_smite_attack(const monster* mons)
{
    const monster_spells &hspell_pass = mons->spells;
    for (unsigned i = 0; i < hspell_pass.size(); ++i)
        if (hspell_pass[i] == SPELL_SYMBOL_OF_TORMENT
            || hspell_pass[i] == SPELL_SMITING
            || hspell_pass[i] == SPELL_HELLFIRE_BURST
            || hspell_pass[i] == SPELL_FIRE_STORM
            || hspell_pass[i] == SPELL_AIRSTRIKE
            || hspell_pass[i] == SPELL_MISLEAD)
        {
            return (true);
        }

    return (false);
}

// Determines if a monster is smart and pushy enough to displace other
// monsters.  A shover should not cause damage to the shovee by
// displacing it, so monsters that trail clouds of badness are
// ineligible.  The shover should also benefit from shoving, so monsters
// that can smite/torment are ineligible.
//
// (Smiters would be eligible for shoving when fleeing if the AI allowed
// for smart monsters to flee.)
bool monster_shover(const monster* m)
{
    const monsterentry *me = get_monster_data(m->type);
    if (!me)
        return (false);

    // Efreet and fire elementals are disqualified because they leave behind
    // clouds of flame. Rotting devils are disqualified because they trail
    // clouds of miasma.
    if (mons_genus(m->type) == MONS_EFREET || m->type == MONS_FIRE_ELEMENTAL
        || m->type == MONS_ROTTING_DEVIL || m->type == MONS_CURSE_TOE)
    {
        return (false);
    }

    // Monsters too stupid to use stairs (e.g. non-spectral zombified undead)
    // are also disqualified.
    if (!mons_can_use_stairs(m))
        return (false);

    // Smiters profit from staying back and smiting.
    if (_mons_has_smite_attack(m))
        return (false);

    char mchar = me->showchar;

    // Somewhat arbitrary: giants and dragons are too big to get past anything,
    // beetles are too dumb (arguable), dancing weapons can't communicate, eyes
    // aren't pushers and shovers. Worms and elementals are on the list because
    // all 'w' are currently unrelated.
    return (mchar != 'C' && mchar != 'B' && mchar != '(' && mchar != 'D'
            && mchar != 'G' && mchar != 'w' && mchar != '#');
}

// Returns true if m1 and m2 are related, and m1 is higher up the totem pole
// than m2. The criteria for being related are somewhat loose, as you can see
// below.
bool monster_senior(const monster* m1, const monster* m2, bool fleeing)
{
    const monsterentry *me1 = get_monster_data(m1->type),
                       *me2 = get_monster_data(m2->type);

    if (!me1 || !me2)
        return (false);

    // Band leaders can displace followers regardless of type considerations.
    // -cao
    if (m2->props.exists("band_leader"))
    {
        unsigned leader_mid = m2->props["band_leader"].get_int();
        if (leader_mid == m1->mid)
            return (true);
    }

    char mchar1 = me1->showchar;
    char mchar2 = me2->showchar;

    // If both are demons, the smaller number is the nastier demon.
    if (isadigit(mchar1) && isadigit(mchar2))
        return (fleeing || mchar1 < mchar2);

    // &s are the evillest demons of all, well apart from Geryon, who really
    // profits from *not* pushing past beasts.
    if (mchar1 == '&' && isadigit(mchar2) && m1->type != MONS_GERYON)
        return (fleeing || m1->hit_dice > m2->hit_dice);

    // If they're the same holiness, monsters smart enough to use stairs can
    // push past monsters too stupid to use stairs (so that e.g. non-zombified
    // or spectral zombified undead can push past non-spectral zombified
    // undead).
    if (m1->holiness() == m2->holiness() && mons_can_use_stairs(m1)
        && !mons_can_use_stairs(m2))
    {
        return (true);
    }

    if (mons_genus(m1->type) == MONS_KILLER_BEE
        && mons_genus(m2->type) == MONS_KILLER_BEE)
    {
        if (fleeing)
            return (true);

        if (m1->type == MONS_QUEEN_BEE && m2->type != MONS_QUEEN_BEE)
            return (true);

        if (m1->type == MONS_KILLER_BEE && m2->type == MONS_KILLER_BEE_LARVA)
            return (true);
    }

    // Special-case gnolls, so they can't get past (hob)goblins.
    if (m1->type == MONS_GNOLL && m2->type != MONS_GNOLL)
        return (false);

    return (mchar1 == mchar2 && (fleeing || m1->hit_dice > m2->hit_dice));
}

bool mons_class_can_pass(int mc, const dungeon_feature_type grid)
{
    if (mons_class_wall_shielded(mc))
    {
        // Permanent walls can't be passed through.
        return (!feat_is_solid(grid)
                || feat_is_rock(grid) && !feat_is_permarock(grid));
    }

    return (!feat_is_solid(grid));
}

static bool _mons_can_open_doors(const monster* mon)
{
    return (mons_itemuse(mon) >= MONUSE_OPEN_DOORS);
}

// Some functions that check whether a monster can open/eat/pass a
// given door. These all return false if there's no closed door there.

// Normal/smart monsters know about secret doors, since they live in
// the dungeon, unless they're marked specifically not to be opened unless
// already opened by the player {bookofjude}.
bool mons_can_open_door(const monster* mon, const coord_def& pos)
{
    if (env.markers.property_at(pos, MAT_ANY, "door_restrict") == "veto")
        return (false);

    if (!_mons_can_open_doors(mon))
        return (false);

    dungeon_feature_type feat = env.grid(pos);
    return (feat == DNGN_CLOSED_DOOR
            || feat_is_secret_door(feat) && mons_intel(mon) >= I_NORMAL);
}

// Monsters that eat items (currently only jellies) also eat doors.
// However, they don't realise that secret doors make good eating.
bool mons_can_eat_door(const monster* mon, const coord_def& pos)
{
    if (env.markers.property_at(pos, MAT_ANY, "door_restrict") == "veto")
        return (false);

    if (mons_itemeat(mon) != MONEAT_ITEMS)
        return (false);

    dungeon_feature_type feat = env.grid(pos);
    if (feat == DNGN_OPEN_DOOR)
        return (true);

    if (env.markers.property_at(pos, MAT_ANY, "door_restrict") == "veto"
         // Doors with permarock marker cannot be eaten.
        || feature_marker_at(pos, DNGN_PERMAROCK_WALL))
    {
        return (false);
    }

    return (feat == DNGN_CLOSED_DOOR);
}

static bool _mons_can_pass_door(const monster* mon, const coord_def& pos)
{
    return (mon->can_pass_through_feat(DNGN_FLOOR)
            && (mons_can_open_door(mon, pos)
                || mons_can_eat_door(mon, pos)));
}

bool mons_can_traverse(const monster* mon, const coord_def& p,
                       bool checktraps)
{
    if (_mons_can_pass_door(mon, p))
        return (true);

    if (!mon->is_habitable(p))
        return (false);

    // Your friends only know about doors you know about, unless they feel
    // at home in this branch.
    if (grd(p) == DNGN_SECRET_DOOR && mon->friendly()
        && (mons_intel(mon) < I_NORMAL || !mons_is_native_in_branch(mon)))
    {
        return (false);
    }

    const trap_def* ptrap = find_trap(p);
    if (checktraps && ptrap)
    {
        const trap_type tt = ptrap->type;

        // Don't allow allies to pass over known (to them) Zot traps.
        if (tt == TRAP_ZOT
            && ptrap->is_known(mon)
            && mon->friendly())
        {
            return (false);
        }

        // Monsters cannot travel over teleport traps.
        if (!can_place_on_trap(mons_base_type(mon), tt))
            return (false);
    }

    return (true);
}

void mons_remove_from_grid(const monster* mon)
{
    const coord_def pos = mon->pos();
    if (map_bounds(pos) && mgrd(pos) == mon->mindex())
        mgrd(pos) = NON_MONSTER;
}

mon_inv_type equip_slot_to_mslot(equipment_type eq)
{
    switch (eq)
    {
    case EQ_WEAPON:      return MSLOT_WEAPON;
    case EQ_BODY_ARMOUR: return MSLOT_ARMOUR;
    case EQ_SHIELD:      return MSLOT_SHIELD;
    default: return (NUM_MONSTER_SLOTS);
    }
}

mon_inv_type item_to_mslot(const item_def &item)
{
    switch (item.base_type)
    {
    case OBJ_WEAPONS:
    case OBJ_STAVES:
        return MSLOT_WEAPON;
    case OBJ_MISSILES:
        return MSLOT_MISSILE;
    case OBJ_ARMOUR:
        return equip_slot_to_mslot(get_armour_slot(item));
    case OBJ_WANDS:
        return MSLOT_WAND;
    case OBJ_SCROLLS:
        return MSLOT_SCROLL;
    case OBJ_POTIONS:
        return MSLOT_POTION;
    case OBJ_MISCELLANY:
        return MSLOT_MISCELLANY;
    case OBJ_GOLD:
        return MSLOT_GOLD;
    default:
        return NUM_MONSTER_SLOTS;
    }
}

monster_type royal_jelly_ejectable_monster()
{
    return static_cast<monster_type>(
        random_choose(MONS_ACID_BLOB,
                      MONS_AZURE_JELLY,
                      MONS_DEATH_OOZE,
                      -1));
}

// Replaces @foe_god@ and @god_is@ with foe's god name.
//
// Atheists get "You"/"you", and worshippers of nameless gods get "Your
// god"/"your god".
static std::string _replace_god_name(god_type god, bool need_verb = false,
                                     bool capital = false)
{
    std::string result =
          ((god == GOD_NO_GOD)    ? (capital ? "You"      : "you") :
           (god == GOD_NAMELESS)  ? (capital ? "Your god" : "your god")
                                  : god_name(god, false));
    if (need_verb)
        result += (god == GOD_NO_GOD) ? " are" : " is";

    return (result);
}

static std::string _get_species_insult(const std::string &species,
                                       const std::string &type)
{
    std::string insult;
    std::string lookup;

    // Get species genus.
    if (!species.empty())
    {
        lookup  = "insult ";
        lookup += species;
        lookup += " ";
        lookup += type;

        insult  = getSpeakString(lowercase(lookup));
    }

    if (insult.empty()) // Species too specific?
    {
        lookup  = "insult general ";
        lookup += type;

        insult  = getSpeakString(lookup);
    }

    return (insult);
}

static std::string _pluralise_player_genus()
{
    std::string sp = species_name(you.species, true, false);
    if (player_genus(GENPC_ELVEN, you.species)
        || player_genus(GENPC_DWARVEN, you.species))
    {
        sp = sp.substr(0, sp.find("f"));
        sp += "ves";
    }
    else if (you.species != SP_DEMONSPAWN)
        sp += "s";

    return (sp);
}

// Replaces the "@foo@" strings in monster shout and monster speak
// definitions.
std::string do_mon_str_replacements(const std::string &in_msg,
                                    const monster* mons, int s_type)
{
    std::string msg = in_msg;

    const actor*    foe   = (mons->wont_attack()
                             && invalid_monster_index(mons->foe)) ?
                            &you : mons->get_foe();

    if (s_type < 0 || s_type >= NUM_LOUDNESS || s_type == NUM_SHOUTS)
        s_type = mons_shouts(mons->type);

    // FIXME: Handle player_genus in case it was not generalized to foe_genus.
    msg = replace_all(msg, "@player_genus@", species_name(you.species, true));
    msg = replace_all(msg, "@player_genus_plural@", _pluralise_player_genus());

    std::string foe_species;

    if (foe == NULL)
        ;
    else if (foe->atype() == ACT_PLAYER)
    {
        foe_species = species_name(you.species, true);

        msg = replace_all(msg, " @to_foe@", "");
        msg = replace_all(msg, " @at_foe@", "");

        msg = replace_all(msg, "@player_only@", "");
        msg = replace_all(msg, " @foe,@", ",");

        msg = replace_all(msg, "@player", "@foe");
        msg = replace_all(msg, "@Player", "@Foe");

        msg = replace_all(msg, "@foe_possessive@", "your");
        msg = replace_all(msg, "@foe@", "you");
        msg = replace_all(msg, "@Foe@", "You");

        msg = replace_all(msg, "@foe_name@", you.your_name);
        msg = replace_all(msg, "@foe_species@", species_name(you.species));
        msg = replace_all(msg, "@foe_genus@", foe_species);
        msg = replace_all(msg, "@foe_genus_plural@",
                          _pluralise_player_genus());
    }
    else
    {
        std::string foe_name;
        const monster* m_foe = foe->as_monster();
        if (you.can_see(foe) || crawl_state.game_is_arena())
        {
            if (m_foe->attitude == ATT_FRIENDLY
                && !mons_is_unique(m_foe->type)
                && !crawl_state.game_is_arena())
            {
                foe_name = foe->name(DESC_NOCAP_YOUR);
                const std::string::size_type pos = foe_name.find("'");
                if (pos != std::string::npos)
                    foe_name = foe_name.substr(0, pos);
            }
            else
                foe_name = foe->name(DESC_NOCAP_THE);
        }
        else
            foe_name = "something";

        std::string prep = "at";
        if (s_type == S_SILENT || s_type == S_SHOUT || s_type == S_NORMAL)
            prep = "to";
        msg = replace_all(msg, "@says@ @to_foe@", "@says@ " + prep + " @foe@");

        msg = replace_all(msg, " @to_foe@", " to @foe@");
        msg = replace_all(msg, " @at_foe@", " at @foe@");
        msg = replace_all(msg, "@foe,@",    "@foe@,");

        msg = replace_all(msg, "@foe_possessive@", "@foe@'s");
        msg = replace_all(msg, "@foe@", foe_name);
        msg = replace_all(msg, "@Foe@", upcase_first(foe_name));

        if (m_foe->is_named())
            msg = replace_all(msg, "@foe_name@", foe->name(DESC_PLAIN, true));

        std::string species = mons_type_name(mons_species(m_foe->type),
                                             DESC_PLAIN);

        msg = replace_all(msg, "@foe_species@", species);

        std::string genus = mons_type_name(mons_genus(m_foe->type), DESC_PLAIN);

        msg = replace_all(msg, "@foe_genus@", genus);
        msg = replace_all(msg, "@foe_genus_plural@", pluralise(genus));

        foe_species = genus;
    }

    description_level_type nocap = DESC_NOCAP_THE, cap = DESC_CAP_THE;

    if (mons->is_named() && you.can_see(mons))
    {
        const std::string name = mons->name(DESC_CAP_THE);

        msg = replace_all(msg, "@the_something@", name);
        msg = replace_all(msg, "@The_something@", name);
        msg = replace_all(msg, "@the_monster@",   name);
        msg = replace_all(msg, "@The_monster@",   name);
    }
    else if (mons->attitude == ATT_FRIENDLY
             && !mons_is_unique(mons->type)
             && !crawl_state.game_is_arena()
             && you.can_see(mons))
    {
        nocap = DESC_PLAIN;
        cap   = DESC_PLAIN;

        msg = replace_all(msg, "@the_something@", "your @the_something@");
        msg = replace_all(msg, "@The_something@", "Your @The_something@");
        msg = replace_all(msg, "@the_monster@",   "your @the_monster@");
        msg = replace_all(msg, "@The_monster@",   "Your @the_monster@");
    }

    if (you.see_cell(mons->pos()))
    {
        dungeon_feature_type feat = grd(mons->pos());
        if (feat < DNGN_MINMOVE || feat >= NUM_FEATURES)
            msg = replace_all(msg, "@surface@", "buggy surface");
        else if (feat == DNGN_LAVA)
            msg = replace_all(msg, "@surface@", "lava");
        else if (feat_is_water(feat))
            msg = replace_all(msg, "@surface@", "water");
        else if (feat >= DNGN_ALTAR_FIRST_GOD && feat <= DNGN_ALTAR_LAST_GOD)
            msg = replace_all(msg, "@surface@", "altar");
        else
            msg = replace_all(msg, "@surface@", "ground");

        msg = replace_all(msg, "@feature@", raw_feature_description(feat));
    }
    else
    {
        msg = replace_all(msg, "@surface@", "buggy unseen surface");
        msg = replace_all(msg, "@feature@", "buggy unseen feature");
    }

    if (you.can_see(mons))
    {
        std::string something = mons->name(DESC_PLAIN);
        msg = replace_all(msg, "@something@",   something);
        msg = replace_all(msg, "@a_something@", mons->name(DESC_NOCAP_A));
        msg = replace_all(msg, "@the_something@", mons->name(nocap));

        something[0] = toupper(something[0]);
        msg = replace_all(msg, "@Something@",   something);
        msg = replace_all(msg, "@A_something@", mons->name(DESC_CAP_A));
        msg = replace_all(msg, "@The_something@", mons->name(cap));
    }
    else
    {
        msg = replace_all(msg, "@something@",     "something");
        msg = replace_all(msg, "@a_something@",   "something");
        msg = replace_all(msg, "@the_something@", "something");

        msg = replace_all(msg, "@Something@",     "Something");
        msg = replace_all(msg, "@A_something@",   "Something");
        msg = replace_all(msg, "@The_something@", "Something");
    }

    // Player name.
    msg = replace_all(msg, "@player_name@", you.your_name);

    std::string plain = mons->name(DESC_PLAIN);
    msg = replace_all(msg, "@monster@",     plain);
    msg = replace_all(msg, "@a_monster@",   mons->name(DESC_NOCAP_A));
    msg = replace_all(msg, "@the_monster@", mons->name(nocap));

    plain[0] = toupper(plain[0]);
    msg = replace_all(msg, "@Monster@",     plain);
    msg = replace_all(msg, "@A_monster@",   mons->name(DESC_CAP_A));
    msg = replace_all(msg, "@The_monster@", mons->name(cap));

    msg = replace_all(msg, "@Pronoun@",
                      mons->pronoun(PRONOUN_CAP));
    msg = replace_all(msg, "@pronoun@",
                      mons->pronoun(PRONOUN_NOCAP));
    msg = replace_all(msg, "@Possessive@",
                      mons->pronoun(PRONOUN_CAP_POSSESSIVE));
    msg = replace_all(msg, "@possessive@",
                      mons->pronoun(PRONOUN_NOCAP_POSSESSIVE));
    msg = replace_all(msg, "@objective@",
                      mons->pronoun(PRONOUN_OBJECTIVE));

    // Body parts.
    bool        can_plural = false;
    std::string part_str   = mons->hand_name(false, &can_plural);

    msg = replace_all(msg, "@hand@", part_str);
    msg = replace_all(msg, "@Hand@", upcase_first(part_str));

    if (!can_plural)
        part_str = "NO PLURAL HANDS";
    else
        part_str = mons->hand_name(true);

    msg = replace_all(msg, "@hands@", part_str);
    msg = replace_all(msg, "@Hands@", upcase_first(part_str));

    can_plural = false;
    part_str   = mons->arm_name(false, &can_plural);

    msg = replace_all(msg, "@arm@", part_str);
    msg = replace_all(msg, "@Arm@", upcase_first(part_str));

    if (!can_plural)
        part_str = "NO PLURAL ARMS";
    else
        part_str = mons->arm_name(true);

    msg = replace_all(msg, "@arms@", part_str);
    msg = replace_all(msg, "@Arms@", upcase_first(part_str));

    can_plural = false;
    part_str   = mons->foot_name(false, &can_plural);

    msg = replace_all(msg, "@foot@", part_str);
    msg = replace_all(msg, "@Foot@", upcase_first(part_str));

    if (!can_plural)
        part_str = "NO PLURAL FOOT";
    else
        part_str = mons->foot_name(true);

    msg = replace_all(msg, "@feet@", part_str);
    msg = replace_all(msg, "@Feet@", upcase_first(part_str));

    if (foe != NULL)
    {
        const god_type god = foe->deity();

        // Replace with "you are" for atheists.
        msg = replace_all(msg, "@god_is@",
                          _replace_god_name(god, true, false));
        msg = replace_all(msg, "@God_is@", _replace_god_name(god, true, true));

        // No verb needed.
        msg = replace_all(msg, "@foe_god@",
                          _replace_god_name(god, false, false));
        msg = replace_all(msg, "@foe_god@",
                          _replace_god_name(god, false, true));
    }

    // The monster's god, not the player's.  Atheists get
    // "NO GOD"/"NO GOD", and worshippers of nameless gods get
    // "a god"/"its god".
    if (mons->god == GOD_NO_GOD)
    {
        msg = replace_all(msg, "@God@", "NO GOD");
        msg = replace_all(msg, "@possessive_God@", "NO GOD");
    }
    else if (mons->god == GOD_NAMELESS)
    {
        msg = replace_all(msg, "@God@", "a god");
        std::string possessive = mons->pronoun(PRONOUN_NOCAP_POSSESSIVE);
        possessive += " god";
        msg = replace_all(msg, "@possessive_God@", possessive.c_str());
    }
    else
    {
        msg = replace_all(msg, "@God@", god_name(mons->god));
        msg = replace_all(msg, "@possessive_God@", god_name(mons->god));
    }

    // Replace with species specific insults.
    if (msg.find("@species_insult_") != std::string::npos)
    {
        msg = replace_all(msg, "@species_insult_adj1@",
                               _get_species_insult(foe_species, "adj1"));
        msg = replace_all(msg, "@species_insult_adj2@",
                               _get_species_insult(foe_species, "adj2"));
        msg = replace_all(msg, "@species_insult_noun@",
                               _get_species_insult(foe_species, "noun"));
    }

    static const char * sound_list[] =
    {
        "says",         // actually S_SILENT
        "shouts",
        "barks",
        "shouts",
        "roars",
        "screams",
        "bellows",
        "trumpets",
        "screeches",
        "buzzes",
        "moans",
        "gurgles",
        "whines",
        "croaks",
        "growls",
        "hisses",
        "sneers",       // S_DEMON_TAUNT
        "caws",
        "buggily says", // NUM_SHOUTS
        "breathes",     // S_VERY_SOFT
        "whispers",     // S_SOFT
        "says",         // S_NORMAL
        "shouts",       // S_LOUD
        "screams"       // S_VERY_LOUD
    };
    COMPILE_CHECK(ARRAYSZ(sound_list) == NUM_LOUDNESS, shout_list);

    if (s_type < 0 || s_type >= NUM_LOUDNESS || s_type == NUM_SHOUTS)
    {
        mpr("Invalid @says@ type.", MSGCH_DIAGNOSTICS);
        msg = replace_all(msg, "@says@", "buggily says");
    }
    else
        msg = replace_all(msg, "@says@", sound_list[s_type]);

    msg = apostrophise_fixup(msg);

    return (msg);
}

static mon_body_shape _get_ghost_shape(const monster* mon)
{
    const ghost_demon &ghost = *(mon->ghost);

    switch (ghost.species)
    {
    case SP_NAGA:
        return (MON_SHAPE_NAGA);

    case SP_CENTAUR:
        return (MON_SHAPE_CENTAUR);

    case SP_KENKU:
        return (MON_SHAPE_HUMANOID_WINGED);

    case SP_RED_DRACONIAN:
    case SP_WHITE_DRACONIAN:
    case SP_GREEN_DRACONIAN:
    case SP_YELLOW_DRACONIAN:
    case SP_GREY_DRACONIAN:
    case SP_BLACK_DRACONIAN:
    case SP_PURPLE_DRACONIAN:
    case SP_MOTTLED_DRACONIAN:
    case SP_PALE_DRACONIAN:
    case SP_BASE_DRACONIAN:
        return (MON_SHAPE_HUMANOID_TAILED);

    default:
        return (MON_SHAPE_HUMANOID);
    }
}

mon_body_shape get_mon_shape(const monster* mon)
{
    if (mons_is_pghost(mon->type))
        return _get_ghost_shape(mon);
    else if (mons_is_zombified(mon))
        return get_mon_shape(mon->base_monster);
    else
        return get_mon_shape(mon->type);
}

mon_body_shape get_mon_shape(const int type)
{
    if (type == MONS_CHAOS_SPAWN)
        return (static_cast<mon_body_shape>(random2(MON_SHAPE_MISC + 1)));

    const bool flies = (mons_class_flies(type) == FL_FLY);

    switch (mons_base_char(type))
    {
    case 'a': // ants
        return (MON_SHAPE_INSECT);
    case 'b':  // bats and butterflies
        if (type == MONS_BUTTERFLY)
            return (MON_SHAPE_INSECT_WINGED);
        else
            return (MON_SHAPE_BAT);
    case 'c': // centaurs
        return (MON_SHAPE_CENTAUR);
    case 'd': // draconions and drakes
        if (mons_genus(type) == MONS_DRACONIAN)
        {
            if (flies)
                return (MON_SHAPE_HUMANOID_WINGED_TAILED);
            else
                return (MON_SHAPE_HUMANOID_TAILED);
        }
        else if (flies)
            return (MON_SHAPE_QUADRUPED_WINGED);
        else
            return (MON_SHAPE_QUADRUPED);
    case 'e': // elves
        return (MON_SHAPE_HUMANOID);
    case 'f': // fungi
        return (MON_SHAPE_FUNGUS);
    case 'g': // gargoyles, gnolls, goblins and hobgoblins
        if (type == MONS_GARGOYLE)
            return (MON_SHAPE_HUMANOID_WINGED_TAILED);
        else
            return (MON_SHAPE_HUMANOID);
    case 'h': // hounds
        return (MON_SHAPE_QUADRUPED);
    case 'i': // spriggans
        return (MON_SHAPE_HUMANOID);
    case 'j': // snails
        return (MON_SHAPE_SNAIL);
    case 'k': // killer bees
        return (MON_SHAPE_INSECT_WINGED);
    case 'l': // lizards
        return (MON_SHAPE_QUADRUPED);
    case 'm': // merfolk
        return (MON_SHAPE_HUMANOID);
    case 'n': // necrophages and ghouls
        return (MON_SHAPE_HUMANOID);
    case 'o': // orcs
        return (MON_SHAPE_HUMANOID);
    case 'p': // ghosts
        if (type != MONS_INSUBSTANTIAL_WISP
            && !mons_is_pghost(type)) // handled elsewhere
        {
            return (MON_SHAPE_HUMANOID);
        }
        break;
    case 'q': // dwarves
        return (MON_SHAPE_HUMANOID);
    case 'r': // rodents
        return (MON_SHAPE_QUADRUPED);
    case 's': // arachnids and centipedes/cockroaches
        if (type == MONS_GIANT_CENTIPEDE || type == MONS_DEMONIC_CRAWLER)
            return (MON_SHAPE_CENTIPEDE);
        else if (type == MONS_GIANT_COCKROACH)
            return (MON_SHAPE_INSECT);
        else
            return (MON_SHAPE_ARACHNID);
    case 'u': // mutated type, not enough info to determine shape
        return (MON_SHAPE_MISC);
    case 't': // crocodiles/turtles
        if (type == MONS_SNAPPING_TURTLE
            || type == MONS_ALLIGATOR_SNAPPING_TURTLE)
        {
            return (MON_SHAPE_QUADRUPED_TAILLESS);
        }
        return (MON_SHAPE_QUADRUPED);
    case 'v': // vortices
        return (MON_SHAPE_MISC);
    case 'w': // worms
        return (MON_SHAPE_SNAKE);
    case 'x': // small abominations
        return (MON_SHAPE_MISC);
    case 'y': // winged insects
        return (MON_SHAPE_INSECT_WINGED);
    case 'z': // small zombies, etc.
        if (type == MONS_WIGHT
            || type == MONS_SKELETAL_WARRIOR
            || type == MONS_FLAMING_CORPSE)
        {
            return (MON_SHAPE_HUMANOID);
        }
        else
        {
            // constructed type, not enough info to determine shape
            return (MON_SHAPE_MISC);
        }
    case 'A': // angelic beings
        return (MON_SHAPE_HUMANOID_WINGED);
    case 'B': // beetles
        return (MON_SHAPE_INSECT);
    case 'C': // giants
        return (MON_SHAPE_HUMANOID);
    case 'D': // dragons
        if (flies)
            return (MON_SHAPE_QUADRUPED_WINGED);
        else
            return (MON_SHAPE_QUADRUPED);
    case 'E': // elementals
        return (MON_SHAPE_MISC);
    case 'F': // frogs
        return (MON_SHAPE_QUADRUPED_TAILLESS);
    case 'G': // floating eyeballs and orbs
        return (MON_SHAPE_ORB);
    case 'H': // minotaurs, manticores, hippogriffs and griffins
        if (type == MONS_MINOTAUR)
            return (MON_SHAPE_HUMANOID);
        else if (type == MONS_MANTICORE)
            return (MON_SHAPE_QUADRUPED);
        else
            return (MON_SHAPE_QUADRUPED_WINGED);
    case 'I': // ice beasts
        return (MON_SHAPE_QUADRUPED);
    case 'J': // jellies and jellyfish
        return (MON_SHAPE_BLOB);
    case 'K': // kobolds
        return (MON_SHAPE_HUMANOID);
    case 'L': // liches
        return (MON_SHAPE_HUMANOID);
    case 'M': // mummies
        return (MON_SHAPE_HUMANOID);
    case 'N': // nagas
        if (mons_genus(type) == MONS_GUARDIAN_SERPENT)
            return (MON_SHAPE_SNAKE);
        else
            return (MON_SHAPE_NAGA);
    case 'O': // ogres
        return (MON_SHAPE_HUMANOID);
    case 'P': // plants
        return (MON_SHAPE_PLANT);
    case 'R': // rakshasa and efreeti; humanoid?
        return (MON_SHAPE_HUMANOID);
    case 'S': // snakes
        return (MON_SHAPE_SNAKE);
    case 'T': // trolls
        return (MON_SHAPE_HUMANOID);
    case 'U': // bears
        return (MON_SHAPE_QUADRUPED_TAILLESS);
    case 'V': // vampires
        return (MON_SHAPE_HUMANOID);
    case 'W': // wraiths, humanoid if not a spectral thing
        if (type == MONS_SPECTRAL_THING)
            // constructed type, not enough info to determine shape
            return (MON_SHAPE_MISC);
        else
            return (MON_SHAPE_HUMANOID);
    case 'X': // large abominations
        return (MON_SHAPE_MISC);
    case 'Y': // yaks, sheep and elephants
        if (type == MONS_SHEEP)
            return (MON_SHAPE_QUADRUPED_TAILLESS);
        else
            return (MON_SHAPE_QUADRUPED);
    case 'Z': // large zombies, etc.
        // constructed type, not enough info to determine shape
        return (MON_SHAPE_MISC);
    case ';': // water monsters
        if (type == MONS_ELECTRIC_EEL)
            return (MON_SHAPE_SNAKE);
        else
            return (MON_SHAPE_FISH);

    // The various demons, plus some golems and statues.  And humanoids.
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '&':
    case '8':
    case '@':
        // Assume that a demon has wings if it can fly, and that it has
        // a tail if it has a sting or tail-slap attack.
        monsterentry *mon_data = get_monster_data(type);
        bool tailed = false;
        for (int i = 0; i < 4; ++i)
            if (mon_data->attack[i].type == AT_STING
                || mon_data->attack[i].type == AT_TAIL_SLAP)
            {
                tailed = true;
                break;
            }

        if (flies && tailed)
            return (MON_SHAPE_HUMANOID_WINGED_TAILED);
        else if (flies && !tailed)
            return (MON_SHAPE_HUMANOID_WINGED);
        else if (!flies && tailed)
            return (MON_SHAPE_HUMANOID_TAILED);
        else
            return (MON_SHAPE_HUMANOID);
    }

    return (MON_SHAPE_MISC);
}

std::string get_mon_shape_str(const monster* mon)
{
    return get_mon_shape_str(get_mon_shape(mon));
}

std::string get_mon_shape_str(const int type)
{
    return get_mon_shape_str(get_mon_shape(type));
}

std::string get_mon_shape_str(const mon_body_shape shape)
{
    ASSERT(shape >= MON_SHAPE_HUMANOID && shape <= MON_SHAPE_MISC);

    if (shape < MON_SHAPE_HUMANOID || shape > MON_SHAPE_MISC)
        return "buggy shape";

    static const char *shape_names[] =
    {
        "humanoid", "winged humanoid", "tailed humanoid",
        "winged tailed humanoid", "centaur", "naga",
        "quadruped", "tailless quadruped", "winged quadruped",
        "bat", "snake", "fish",  "insect", "winged insect",
        "arachnid", "centipede", "snail", "plant", "fungus", "orb",
        "blob", "misc"
    };

    return (shape_names[shape]);
}

bool player_or_mon_in_sanct(const monster* mons)
{
    return (is_sanctuary(you.pos())
            || is_sanctuary(mons->pos()));
}

bool mons_landlubbers_in_reach(const monster* mons)
{
    if (mons_has_ranged_spell(mons)
        || mons_has_ranged_ability(mons)
        || mons_has_ranged_attack(mons))
    {
        return (true);
    }

    const reach_type range = mons->reach_range();
    actor *act;
    for (radius_iterator ai(mons->pos(),
                            range ? 2 : 1,
                            (range == REACH_KNIGHT) ? C_ROUND : C_SQUARE,
                            NULL,
                            true);
                         ai; ++ai)
        if ((act = actor_at(*ai)) && !mons_aligned(mons, act))
            return (true);

    return (false);
}

int get_dist_to_nearest_monster()
{
    int minRange = LOS_RADIUS_SQ + 1;
    for (radius_iterator ri(you.get_los_no_trans(), true); ri; ++ri)
    {
        const monster* mon = monster_at(*ri);
        if (mon == NULL)
            continue;

        if (!mon->visible_to(&you) || mons_is_unknown_mimic(mon))
            continue;

        // Plants/fungi don't count.
        if (mons_class_flag(mon->type, M_NO_EXP_GAIN)
            && (mon->type != MONS_BALLISTOMYCETE || mon->number == 0))
        {
            continue;
        }

        if (mon->wont_attack())
            continue;

        int dist = distance(you.pos(), *ri);
        if (dist < minRange)
            minRange = dist;
    }
    return (minRange);
}

actor *actor_by_mid(mid_t m)
{
    if (m == MID_PLAYER)
        return &you;
    return monster_by_mid(m);
}

monster *monster_by_mid(mid_t m)
{
    if (m == MID_ANON_FRIEND)
        return &menv[ANON_FRIENDLY_MONSTER];
    for (int i = 0; i < MAX_MONSTERS; i++)
        if (menv[i].mid == m && menv[i].alive())
            return &menv[i];
    return 0;
}

bool mons_is_tentacle(int mc)
{
    return (mc == MONS_KRAKEN_TENTACLE
            || mc == MONS_KRAKEN_TENTACLE_SEGMENT
            || mc == MONS_ELDRITCH_TENTACLE
            || mc == MONS_ELDRITCH_TENTACLE_SEGMENT);
}

bool mons_is_tentacle_segment(int mc)
{
    return (mc == MONS_KRAKEN_TENTACLE_SEGMENT
            || mc == MONS_ELDRITCH_TENTACLE_SEGMENT);
}

void init_anon()
{
    monster &mon = menv[ANON_FRIENDLY_MONSTER];
    mon.reset();
    mon.type = MONS_PROGRAM_BUG;
    mon.mid = MID_ANON_FRIEND;
    mon.attitude = ATT_FRIENDLY;
    mon.hit_points = mon.max_hit_points = 1000;
}

actor *find_agent(mid_t m, kill_category kc)
{
    actor *agent = actor_by_mid(m);
    if (agent)
        return agent;
    switch (kc)
    {
    case KC_YOU:
        // shouldn't happen, there ought to be a valid mid
        return &you;
    case KC_FRIENDLY:
        return &menv[ANON_FRIENDLY_MONSTER];
    case KC_OTHER:
        // currently hostile dead/gone monsters are no different from env
        return 0;
    case KC_NCATEGORIES:
    default:
        die("invalid kill category");
    }
}

const char* mons_class_name(monster_type mc)
{
    // Usually, all invalids return "program bug", but since it has value of 0,
    // it's good to tell them apart in error messages.
    if (invalid_monster_type(mc) && mc != MONS_PROGRAM_BUG)
        return "INVALID";

    return get_monster_data(mc)->name;
}

bool mons_is_tentacle_end(const int mtype)
{
    return (mtype == MONS_KRAKEN_TENTACLE
            || mtype == MONS_ELDRITCH_TENTACLE);
}
