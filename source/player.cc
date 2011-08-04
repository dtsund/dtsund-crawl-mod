/**
 * @file
 * @brief Player related functions.
**/

#include "AppHdr.h"

#include "player.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include <sstream>
#include <algorithm>

#include "areas.h"
#include "artefact.h"
#include "branch.h"
#include "cloud.h"
#include "clua.h"
#include "coord.h"
#include "coordit.h"
#include "delay.h"
#include "dgnevent.h"
#include "directn.h"
#include "effects.h"
#include "env.h"
#include "errors.h"
#include "exercise.h"
#include "fight.h"
#include "godabil.h"
#include "godconduct.h"
#include "godpassive.h"
#include "godwrath.h"
#include "hints.h"
#include "hiscores.h"
#include "item_use.h"
#include "itemname.h"
#include "itemprop.h"
#include "items.h"
#include "kills.h"
#include "libutil.h"
#include "macro.h"
#include "map_knowledge.h"
#include "message.h"
#include "misc.h"
#include "mon-util.h"
#include "mutation.h"
#include "notes.h"
#include "options.h"
#include "ouch.h"
#include "output.h"
#include "place.h"
#include "player-stats.h"
#include "potion.h"
#include "quiver.h"
#include "random.h"
#include "religion.h"
#include "shopping.h"
#include "shout.h"
#include "skills.h"
#include "skills2.h"
#include "species.h"
#include "spl-other.h"
#include "spl-selfench.h"
#include "spl-transloc.h"
#include "spl-util.h"
#include "sprint.h"
#include "stairs.h"
#include "state.h"
#include "status.h"
#include "stuff.h"
#include "tagstring.h"
#include "terrain.h"
#ifdef USE_TILE
 #include "tileview.h"
#endif
#include "transform.h"
#include "traps.h"
#include "travel.h"
#include "view.h"
#include "viewgeom.h"
#include "xom.h"

static void _moveto_maybe_repel_stairs()
{
    const dungeon_feature_type new_grid = env.grid(you.pos());
    const command_type stair_dir = feat_stair_direction(new_grid);

    if (stair_dir == CMD_NO_CMD
        || new_grid == DNGN_ENTER_SHOP
        ||  !you.duration[DUR_REPEL_STAIRS_MOVE])
    {
        return;
    }

    int pct = you.duration[DUR_REPEL_STAIRS_CLIMB] ? 29 : 50;

    // When the effect is still strong, the chance to actually catch
    // a stair is smaller. (Assuming the duration starts out at 1000.)
    const int dur = std::max(0, you.duration[DUR_REPEL_STAIRS_MOVE] - 700);
    pct += dur/10;

    if (x_chance_in_y(pct, 100))
    {
        if (slide_feature_over(you.pos(), coord_def(-1, -1), false))
        {
            std::string stair_str =
                feature_description(new_grid, NUM_TRAPS, "",
                                    DESC_CAP_THE, false);
            std::string prep = feat_preposition(new_grid, true, &you);

            mprf("%s slides away as you move %s it!", stair_str.c_str(),
                 prep.c_str());

            if (player_in_a_dangerous_place() && one_chance_in(5))
                xom_is_stimulated(32);
        }
    }
}

static bool _check_moveto_cloud(const coord_def& p,
                                const std::string &move_verb)
{
    const int cloud = env.cgrid(p);
    if (cloud != EMPTY_CLOUD && !you.confused())
    {
        const cloud_type ctype = env.cloud[ cloud ].type;
        // Don't prompt if already in a cloud of the same type.
        if (is_damaging_cloud(ctype, true)
            && (env.cgrid(you.pos()) == EMPTY_CLOUD
                || ctype != env.cloud[ env.cgrid(you.pos()) ].type))
        {
            std::string prompt = make_stringf(
                                    "Really %s into that cloud of %s?",
                                    move_verb.c_str(),
                                    cloud_name_at_index(cloud).c_str());
            learned_something_new(HINT_CLOUD_WARNING);

            if (!yesno(prompt.c_str(), false, 'n'))
            {
                canned_msg(MSG_OK);
                return (false);
            }
        }
    }
    return (true);
}

static bool _check_moveto_trap(const coord_def& p, const std::string &move_verb)
{
    // If we're walking along, give a chance to avoid traps.
    const dungeon_feature_type new_grid = env.grid(p);
    if (new_grid == DNGN_UNDISCOVERED_TRAP)
    {
        const int skill =
            (4 + you.traps_skill()
             + player_mutation_level(MUT_ACUTE_VISION)
             - 2 * player_mutation_level(MUT_BLURRY_VISION));

        if (random2(skill) > 6)
        {
            if (trap_def* ptrap = find_trap(p))
                ptrap->reveal();

            viewwindow();

            mprf(MSGCH_WARN,
                 "Wait a moment, %s! Do you really want to %s there?",
                 you.your_name.c_str(),
                 move_verb.c_str());

            if (!you.running.is_any_travel())
                more();

            practise(EX_TRAP_PASSIVE);
            print_stats();
            return (false);
        }
    }
    else if (new_grid == DNGN_TRAP_MAGICAL
#ifdef CLUA_BINDINGS
             || new_grid == DNGN_TRAP_MECHANICAL
                && Options.trap_prompt
#endif
             || new_grid == DNGN_TRAP_NATURAL)
    {
        const trap_type type = get_trap_type(p);
        if (type == TRAP_ZOT)
        {
            std::string prompt = make_stringf(
                "Do you really want to %s into the Zot trap",
                move_verb.c_str()
          );
            if (!yes_or_no(prompt.c_str()))
            {
                canned_msg(MSG_OK);
                return (false);
            }
        }
        else if (new_grid != DNGN_TRAP_MAGICAL &&
                 (you.airborne() || you.can_cling_to(p)))
        {
            // No prompt (shaft and mechanical traps
            // ineffective, if flying)
        }
        else if (type == TRAP_TELEPORT
                 && (player_equip(EQ_AMULET, AMU_STASIS, true)
                     || scan_artefacts(ARTP_PREVENT_TELEPORTATION,
                                       false)))
        {
            // No prompt (teleport traps are ineffective if
            // wearing an amulet of stasis)
        }
        else
#ifdef CLUA_BINDINGS
             // Prompt for any trap where you might not have enough hp
             // as defined in init.txt (see trapwalk.lua)
             if (type != TRAP_SHAFT // Known shafts aren't traps
                 && type != TRAP_GOLUBRIA // don't need a warning here
                 && (new_grid != DNGN_TRAP_MECHANICAL
                 || !clua.callbooleanfn(false, "ch_cross_trap",
                                        "s", trap_name_at(p)))
                && !crawl_state.game_is_zotdef())
#endif
        {
            std::string prompt = make_stringf(
                "Really %s %s that %s?",
                move_verb.c_str(),
                (type == TRAP_ALARM || type == TRAP_PLATE) ? "onto" : "into",
                feature_description(new_grid, type,
                                    "", DESC_BASENAME,
                                    false).c_str());

            if (!yesno(prompt.c_str(), true, 'n'))
            {
                canned_msg(MSG_OK);
                return (false);
            }
        }
    }
    return (true);
}

static bool _check_moveto_dangerous(const coord_def& p)
{
    if (you.can_swim() && feat_is_water(env.grid(p))
        || you.airborne() || you.can_cling_to(p)
        || !is_feat_dangerous(env.grid(p)))
    {
        return (true);
    }
    
    if (you.species == SP_MERFOLK && feat_is_water(env.grid(p)))
        mpr("You cannot swim in your current form.");
    else
        canned_msg(MSG_UNTHINKING_ACT);
    return (false);
}

static bool _check_moveto_terrain(const coord_def& p,
                                  const std::string &move_verb)
{
    if (you.is_wall_clinging() && move_verb == "blink")
        return (_check_moveto_dangerous(p));

    if (!need_expiration_warning() && need_expiration_warning(p))
    {
        if (!_check_moveto_dangerous(p))
            return false;

        std::string prompt = "Are you sure you want to " + move_verb;

        if (you.ground_level())
            prompt += " into ";
        else
            prompt += " over ";

        prompt += env.grid(p) == DNGN_DEEP_WATER ? "deep water" : "lava";

        prompt += need_expiration_warning(DUR_LEVITATION, p)
                      ? " while you are losing your buoyancy?"
                      : " while your transformation is expiring?";

        if (!yesno(prompt.c_str(), false, 'n'))
        {
            canned_msg(MSG_OK);
            return false;
        }
    }

    return _check_moveto_dangerous(p);
}

bool check_moveto(const coord_def& p, const std::string &move_verb)
{
    return (_check_moveto_terrain(p, move_verb)
            && _check_moveto_cloud(p, move_verb)
            && _check_moveto_trap(p, move_verb));
}

static void _splash()
{
    if (you.can_swim())
        noisy(4, you.pos(), "Floosh!");
    else if (!beogh_water_walk())
        noisy(8, you.pos(), "Splash!");
}

void moveto_location_effects(dungeon_feature_type old_feat,
                             bool stepped, bool allow_shift,
                             const coord_def& old_pos)
{
    const dungeon_feature_type new_grid = env.grid(you.pos());

    // Terrain effects.
    if (is_feat_dangerous(new_grid) && !you.is_wall_clinging())
    {
        // Lava and dangerous deep water (ie not merfolk).
        const coord_def& entry = (stepped) ? old_pos : you.pos();

        // If true, we were shifted and so we're done.
        if (fall_into_a_pool(entry, allow_shift, new_grid))
            return;
    }

    if (you.ground_level())
    {
        if (you.species == SP_MERFOLK)
        {
            if (feat_is_water(new_grid) // We're entering water
                // We're not transformed, or with a form compatible with tail
                && (you.form == TRAN_NONE
                    || you.form == TRAN_BLADE_HANDS))
            {
                merfolk_start_swimming(stepped);
            }
            else if (!feat_is_water(new_grid) && !is_feat_dangerous(new_grid))
                merfolk_stop_swimming();
        }

        if (feat_is_water(new_grid) && !stepped)
            _splash();

        if (feat_is_water(new_grid) && !you.can_swim() && !beogh_water_walk())
        {
            you.time_taken *= 13 + random2(8);
            you.time_taken /= 10;
            const bool will_cling = you.can_cling_to_walls()
                                    && cell_is_clingable(you.pos());

            if (!feat_is_water(old_feat))
            {
                if (stepped && will_cling)
                {
                    mpr("You slowly cross the shallow water and cling to the "
                        "wall.");
                }
                else
                {
                    mprf("You %s the %s water.",
                         stepped ? "enter" : "fall into",
                         new_grid == DNGN_SHALLOW_WATER ? "shallow" : "deep");
                }
            }

            if (new_grid == DNGN_DEEP_WATER && old_feat != DNGN_DEEP_WATER)
                mpr("You sink to the bottom.");

            if (!feat_is_water(old_feat) && !will_cling)
            {
                mpr("Moving in this stuff is going to be slow.");
                if (you.invisible())
                    mpr("...and don't expect to remain undetected.");
            }
        }
    }

    const bool was_clinging = you.is_wall_clinging();
    const bool is_clinging = stepped && you.check_clinging(stepped);

    if (feat_is_water(new_grid) && was_clinging && !is_clinging)
        _splash();

    // Icy shield goes down over lava.
    if (new_grid == DNGN_LAVA)
        expose_player_to_element(BEAM_LAVA);

    // Traps go off.
    if (trap_def* ptrap = find_trap(you.pos()))
        ptrap->trigger(you, !stepped); // blinking makes it hard to evade

    if (stepped)
        _moveto_maybe_repel_stairs();
}

// Use this function whenever the player enters (or lands and thus re-enters)
// a grid.
//
// stepped     - normal walking moves
// allow_shift - allowed to scramble in any direction out of lava/water
void move_player_to_grid(const coord_def& p, bool stepped, bool allow_shift)
{
    ASSERT(!crawl_state.game_is_arena());
    ASSERT(in_bounds(p));

    if (!stepped)
        you.clear_clinging();

    // assuming that entering the same square means coming from above (levitate)
    const coord_def old_pos = you.pos();
    const bool from_above = (old_pos == p);
    const dungeon_feature_type old_grid =
        (from_above) ? DNGN_FLOOR : grd(old_pos);

    // Really must be clear.
    ASSERT(you.can_pass_through_feat(grd(p)));

    // Better not be an unsubmerged monster either.
    ASSERT(!monster_at(p) || monster_at(p)->submerged()
           || fedhas_passthrough(monster_at(p)));

    // Move the player to new location.
    you.moveto(p);
    viewwindow();

    // Checking new squares for interesting features.
    if (!you.running)
        check_for_interesting_features();

    moveto_location_effects(old_grid, stepped, allow_shift, old_pos);
}

bool is_feat_dangerous(dungeon_feature_type grid, bool permanently)
{
    if (you.permanent_flight() || you.permanent_levitation()
        || you.airborne() && !permanently)
    {
        return false;
    }
    else if (grid == DNGN_DEEP_WATER && !player_likes_water(permanently)
             || grid == DNGN_LAVA)
    {
        return true;
    }
    else
        return false;
}

bool player_in_mappable_area(void)
{
    return (!testbits(env.level_flags, LFLAG_NOT_MAPPABLE)
            && !testbits(get_branch_flags(), BFLAG_NOT_MAPPABLE));
}

bool player_in_branch(int branch)
{
    return (you.level_type == LEVEL_DUNGEON && you.where_are_you == branch);
}

bool player_in_level_area(level_area_type area)
{
    return (you.level_type == area);
}

bool player_in_hell(void)
{
    return (you.level_type == LEVEL_DUNGEON
            && is_hell_subbranch(you.where_are_you));
}

bool player_likes_water(bool permanently)
{
    return (!permanently && beogh_water_walk()
            || (species_likes_water(you.species) || !permanently)
                && form_likes_water());
}

bool player_in_bat_form()
{
    return (you.form == TRAN_BAT);
}

bool player_can_open_doors()
{
    // Bats and pigs can't open/close doors.
    return (you.form != TRAN_BAT
            && you.form != TRAN_PIG);
}

bool player_under_penance(void)
{
    if (you.religion != GOD_NO_GOD)
        return (you.penance[you.religion]);
    else
        return (false);
}

// TODO: get rid of this.
bool player_genus(genus_type which_genus, species_type species)
{
    if (species == SP_UNKNOWN)
        species = you.species;

    return (species_genus(species) == which_genus);
}

// If transform is true, compare with current transformation instead
// of (or in addition to) underlying species.
// (See mon-data.h for species/genus use.)
bool is_player_same_species(const int mon, bool transform)
{
    if (transform)
    {
        switch (you.form)
        {
        // Unique monsters.
        case TRAN_BAT:
            return (mon == MONS_MEGABAT);
        case TRAN_ICE_BEAST:
            return (mon == MONS_ICE_BEAST);
        // Compare with monster *species*.
        case TRAN_LICH:
            return (mons_species(mon) == MONS_LICH);
        // Compare with monster *genus*.
        case TRAN_SPIDER:
            return (mons_genus(mon) == MONS_SPIDER);
        case TRAN_DRAGON:
            return (mons_genus(mon) == MONS_DRAGON); // Includes all drakes.
        case TRAN_PIG:
            return (mons_genus(mon) == MONS_HOG);
        case TRAN_STATUE:
        case TRAN_BLADE_HANDS:
        case TRAN_NONE:
            break; // Check real (non-transformed) form.
        }
    }

    // Genus would include necrophage and rotting hulk.
    if (you.species == SP_GHOUL)
        return (mons_species(mon) == MONS_GHOUL);

    if (you.species == SP_MERFOLK && mons_genus(mon) == MONS_MERMAID)
        return (true);

    // Note that these are currently considered to be the same species:
    // * halflings and humans
    // * dwarves and deep dwarves
    // * all elf races
    return (mons_genus(mon) == mons_genus(player_mons(false)));
}

void update_player_symbol()
{
    you.symbol = Options.show_player_species ? player_mons() : transform_mons();
}

monster_type player_mons(bool transform)
{
    monster_type mons;

    if (transform)
    {
        mons = transform_mons();
        if (mons != MONS_PLAYER)
            return (mons);
    }

    mons = player_species_to_mons_species(you.species);

    if (mons == MONS_ORC)
    {
        if (you.religion == GOD_BEOGH)
            mons = (you.piety >= piety_breakpoint(4)) ? MONS_ORC_HIGH_PRIEST
                                                      : MONS_ORC_PRIEST;
    }
    else if (mons == MONS_OGRE)
    {
        const skill_type sk = best_skill(SK_FIRST_SKILL, SK_LAST_SKILL);
        if (sk >= SK_SPELLCASTING && sk < SK_INVOCATIONS)
            mons = MONS_OGRE_MAGE;
    }

    return (mons);
}

void update_vision_range()
{
    you.normal_vision = LOS_RADIUS;
    int nom   = 1;
    int denom = 1;

    // Nightstalker gives -1/-2/-3.
    if (player_mutation_level(MUT_NIGHTSTALKER))
    {
        nom *= LOS_RADIUS - player_mutation_level(MUT_NIGHTSTALKER);
        denom *= LOS_RADIUS;
    }

    // Lantern of shadows.
    if (you.attribute[ATTR_SHADOWS])
        nom *= 3, denom *= 4;

    // the Darkness spell.
    if (you.duration[DUR_DARKNESS])
        nom *= 3, denom *= 4;

    you.current_vision = (you.normal_vision * nom + denom / 2) / denom;
    ASSERT(you.current_vision > 0);
    set_los_radius(you.current_vision);
}

// Checks whether the player's current species can use
// use (usually wear) a given piece of equipment.
// Note that EQ_BODY_ARMOUR and EQ_HELMET only check
// the ill-fitting variant (i.e., not caps and robes).
// If special_armour is set to true, special cases
// such as bardings, light armour and caps are
// considered. Otherwise, these simply return false.
// ---------------------------------------------------
bool you_can_wear(int eq, bool special_armour)
{
    if (you.species == SP_CAT)
        return (eq == EQ_LEFT_RING || eq == EQ_RIGHT_RING || eq == EQ_AMULET);

    switch (eq)
    {
    case EQ_LEFT_RING:
    case EQ_RIGHT_RING:
    case EQ_AMULET:
    case EQ_CLOAK:
        return (true);

    case EQ_GLOVES:
        if (you.species == SP_TROLL
            || you.species == SP_SPRIGGAN
            || player_genus(GENPC_OGREISH)
            || player_genus(GENPC_DRACONIAN))
        {
            return (false);
        }
        if (player_mutation_level(MUT_CLAWS) >= 3)
            return (false);
        return (true);

    case EQ_BOOTS:
        // Bardings.
        if (you.species == SP_NAGA || you.species == SP_CENTAUR)
            return (special_armour);
        if (player_mutation_level(MUT_HOOVES) >= 3
            || player_mutation_level(MUT_TALONS) >= 3)
        {
            return (false);
        }
        // These species cannot wear boots.
        if (you.species == SP_TROLL
            || you.species == SP_SPRIGGAN
            || player_genus(GENPC_OGREISH)
            || player_genus(GENPC_DRACONIAN))
        {
            return (false);
        }
        return (true);

    case EQ_BODY_ARMOUR:
    case EQ_SHIELD:
        // Anyone can wear robes or a buckler/shield.
        if (special_armour)
            return (true);
        if (you.species == SP_TROLL
            || you.species == SP_SPRIGGAN
            || player_genus(GENPC_OGREISH)
            || player_genus(GENPC_DRACONIAN))
        {
            return (false);
        }
        return (true);

    case EQ_HELMET:
        // No caps or hats with Horns 3 or Antennae 3.
        if (player_mutation_level(MUT_HORNS) == 3
            || player_mutation_level(MUT_ANTENNAE) == 3)
            return(false);
        // Anyone else can wear caps.
        if (special_armour)
            return (true);
        if (player_mutation_level(MUT_HORNS)
            || player_mutation_level(MUT_BEAK)
            || player_mutation_level(MUT_ANTENNAE))
        {
            return (false);
        }
        if (you.species == SP_TROLL
            || you.species == SP_SPRIGGAN
            || player_genus(GENPC_OGREISH)
            || player_genus(GENPC_DRACONIAN))
        {
            return (false);
        }
        return (true);

    default:
        return (true);
    }
}

bool player_has_feet()
{
    if (you.species == SP_NAGA || you.species == SP_CAT || you.fishtail)
        return (false);

    if (player_mutation_level(MUT_HOOVES) >= 3
        || player_mutation_level(MUT_TALONS) >= 3)
    {
        return (false);
    }

    return (true);
}

bool player_wearing_slot(int eq)
{
    ASSERT(you.equip[eq] != -1 || !you.melded[eq]);
    return (you.equip[eq] != -1 && !you.melded[eq]);
}

bool you_tran_can_wear(const item_def &item)
{
    switch (item.base_type)
    {
    case OBJ_WEAPONS:
        return you_tran_can_wear(EQ_WEAPON);

    case OBJ_JEWELLERY:
        return you_tran_can_wear(jewellery_is_amulet(item) ? EQ_AMULET
                                                           : EQ_LEFT_RING);
    case OBJ_ARMOUR:
        if (item.sub_type == ARM_NAGA_BARDING)
            return (you.species == SP_NAGA && you_tran_can_wear(EQ_BOOTS));
        else if (item.sub_type == ARM_CENTAUR_BARDING)
            return (you.species == SP_CENTAUR && you_tran_can_wear(EQ_BOOTS));

        if (get_armour_slot(item) == EQ_HELMET
            && !is_hard_helmet(item)
            && (you.form == TRAN_SPIDER
                || you.form == TRAN_ICE_BEAST))
        {
            return (true);
        }

        if (fit_armour_size(item, you.body_size()) != 0)
            return (false);

        return you_tran_can_wear(get_armour_slot(item), true);

    default:
        return (true);
    }
}

bool you_tran_can_wear(int eq, bool check_mutation)
{
    if (eq == EQ_NONE)
        return (true);

    if (eq == EQ_STAFF)
        eq = EQ_WEAPON;
    else if (eq >= EQ_RINGS && eq <= EQ_RINGS_PLUS2)
        eq = EQ_LEFT_RING;

    // Everybody can wear at least some type of armour.
    if (eq == EQ_ALL_ARMOUR)
        return (true);

    // Not a transformation, but also temporary -> check first.
    if (check_mutation)
    {
        if (eq == EQ_GLOVES && you.has_claws(false) >= 3)
            return (false);

        if (eq == EQ_HELMET && player_mutation_level(MUT_HORNS) >= 3)
            return (false);

        if (eq == EQ_HELMET && player_mutation_level(MUT_ANTENNAE) >= 3)
            return (false);

        if (eq == EQ_BOOTS
            && (you.fishtail
                || player_mutation_level(MUT_HOOVES) >= 3
                || player_mutation_level(MUT_TALONS) >= 3))
        {
            return (false);
        }
    }

    // No further restrictions.
    if (you.form == TRAN_NONE || you.form == TRAN_LICH)
        return (true);

    // Bats and pigs cannot wear anything except amulets.
    if ((you.form == TRAN_BAT || you.form == TRAN_PIG) && eq != EQ_AMULET)
        return (false);

    // Everyone else can wear jewellery, at least.
    if (eq == EQ_LEFT_RING || eq == EQ_RIGHT_RING || eq == EQ_AMULET)
        return (true);

    // These cannot use anything but jewellery.
    if (you.form == TRAN_SPIDER || you.form == TRAN_DRAGON)
        return (false);

    if (you.form == TRAN_BLADE_HANDS)
    {
        if (eq == EQ_WEAPON || eq == EQ_GLOVES || eq == EQ_SHIELD)
            return (false);
        return (true);
    }

    if (you.form == TRAN_ICE_BEAST)
    {
        if (eq != EQ_CLOAK)
            return (false);
        return (true);
    }

    if (you.form == TRAN_STATUE)
    {
        if (eq == EQ_WEAPON || eq == EQ_SHIELD
            || eq == EQ_CLOAK || eq == EQ_HELMET)
            return (true);
        return (false);
    }

    return (true);
}

bool player_weapon_wielded()
{
    if (you.melded[EQ_WEAPON])
        return (false);

    const int wpn = you.equip[EQ_WEAPON];

    if (wpn == -1)
        return (false);

    if (you.inv[wpn].base_type != OBJ_WEAPONS
        && you.inv[wpn].base_type != OBJ_STAVES)
    {
        return (false);
    }

    return (true);
}

// Returns false if the player is wielding a weapon inappropriate for Berserk.
bool berserk_check_wielded_weapon()
{
    if (!you.weapon())
        return (true);

    const item_def weapon = *you.weapon();
    if (weapon.defined() && weapon.base_type != OBJ_STAVES
           && (weapon.base_type != OBJ_WEAPONS || is_range_weapon(weapon))
        || you.attribute[ATTR_WEAPON_SWAP_INTERRUPTED])
    {
        std::string prompt = "Do you really want to go berserk while "
                             "wielding " + weapon.name(DESC_NOCAP_YOUR)
                             + "?";

        if (!yesno(prompt.c_str(), true, 'n'))
        {
            canned_msg(MSG_OK);
            return (false);
        }

        you.attribute[ATTR_WEAPON_SWAP_INTERRUPTED] = 0;
    }

    return (true);
}

// Looks in equipment "slot" to see if there is an equipped "sub_type".
// Returns number of matches (in the case of rings, both are checked)
int player_equip(equipment_type slot, int sub_type, bool calc_unid)
{
    int ret = 0;

    item_def* item;

    switch (slot)
    {
    case EQ_WEAPON:
        // Hands can have more than just weapons.
        if (you.weapon()
            && you.weapon()->base_type == OBJ_WEAPONS
            && you.weapon()->sub_type == sub_type)
        {
            ret++;
        }
        break;

    case EQ_STAFF:
        // Like above, but must be magical staff.
        if (you.weapon()
            && you.weapon()->base_type == OBJ_STAVES
            && you.weapon()->sub_type == sub_type
            && (calc_unid || item_type_known(*you.weapon())))
        {
            ret++;
        }
        break;

    case EQ_RINGS:
        if ((item = you.slot_item(EQ_LEFT_RING))
            && item->sub_type == sub_type
            && (calc_unid
                || item_type_known(*item)))
        {
            ret++;
        }

        if ((item = you.slot_item(EQ_RIGHT_RING))
            && item->sub_type == sub_type
            && (calc_unid
                || item_type_known(*item)))
        {
            ret++;
        }
        break;

    case EQ_RINGS_PLUS:
        if ((item = you.slot_item(EQ_LEFT_RING))
            && item->sub_type == sub_type
            && (calc_unid
                || item_type_known(*item)))
        {
            ret += item->plus;
        }

        if ((item = you.slot_item(EQ_RIGHT_RING))
            && item->sub_type == sub_type
            && (calc_unid
                || item_type_known(*item)))
        {
            ret += item->plus;
        }
        break;

    case EQ_RINGS_PLUS2:
        if ((item = you.slot_item(EQ_LEFT_RING))
            && item->sub_type == sub_type
            && (calc_unid
                || item_type_known(*item)))
        {
            ret += item->plus2;
        }

        if ((item = you.slot_item(EQ_RIGHT_RING))
            && item->sub_type == sub_type
            && (calc_unid
                || item_type_known(*item)))
        {
            ret += item->plus2;
        }
        break;

    case EQ_ALL_ARMOUR:
        // Doesn't make much sense here... be specific. -- bwr
        die("EQ_ALL_ARMOUR is not a proper slot");
        break;

    default:
        if (! (slot > EQ_NONE && slot < NUM_EQUIP))
            die("invalid slot");
        if ((item = you.slot_item(slot))
            && item->sub_type == sub_type
            && (calc_unid || item_type_known(*item)))
        {
            ret++;
        }
        break;
    }

    return (ret);
}

// Looks in equipment "slot" to see if equipped item has "special" ego-type
// Returns number of matches (jewellery returns zero -- no ego type).
// [ds] There's no equivalent of calc_unid or req_id because as of now, weapons
// and armour type-id on wield/wear.
int player_equip_ego_type(int slot, int special, bool calc_unid)
{
    int ret = 0;
    bool melded = (special == SPARM_PONDEROUSNESS);

    item_def* item;
    switch (slot)
    {
    case EQ_WEAPON:
        // Hands can have more than just weapons.
        if ((item = you.slot_item(EQ_WEAPON))
            && item->base_type == OBJ_WEAPONS
            && get_weapon_brand(*item) == special)
        {
            ret++;
        }
        break;

    case EQ_LEFT_RING:
    case EQ_RIGHT_RING:
    case EQ_AMULET:
    case EQ_STAFF:
    case EQ_RINGS:
    case EQ_RINGS_PLUS:
    case EQ_RINGS_PLUS2:
        // no ego types for these slots
        break;

    case EQ_ALL_ARMOUR:
        // Check all armour slots:
        for (int i = EQ_MIN_ARMOUR; i <= EQ_MAX_ARMOUR; i++)
        {
            if ((item = you.slot_item(static_cast<equipment_type>(i), melded))
                && get_armour_ego_type(*item) == special
                && (calc_unid || item_type_known(*item)))
            {
                ret++;
            }
        }
        break;

    default:
        if (slot < EQ_MIN_ARMOUR || slot > EQ_MAX_ARMOUR)
            die("invalid slot: %d", slot);
        // Check a specific armour slot for an ego type:
        if ((item = you.slot_item(static_cast<equipment_type>(slot), melded))
            && get_armour_ego_type(*item) == special
            && (calc_unid || item_type_known(*item)))
        {
            ret++;
        }
        break;
    }

    return (ret);
}

// Return's true if the indicated unrandart is equipped
// [ds] There's no equivalent of calc_unid or req_id because as of now, weapons
// and armour type-id on wield/wear.
bool player_equip_unrand(int unrand_index)
{
    unrandart_entry* entry = get_unrand_entry(unrand_index);
    equipment_type   slot  = get_item_slot(entry->base_type,
                                           entry->sub_type);

    item_def* item;

    switch (slot)
    {
    case EQ_WEAPON:
        // Hands can have more than just weapons.
        if ((item = you.slot_item(slot))
            && item->base_type == OBJ_WEAPONS
            && is_unrandom_artefact(*item)
            && item->special == unrand_index)
        {
            return (true);
        }
        break;

    case EQ_RINGS:
        if ((item = you.slot_item(EQ_LEFT_RING))
            && is_unrandom_artefact(*item)
            && item->special == unrand_index)
        {
            return (true);
        }

        if ((item = you.slot_item(EQ_RIGHT_RING))
            && is_unrandom_artefact(*item)
            && item->special == unrand_index)
        {
            return (true);
        }
        break;

    case EQ_NONE:
    case EQ_STAFF:
    case EQ_LEFT_RING:
    case EQ_RIGHT_RING:
    case EQ_RINGS_PLUS:
    case EQ_RINGS_PLUS2:
    case EQ_ALL_ARMOUR:
        // no unrandarts for these slots.
        break;

    default:
        if (slot <= EQ_NONE || slot >= NUM_EQUIP)
            die("invalid slot: %d", slot);
        // Check a specific slot.
        if ((item = you.slot_item(slot))
            && is_unrandom_artefact(*item)
            && item->special == unrand_index)
        {
            return (true);
        }
        break;
    }

    return (false);
}

int player_evokable_levitation()
{
    return player_equip(EQ_RINGS, RING_LEVITATION)
           + player_equip_ego_type(EQ_BOOTS, SPARM_LEVITATION)
           + scan_artefacts(ARTP_LEVITATE);
}

int player_evokable_invis()
{
    return (player_equip(EQ_RINGS, RING_INVISIBILITY)
            + player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_DARKNESS)
            + scan_artefacts(ARTP_INVISIBLE));
}

// Given an adjacent monster, returns true if the player can hit it (the
// monster should not be submerged, or be submerged in shallow water if
// the player has a polearm).
bool player_can_hit_monster(const monster* mon)
{
    if (!mon->submerged())
        return (true);

    if (grd(mon->pos()) != DNGN_SHALLOW_WATER)
        return (false);

    const item_def *weapon = you.weapon();
    return (weapon && weapon_skill(*weapon) == SK_POLEARMS);
}

int player_teleport(bool calc_unid)
{
    ASSERT(!crawl_state.game_is_arena());

    // Don't allow any form of teleportation in Sprint.
    if (crawl_state.game_is_sprint())
        return 0;

    int tp = 0;

    // rings
    tp += 8 * player_equip(EQ_RINGS, RING_TELEPORTATION, calc_unid);

    // mutations
    tp += player_mutation_level(MUT_TELEPORT) * 3;

    // randart weapons only
    if (you.weapon()
        && you.weapon()->base_type == OBJ_WEAPONS
        && is_artefact(*you.weapon()))
    {
        tp += scan_artefacts(ARTP_CAUSE_TELEPORTATION, calc_unid);
    }

    return tp;
}

int player_regen()
{
    int rr = you.hp_max / 3;

    if (rr > 20)
        rr = 20 + ((rr - 20) / 2);

    // Rings.
    rr += 40 * player_equip(EQ_RINGS, RING_REGENERATION);

    // Spell.
    if (you.duration[DUR_REGENERATION]
        && !you.attribute[ATTR_DIVINE_REGENERATION])
    {
        rr += 100;
    }

    // Troll leather (except for trolls).
    if (player_equip(EQ_BODY_ARMOUR, ARM_TROLL_LEATHER_ARMOUR)
        && you.species != SP_TROLL)
    {
        rr += 30;
    }

    // Fast heal mutation.
    rr += player_mutation_level(MUT_REGENERATION) * 20;

    // Before applying other effects, make sure that there's something
    // to heal.
    rr = std::max(1, rr);

    // Healing depending on satiation.
    // The better-fed you are, the faster you heal.
    if (you.species == SP_VAMPIRE)
    {
        if (you.hunger_state == HS_STARVING)
            // No regeneration for starving vampires.
            rr = 0;
        else if (you.hunger_state == HS_ENGORGED)
            // More bonus regeneration for engorged vampires.
            rr += 20;
        else if (you.hunger_state < HS_SATIATED)
            // Halved regeneration for hungry vampires.
            rr /= 2;
        else if (you.hunger_state >= HS_FULL)
            // Bonus regeneration for full vampires.
            rr += 10;
    }

    // Powered By Death mutation, boosts regen by 10 per corpse in
    // a mutation_level * 3 (3/6/9) radius, to a maximum of 7
    // corpses.  If and only if the duration of the effect is
    // still active.
    if (you.duration[DUR_POWERED_BY_DEATH])
        rr += handle_pbd_corpses(false) * 100;

    // Slow heal mutation.  Each level reduces your natural healing by
    // one third.
    if (player_mutation_level(MUT_SLOW_HEALING) > 0)
    {
        rr *= 3 - player_mutation_level(MUT_SLOW_HEALING);
        rr /= 3;
    }

    if (you.stat_zero[STAT_STR])
        rr /= 4;

    // Trog's Hand.  This circumvents the slow healing effect.
    if (you.attribute[ATTR_DIVINE_REGENERATION])
        rr += 100;

    return (rr);
}

int player_hunger_rate(void)
{
    int hunger = 3;

    if (player_in_bat_form())
        return (1);

    if (you.species == SP_TROLL)
        hunger += 3;            // in addition to the +3 for fast metabolism

    if (you.duration[DUR_REGENERATION] && you.hp < you.hp_max)
        hunger += 4;

    // If Cheibriados has slowed your life processes, you will hunger less.
    if (you.religion == GOD_CHEIBRIADOS && you.piety >= piety_breakpoint(0))
        hunger--;

    // Moved here from main.cc... maintaining the >= 40 behaviour.
    if (you.hunger >= 40)
    {
        if (you.duration[DUR_INVIS])
            hunger += 5;

        // Berserk has its own food penalty - excluding berserk haste.
        // Doubling the hunger cost for haste so that the per turn hunger
        // is consistent now that a hasted turn causes 50% the normal hunger
        // -cao
        if (you.duration[DUR_HASTE])
            hunger += haste_mul(5);
    }

    if (you.species == SP_VAMPIRE)
    {
        switch (you.hunger_state)
        {
        case HS_STARVING:
        case HS_NEAR_STARVING:
            hunger -= 3;
            break;
        case HS_VERY_HUNGRY:
            hunger -= 2;
            break;
        case HS_HUNGRY:
            hunger--;
            break;
        case HS_SATIATED:
            break;
        case HS_FULL:
            hunger++;
            break;
        case HS_VERY_FULL:
            hunger += 2;
            break;
        case HS_ENGORGED:
            hunger += 3;
        }
    }
    else
    {
        hunger += player_mutation_level(MUT_FAST_METABOLISM);

        if (player_mutation_level(MUT_SLOW_METABOLISM) > 2)
            hunger -= 2;
        else if (player_mutation_level(MUT_SLOW_METABOLISM) > 0)
            hunger--;
    }

    // rings
    if (you.hp < you.hp_max
        && player_mutation_level(MUT_SLOW_HEALING) < 3)
    {
        hunger += 3 * player_equip(EQ_RINGS, RING_REGENERATION);
    }

    hunger += 4 * player_equip(EQ_RINGS, RING_HUNGER);
    hunger -= 2 * player_equip(EQ_RINGS, RING_SUSTENANCE);

    // troll leather armour
    if (you.species != SP_TROLL && you.hp < you.hp_max)
    {
        if (player_equip(EQ_BODY_ARMOUR, ARM_TROLL_LEATHER_ARMOUR))
            hunger += coinflip() ? 2 : 1;
    }

    // randarts
    hunger += scan_artefacts(ARTP_METABOLISM);

    // burden
    hunger += you.burden_state;

    if (hunger < 1)
        hunger = 1;

    return (hunger);
}

int player_spell_levels(void)
{
    int sl = (you.experience_level - 1) + (you.skills[SK_SPELLCASTING] * 2);

    bool fireball = false;
    bool delayed_fireball = false;

    if (sl > 99)
        sl = 99;

    for (int i = 0; i < MAX_KNOWN_SPELLS; i++)
    {
        if (you.spells[i] == SPELL_FIREBALL)
            fireball = true;
        else if (you.spells[i] == SPELL_DELAYED_FIREBALL)
            delayed_fireball = true;

        if (you.spells[i] != SPELL_NO_SPELL)
            sl -= spell_difficulty(you.spells[i]);
    }

    // Fireball is free for characters with delayed fireball
    if (fireball && delayed_fireball)
        sl += spell_difficulty(SPELL_FIREBALL);

    // Note: This can happen because of level drain.  Maybe we should
    // force random spells out when that happens. -- bwr
    if (sl < 0)
        sl = 0;

    return (sl);
}

bool player_likes_chunks(bool permanently)
{
    return (player_mutation_level(MUT_GOURMAND) > 0
            || player_mutation_level(MUT_CARNIVOROUS) > 0
            || (!permanently && wearing_amulet(AMU_THE_GOURMAND)));
}

// If temp is set to false, temporary sources or resistance won't be counted.
int player_res_fire(bool calc_unid, bool temp, bool items)
{
    int rf = 0;

    if (items)
    {
        // rings of fire resistance/fire
        rf += player_equip(EQ_RINGS, RING_PROTECTION_FROM_FIRE, calc_unid);
        rf += player_equip(EQ_RINGS, RING_FIRE, calc_unid);

        // rings of ice
        rf -= player_equip(EQ_RINGS, RING_ICE, calc_unid);

        // Staves
        rf += player_equip(EQ_STAFF, STAFF_FIRE, calc_unid);

        // body armour:
        rf += 2 * player_equip(EQ_BODY_ARMOUR, ARM_DRAGON_ARMOUR);
        rf += player_equip(EQ_BODY_ARMOUR, ARM_GOLD_DRAGON_ARMOUR);
        rf -= player_equip(EQ_BODY_ARMOUR, ARM_ICE_DRAGON_ARMOUR);
        rf += 2 * player_equip(EQ_BODY_ARMOUR, ARM_DRAGON_HIDE);
        rf += player_equip(EQ_BODY_ARMOUR, ARM_GOLD_DRAGON_HIDE);
        rf -= player_equip(EQ_BODY_ARMOUR, ARM_ICE_DRAGON_HIDE);

        // ego armours
        rf += player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_FIRE_RESISTANCE);
        rf += player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_RESISTANCE);

        // randart weapons:
        rf += scan_artefacts(ARTP_FIRE, calc_unid);

        // Che bonus
        rf += che_boost(CB_RFIRE);
    }

    // species:
    if (you.species == SP_MUMMY)
        rf--;

    // mutations:
    rf += player_mutation_level(MUT_HEAT_RESISTANCE);
    rf += player_mutation_level(MUT_MOLTEN_SCALES) == 3 ? 1 : 0;

    // spells:
    if (temp)
    {
        if (you.duration[DUR_RESIST_FIRE] > 0)
            rf++;

        if (you.duration[DUR_FIRE_SHIELD])
            rf += 2;

        // transformations:
        switch (you.form)
        {
        case TRAN_ICE_BEAST:
            rf--;
            break;
        case TRAN_DRAGON:
        {
            monster_type drag = dragon_form_dragon_type();
            if (drag == MONS_DRAGON)
                rf += 2;
            else if (drag == MONS_ICE_DRAGON)
                rf--;
            break;
        }
        default:
            break;
        }
    }

    if (rf < -3)
        rf = -3;
    else if (rf > 3)
        rf = 3;

    return (rf);
}

int player_res_steam(bool calc_unid, bool temp, bool items)
{
    int res = 0;

    if (you.species == SP_PALE_DRACONIAN)
        res += 2;

    if (items && player_equip(EQ_BODY_ARMOUR, ARM_STEAM_DRAGON_ARMOUR))
        res += 2;

    if (items && player_equip(EQ_BODY_ARMOUR, ARM_STEAM_DRAGON_HIDE))
        res += 2;

    return (res + player_res_fire(calc_unid, temp, items) / 2);
}

int player_res_cold(bool calc_unid, bool temp, bool items)
{
    int rc = 0;

    if (temp)
    {
        // spells:
        if (you.duration[DUR_RESIST_COLD] > 0)
            rc++;

        if (you.duration[DUR_FIRE_SHIELD])
            rc -= 2;

        // transformations:
        switch (you.form)
        {
        case TRAN_ICE_BEAST:
            rc += 3;
            break;
        case TRAN_DRAGON:
        {
            monster_type form = dragon_form_dragon_type();
            if (form == MONS_DRAGON)
                rc--;
            else if (form == MONS_ICE_DRAGON)
                rc += 2;
            break;
        }
        case TRAN_LICH:
            rc++;
            break;
        default:
            break;
        }

        if (you.species == SP_VAMPIRE)
        {
            if (you.hunger_state <= HS_NEAR_STARVING)
                rc += 2;
            else if (you.hunger_state < HS_SATIATED)
                rc++;
        }
    }

    if (items)
    {
        // rings of cold resistance/ice
        rc += player_equip(EQ_RINGS, RING_PROTECTION_FROM_COLD, calc_unid);
        rc += player_equip(EQ_RINGS, RING_ICE, calc_unid);

        // rings of fire
        rc -= player_equip(EQ_RINGS, RING_FIRE, calc_unid);

        // Staves
        rc += player_equip(EQ_STAFF, STAFF_COLD, calc_unid);

        // body armour:
        rc += 2 * player_equip(EQ_BODY_ARMOUR, ARM_ICE_DRAGON_ARMOUR);
        rc += player_equip(EQ_BODY_ARMOUR, ARM_GOLD_DRAGON_ARMOUR);
        rc -= player_equip(EQ_BODY_ARMOUR, ARM_DRAGON_ARMOUR);
        rc += 2 * player_equip(EQ_BODY_ARMOUR, ARM_ICE_DRAGON_HIDE);
        rc += player_equip(EQ_BODY_ARMOUR, ARM_GOLD_DRAGON_HIDE);
        rc -= player_equip(EQ_BODY_ARMOUR, ARM_DRAGON_HIDE);

        // ego armours
        rc += player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_COLD_RESISTANCE);
        rc += player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_RESISTANCE);

        // randart weapons:
        rc += scan_artefacts(ARTP_COLD, calc_unid);

        // Che bonus
        rc += che_boost(CB_RCOLD);
    }

    // mutations:
    rc += player_mutation_level(MUT_COLD_RESISTANCE);
    rc += player_mutation_level(MUT_ICY_BLUE_SCALES) == 3 ? 1 : 0;
    rc += player_mutation_level(MUT_SHAGGY_FUR) == 3 ? 1 : 0;

    if (rc < -3)
        rc = -3;
    else if (rc > 3)
        rc = 3;

    return (rc);
}

int player_res_corr(bool calc_unid, bool items)
{
    if (items && wearing_amulet(AMU_RESIST_CORROSION, calc_unid))
        return 1;
    else
        return 0;
}

int player_res_acid(bool calc_unid, bool items)
{
    int res = 0;
    if (!form_changed_physiology()
        || you.form == TRAN_DRAGON)
    {
        if (you.species == SP_YELLOW_DRACONIAN)
            res += 2;
    }

    if (items)
    {
        if (wearing_amulet(AMU_RESIST_CORROSION, calc_unid, true))
            res++;

        if (player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_PRESERVATION))
            res++;
    }

    // mutations:
    res += std::max(0, player_mutation_level(MUT_YELLOW_SCALES) - 1);

    if (res > 3)
        res = 3;

    return (res);
}

// Returns a factor X such that post-resistance acid damage can be calculated
// as pre_resist_damage * X / 100.
int player_acid_resist_factor()
{
    int res = player_res_acid();

    int factor = 100;

    if (res == 1)
        factor = 50;
    else if (res == 2)
        factor = 34;
    else if (res > 2)
    {
        factor = 30;
        while (res-- > 2 && factor >= 20)
            factor = factor * 90 / 100;
    }

    return (factor);
}

int player_res_electricity(bool calc_unid, bool temp, bool items)
{
    int re = 0;

    if (temp)
    {
        if (you.duration[DUR_INSULATION])
            re++;

        // transformations:
        if (you.form == TRAN_STATUE)
            re += 1;

        if (you.attribute[ATTR_DIVINE_LIGHTNING_PROTECTION])
            re = 3;
        else if (re > 1)
            re = 1;
    }

    if (items)
    {
        // staff
        re += player_equip(EQ_STAFF, STAFF_AIR, calc_unid);

        // body armour:
        re += player_equip(EQ_BODY_ARMOUR, ARM_STORM_DRAGON_ARMOUR);
        re += player_equip(EQ_BODY_ARMOUR, ARM_STORM_DRAGON_HIDE);

        // randart weapons:
        re += scan_artefacts(ARTP_ELECTRICITY, calc_unid);
    }

    // mutations:
    re += player_mutation_level(MUT_THIN_METALLIC_SCALES) == 3 ? 1 : 0;
    re += player_mutation_level(MUT_SHOCK_RESISTANCE);

    return (re);
}

bool player_control_teleport(bool calc_unid, bool temp, bool items)
{
    return ((temp && you.duration[DUR_CONTROL_TELEPORT])
            || (items
                && player_equip(EQ_RINGS, RING_TELEPORT_CONTROL, calc_unid))
            || player_mutation_level(MUT_TELEPORT_CONTROL));
}

int player_res_torment(bool, bool temp)
{
    return (player_mutation_level(MUT_TORMENT_RESISTANCE)
            || you.form == TRAN_LICH
            || you.species == SP_VAMPIRE && you.hunger_state == HS_STARVING
            || (temp &&
                (20 * player_mutation_level(MUT_STOCHASTIC_TORMENT_RESISTANCE)
                 > random2(100))));
}

// Funny that no races are susceptible to poisons. {dlb}
// If temp is set to false, temporary sources or resistance won't be counted.
int player_res_poison(bool calc_unid, bool temp, bool items)
{
    int rp = 0;

    if (items)
    {
        // rings of poison resistance
        rp += player_equip(EQ_RINGS, RING_POISON_RESISTANCE, calc_unid);

        // Staves
        rp += player_equip(EQ_STAFF, STAFF_POISON, calc_unid);

        // ego armour:
        rp += player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_POISON_RESISTANCE);

        // body armour:
        rp += player_equip(EQ_BODY_ARMOUR, ARM_GOLD_DRAGON_ARMOUR);
        rp += player_equip(EQ_BODY_ARMOUR, ARM_SWAMP_DRAGON_ARMOUR);
        rp += player_equip(EQ_BODY_ARMOUR, ARM_GOLD_DRAGON_HIDE);
        rp += player_equip(EQ_BODY_ARMOUR, ARM_SWAMP_DRAGON_HIDE);

        // randart weapons:
        rp += scan_artefacts(ARTP_POISON, calc_unid);
    }

    // mutations:
    rp += player_mutation_level(MUT_POISON_RESISTANCE);
    rp += player_mutation_level(MUT_SLIMY_GREEN_SCALES) == 3 ? 1 : 0;

    // Only thirsty vampires are naturally poison resistant.
    if (you.species == SP_VAMPIRE && you.hunger_state < HS_SATIATED)
        rp++;

    if (temp)
    {
        // spells:
        if (you.duration[DUR_RESIST_POISON] > 0)
            rp++;

        // transformations:
        switch (you.form)
        {
        case TRAN_LICH:
        case TRAN_ICE_BEAST:
        case TRAN_STATUE:
        case TRAN_DRAGON:
            rp++;
            break;
        default:
            break;
        }
    }

    if (rp > 1)
        rp = 1;

    return (rp);
}

int player_res_sticky_flame(bool calc_unid, bool temp, bool items)
{
    int rsf = 0;

    if (you.species == SP_MOTTLED_DRACONIAN)
        rsf++;

    if (items && player_equip(EQ_BODY_ARMOUR, ARM_MOTTLED_DRAGON_ARMOUR))
        rsf++;
    if (items && player_equip(EQ_BODY_ARMOUR, ARM_MOTTLED_DRAGON_HIDE))
        rsf++;

    if (rsf > 1)
        rsf = 1;

    return (rsf);
}

int player_spec_death()
{
    int sd = 0;

    // Staves
    sd += player_equip(EQ_STAFF, STAFF_DEATH);

    // species:
    if (you.species == SP_MUMMY)
    {
        if (you.experience_level >= 13)
            sd++;
        if (you.experience_level >= 26)
            sd++;
    }

    // transformations:
    if (you.form == TRAN_LICH)
        sd++;

    return sd;
}

int player_spec_holy()
{
    //if (you.char_class == JOB_PRIEST || you.char_class == JOB_PALADIN)
    //  return 1;
    return 0;
}

int player_spec_fire()
{
    int sf = 0;

    // staves:
    sf += player_equip(EQ_STAFF, STAFF_FIRE);

    // rings of fire:
    sf += player_equip(EQ_RINGS, RING_FIRE);

    if (you.duration[DUR_FIRE_SHIELD])
        sf++;

    // if it's raining... {due}
    if (in_what_cloud(CLOUD_RAIN))
        sf--;

    return sf;
}

int player_spec_cold()
{
    int sc = 0;

    // staves:
    sc += player_equip(EQ_STAFF, STAFF_COLD);

    // rings of ice:
    sc += player_equip(EQ_RINGS, RING_ICE);

    return sc;
}

int player_spec_earth()
{
    int se = 0;

    // Staves
    se += player_equip(EQ_STAFF, STAFF_EARTH);

    return se;
}

int player_spec_air()
{
    int sa = 0;

    // Staves
    sa += player_equip(EQ_STAFF, STAFF_AIR);

    return sa;
}

int player_spec_conj()
{
    int sc = 0;

    // Staves
    sc += player_equip(EQ_STAFF, STAFF_CONJURATION);

    return sc;
}

int player_spec_ench()
{
    int se = 0;

    // Staves
    se += player_equip(EQ_STAFF, STAFF_ENCHANTMENT);

    return se;
}

int player_spec_summ()
{
    int ss = 0;

    // Staves
    ss += player_equip(EQ_STAFF, STAFF_SUMMONING);

    return ss;
}

int player_spec_poison()
{
    int sp = 0;

    // Staves
    sp += player_equip(EQ_STAFF, STAFF_POISON);

    if (player_equip_unrand(UNRAND_OLGREB))
        sp++;

    return sp;
}

int player_energy()
{
    int pe = 0;

    // Staves
    pe += player_equip(EQ_STAFF, STAFF_ENERGY);

    return pe;
}

// If temp is set to false, temporary sources of resistance won't be
// counted.
int player_prot_life(bool calc_unid, bool temp, bool items)
{
    int pl = 0;

    // Hunger is temporary, true, but that's something you can control,
    // especially as life protection only increases the hungrier you
    // get.
    if (you.species == SP_VAMPIRE)
    {
        switch (you.hunger_state)
        {
        case HS_STARVING:
        case HS_NEAR_STARVING:
            pl = 3;
            break;
        case HS_VERY_HUNGRY:
        case HS_HUNGRY:
            pl = 2;
            break;
        case HS_SATIATED:
            pl = 1;
            break;
        default:
            break;
        }
    }

    // Same here.  Your piety status, and, hence, TSO's protection, is
    // something you can more or less control.
    if (you.religion == GOD_SHINING_ONE && you.piety > pl * 50)
        pl = you.piety / 50;

    if (temp)
    {
        // Now, transformations could stop at any time.
        switch (you.form)
        {
        case TRAN_STATUE:
            pl++;
            break;
        case TRAN_LICH:
            pl += 3;
            break;
        default:
           break;
        }
    }

    if (items)
    {
        if (wearing_amulet(AMU_WARDING, calc_unid))
            pl++;

        // rings
        pl += player_equip(EQ_RINGS, RING_LIFE_PROTECTION, calc_unid);

        // armour (checks body armour only)
        pl += player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_POSITIVE_ENERGY);

        // pearl dragon counts
        pl += player_equip(EQ_BODY_ARMOUR, ARM_PEARL_DRAGON_ARMOUR);
        pl += player_equip(EQ_BODY_ARMOUR, ARM_PEARL_DRAGON_HIDE);

        // randart wpns
        pl += scan_artefacts(ARTP_NEGATIVE_ENERGY, calc_unid);

        // Che bonus
        pl += che_boost(CB_RNEG);

        pl += player_equip(EQ_STAFF, STAFF_DEATH, calc_unid);
    }

    // undead/demonic power
    pl += player_mutation_level(MUT_NEGATIVE_ENERGY_RESISTANCE);

    pl = std::min(3, pl);

    return (pl);
}

// New player movement speed system... allows for a bit more than
// "player runs fast" and "player walks slow" in that the speed is
// actually calculated (allowing for centaurs to get a bonus from
// swiftness and other such things).  Levels of the mutation now
// also have meaning (before they all just meant fast).  Most of
// this isn't as fast as it used to be (6 for having anything), but
// even a slight speed advantage is very good... and we certainly don't
// want to go past 6 (see below). -- bwr
int player_movement_speed(bool ignore_burden)
{
    int mv = 10;

    // transformations
    if (you.form == TRAN_SPIDER)
        mv = 8;
    else if (player_in_bat_form())
        mv = 5; // but allowed minimum is six
    else if (you.form == TRAN_PIG)
        mv = 7;
    else if (you.fishtail)
        mv = 6;

    // moving on liquefied ground takes longer
    if (liquefied(you.pos()) && you.ground_level())
        mv += 3;

    // armour
    if (player_equip_ego_type(EQ_BOOTS, SPARM_RUNNING))
        mv -= 2;

    // ponderous brand and artefact property
    mv += 2 * player_ponderousness();

    // In the air, can fly fast (should be lightly burdened).
    if (!ignore_burden && you.light_flight())
        mv--;

    // Swiftness is an Air spell, it doesn't work in water, but
    // flying players will move faster.
    if (you.duration[DUR_SWIFTNESS] > 0 && !you.in_water())
        mv -= (you.flight_mode() == FL_FLY ? 4 : 2);

    // Mutations: -2, -3, -4, unless innate and shapechanged.
    // Not when swimming, since it is "cover the ground quickly".
    if (player_mutation_level(MUT_FAST) > 0
        && (!you.innate_mutations[MUT_FAST] || !player_is_shapechanged())
        && !you.swimming())
    {
        mv -= player_mutation_level(MUT_FAST) + 1;
    }

    if (player_mutation_level(MUT_SLOW) > 0 && !player_is_shapechanged())
    {
        mv *= 10 + player_mutation_level(MUT_SLOW) * 2;
        mv /= 10;
    }

    // Burden
    if (!ignore_burden)
    {
        if (you.burden_state == BS_ENCUMBERED)
            mv++;
        else if (you.burden_state == BS_OVERLOADED)
            mv += 3;
    }

    // We'll use the old value of six as a minimum, with haste this could
    // end up as a speed of three, which is about as fast as we want
    // the player to be able to go (since 3 is 3.33x as fast and 2 is 5x,
    // which is a bit of a jump, and a bit too fast) -- bwr
    if (mv < 6)
        mv = 6;

    return (mv);
}

// This function differs from the above in that it's used to set the
// initial time_taken value for the turn.  Everything else (movement,
// spellcasting, combat) applies a ratio to this value.
int player_speed(void)
{
    int ps = 10;

    for (int i = 0; i < NUM_STATS; ++i)
        if (you.stat_zero[i] > 0)
            ps *= 2;

    if (you.duration[DUR_SLOW])
        ps = haste_mul(ps);

    if (you.duration[DUR_BERSERK] && you.religion != GOD_CHEIBRIADOS)
        ps /= 2;
    else if (you.duration[DUR_HASTE])
        ps = haste_div(ps);

    if (you.form == TRAN_STATUE)
    {
        ps *= 15;
        ps /= 10;
    }

    return ps;
}

int player_ponderousness()
{
    return (scan_artefacts(ARTP_PONDEROUS)
            + player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_PONDEROUSNESS));
}

static int _player_armour_racial_bonus(const item_def& item)
{
    if (item.base_type != OBJ_ARMOUR)
        return 0;

    int racial_bonus = 0;
    const iflags_t armour_race = get_equip_race(item);

    // get the armour race value that corresponds to the character's race:
    const iflags_t racial_type
                            = ((player_genus(GENPC_DWARVEN)) ? ISFLAG_DWARVEN :
                               (player_genus(GENPC_ELVEN))   ? ISFLAG_ELVEN :
                               (you.species == SP_HILL_ORC)  ? ISFLAG_ORCISH
                                                             : 0);

    // Dwarven armour is universally good -- bwr
    if (armour_race == ISFLAG_DWARVEN)
        racial_bonus += 4;

    if (racial_type && armour_race == racial_type)
    {
        // Elven armour is light, but still gives one level to elves.
        // Orcish and Dwarven armour are worth +2 to the correct
        // species, plus the plus that anyone gets with dwarven armour.
        // -- bwr
        if (racial_type == ISFLAG_ELVEN)
            racial_bonus += 2;
        else
            racial_bonus += 4;

        // an additional bonus for Beogh worshippers
        if (you.religion == GOD_BEOGH && !player_under_penance())
        {
            if (you.piety >= 185)
                racial_bonus += racial_bonus * 9 / 4;
            else if (you.piety >= 160)
                racial_bonus += racial_bonus * 2;
            else if (you.piety >= 120)
                racial_bonus += racial_bonus * 7 / 4;
            else if (you.piety >= 80)
                racial_bonus += racial_bonus * 5 / 4;
            else if (you.piety >= 40)
                racial_bonus += racial_bonus * 3 / 4;
            else
                racial_bonus += racial_bonus / 4;
        }
    }

    return racial_bonus;
}

bool is_effectively_light_armour(const item_def *item)
{
    return (!item
            || (abs(property(*item, PARM_EVASION)) < 2));
}

bool player_effectively_in_light_armour()
{
    const item_def *armour = you.slot_item(EQ_BODY_ARMOUR, false);
    return is_effectively_light_armour(armour);
}

// This function returns true if the player has a radically different
// shape... minor changes like blade hands don't count, also note
// that lich transformation doesn't change the character's shape
// (so we end up with Naga-lichs, Spiggan-lichs, Minotaur-lichs)
// it just makes the character undead (with the benefits that implies). - bwr
bool player_is_shapechanged(void)
{
    if (you.form == TRAN_NONE
        || you.form == TRAN_BLADE_HANDS
        || you.form == TRAN_LICH)
    {
        return (false);
    }

    return (true);
}

// An evasion factor based on the player's body size, smaller == higher
// evasion size factor.
int player_evasion_size_factor()
{
    // XXX: you.body_size() implementations are incomplete, fix.
    const size_type size = you.body_size(PSIZE_BODY);
    return 2 * (SIZE_MEDIUM - size);
}

// The EV penalty to the player for wearing their current shield.
int player_adjusted_shield_evasion_penalty(int scale)
{
    const item_def *shield = you.slot_item(EQ_SHIELD, false);
    if (!shield)
        return (0);

    const int base_shield_penalty = -property(*shield, PARM_EVASION);
    return std::max(0,
                    (base_shield_penalty * scale
                     - you.skill(SK_SHIELDS) * scale
                     / std::max(1, 5 + player_evasion_size_factor())));
}

int player_raw_body_armour_evasion_penalty()
{
    const item_def *body_armour = you.slot_item(EQ_BODY_ARMOUR, false);
    if (!body_armour)
        return (0);

    const int base_ev_penalty = -property(*body_armour, PARM_EVASION);
    return base_ev_penalty;
}

int player_size_adjusted_body_armour_evasion_penalty(int scale)
{
    const int base_ev_penalty = player_raw_body_armour_evasion_penalty();
    if (!base_ev_penalty)
        return (0);

    const int size = you.body_size(PSIZE_BODY);

    const int size_bonus_factor = (size - SIZE_MEDIUM) * scale / 4;

    const int size_adjusted_penalty =
        std::max(0,
                 scale * base_ev_penalty
                 - size_bonus_factor * base_ev_penalty);

    return (size_adjusted_penalty);
}

// The EV penalty to the player for their worn body armour.
int player_adjusted_body_armour_evasion_penalty(int scale)
{
    const int base_ev_penalty = player_raw_body_armour_evasion_penalty();
    if (!base_ev_penalty)
        return (0);

    return ((base_ev_penalty
             + std::max(0, 3 * base_ev_penalty - you.strength()))
            * (45 - you.skill(SK_ARMOUR))
            * scale
            / 45);
}

// The total EV penalty to the player for all their worn armour items
// with a base EV penalty (i.e. EV penalty as a base armour property,
// not as a randart property).
static int _player_adjusted_evasion_penalty(const int scale)
{
    int piece_armour_evasion_penalty = 0;

    // Some lesser armours have small penalties now (barding).
    for (int i = EQ_MIN_ARMOUR; i < EQ_MAX_ARMOUR; i++)
    {
        if (i == EQ_SHIELD || !player_wearing_slot(i))
            continue;

        // [ds] Evasion modifiers for armour are negatives, change
        // those to positive for penalty calc.
        const int penalty = -property(you.inv[you.equip[i]], PARM_EVASION);
        if (penalty > 0)
            piece_armour_evasion_penalty += penalty;
    }

    return (piece_armour_evasion_penalty * scale +
            player_adjusted_body_armour_evasion_penalty(scale));
}

// EV bonuses that work even when helpless.
static int _player_para_evasion_bonuses(ev_ignore_type evit)
{
    int evbonus = 0;

    if (you.duration[DUR_PHASE_SHIFT] && !(evit & EV_IGNORE_PHASESHIFT))
        evbonus += 8;

    if (player_mutation_level(MUT_DISTORTION_FIELD) > 0)
        evbonus += player_mutation_level(MUT_DISTORTION_FIELD) + 1;

    return (evbonus);
}

// Player EV bonuses for various effects and transformations. This
// does not include kenku/merfolk EV bonuses for flight/swimming.
int player_evasion_bonuses(ev_ignore_type evit)
{
    int evbonus = _player_para_evasion_bonuses(evit);

    if (you.duration[DUR_AGILITY])
        evbonus += 5;

    evbonus += player_equip(EQ_RINGS_PLUS, RING_EVASION);

    if (player_equip_ego_type(EQ_WEAPON, SPWPN_EVASION))
        evbonus += 5;

    evbonus += scan_artefacts(ARTP_EVASION);

    // mutations
    if (player_mutation_level(MUT_ICY_BLUE_SCALES) > 1)
        evbonus--;
    if (player_mutation_level(MUT_MOLTEN_SCALES) > 1)
        evbonus--;
    evbonus -= std::max(0, player_mutation_level(MUT_SLIMY_GREEN_SCALES) - 1);
    evbonus += std::max(0, player_mutation_level(MUT_GELATINOUS_BODY) - 1);

    // transformation penalties/bonuses not covered by size alone:
    if (you.form == TRAN_STATUE)
        evbonus -= 5;                // stiff

    return (evbonus);
}

// Player EV scaling for being flying kenku or swimming merfolk.
int player_scale_evasion(const int prescaled_ev, const int scale)
{
    switch (you.species)
    {
    case SP_MERFOLK:
        // Merfolk get an evasion bonus in water.
        if (you.fishtail)
        {
            const int ev_bonus =
                std::min(9 * scale,
                         std::max(2 * scale,
                                  prescaled_ev / 4));
            return (prescaled_ev + ev_bonus);
        }
        break;

    case SP_KENKU:
        // Flying Kenku get an evasion bonus.
        if (you.flight_mode() == FL_FLY)
        {
            const int ev_bonus =
                std::min(9 * scale,
                         std::max(1 * scale,
                                  prescaled_ev / 5));
            return (prescaled_ev + ev_bonus);
        }
        break;

    default:
        break;
    }
    return (prescaled_ev);
}

// Total EV for player using the revised 0.6 evasion model.
int player_evasion(ev_ignore_type evit)
{
    const int size_factor = player_evasion_size_factor();
    // Repulsion fields and size are all that matters when paralysed or
    // at 0 dex.
    if ((you.cannot_move() || you.stat_zero[STAT_DEX])
        && !(evit & EV_IGNORE_HELPLESS))
    {
        const int paralysed_base_ev = 2 + size_factor / 2;
        const int repulsion_ev = _player_para_evasion_bonuses(evit);
        return std::max(1, paralysed_base_ev + repulsion_ev);
    }

    const int scale = 100;
    const int size_base_ev = (10 + size_factor) * scale;

    const int adjusted_evasion_penalty =
        _player_adjusted_evasion_penalty(scale);

    // The last two parameters are not important.
    const int ev_dex = stepdown_value(you.dex(), 10, 24, 72, 72);

    const int dodge_bonus =
        (7 + you.skill(SK_DODGING) * ev_dex) * scale
        / (20 - size_factor);

    // [ds] Dodging penalty for being in high EVP armour, almost
    // identical to v0.5/4.1 penalty, but with the EVP discount being
    // 1 instead of 0.5 so that leather armour is fully discounted.
    // The 1 EVP of leather armour may still incur an
    // adjusted_evasion_penalty, however.
    const int armour_dodge_penalty =
        std::max(0,
                 (30 * player_size_adjusted_body_armour_evasion_penalty(scale)
                  - 30 * scale)
                 / std::max(1, (int) you.strength()));

    // Adjust dodge bonus for the effects of being suited up in armour.
    const int armour_adjusted_dodge_bonus =
        std::max(0, dodge_bonus - armour_dodge_penalty);

    const int adjusted_shield_penalty =
        player_adjusted_shield_evasion_penalty(scale);

    const int prestepdown_evasion =
        size_base_ev
        + armour_adjusted_dodge_bonus
        - adjusted_evasion_penalty
        - adjusted_shield_penalty;

    const int poststepdown_evasion =
        stepdown_value(prestepdown_evasion, 20*scale, 30*scale, 60*scale, -1);

    const int evasion_bonuses = player_evasion_bonuses(evit) * scale;

    const int prescaled_evasion =
        poststepdown_evasion + evasion_bonuses;

    const int final_evasion =
        player_scale_evasion(prescaled_evasion, scale);

    return (unscale_round_up(final_evasion, scale));
}

int player_body_armour_racial_spellcasting_bonus(const int scale)
{
    const item_def *body_armour = you.slot_item(EQ_BODY_ARMOUR, false);
    if (!body_armour)
        return (0);

    const iflags_t armour_race = get_equip_race(*body_armour);

    // Get the armour race value that corresponds to the character's
    // race:
    const iflags_t player_race
                            = ((player_genus(GENPC_DWARVEN)) ? ISFLAG_DWARVEN :
                               (player_genus(GENPC_ELVEN))   ? ISFLAG_ELVEN :
                               (you.species == SP_HILL_ORC)  ? ISFLAG_ORCISH
                                                             : 0);
    int armour_racial_spellcasting_bonus = 0;
    if (armour_race & ISFLAG_ELVEN)
        armour_racial_spellcasting_bonus += 25;

    if (armour_race & ISFLAG_DWARVEN)
        armour_racial_spellcasting_bonus -= 15;

    if (armour_race & player_race)
        armour_racial_spellcasting_bonus += 15;

    return (armour_racial_spellcasting_bonus * scale);
}

// Returns the spellcasting penalty (increase in spell failure) for the
// player's worn body armour and shield.
int player_armour_shield_spell_penalty()
{
    const int scale = 100;

    const int body_armour_penalty =
        std::max(25 * player_adjusted_body_armour_evasion_penalty(scale)
                    - player_body_armour_racial_spellcasting_bonus(scale),
                 0);

    const int total_penalty = body_armour_penalty
                 + 25 * player_adjusted_shield_evasion_penalty(scale)
                 - 20 * scale;

    return (std::max(total_penalty, 0) / scale);
}

int player_magical_power(void)
{
    int ret = 0;

    ret += 13 * player_equip(EQ_STAFF, STAFF_POWER);
    ret +=  9 * player_equip(EQ_RINGS, RING_MAGICAL_POWER);
    ret +=      scan_artefacts(ARTP_MAGICAL_POWER);

    return (ret);
}

int player_mag_abil(bool is_weighted)
{
    int ma = 0;

    // Brilliance Potion
    ma += 6 * (you.duration[DUR_BRILLIANCE] ? 1 : 0);

    // Rings
    ma += 3 * player_equip(EQ_RINGS, RING_WIZARDRY);

    // Staves
    ma += 4 * player_equip(EQ_STAFF, STAFF_WIZARDRY);

    return ((is_weighted) ? ((ma * you.intel()) / 10) : ma);
}

int player_shield_class(void)   //jmf: changes for new spell
{
    int shield = 0;
    int stat = 0;

    if (you.incapacitated())
        return (0);

    if (player_wearing_slot(EQ_SHIELD))
    {
        const item_def& item = you.inv[you.equip[EQ_SHIELD]];
        int base_shield = property(item, PARM_AC);

        int racial_bonus = _player_armour_racial_bonus(item);

        // bonus applied only to base, see above for effect:
        shield += base_shield * 100;
        shield += base_shield * 5 * you.skill(SK_SHIELDS);
        shield += base_shield * racial_bonus * 10 / 3;

        shield += item.plus * 100;

        if (item.sub_type == ARM_BUCKLER)
            stat = you.dex() * 38;
        else if (item.sub_type == ARM_LARGE_SHIELD)
            stat = you.dex() * 12 + you.strength() * 26;
        else
            stat = you.dex() * 19 + you.strength() * 19;
    }
    else
    {
        if (you.duration[DUR_MAGIC_SHIELD])
        {
            stat   =  600 + you.skill(SK_EVOCATIONS) * 50;
            shield += 300 + you.skill(SK_EVOCATIONS) * 25;
        }

        if (!you.duration[DUR_FIRE_SHIELD]
            && you.duration[DUR_CONDENSATION_SHIELD])
        {
            shield += 300 + (you.skill(SK_ICE_MAGIC) * 25);
            stat    = std::max(stat, you.intel() * 38);
        }
    }

    if (you.duration[DUR_DIVINE_SHIELD])
    {
        shield += you.attribute[ATTR_DIVINE_SHIELD] * 150;
        stat = std::max(stat, int(you.attribute[ATTR_DIVINE_SHIELD] * 300));
    }

    if (shield + stat > 0)
        shield += skill_bump(SK_SHIELDS) * 38;

    // mutations
    shield += player_mutation_level(MUT_LARGE_BONE_PLATES) > 0 ? 100 + player_mutation_level(MUT_LARGE_BONE_PLATES) * 100 : 0;      // +2, +3, +4

    return (shield + stat + 50) / 100;
}

int player_sust_abil(bool calc_unid)
{
    int sa = 0;

    sa += player_equip(EQ_RINGS, RING_SUSTAIN_ABILITIES, calc_unid);

    return (sa);
}

int player_warding(bool calc_unid)
{
    int ward = 0;

    ward += player_equip(EQ_AMULET, AMU_WARDING, calc_unid);

    ward += player_equip(EQ_STAFF, STAFF_SUMMONING, calc_unid);

    return (ward);
}

int carrying_capacity(burden_state_type bs)
{
    // Yuck.  We need this for gameplay - it nerfs small forms too much
    // otherwise - but there's no good way to rationalize here...  --sorear
    const int used_weight = std::max(you.body_weight(), you.body_weight(true));

    int cap = ((2 * used_weight) + (you.strength() * 300)
               + (you.airborne() ? 1000 : 0));
    // We are nice to the lighter species in that strength adds absolutely
    // instead of relatively to body weight. --dpeg

    if (you.stat_zero[STAT_STR])
        cap /= 2;

    if (bs == BS_UNENCUMBERED)
        return ((cap * 5) / 6);
    else if (bs == BS_ENCUMBERED)
        return ((cap * 11) / 12);
    else
        return (cap);
}

int burden_change(void)
{
    const burden_state_type old_burdenstate = you.burden_state;
    const bool was_flying_light = you.light_flight();

    you.burden = 0;

    for (int bu = 0; bu < ENDOFPACK; bu++)
    {
        if (you.inv[bu].quantity < 1)
            continue;

        you.burden += item_mass(you.inv[bu]) * you.inv[bu].quantity;
    }

    you.burden_state = BS_UNENCUMBERED;
    set_redraw_status(REDRAW_BURDEN);
    you.redraw_evasion = true;

    // changed the burdened levels to match the change to max_carried
    if (you.burden <= carrying_capacity(BS_UNENCUMBERED))
    {
        you.burden_state = BS_UNENCUMBERED;

        // this message may have to change, just testing {dlb}
        if (old_burdenstate != you.burden_state)
            mpr("Your possessions no longer seem quite so burdensome.");
    }
    else if (you.burden <= carrying_capacity(BS_ENCUMBERED))
    {
        you.burden_state = BS_ENCUMBERED;

        if (old_burdenstate != you.burden_state)
        {
            mpr("You are being weighed down by all of your possessions.");
            learned_something_new(HINT_HEAVY_LOAD);
        }
    }
    else
    {
        you.burden_state = BS_OVERLOADED;

        if (old_burdenstate != you.burden_state)
        {
            mpr("You are being crushed by all of your possessions.");
            learned_something_new(HINT_HEAVY_LOAD);
        }
    }

    // Stop travel if we get burdened (as from potions of might/levitation
    // wearing off).
    if (you.burden_state > old_burdenstate)
        interrupt_activity(AI_BURDEN_CHANGE);

    const bool is_flying_light = you.light_flight();

    if (is_flying_light != was_flying_light)
    {
        mpr(is_flying_light ? "You feel quicker in the air."
                            : "You feel heavier in the air.");
    }

    return (you.burden);
}

// force is true for forget_map command on level map.
void forget_map(int chance_forgotten, bool force)
{
    ASSERT(!crawl_state.game_is_arena());

    if (force && !yesno("Really forget level map?", true, 'n'))
        return;

    // The radius is only used in labyrinths.
    const bool use_lab_check = (!force && you.level_type == LEVEL_LABYRINTH
                                && chance_forgotten < 100);
    const int radius = (use_lab_check && you.species == SP_MINOTAUR) ? 40*40
                                                                     : 25*25;
    for (rectangle_iterator ri(0); ri; ++ri)
    {
        if (!you.see_cell(*ri)
            && (force || x_chance_in_y(chance_forgotten, 100)
                || use_lab_check && (you.pos()-*ri).abs() > radius))
        {
            env.map_knowledge(*ri).clear();
#ifdef USE_TILE
            tile_forget_map(*ri);
#endif
        }
    }

    ash_detect_portals(player_in_mappable_area());
#ifdef USE_TILE
    tiles.update_minimap_bounds();
#endif
}

void gain_exp(unsigned int exp_gained, unsigned int* actual_gain,
              unsigned int* actual_avail_gain)
{
    if (crawl_state.game_is_arena())
        return;
    if (crawl_state.game_is_sprint() && you.level_type == LEVEL_ABYSS)
        return;

    if (you.religion == GOD_ASHENZARI && you.piety > piety_breakpoint(0))
        exp_gained = div_rand_round(exp_gained * (8 + ash_bondage_level()), 8);
    exp_gained -= ash_reduce_xp(exp_gained);

    const unsigned int  old_exp   = you.experience;
    const int           old_avail = you.exp_available;

    dprf("gain_exp: %d", exp_gained);

    if (you.transfer_skill_points > 0)
    {
        // Can happen if the game got interrupted during target skill choice.
        if (is_invalid_skill(you.transfer_to_skill))
        {
            you.transfer_from_skill = SK_NONE;
            you.transfer_skill_points = 0;
            you.transfer_total_skill_points = 0;
        }
        else
        {
            int amount = exp_gained * 10
                                / calc_skill_cost(you.skill_cost_level,
                                            you.skills[you.transfer_to_skill]);
            if (amount >= 20 || one_chance_in(20 - amount))
            {
                amount = std::max(20, amount);
                transfer_skill_points(you.transfer_from_skill,
                                      you.transfer_to_skill, amount, false);
            }
        }
    }

    if (you.experience + exp_gained > (unsigned int)MAX_EXP_TOTAL)
        you.experience = MAX_EXP_TOTAL;
    else
        you.experience += exp_gained;

    if (you.duration[DUR_SAGE])
    {
        // Bonus skill training from Sage.
        you.exp_available =
            (exp_gained * you.sage_bonus_degree) / 100 + exp_gained / 2;
        practise(EX_SAGE, you.sage_bonus_skill);
        you.exp_available = old_avail;
        exp_gained /= 2;
    }

    if (crawl_state.game_is_sprint())
    {
        exp_gained = sprint_modify_exp(exp_gained);
    }

    you.exp_available += exp_gained;
    autotrain();

    level_change();

    if (actual_gain != NULL)
        *actual_gain = you.experience - old_exp;

    if (actual_avail_gain != NULL)
        *actual_avail_gain = you.exp_available - old_avail;
}

static void _draconian_scale_colour_message()
{
    switch (you.species)
    {
    case SP_RED_DRACONIAN:
        mpr("Your scales start taking on a fiery red colour.",
            MSGCH_INTRINSIC_GAIN);
        perma_mutate(MUT_HEAT_RESISTANCE, 1);
        break;

    case SP_WHITE_DRACONIAN:
        mpr("Your scales start taking on an icy white colour.",
            MSGCH_INTRINSIC_GAIN);
        perma_mutate(MUT_COLD_RESISTANCE, 1);
        break;

    case SP_GREEN_DRACONIAN:
        mpr("Your scales start taking on a lurid green colour.",
            MSGCH_INTRINSIC_GAIN);
        perma_mutate(MUT_POISON_RESISTANCE, 1);
        break;

    case SP_YELLOW_DRACONIAN:
        mpr("Your scales start taking on a golden yellow colour.",
            MSGCH_INTRINSIC_GAIN);
        break;

    case SP_GREY_DRACONIAN:
        mpr("Your scales start taking on a dull grey colour.",
            MSGCH_INTRINSIC_GAIN);
        perma_mutate(MUT_UNBREATHING, 1);
        break;

    case SP_BLACK_DRACONIAN:
        mpr("Your scales start taking on a glossy black colour.",
            MSGCH_INTRINSIC_GAIN);
        perma_mutate(MUT_SHOCK_RESISTANCE, 1);
        break;

    case SP_PURPLE_DRACONIAN:
        mpr("Your scales start taking on a rich purple colour.",
            MSGCH_INTRINSIC_GAIN);
        break;

    case SP_MOTTLED_DRACONIAN:
        mpr("Your scales start taking on a weird mottled pattern.",
            MSGCH_INTRINSIC_GAIN);
        break;

    case SP_PALE_DRACONIAN:
        mpr("Your scales start fading to a pale cyan-grey colour.",
            MSGCH_INTRINSIC_GAIN);
        break;

    case SP_BASE_DRACONIAN:
        mpr("");
        break;

    default:
        break;
    }
}

static void _felid_extra_life()
{
    if (you.lives + you.deaths < you.max_level/3 && you.lives < 2)
    {
        you.lives++;
        mpr("Extra life!", MSGCH_INTRINSIC_GAIN);
    }
    // Should play the 1UP sound from SMB...
}

void level_change(bool skip_attribute_increase)
{
    const bool wiz_cmd = crawl_state.prev_cmd == CMD_WIZARD
                      || crawl_state.repeat_cmd == CMD_WIZARD;

    // necessary for the time being, as level_change() is called
    // directly sometimes {dlb}
    you.redraw_experience = true;

    while (you.experience < exp_needed(you.experience_level))
        lose_level();

    while (you.experience_level < 27
           && you.experience >= exp_needed(you.experience_level + 1))
    {
        if (!skip_attribute_increase && !wiz_cmd)
        {
            crawl_state.cancel_cmd_all();

            if (is_processing_macro())
                flush_input_buffer(FLUSH_ABORT_MACRO);
        }

        const int old_hp = you.hp;
        const int old_maxhp = you.hp_max;

        int hp_adjust = 0;
        int mp_adjust = 0;

        // [ds] Make sure we increment you.experience_level and apply
        // any stat/hp increases only after we've cleared all prompts
        // for this experience level. If we do part of the work before
        // the prompt, and a player on telnet gets disconnected, the
        // SIGHUP will save Crawl in the in-between state and rob the
        // player of their level-up perks.

        const int new_exp = you.experience_level + 1;

        if (new_exp <= you.max_level)
        {
            mprf(MSGCH_INTRINSIC_GAIN,
                 "Welcome back to level %d!", new_exp);

            // No more prompts for this XL past this point.

            you.experience_level = new_exp;
            // Gain back the hp and mp we lose in lose_level().  -- bwr
            inc_hp(4, true);
            inc_mp(1, true);
        }
        else  // Character has gained a new level
        {
            if (new_exp == 27)
            {
               mprf(MSGCH_INTRINSIC_GAIN,
                    "You have reached level 27, the final one!");
            }
            else
            {
               mprf(MSGCH_INTRINSIC_GAIN, "You have reached level %d!",
                    new_exp);
            }

            if (!(new_exp % 3) && !skip_attribute_increase)
                attribute_increase();

            int brek = 0;

            if (new_exp > 21)
                brek = 2 + new_exp % 2;
            else if (new_exp > 12)
                brek = 4;                  // up from 2 + rand(3) -- bwr
            else
                brek = 5 + new_exp % 2;    // up from 3 + rand(4) -- bwr

            you.experience_level = new_exp;
            inc_hp(brek, true);
            inc_mp(1, true);

            switch (you.species)
            {
            case SP_HUMAN:
                if (!(you.experience_level % 5))
                    modify_stat(STAT_RANDOM, 1, false, "level gain");
                break;

            case SP_HIGH_ELF:
                if (you.experience_level % 3)
                    hp_adjust--;

                if (!(you.experience_level % 2))
                    mp_adjust++;

                if (!(you.experience_level % 3))
                {
                    modify_stat((coinflip() ? STAT_INT
                                            : STAT_DEX), 1, false,
                                "level gain");
                }
                break;

            case SP_DEEP_ELF:
                if (you.experience_level < 17)
                    hp_adjust--;

                if (!(you.experience_level % 3))
                    hp_adjust--;

                mp_adjust++;

                if (!(you.experience_level % 4))
                    modify_stat(STAT_INT, 1, false, "level gain");
                break;

            case SP_SLUDGE_ELF:
                if (you.experience_level % 3)
                    hp_adjust--;
                else
                    mp_adjust++;

                if (!(you.experience_level % 4))
                {
                    modify_stat((coinflip() ? STAT_INT
                                            : STAT_DEX), 1, false,
                                "level gain");
                }
                break;

            case SP_MOUNTAIN_DWARF:
                if (!(you.experience_level % 2))
                    hp_adjust++;

                if (!(you.experience_level % 3))
                    mp_adjust--;

                if (!(you.experience_level % 4))
                    modify_stat(STAT_STR, 1, false, "level gain");
                break;

            case SP_DEEP_DWARF:
                hp_adjust++;

                if (you.experience_level == 14)
                {
                    mpr("You feel somewhat more resistant.",
                        MSGCH_INTRINSIC_GAIN);
                    perma_mutate(MUT_NEGATIVE_ENERGY_RESISTANCE, 1);
                }

                if ((you.experience_level == 9)
                    || (you.experience_level == 18))
                {
                    perma_mutate(MUT_PASSIVE_MAPPING, 1);
                }

                if (!(you.experience_level % 4))
                {
                    modify_stat(coinflip() ? STAT_STR
                                           : STAT_INT, 1, false,
                                "level gain");
                }
                break;

            case SP_HALFLING:
                if (!(you.experience_level % 5))
                    modify_stat(STAT_DEX, 1, false, "level gain");

                if (you.experience_level % 3)
                    hp_adjust--;

                break;

            case SP_KOBOLD:
                if (!(you.experience_level % 5))
                {
                    modify_stat((coinflip() ? STAT_STR
                                            : STAT_DEX), 1, false,
                                "level gain");
                }

                if (you.experience_level < 17)
                    hp_adjust--;

                if (!(you.experience_level % 2))
                    hp_adjust--;
                break;

            case SP_HILL_ORC:
                // lower because of HD raise -- bwr
                // if (you.experience_level < 17)
                //     hp_adjust++;

                if (!(you.experience_level % 2))
                    hp_adjust++;

                if (!(you.experience_level % 3))
                    mp_adjust--;

                if (!(you.experience_level % 5))
                    modify_stat(STAT_STR, 1, false, "level gain");
                break;

            case SP_MUMMY:
                if (you.experience_level == 13 || you.experience_level == 26)
                {
                    mpr("You feel more in touch with the powers of death.",
                        MSGCH_INTRINSIC_GAIN);
                }

                if (you.experience_level == 13)  // level 13 for now -- bwr
                {
                    mpr("You can now infuse your body with magic to restore "
                        "decomposition.", MSGCH_INTRINSIC_GAIN);
                }
                break;

            case SP_VAMPIRE:
                if (you.experience_level == 3)
                {
                    if (you.hunger_state > HS_SATIATED)
                    {
                        mpr("If you weren't so full you could now transform "
                            "into a vampire bat.", MSGCH_INTRINSIC_GAIN);
                    }
                    else
                    {
                        mpr("You can now transform into a vampire bat.",
                            MSGCH_INTRINSIC_GAIN);
                    }
                }
                else if (you.experience_level == 6)
                {
                    mpr("You can now bottle potions of blood from corpses.",
                        MSGCH_INTRINSIC_GAIN);
                }
                break;

            case SP_NAGA:
                hp_adjust++;

                if (!(you.experience_level % 4))
                    modify_stat(STAT_RANDOM, 1, false, "level gain");

                if (!(you.experience_level % 3))
                {
                    mpr("Your skin feels tougher.", MSGCH_INTRINSIC_GAIN);
                    you.redraw_armour_class = true;
                }
                break;

            case SP_TROLL:
                hp_adjust++;

                if (!(you.experience_level % 2))
                    hp_adjust++;

                if (you.experience_level % 3)
                    mp_adjust--;

                if (!(you.experience_level % 3))
                    modify_stat(STAT_STR, 1, false, "level gain");
                break;

            case SP_OGRE:
                hp_adjust += 2;

                if (!(you.experience_level % 3))
                    modify_stat(STAT_STR, 1, false, "level gain");
                break;

            case SP_BASE_DRACONIAN:
                if (you.experience_level >= 7)
                {
                    you.species = random_draconian_player_species();
                    update_player_symbol();
#ifdef USE_TILE
                    init_player_doll();
#endif
                    redraw_screen();
                    _draconian_scale_colour_message();

                    // We check if any skill has changed level because of
                    // changed aptitude
                    for (int i = SK_FIRST_SKILL; i < NUM_SKILLS; ++i)
                        check_skill_level_change(static_cast<skill_type>(i));
                }
            case SP_RED_DRACONIAN:
            case SP_WHITE_DRACONIAN:
            case SP_GREEN_DRACONIAN:
            case SP_YELLOW_DRACONIAN:
            case SP_GREY_DRACONIAN:
            case SP_BLACK_DRACONIAN:
            case SP_PURPLE_DRACONIAN:
            case SP_MOTTLED_DRACONIAN:
            case SP_PALE_DRACONIAN:
                if (!(you.experience_level % 3))
                    hp_adjust++;

                if (!(you.experience_level % 3))
                {
                    mpr("Your scales feel tougher.", MSGCH_INTRINSIC_GAIN);
                    you.redraw_armour_class = true;
                }

                if (!(you.experience_level % 4) && you.experience_level > 7)
                {
                    modify_stat(STAT_RANDOM, 1, false, "level gain");
                }

                if (you.experience_level == 14)
                {
                    switch (you.species)
                    {
                        case SP_GREEN_DRACONIAN:
                             perma_mutate(MUT_STINGER, 1);
                             break;
                        case SP_YELLOW_DRACONIAN:
                             perma_mutate(MUT_ACIDIC_BITE, 1);
                             break;
                        case SP_BLACK_DRACONIAN:
                             perma_mutate(MUT_BIG_WINGS, 1);
                             break;
                        default:
                             break;
                    }
                }
                break;

            case SP_CENTAUR:
                if (!(you.experience_level % 4))
                {
                    modify_stat((coinflip() ? STAT_STR
                                            : STAT_DEX), 1, false,
                                "level gain");
                }

                // lowered because of HD raise -- bwr
                // if (you.experience_level < 17)
                //     hp_adjust++;

                if (!(you.experience_level % 2))
                    hp_adjust++;

                if (!(you.experience_level % 3))
                    mp_adjust--;
                break;

            case SP_DEMIGOD:
                if (!(you.experience_level % 2))
                    modify_stat(STAT_RANDOM, 1, false, "level gain");

                // lowered because of HD raise -- bwr
                // if (you.experience_level < 17)
                //    hp_adjust++;

                if (!(you.experience_level % 2))
                    hp_adjust++;

                if (you.experience_level % 3)
                    mp_adjust++;
                break;

            case SP_SPRIGGAN:
                if (you.experience_level < 17)
                    hp_adjust--;

                if (you.experience_level % 3)
                    hp_adjust--;

                mp_adjust++;

                if (!(you.experience_level % 5))
                {
                    modify_stat((coinflip() ? STAT_INT
                                            : STAT_DEX), 1, false,
                                "level gain");
                }
                break;

            case SP_MINOTAUR:
                // lowered because of HD raise -- bwr
                // if (you.experience_level < 17)
                //     hp_adjust++;

                if (!(you.experience_level % 2))
                    hp_adjust++;

                if (!(you.experience_level % 2))
                    mp_adjust--;

                if (!(you.experience_level % 4))
                {
                    modify_stat((coinflip() ? STAT_STR
                                            : STAT_DEX), 1, false,
                                "level gain");
                }
                break;

            case SP_DEMONSPAWN:
            {
                bool gave_message = false;
                int level = 0;
                mutation_type first_body_facet = NUM_MUTATIONS;

                for (unsigned i = 0; i < you.demonic_traits.size(); ++i)
                {
                    if (is_body_facet(you.demonic_traits[i].mutation))
                    {
                        if (first_body_facet < NUM_MUTATIONS
                            && you.demonic_traits[i].mutation
                                != first_body_facet)
                        {
                            if (you.experience_level == level)
                            {
                                mpr("You feel monstrous as your "
                                     "demonic heritage exerts itself.",
                                     MSGCH_MUTATION);

                                mark_milestone("monstrous", "is a "
                                               "monstrous demonspawn!");
                            }

                            i = you.demonic_traits.size();
                            break;
                        }

                        if (first_body_facet == NUM_MUTATIONS)
                        {
                            first_body_facet = you.demonic_traits[i].mutation;
                            level = you.demonic_traits[i].level_gained;
                        }
                    }
                }

                for (unsigned i = 0; i < you.demonic_traits.size(); ++i)
                {
                    if (you.demonic_traits[i].level_gained
                        == you.experience_level)
                    {
                        if (!gave_message)
                        {
                            mpr("Your demonic ancestry asserts itself...",
                                MSGCH_INTRINSIC_GAIN);

                            gave_message = true;
                        }
                        perma_mutate(you.demonic_traits[i].mutation, 1);
                    }
                }

                if (!(you.experience_level % 4))
                    modify_stat(STAT_RANDOM, 1, false, "level gain");
                break;
            }

            case SP_GHOUL:
                // lowered because of HD raise -- bwr
                // if (you.experience_level < 17)
                //     hp_adjust++;

                if (!(you.experience_level % 2))
                    hp_adjust++;

                if (!(you.experience_level % 3))
                    mp_adjust--;

                if (!(you.experience_level % 5))
                    modify_stat(STAT_STR, 1, false, "level gain");
                break;

            case SP_KENKU:
                if (you.experience_level < 17)
                    hp_adjust--;

                if (!(you.experience_level % 3))
                    hp_adjust--;

                if (!(you.experience_level % 4))
                    modify_stat(STAT_RANDOM, 1, false, "level gain");

                if (you.experience_level == 5)
                {
                    mpr("You have gained the ability to fly.",
                        MSGCH_INTRINSIC_GAIN);
                }
                else if (you.experience_level == 15)
                    mpr("You can now fly continuously.", MSGCH_INTRINSIC_GAIN);
                break;

            case SP_MERFOLK:
                if (!(you.experience_level % 5))
                    modify_stat(STAT_RANDOM, 1, false, "level gain");
                break;

            case SP_CAT:
                hp_adjust--;
                hp_adjust--;

                if (you.experience_level % 2)
                    mp_adjust++;

                if (!(you.experience_level % 5))
                {
                    modify_stat((coinflip() ? STAT_INT
                                            : STAT_DEX), 1, false,
                                "level gain");
                }

                if (you.experience_level == 6 || you.experience_level == 12)
                    perma_mutate(MUT_SHAGGY_FUR, 1);

                _felid_extra_life();
                break;


            default:
                break;
            }
        }

        // zot defence abilities; must also be updated in abl-show.cc when these levels are changed
        if (crawl_state.game_is_zotdef())
        {
            if (you.experience_level == 1)
                mpr("Your Zot abilities now extend through the making of dart traps.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 2)
                mpr("Your Zot abilities now extend through the making of oklob saplings.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 3)
                mpr("Your Zot abilities now extend through the making of arrow traps.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 4)
                mpr("Your Zot abilities now extend through the making of plants.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 4)
                mpr("Your Zot abilities now extend through removing curses.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 5)
                mpr("Your Zot abilities now extend through the making of burning bushes.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 6)
                mpr("Your Zot abilities now extend through the making of altars and grenades.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 7)
                mpr("Your Zot abilities now extend through the making of oklob plants.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 8)
                mpr("Your Zot abilities now extend through the making of net traps.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 9)
                mpr("Your Zot abilities now extend through the making of ice statues.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 10)
                mpr("Your Zot abilities now extend through the making of spear traps.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 11)
                mpr("Your Zot abilities now extend through the making of alarm traps.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 12)
                mpr("Your Zot abilities now extend through the making of mushroom circles.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 13)
                mpr("Your Zot abilities now extend through the making of bolt traps.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 14)
                mpr("Your Zot abilities now extend through the making of orange crystal statues.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 15)
                mpr("Your Zot abilities now extend through the making of needle traps.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 16)
                mpr("Your Zot abilities now extend through self-teleportation.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 17)
                mpr("Your Zot abilities now extend through making water.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 18)
                mpr("Your Zot abilities now extend through the making of axe traps.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 19)
                mpr("Your Zot abilities now extend through the making of electric eels.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 20)
                mpr("Your Zot abilities now extend through the making of silver statues.", MSGCH_INTRINSIC_GAIN);
            // gold and bazaars gained together
            if (you.experience_level == 21)
                mpr("Your Zot abilities now extend through the making of bazaars.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 21)
                mpr("Your Zot abilities now extend through acquiring gold.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 22)
                mpr("Your Zot abilities now extend through the making of oklob circles.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 23)
                mpr("Your Zot abilities now extend through invoking Sage effects.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 24)
                mpr("Your Zot abilities now extend through acquirement.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 25)
                mpr("Your Zot abilities now extend through the making of blade traps.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 26)
                mpr("Your Zot abilities now extend through the making of curse skulls.", MSGCH_INTRINSIC_GAIN);
            if (you.experience_level == 27)
                mpr("Your Zot abilities now extend through the making of teleport traps.", MSGCH_INTRINSIC_GAIN);
        }

#if TAG_MAJOR_VERSION == 32
        note_montiers();
#endif
        // add hp and mp adjustments - GDL
        inc_max_hp(hp_adjust);
        inc_max_mp(mp_adjust);

        deflate_hp(you.hp_max, false);

        you.hp = old_hp * you.hp_max / old_maxhp;
        you.magic_points = std::max(0, you.magic_points);

        // Get "real" values for note-taking, i.e. ignore Berserk,
        // transformations or equipped items.
        const int note_maxhp = get_real_hp(false, false);
        const int note_maxmp = get_real_mp(false);

        char buf[200];
        sprintf(buf, "HP: %d/%d MP: %d/%d",
                std::min(you.hp, note_maxhp), note_maxhp,
                std::min(you.magic_points, note_maxmp), note_maxmp);
        take_note(Note(NOTE_XP_LEVEL_CHANGE, you.experience_level, 0, buf));

        // recalculate for game
        calc_hp();
        calc_mp();

        if (you.experience_level > you.max_level)
            you.max_level = you.experience_level;

        xom_is_stimulated(16);

        learned_something_new(HINT_NEW_LEVEL);
    }

    while (you.experience >= exp_needed(you.max_level + 1))
    {
        ASSERT(you.experience_level == 27);
        ASSERT(you.max_level < 127);
        you.max_level++;
        if (you.species == SP_CAT)
            _felid_extra_life();
    }

    redraw_skill(you.your_name, player_title());

    // Hints mode arbitrarily ends at xp 7.
    if (crawl_state.game_is_hints() && you.experience_level >= 7)
        hints_finished();
}

void adjust_level(int diff, bool just_xp)
{
    if (you.experience_level + diff < 1)
        you.experience = 0;
    else if (you.experience_level + diff >= 27)
        you.experience = std::max(you.experience, exp_needed(27));
    else
    {
        while (diff < 0 && you.experience >= exp_needed(27))
        {
            // Having XP for level 53 and going back to 26 due to a single
            // card would mean your felid is not going to get any extra lives
            // in foreseable future.
            you.experience -= exp_needed(27) - exp_needed(26);
            diff++;
        }
        int old_min = exp_needed(you.experience_level);
        int old_max = exp_needed(you.experience_level + 1);
        int new_min = exp_needed(you.experience_level + diff);
        int new_max = exp_needed(you.experience_level + 1 + diff);
        dprf("XP before: %d\n", you.experience);
        dprf("%4.2f of %d..%d to %d..%d",
             (you.experience - old_min) * 1.0 / (old_max - old_min),
             old_min, old_max, new_min, new_max);

        you.experience = ((uint64_t)(new_max - new_min))
                       * (you.experience - old_min)
                       / (old_max - old_min)
                       + new_min;
        dprf("XP after: %d\n", you.experience);
    }

    if (!just_xp)
        level_change();
}

// Here's a question for you: does the ordering of mods make a difference?
// (yes) -- are these things in the right order of application to stealth?
// - 12mar2000 {dlb}
int check_stealth(void)
{
    ASSERT(!crawl_state.game_is_arena());
#ifdef WIZARD
    // Extreme stealthiness can be enforced by wizmode stealth setting.
    if (you.skills[SK_STEALTH] > 27)
        return (1000);
#endif

    if (you.attribute[ATTR_SHADOWS] || you.berserk() || you.stat_zero[STAT_DEX])
        return (0);

    int stealth = you.dex() * 3;

    int race_mod = 0;
    if (player_genus(GENPC_DRACONIAN) && you.species != SP_GREY_DRACONIAN)
        race_mod = 12;
    else
    {
        switch (you.species) // why not use body_size here?
        {
        case SP_TROLL:
        case SP_OGRE:
        case SP_CENTAUR:
            race_mod = 9;
            break;
        case SP_MINOTAUR:
            race_mod = 12;
            break;
        case SP_VAMPIRE:
            // Thirsty/bat-form vampires are (much) more stealthy
            if (you.hunger_state == HS_STARVING)
                race_mod = 21;
            else if (player_in_bat_form()
                     || you.hunger_state <= HS_NEAR_STARVING)
            {
                race_mod = 20;
            }
            else if (you.hunger_state < HS_SATIATED)
                race_mod = 19;
            else
                race_mod = 18;
            break;
        case SP_HALFLING:
        case SP_KOBOLD:
        case SP_SPRIGGAN:
        case SP_GREY_DRACONIAN:
        case SP_NAGA:       // not small but very good at stealth
        case SP_CAT:
            race_mod = 18;
            break;
        default:
            race_mod = 15;
            break;
        }
    }

    switch(you.form)
    {
    case TRAN_SPIDER:
        race_mod = 21;
        break;
    case TRAN_ICE_BEAST:
        race_mod = 15;
        break;
    case TRAN_STATUE:
        race_mod -= 3; // depends on the base race
        break;
    case TRAN_DRAGON:
        race_mod = 6;
        break;
    case TRAN_PIG:
        race_mod = 9; // trotters, oinking...
        break;
    case TRAN_BAT:
        if (you.species != SP_VAMPIRE)
            race_mod = 17;
        break;
    case TRAN_BLADE_HANDS:
        if (you.species == SP_CAT && !you.airborne())
            stealth -= 50; // a constant penalty
        break;
    case TRAN_LICH:
        race_mod++; // intentionally tiny, lich form is already overpowered
        break;
    case TRAN_NONE:
        break;
    }

    stealth += you.skill(SK_STEALTH) * race_mod;

    if (you.burden_state > BS_UNENCUMBERED)
        stealth /= you.burden_state;

    if (you.confused())
        stealth /= 3;

    const item_def *arm = you.slot_item(EQ_BODY_ARMOUR, false);
    const item_def *cloak = you.slot_item(EQ_CLOAK, false);
    const item_def *boots = you.slot_item(EQ_BOOTS, false);

    if (arm)
    {
        // [ds] New stealth penalty formula from rob: SP = 6 * (EP^2)
        const int ep = -property(*arm, PARM_EVASION);
        const int penalty = 6 * ep * ep;
#if 0
        dprf("Stealth penalty for armour (ep: %d): %d", ep, penalty);
#endif
        stealth -= penalty;
    }

    if (cloak && get_equip_race(*cloak) == ISFLAG_ELVEN)
        stealth += 20;

    if (you.duration[DUR_STEALTH])
        stealth += 80;

    if (you.duration[DUR_AGILITY])
        stealth += 50;

    stealth += scan_artefacts(ARTP_STEALTH);

    if (you.airborne())
        stealth += 10;

    else if (you.in_water())
    {
        // Merfolk can sneak up on monsters underwater -- bwr
        if (you.fishtail)
            stealth += 50;
        else if (!you.can_swim() && !you.extra_balanced())
            stealth /= 2;       // splashy-splashy
    }

    // No stealth bonus from boots if you're airborne or in water
    else if (boots)
    {
        if (get_armour_ego_type(*boots) == SPARM_STEALTH)
            stealth += 50;

        if (get_equip_race(*boots) == ISFLAG_ELVEN)
            stealth += 20;
    }

    else if (you.species == SP_CAT && !you.form)
        stealth += 20;  // paws

    // Radiating silence is the negative complement of shouting all the
    // time... a sudden change from background noise to no noise is going
    // to clue anything in to the fact that something is very wrong...
    // a personal silence spell would naturally be different, but this
    // silence radiates for a distance and prevents monster spellcasting,
    // which pretty much gives away the stealth game.
    // this penalty is dependent on the actual amount of ambient noise
    // in the level -doy
    if (you.duration[DUR_SILENCE])
        stealth -= 50 + current_level_ambient_noise();

    // Mutations.
    stealth += 25 * player_mutation_level(MUT_THIN_SKELETAL_STRUCTURE);
    stealth += 40 * player_mutation_level(MUT_NIGHTSTALKER);
    if (player_mutation_level(MUT_TRANSLUCENT_SKIN) > 1)
        stealth += 20 * (player_mutation_level(MUT_TRANSLUCENT_SKIN) - 1);

    // it's easier to be stealthy when there's a lot of background noise
    stealth += 2 * current_level_ambient_noise();

    stealth = std::max(0, stealth);

    return (stealth);
}

// Returns the medium duration value which is usually announced by a special
// message ("XY is about to time out") or a change of colour in the
// status display.
// Note that these values cannot be relied on when playing since there are
// random decrements precisely to avoid this.
int get_expiration_threshold(duration_type dur)
{
    switch (dur)
    {
    case DUR_QUAD_DAMAGE:
        return (3 * BASELINE_DELAY); // per client.qc

    case DUR_FIRE_SHIELD:
    case DUR_SILENCE: // no message
        return (5 * BASELINE_DELAY);

    case DUR_DEFLECT_MISSILES:
    case DUR_REPEL_MISSILES:
    case DUR_REGENERATION:
    case DUR_INSULATION:
    case DUR_RESIST_POISON:
    case DUR_RESIST_FIRE:
    case DUR_RESIST_COLD:
    case DUR_SWIFTNESS:
    case DUR_INVIS:
    case DUR_HASTE:
    case DUR_BERSERK:
    case DUR_ICY_ARMOUR:
    case DUR_PHASE_SHIFT:
    case DUR_CONTROL_TELEPORT:
    case DUR_DEATH_CHANNEL:
    case DUR_SEE_INVISIBLE:
        return (6 * BASELINE_DELAY);

    case DUR_LEVITATION:
    case DUR_TRANSFORMATION: // not on status
    case DUR_DEATHS_DOOR:    // not on status
    case DUR_SLIMIFY:
        return (10 * BASELINE_DELAY);

    // These get no messages when they "flicker".
    case DUR_SAGE:
    case DUR_BARGAIN:
        return (15 * BASELINE_DELAY);

    case DUR_CONFUSING_TOUCH:
        return (20 * BASELINE_DELAY);

    default:
        return (0);
    }
}

// Is a given duration about to expire?
bool dur_expiring(duration_type dur)
{
    const int value = you.duration[dur];
    if (value <= 0)
        return (false);

    return (value <= get_expiration_threshold(dur));
}

static void _output_expiring_message(duration_type dur, const char* msg)
{
    if (you.duration[dur])
    {
        const bool expires = dur_expiring(dur);
        mprf("%s%s", expires ? "Expiring: " : "", msg);
    }
}

static void _display_vampire_status()
{
    std::string msg = "At your current hunger state you ";
    std::vector<std::string> attrib;

    switch (you.hunger_state)
    {
        case HS_STARVING:
            attrib.push_back("resist poison");
            attrib.push_back("significantly resist cold");
            attrib.push_back("strongly resist negative energy");
            attrib.push_back("resist torment");
            attrib.push_back("do not heal.");
            break;
        case HS_NEAR_STARVING:
            attrib.push_back("resist poison");
            attrib.push_back("significantly resist cold");
            attrib.push_back("strongly resist negative energy");
            attrib.push_back("have an extremely slow metabolism");
            attrib.push_back("heal slowly.");
            break;
        case HS_HUNGRY:
        case HS_VERY_HUNGRY:
            attrib.push_back("resist poison");
            attrib.push_back("resist cold");
            attrib.push_back("significantly resist negative energy");
            if (you.hunger_state == HS_HUNGRY)
                attrib.push_back("have a slow metabolism");
            else
                attrib.push_back("have a very slow metabolism");
            attrib.push_back("heal slowly.");
            break;
        case HS_SATIATED:
            attrib.push_back("resist negative energy.");
            break;
        case HS_FULL:
            attrib.push_back("have a fast metabolism");
            attrib.push_back("heal quickly.");
            break;
        case HS_VERY_FULL:
            attrib.push_back("have a very fast metabolism");
            attrib.push_back("heal quickly.");
            break;
        case HS_ENGORGED:
            attrib.push_back("have an extremely fast metabolism");
            attrib.push_back("heal extremely quickly.");
            break;
    }

    if (!attrib.empty())
    {
        msg += comma_separated_line(attrib.begin(), attrib.end());
        mpr(msg.c_str());
    }
}

static void _display_movement_speed()
{
    const int move_cost = (player_speed() * player_movement_speed()) / 10;

    const bool water  = you.in_water();
    const bool swim   = you.swimming();

    const bool lev    = you.airborne();
    const bool fly    = (you.flight_mode() == FL_FLY);
    const bool swift  = (you.duration[DUR_SWIFTNESS] > 0);

    mprf("Your %s speed is %s%s%s.",
          // order is important for these:
          (swim)    ? "swimming" :
          (water)   ? "wading" :
          (fly)     ? "flying" :
          (lev)     ? "levitating"
                    : "movement",

          (water && !swim)  ? "uncertain and " :
          (!water && swift) ? "aided by the wind" : "",

          (!water && swift) ? ((move_cost >= 10) ? ", but still "
                                                 : " and ")
                            : "",

          (move_cost <   8) ? "very quick" :
          (move_cost <  10) ? "quick" :
          (move_cost == 10) ? "average" :
          (move_cost <  13) ? "slow"
                            : "very slow");
}

static void _display_tohit()
{
    const int to_hit = calc_your_to_hit(false) * 2;
    dprf("To-hit: %d", to_hit);
/*
    // Messages based largely on percentage chance of missing the
    // average EV 10 humanoid, and very agile EV 30 (pretty much
    // max EV for monsters currently).
    //
    // "awkward"    - need lucky hit (less than EV)
    // "difficult"  - worse than 2 in 3
    // "hard"       - worse than fair chance
    mprf("%s given your current equipment.",
         (to_hit <   1) ? "You are completely incapable of fighting" :
         (to_hit <   5) ? "Hitting even clumsy monsters is extremely awkward" :
         (to_hit <  10) ? "Hitting average monsters is awkward" :
         (to_hit <  15) ? "Hitting average monsters is difficult" :
         (to_hit <  20) ? "Hitting average monsters is hard" :
         (to_hit <  30) ? "Very agile monsters are a bit awkward to hit" :
         (to_hit <  45) ? "Very agile monsters are a bit difficult to hit" :
         (to_hit <  60) ? "Very agile monsters are a bit hard to hit" :
         (to_hit < 100) ? "You feel comfortable with your ability to fight"
                        : "You feel confident with your ability to fight");
*/
}

static std::string _attack_delay_desc(int attack_delay)
{
    return ((attack_delay >= 200) ? "extremely slow" :
            (attack_delay >= 155) ? "very slow" :
            (attack_delay >= 125) ? "quite slow" :
            (attack_delay >= 105) ? "below average" :
            (attack_delay >=  95) ? "average" :
            (attack_delay >=  75) ? "above average" :
            (attack_delay >=  55) ? "quite fast" :
            (attack_delay >=  45) ? "very fast" :
            (attack_delay >=  35) ? "extremely fast" :
                                    "blindingly fast");
}

static void _display_attack_delay()
{
    const random_var delay = calc_your_attack_delay();

    // Scale to fit the displayed weapon base delay, i.e.,
    // normal speed is 100 (as in 100%).
    // We could also compute the variance if desired.
    int avg;
    const item_def* weapon = you.weapon();
    if (weapon && is_range_weapon(*weapon))
        avg = launcher_final_speed(*weapon, you.shield());
    else
        avg = static_cast<int>(round(10 * delay.expected()));

    // Haste wasn't counted here, but let's show finesse.
    // Can't be done in the above function because of interactions with
    // haste and caps.
    if (you.duration[DUR_FINESSE])
        avg = std::max(20, avg / 2);

    std::string msg = "Your attack speed is " + _attack_delay_desc(avg) + ".";

#ifdef DEBUG_DIAGNOSTICS
    if (you.wizard)
    {
        const int max = 10 * delay.max();

        msg += colour_string(make_stringf(" %d%% (max %d%%)", avg, max),
                             channel_to_colour(MSGCH_DIAGNOSTICS));
    }
#endif

    mpr(msg);
}

void display_char_status()
{
    if (you.is_undead == US_SEMI_UNDEAD && you.hunger_state == HS_ENGORGED)
        mpr("You feel almost alive.");
    else if (you.is_undead)
        mpr("You are undead.");
    else if (you.duration[DUR_DEATHS_DOOR])
    {
        _output_expiring_message(DUR_DEATHS_DOOR,
                                 "You are standing in death's doorway.");
    }
    else
        mpr("You are alive.");

    const int halo_size = you.halo_radius2();
    if (halo_size >= 0)
    {
        if (halo_size > 37)
            mpr("You are illuminated by a large divine halo.");
        else if (halo_size > 10)
            mpr("You are illuminated by a divine halo.");
        else
            mpr("You are illuminated by a small divine halo.");
    }
    else if (you.haloed())
        mpr("An external divine halo illuminates you.");

    if (you.species == SP_VAMPIRE)
        _display_vampire_status();

    static int statuses[] = {
        STATUS_STR_ZERO, STATUS_INT_ZERO, STATUS_DEX_ZERO,
        DUR_TRANSFORMATION, STATUS_BURDEN, DUR_SAGE, DUR_BARGAIN,
        DUR_BREATH_WEAPON, DUR_LIQUID_FLAMES, DUR_FIRE_SHIELD, DUR_ICY_ARMOUR,
        DUR_REPEL_MISSILES, DUR_DEFLECT_MISSILES, DUR_JELLY_PRAYER,
        STATUS_REGENERATION, DUR_SWIFTNESS, DUR_RESIST_POISON,
        DUR_RESIST_COLD, DUR_RESIST_FIRE, DUR_INSULATION, DUR_TELEPORT,
        DUR_CONTROL_TELEPORT, DUR_DEATH_CHANNEL, DUR_PHASE_SHIFT, DUR_SILENCE,
        DUR_STONESKIN, DUR_SEE_INVISIBLE, DUR_INVIS, DUR_CONF, STATUS_BEHELD,
        DUR_PARALYSIS, DUR_PETRIFIED, DUR_SLEEP, DUR_EXHAUSTED,
        STATUS_SPEED, DUR_MIGHT, DUR_BRILLIANCE, DUR_AGILITY,
        DUR_DIVINE_VIGOUR, DUR_DIVINE_STAMINA, DUR_BERSERK,
        STATUS_AIRBORNE, STATUS_NET, DUR_POISONING, STATUS_SICK,
        STATUS_ROT, STATUS_GLOW, DUR_CONFUSING_TOUCH, DUR_SURE_BLADE,
        DUR_AFRAID, DUR_MIRROR_DAMAGE, DUR_SCRYING, STATUS_CLINGING,
    };

    status_info inf;
    for (unsigned i = 0; i < ARRAYSZ(statuses); ++i)
    {
        fill_status_info(statuses[i], &inf);
        if (!inf.long_text.empty())
            mpr(inf.long_text);
    }

    _display_movement_speed();
    _display_tohit();
    _display_attack_delay();

    // magic resistance
    mprf("You are %s resistant to hostile enchantments.",
         magic_res_adjective(player_res_magic(false)).c_str());

    // character evaluates their ability to sneak around:
    mprf("You feel %s.", stealth_desc(check_stealth()).c_str());
    dprf("stealth: %d", check_stealth());
}

bool player_item_conserve(bool calc_unid)
{
    return (player_equip(EQ_AMULET, AMU_CONSERVATION, calc_unid)
            || player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_PRESERVATION));
}

int player_mental_clarity(bool calc_unid, bool items)
{
    int ret = 3 * player_equip(EQ_AMULET, AMU_CLARITY, calc_unid) * items
              + player_mutation_level(MUT_CLARITY);

    if (you.religion == GOD_ASHENZARI && you.piety >= piety_breakpoint(2)
        && !player_under_penance())
    {
        ret++;
    }

    return ((ret > 3) ? 3 : ret);
}

int player_spirit_shield(bool calc_unid)
{
    return player_equip(EQ_AMULET, AMU_GUARDIAN_SPIRIT, calc_unid)
           + player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_SPIRIT_SHIELD);
}

// Returns whether the player has the effect of the amulet from a
// non-amulet source.
bool extrinsic_amulet_effect(jewellery_type amulet)
{
    switch (amulet)
    {
    case AMU_CONTROLLED_FLIGHT:
        return (you.duration[DUR_CONTROLLED_FLIGHT]
                || player_genus(GENPC_DRACONIAN)
                || (you.species == SP_KENKU && you.experience_level >= 5)
                || you.form == TRAN_DRAGON
                || you.form == TRAN_BAT);
    case AMU_CLARITY:
        return player_mental_clarity(true, false);
    case AMU_RESIST_CORROSION:
        if (you.religion == GOD_JIYVA && you.piety >= piety_breakpoint(2))
            return (true);
        // else fall-through
    case AMU_CONSERVATION:
        return (player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_PRESERVATION));
    case AMU_THE_GOURMAND:
        return (player_mutation_level(MUT_GOURMAND) > 0);
    default:
        return (false);
    }
}

bool wearing_amulet(jewellery_type amulet, bool calc_unid, bool ignore_extrinsic)
{
    if (!ignore_extrinsic && extrinsic_amulet_effect(amulet))
        return (true);

    if (!player_wearing_slot(EQ_AMULET))
        return (false);

    const item_def& amu(you.inv[you.equip[EQ_AMULET]]);
    return (amu.sub_type == amulet && (calc_unid || item_type_known(amu)));
}

static int _species_exp_mod(species_type species)
{
    if (player_genus(GENPC_DRACONIAN, species))
        return 14;
    else if (player_genus(GENPC_DWARVEN, species))
        return 13;
    switch (species)
    {
        case SP_HUMAN:
        case SP_HALFLING:
        case SP_HILL_ORC:
        case SP_KOBOLD:
            return 10;
        case SP_OGRE:
            return 11;
        case SP_SLUDGE_ELF:
        case SP_NAGA:
        case SP_GHOUL:
        case SP_MERFOLK:
            return 12;
        case SP_SPRIGGAN:
        case SP_KENKU:
            return 13;
        case SP_DEEP_ELF:
        case SP_CENTAUR:
        case SP_MINOTAUR:
        case SP_MUMMY:
            return 14;
        case SP_HIGH_ELF:
        case SP_VAMPIRE:
        case SP_TROLL:
        case SP_DEMONSPAWN:
            return 15;
        case SP_DEMIGOD:
        case SP_CAT:
            return 16;
        default:
            return 0;
    }
}

unsigned int exp_needed(int lev)
{
    unsigned int level = 0;

    // Basic plan:
    // Section 1: levels  1- 5, second derivative goes 10-10-20-30.
    // Section 2: levels  6-13, second derivative is exponential/doubling.
    // Section 3: levels 14-27, second derivative is constant at 6000.
    //
    // Section three is constant so we end up with high levels at about
    // their old values (level 27 at 850k), without delta2 ever decreasing.
    // The values that are considerably different (ie level 13 is now 29000,
    // down from 41040 are because the second derivative goes from 9040 to
    // 1430 at that point in the original, and then slowly builds back
    // up again).  This function smoothes out the old level 10-15 area
    // considerably.

    // Here's a table:
    //
    // level      xp      delta   delta2
    // =====   =======    =====   ======
    //   1           0        0       0
    //   2          10       10      10
    //   3          30       20      10
    //   4          70       40      20
    //   5         140       70      30
    //   6         270      130      60
    //   7         520      250     120
    //   8        1010      490     240
    //   9        1980      970     480
    //  10        3910     1930     960
    //  11        7760     3850    1920
    //  12       15450     7690    3840
    //  13       29000    13550    5860
    //  14       48500    19500    5950
    //  15       74000    25500    6000
    //  16      105500    31500    6000
    //  17      143000    37500    6000
    //  18      186500    43500    6000
    //  19      236000    49500    6000
    //  20      291500    55500    6000
    //  21      353000    61500    6000
    //  22      420500    67500    6000
    //  23      494000    73500    6000
    //  24      573500    79500    6000
    //  25      659000    85500    6000
    //  26      750500    91500    6000
    //  27      848000    97500    6000


    switch (lev)
    {
    case 1:
        level = 1;
        break;
    case 2:
        level = 10;
        break;
    case 3:
        level = 30;
        break;
    case 4:
        level = 70;
        break;

    default:
        if (lev < 13)
        {
            lev -= 4;
            level = 10 + 10 * lev + (60 << lev);
        }
        else
        {
            lev -= 12;
            level = 15500 + 10500 * lev + 3000 * lev * lev;
        }
        break;
    }

    return ((level - 1) * _species_exp_mod(you.species) / 10);
}

// returns bonuses from rings of slaying, etc.
int slaying_bonus(weapon_property_type which_affected, bool ranged)
{
    int ret = 0;

    if (which_affected == PWPN_HIT)
    {
        ret += player_equip(EQ_RINGS_PLUS, RING_SLAYING);
        ret += scan_artefacts(ARTP_ACCURACY);
        if (player_equip_ego_type(EQ_GLOVES, SPARM_ARCHERY))
            ret += ranged ? 5 : -1;
    }
    else if (which_affected == PWPN_DAMAGE)
    {
        ret += player_equip(EQ_RINGS_PLUS2, RING_SLAYING);
        ret += scan_artefacts(ARTP_DAMAGE);
        if (player_equip_ego_type(EQ_GLOVES, SPARM_ARCHERY))
            ret += ranged ? 3 : -1;
    }

    ret += std::min(you.duration[DUR_SLAYING] / (13 * BASELINE_DELAY), 6);

    return (ret);
}

// Checks each equip slot for an evokable item (jewellery or randart).
// Returns true if any of these has the same ability as the one handed in.
bool items_give_ability(const int slot, artefact_prop_type abil)
{
    for (int i = EQ_WEAPON; i < NUM_EQUIP; i++)
    {
        if (!player_wearing_slot(i))
            continue;

        const int eq = you.equip[i];

        // skip item to compare with
        if (eq == slot)
            continue;

        // only weapons give their effects when in our hands
        if (i == EQ_WEAPON && you.inv[ eq ].base_type != OBJ_WEAPONS)
            continue;

        if (eq == EQ_LEFT_RING || eq == EQ_RIGHT_RING)
        {
            if (abil == ARTP_LEVITATE && you.inv[eq].sub_type == RING_LEVITATION)
                return (true);
            if (abil == ARTP_INVISIBLE && you.inv[eq].sub_type == RING_INVISIBILITY)
                return (true);
        }
        else if (eq == EQ_AMULET)
        {
            if (abil == ARTP_BERSERK && you.inv[eq].sub_type == AMU_RAGE)
                return (true);
        }

        // other items are not evokable
        if (!is_artefact(you.inv[ eq ]))
            continue;

        if (artefact_wpn_property(you.inv[ eq ], abil))
            return (true);
    }

    // none of the equipped items possesses this ability
    return (false);
}

// Checks each equip slot for a randart, and adds up all of those with
// a given property. Slow if any randarts are worn, so avoid where
// possible.
int scan_artefacts(artefact_prop_type which_property, bool calc_unid)
{
    int retval = 0;

    for (int i = EQ_WEAPON; i < NUM_EQUIP; ++i)
    {
        if (you.melded[i] && which_property != ARTP_PONDEROUS
            || you.equip[i] == -1)
        {
            continue;
        }

        const int eq = you.equip[i];

        // Only weapons give their effects when in our hands.
        if (i == EQ_WEAPON && you.inv[ eq ].base_type != OBJ_WEAPONS)
            continue;

        if (!is_artefact(you.inv[ eq ]))
            continue;

        // Ignore unidentified items [TileCrawl dump enhancements].
        if (!item_ident(you.inv[ eq ], ISFLAG_KNOW_PROPERTIES)
            && !calc_unid)
        {
            continue;
        }

        retval += artefact_wpn_property(you.inv[ eq ], which_property);
    }

    return (retval);
}

void dec_hp(int hp_loss, bool fatal, const char *aux)
{
    ASSERT(!crawl_state.game_is_arena());

    if (!fatal && you.hp < 1)
        you.hp = 1;

    if (!fatal && hp_loss >= you.hp)
        hp_loss = you.hp - 1;

    if (hp_loss < 1)
        return;

    // If it's not fatal, use ouch() so that notes can be taken. If it IS
    // fatal, somebody else is doing the bookkeeping, and we don't want to mess
    // with that.
    if (!fatal && aux)
        ouch(hp_loss, NON_MONSTER, KILLED_BY_SOMETHING, aux);
    else
        you.hp -= hp_loss;

    you.redraw_hit_points = true;
}

void dec_mp(int mp_loss)
{
    ASSERT(!crawl_state.game_is_arena());

    if (mp_loss < 1)
        return;

    you.magic_points -= mp_loss;

    you.magic_points = std::max(0, you.magic_points);

    if (Options.magic_point_warning
        && you.magic_points < (you.max_magic_points
                               * Options.magic_point_warning) / 100)
    {
        mpr("* * * LOW MAGIC WARNING * * *", MSGCH_DANGER);
    }

    take_note(Note(NOTE_MP_CHANGE, you.magic_points, you.max_magic_points));
    you.redraw_magic_points = true;
}

bool enough_hp(int minimum, bool suppress_msg)
{
    ASSERT(!crawl_state.game_is_arena());

    // We want to at least keep 1 HP. -- bwr
    if (you.hp < minimum + 1)
    {
        if (!suppress_msg)
            mpr("You haven't enough vitality at the moment.");

        crawl_state.cancel_cmd_again();
        crawl_state.cancel_cmd_repeat();
        return (false);
    }

    return (true);
}

bool enough_mp(int minimum, bool suppress_msg, bool include_items)
{
    ASSERT(!crawl_state.game_is_arena());

    bool rc = false;

    if (get_real_mp(include_items) < minimum)
    {
        if (!suppress_msg)
            mpr("You haven't enough magic capacity.");
    }
    else if (you.magic_points < minimum)
    {
        if (!suppress_msg)
            mpr("You haven't enough magic at the moment.");
    }
    else
        rc = true;

    if (!rc)
    {
        crawl_state.cancel_cmd_again();
        crawl_state.cancel_cmd_repeat();
    }

    return (rc);
}

bool enough_xp(int minimum, bool suppress_msg)
{
    ASSERT(!crawl_state.game_is_arena());

    if (you.exp_available < minimum)
    {
        if (!suppress_msg)
            mpr("You haven't enough experience in your experience pool.");

        crawl_state.cancel_cmd_again();
        crawl_state.cancel_cmd_repeat();
        return (false);
    }
    return (true);
}

// Note that "max_too" refers to the base potential, the actual
// resulting max value is subject to penalties, bonuses, and scalings.
void inc_mp(int mp_gain, bool max_too)
{
    ASSERT(!crawl_state.game_is_arena());

    if (mp_gain < 1)
        return;

    bool wasnt_max = (you.magic_points < you.max_magic_points);

    you.magic_points += mp_gain;

    if (max_too)
        inc_max_mp(mp_gain);

    if (you.magic_points > you.max_magic_points)
        you.magic_points = you.max_magic_points;

    if (wasnt_max && you.magic_points == you.max_magic_points)
        interrupt_activity(AI_FULL_MP);

    take_note(Note(NOTE_MP_CHANGE, you.magic_points, you.max_magic_points));
    you.redraw_magic_points = true;
}

// Note that "max_too" refers to the base potential, the actual
// resulting max value is subject to penalties, bonuses, and scalings.
// To avoid message spam, don't take notes when HP increases.
void inc_hp(int hp_gain, bool max_too)
{
    ASSERT(!crawl_state.game_is_arena());

    if (hp_gain < 1)
        return;

    bool wasnt_max = (you.hp < you.hp_max);

    you.hp += hp_gain;

    if (max_too)
        inc_max_hp(hp_gain);

    if (you.hp > you.hp_max)
        you.hp = you.hp_max;

    if (wasnt_max && you.hp == you.hp_max)
        interrupt_activity(AI_FULL_HP);

    you.redraw_hit_points = true;
}

void rot_hp(int hp_loss)
{
    you.base_hp -= hp_loss;
    calc_hp();

    if (you.species != SP_GHOUL)
        xom_is_stimulated(hp_loss * 32);

    you.redraw_hit_points = true;
}

void unrot_hp(int hp_recovered)
{
    if (hp_recovered >= 5000 - you.base_hp)
        you.base_hp = 5000;
    else
        you.base_hp += hp_recovered;

    calc_hp();

    you.redraw_hit_points = true;
}

int player_rotted()
{
    return (5000 - you.base_hp);
}

void rot_mp(int mp_loss)
{
    you.base_magic_points -= mp_loss;
    calc_mp();

    you.redraw_magic_points = true;
}

void inc_max_hp(int hp_gain)
{
    you.base_hp2 += hp_gain;
    calc_hp();

    take_note(Note(NOTE_MAXHP_CHANGE, you.hp_max));
    you.redraw_hit_points = true;
}

void dec_max_hp(int hp_loss)
{
    you.base_hp2 -= hp_loss;
    calc_hp();

    take_note(Note(NOTE_MAXHP_CHANGE, you.hp_max));
    you.redraw_hit_points = true;
}

void inc_max_mp(int mp_gain)
{
    you.base_magic_points2 += mp_gain;
    calc_mp();

    take_note(Note(NOTE_MAXMP_CHANGE, you.max_magic_points));
    you.redraw_magic_points = true;
}

void dec_max_mp(int mp_loss)
{
    you.base_magic_points2 -= mp_loss;
    calc_mp();

    take_note(Note(NOTE_MAXMP_CHANGE, you.max_magic_points));
    you.redraw_magic_points = true;
}

// Use of floor: false = hp max, true = hp min. {dlb}
void deflate_hp(int new_level, bool floor)
{
    ASSERT(!crawl_state.game_is_arena());

    if (floor && you.hp < new_level)
        you.hp = new_level;
    else if (!floor && you.hp > new_level)
        you.hp = new_level;

    // Must remain outside conditional, given code usage. {dlb}
    you.redraw_hit_points = true;
}

// Note that "max_too" refers to the base potential, the actual
// resulting max value is subject to penalties, bonuses, and scalings.
void set_hp(int new_amount, bool max_too)
{
    ASSERT(!crawl_state.game_is_arena());

    you.hp = new_amount;

    if (max_too && you.hp_max != new_amount)
    {
        you.base_hp2 = 5000 + new_amount;
        calc_hp();
    }

    if (you.hp > you.hp_max)
        you.hp = you.hp_max;

    if (max_too)
        take_note(Note(NOTE_MAXHP_CHANGE, you.hp_max));

    // Must remain outside conditional, given code usage. {dlb}
    you.redraw_hit_points = true;
}

// Note that "max_too" refers to the base potential, the actual
// resulting max value is subject to penalties, bonuses, and scalings.
void set_mp(int new_amount, bool max_too)
{
    ASSERT(!crawl_state.game_is_arena());

    you.magic_points = new_amount;

    if (max_too && you.max_magic_points != new_amount)
    {
        // Note that this gets scaled down for values > 18.
        you.base_magic_points2 = 5000 + new_amount;
        calc_mp();
    }

    if (you.magic_points > you.max_magic_points)
        you.magic_points = you.max_magic_points;

    take_note(Note(NOTE_MP_CHANGE, you.magic_points, you.max_magic_points));
    if (max_too)
        take_note(Note(NOTE_MAXMP_CHANGE, you.max_magic_points));

    // Must remain outside conditional, given code usage. {dlb}
    you.redraw_magic_points = true;
}

// If trans is true, being berserk and/or transformed is taken into account
// here. Else, the base hp is calculated. If rotted is true, calculate the
// real max hp you'd have if the rotting was cured.
int get_real_hp(bool trans, bool rotted)
{
    int hitp;

    hitp  = (you.base_hp - 5000) + (you.base_hp2 - 5000);
    // Important: we shouldn't add Heroism boosts here.
    hitp += (you.experience_level * you.skills[SK_FIGHTING]) /
            (you.species == SP_CAT ? 8 : 5);

    // Being berserk makes you resistant to damage. I don't know why.
    if (trans && you.berserk())
        hitp = hitp * 3 / 2;

    if (trans)
    {
        // Some transformations give you extra hp.
        switch (you.form)
        {
        case TRAN_STATUE:
            hitp *= 15;
            hitp /= 10;
            break;
        case TRAN_ICE_BEAST:
            hitp *= 12;
            hitp /= 10;
            break;
        case TRAN_DRAGON:
            hitp *= 16;
            hitp /= 10;
            break;
        default:
            break;
        }
    }

    if (rotted)
        hitp += player_rotted();

    // Frail and robust mutations, divine vigour, and rugged scale mut.
    hitp *= 100 + (player_mutation_level(MUT_ROBUST) * 10)
                + (you.attribute[ATTR_DIVINE_VIGOUR] * 5)
                + (player_mutation_level(MUT_RUGGED_BROWN_SCALES) ? player_mutation_level(MUT_RUGGED_BROWN_SCALES) * 2 + 1 : 0)
                - (player_mutation_level(MUT_FRAIL) * 10);
    hitp /= 100;

    return (hitp);
}

int get_real_mp(bool include_items)
{
    // base_magic_points2 accounts for species
    int enp = (you.base_magic_points2 - 5000);

    int spell_extra = (you.experience_level * you.skills[SK_SPELLCASTING]) / 4;
    int invoc_extra = (you.experience_level * you.skills[SK_INVOCATIONS]) / 6;

    enp += std::max(spell_extra, invoc_extra);
    enp = stepdown_value(enp, 9, 18, 45, 100);

    // This is our "rotted" base (applied after scaling):
    enp += (you.base_magic_points - 5000);

    // Yes, we really do want this duplication... this is so the stepdown
    // doesn't truncate before we apply the rotted base.  We're doing this
    // the nice way. -- bwr
    enp = std::min(enp, 50);

    // Now applied after scaling so that power items are more useful -- bwr
    if (include_items)
        enp += player_magical_power();

    // Analogous to ROBUST/FRAIL
    enp *= 100 + (player_mutation_level(MUT_HIGH_MAGIC) * 10)
               + (you.attribute[ATTR_DIVINE_VIGOUR] * 5)
               - (player_mutation_level(MUT_LOW_MAGIC) * 10);
    enp /= 100;

    if (enp > 50)
        enp = 50 + ((enp - 50) / 2);

    if (include_items && player_equip_ego_type(EQ_WEAPON, SPWPN_ANTIMAGIC))
        enp /= 3;

    enp = std::max(enp, 0);

    return enp;
}

int get_contamination_level()
{
    const int glow = you.magic_contamination;

    if (glow > 60)
        return (glow / 20 + 3);
    if (glow > 40)
        return (5);
    if (glow > 25)
        return (4);
    if (glow > 15)
        return (3);
    if (glow > 5)
        return (2);
    if (glow > 0)
        return (1);

    return (0);
}

std::string describe_contamination(int cont)
{
    if (cont > 5)
        return ("You are engulfed in a nimbus of crackling magics!");
    else if (cont == 5)
        return ("Your entire body has taken on an eerie glow!");
    else if (cont > 1)
    {
        return (make_stringf("You are %s with residual magics%s",
                   (cont == 4) ? "practically glowing" :
                   (cont == 3) ? "heavily infused" :
                   (cont == 2) ? "contaminated"
                                    : "lightly contaminated",
                   (cont == 4) ? "!" : "."));
    }
    else if (cont == 1)
        return ("You are very lightly contaminated with residual magic.");
    else
        return ("");
}

// controlled is true if the player actively did something to cause
// contamination (such as drink a known potion of resistance),
// status_only is true only for the status output
void contaminate_player(int change, bool controlled, bool msg)
{
    ASSERT(!crawl_state.game_is_arena());

    int old_amount = you.magic_contamination;
    int old_level  = get_contamination_level();
    int new_level  = 0;

    you.magic_contamination =
        std::max(0, std::min(250, you.magic_contamination + change));

    new_level = get_contamination_level();

    if (you.magic_contamination != old_amount)
        dprf("change: %d  radiation: %d", change, you.magic_contamination);

    if (msg && new_level >= 1 && old_level <= 1 && new_level != old_level)
        mpr(describe_contamination(new_level));
    else if (msg && new_level != old_level)
    {
        if (old_level == 1 && new_level == 0)
            mpr("Your magical contamination has completely faded away.");
        else
        {
            mprf((change > 0) ? MSGCH_WARN : MSGCH_RECOVERY,
                 "You feel %s contaminated with magical energies.",
                 (change > 0) ? "more" : "less");
        }

        if (change > 0)
            xom_is_stimulated(new_level * 32);

        if (old_level > 1 && new_level <= 1
            && you.duration[DUR_INVIS] && !you.backlit())
        {
            mpr("You fade completely from view now that you are no longer "
                "glowing from magical contamination.");
        }
    }

    if (you.magic_contamination > 0)
        learned_something_new(HINT_GLOWING);

    // Zin doesn't like mutations or mutagenic radiation.
    if (you.religion == GOD_ZIN)
    {
        // Whenever the glow status is first reached, give a warning message.
        if (old_level < 2 && new_level >= 2)
            did_god_conduct(DID_CAUSE_GLOWING, 0, false);
        // If the player actively did something to increase glowing,
        // Zin is displeased.
        else if (controlled && change > 0 && old_level > 1)
            did_god_conduct(DID_CAUSE_GLOWING, 1 + new_level, true);
    }
}

bool confuse_player(int amount, bool resistable)
{
    ASSERT(!crawl_state.game_is_arena());

    if (amount <= 0)
        return (false);

    if (resistable && player_mental_clarity())
    {
        mpr("You feel momentarily confused.");
        // Identify the amulet if necessary.
        if (player_equip(EQ_AMULET, AMU_CLARITY, true))
        {
            item_def* const amu = you.slot_item(EQ_AMULET, false);
            if (!item_ident(*amu, ISFLAG_KNOW_TYPE))
            {
                set_ident_type(amu->base_type, amu->sub_type, ID_KNOWN_TYPE);
                set_ident_flags(*amu, ISFLAG_KNOW_TYPE);
                mprf("You are wearing: %s",
                     amu->name(DESC_INVENTORY_EQUIP).c_str());
            }
        }
        return (false);
    }

    const int old_value = you.duration[DUR_CONF];
    you.increase_duration(DUR_CONF, amount, 40);

    if (you.duration[DUR_CONF] > old_value)
    {
        you.check_awaken(500);

        mprf(MSGCH_WARN, "You are %sconfused.",
             old_value > 0 ? "more " : "");

        learned_something_new(HINT_YOU_ENCHANTED);

        xom_is_stimulated((you.duration[DUR_CONF] - old_value)
                           / BASELINE_DELAY);
    }

    return (true);
}

bool curare_hits_player(int death_source, int amount, const bolt &beam)
{
    ASSERT(!crawl_state.game_is_arena());

    if (player_res_poison() > 0)
        return (false);

    poison_player(amount, beam.get_source_name(), beam.name);

    int hurted = 0;

    if (you.res_asphyx() <= 0)
    {
        hurted = roll_dice(2, 6);

        if (hurted)
        {
            mpr("You have difficulty breathing.");
            ouch(hurted, death_source, KILLED_BY_CURARE,
                 "curare-induced apnoea");
        }
    }

    potion_effect(POT_SLOWING, 2 + random2(4 + amount));

    return (hurted > 0);
}

bool poison_player(int amount, std::string source, std::string source_aux,
                   bool force)
{
    ASSERT(!crawl_state.game_is_arena());

    if (!force && player_res_poison() > 0 || amount <= 0)
        return (false);

    const int old_value = you.duration[DUR_POISONING];
    you.duration[DUR_POISONING] += amount;

    if (you.duration[DUR_POISONING] > 40)
        you.duration[DUR_POISONING] = 40;

    if (you.duration[DUR_POISONING] > old_value)
    {
        mprf(MSGCH_WARN, "You are %spoisoned.",
             old_value > 0 ? "more " : "");

        learned_something_new(HINT_YOU_POISON);
    }

    you.props["poisoner"] = source;
    you.props["poison_aux"] = source_aux;

    return (true);
}

void dec_poison_player()
{
    // If Cheibriados has slowed your life processes, there's a
    // chance that your poison level is simply unaffected and
    // you aren't hurt by poison.
    if (GOD_CHEIBRIADOS == you.religion
        && you.piety >= piety_breakpoint(0)
        && coinflip())
        return;

    if (you.duration[DUR_POISONING] > 0)
    {
        if (x_chance_in_y(you.duration[DUR_POISONING], 5))
        {
            int hurted = 1;
            msg_channel_type channel = MSGCH_PLAIN;
            const char *adj = "";

            if (you.duration[DUR_POISONING] > 10
                && random2(you.duration[DUR_POISONING]) >= 8)
            {
                hurted = random2(10) + 5;
                channel = MSGCH_DANGER;
                adj = "extremely ";
            }
            else if (you.duration[DUR_POISONING] > 5 && coinflip())
            {
                hurted = coinflip() ? 3 : 2;
                channel = MSGCH_WARN;
                adj = "very ";
            }

            int oldhp = you.hp;
            ouch(hurted, NON_MONSTER, KILLED_BY_POISON);
            if (you.hp < oldhp)
                mprf(channel, "You feel %ssick.", adj);

            if ((you.hp == 1 && one_chance_in(3)) || one_chance_in(8))
                reduce_poison_player(1);
        }
    }
}

void reduce_poison_player(int amount)
{
    if (amount <= 0)
        return;

    const int old_value = you.duration[DUR_POISONING];
    you.duration[DUR_POISONING] -= amount;

    if (you.duration[DUR_POISONING] <= 0)
    {
        you.duration[DUR_POISONING] = 0;
        you.props.erase("poisoner");
        you.props.erase("poison_aux");
    }

    if (you.duration[DUR_POISONING] < old_value)
    {
        mprf(MSGCH_RECOVERY, "You feel %sbetter.",
             you.duration[DUR_POISONING] > 0 ? "a little " : "");
    }
}

bool miasma_player(std::string source, std::string source_aux)
{
    ASSERT(!crawl_state.game_is_arena());

    if (you.res_rotting() || you.duration[DUR_DEATHS_DOOR])
        return (false);

    // Zin's protection.
    if (you.religion == GOD_ZIN && x_chance_in_y(you.piety, MAX_PIETY))
    {
        simple_god_message(" protects your body from miasma!");
        return (false);
    }

    bool success = poison_player(1, source, source_aux);

    if (you.hp_max > 4 && coinflip())
    {
        rot_hp(1);
        success = true;
    }

    if (one_chance_in(3))
    {
        potion_effect(POT_SLOWING, 5);
        success = true;
    }

    return (success);
}

bool napalm_player(int amount)
{
    ASSERT(!crawl_state.game_is_arena());

    if (player_res_sticky_flame() || amount <= 0)
        return (false);

    const int old_value = you.duration[DUR_LIQUID_FLAMES];
    you.increase_duration(DUR_LIQUID_FLAMES, amount, 100);

    if (you.duration[DUR_LIQUID_FLAMES] > old_value)
        mpr("You are covered in liquid flames!", MSGCH_WARN);

    return (true);
}

void dec_napalm_player(int delay)
{
    if (you.duration[DUR_LIQUID_FLAMES] > BASELINE_DELAY)
    {
        if (feat_is_watery(grd(you.pos())))
        {
            mpr("The flames go out!", MSGCH_WARN);
            you.duration[DUR_LIQUID_FLAMES] = 0;
            return;
        }

        mpr("You are covered in liquid flames!", MSGCH_WARN);

        expose_player_to_element(BEAM_NAPALM, 12);

        const int res_fire = player_res_fire();

        if (res_fire > 0)
        {
            ouch((((random2avg(9, 2) + 1) * delay) /
                    (1 + (res_fire * res_fire))) / BASELINE_DELAY, NON_MONSTER,
                    KILLED_BY_BURNING);
        }

        if (res_fire <= 0)
        {
            ouch(((random2avg(9, 2) + 1) * delay) / BASELINE_DELAY,
                 NON_MONSTER, KILLED_BY_BURNING);

            if (res_fire < 0)
            {
                ouch(((random2avg(9, 2) + 1) * delay)
                        / BASELINE_DELAY, NON_MONSTER, KILLED_BY_BURNING);
            }
        }

        if (you.duration[DUR_CONDENSATION_SHIELD] > 0)
            remove_condensation_shield();
        if (you.duration[DUR_ICY_ARMOUR] > 0)
            remove_ice_armour();
    }

    you.duration[DUR_LIQUID_FLAMES] -= delay;
    if (you.duration[DUR_LIQUID_FLAMES] < 0)
        you.duration[DUR_LIQUID_FLAMES] = 0;
}

bool slow_player(int turns)
{
    ASSERT(!crawl_state.game_is_arena());

    if (turns <= 0)
        return (false);

    if (stasis_blocks_effect(true, true, "%s rumbles.", 20, "%s rumbles."))
        return (false);

    // Doubling these values because moving while slowed takes twice the
    // usual delay.
    turns = haste_mul(turns);
    int threshold = haste_mul(100);

    if (you.duration[DUR_SLOW] >= threshold * BASELINE_DELAY)
        mpr("You already are as slow as you could be.");
    else
    {
        if (you.duration[DUR_SLOW] == 0)
            mpr("You feel yourself slow down.");
        else
            mpr("You feel as though you will be slow longer.");

        you.increase_duration(DUR_SLOW, turns, threshold);
        learned_something_new(HINT_YOU_ENCHANTED);
    }

    return (true);
}

void dec_slow_player(int delay)
{
    if (!you.duration[DUR_SLOW])
        return;

    if (you.duration[DUR_SLOW] > BASELINE_DELAY)
    {
        // Make slowing and hasting effects last as long.
        you.duration[DUR_SLOW] -= you.duration[DUR_HASTE]
            ? haste_mul(delay) : delay;
    }
    if (you.duration[DUR_SLOW] <= BASELINE_DELAY)
    {
        mpr("You feel yourself speed up.", MSGCH_DURATION);
        you.duration[DUR_SLOW] = 0;
    }
}

// Exhaustion should last as long as slowing.
void dec_exhaust_player(int delay)
{
    if (!you.duration[DUR_EXHAUSTED])
        return;

    if (you.duration[DUR_EXHAUSTED] > BASELINE_DELAY)
    {
        you.duration[DUR_EXHAUSTED] -= you.duration[DUR_HASTE]
                                       ? haste_mul(delay) : delay;
    }
    if (you.duration[DUR_EXHAUSTED] <= BASELINE_DELAY)
    {
        mpr("You feel less exhausted.", MSGCH_DURATION);
        you.duration[DUR_EXHAUSTED] = 0;
    }
}

bool haste_player(int turns, bool rageext)
{
    ASSERT(!crawl_state.game_is_arena());

    if (turns <= 0)
        return (false);

    if (stasis_blocks_effect(true, true, "%s emits a piercing whistle.", 20,
                             "%s makes your neck tingle."))
    {
        return (false);
    }

    // Cutting the nominal turns in half since hasted actions take half the
    // usual delay.
    turns = haste_div(turns);
    const int threshold = 40;

    if (!you.duration[DUR_HASTE])
        mpr("You feel yourself speed up.");
    else if (you.duration[DUR_HASTE] > threshold * BASELINE_DELAY)
        mpr("You already have as much speed as you can handle.");
    else if (!rageext)
    {
        mpr("You feel as though your hastened speed will last longer.");
        contaminate_player(1, true); // always deliberate
    }

    you.increase_duration(DUR_HASTE, turns, threshold);

    return (true);
}

void dec_haste_player(int delay)
{
    if (!you.duration[DUR_HASTE])
        return;

    if (you.duration[DUR_HASTE] > BASELINE_DELAY)
    {
        int old_dur = you.duration[DUR_HASTE];

        you.duration[DUR_HASTE] -= delay;

        int threshold = 6 * BASELINE_DELAY;
        // message if we cross the threshold
        if (old_dur > threshold && you.duration[DUR_HASTE] <= threshold)
        {
            mpr("Your extra speed is starting to run out.", MSGCH_DURATION);
            if (coinflip())
                you.duration[DUR_HASTE] -= BASELINE_DELAY;
        }
    }
    else if (you.duration[DUR_HASTE] <= BASELINE_DELAY)
    {
        if (!you.duration[DUR_BERSERK])
            mpr("You feel yourself slow down.", MSGCH_DURATION);
        you.duration[DUR_HASTE] = 0;
    }
}

void dec_disease_player(int delay)
{
    if (you.disease > 0)
    {
        // If Cheibriados has slowed your life processes, there's a
        // chance that your disease level is unaffected.
        if (GOD_CHEIBRIADOS == you.religion
            && you.piety >= piety_breakpoint(0)
            && coinflip())
        {
          return;
        }

        you.disease -= delay;
        if (you.disease < 0)
            you.disease = 0;

        // kobolds and regenerators recuperate quickly
        if (you.disease > 5 * BASELINE_DELAY
            && (you.species == SP_KOBOLD
                || you.duration[DUR_REGENERATION]
                || player_mutation_level(MUT_REGENERATION) == 3))
        {
            you.disease -= 2 * BASELINE_DELAY;
        }

        if (you.disease == 0)
            mpr("You feel your health improve.", MSGCH_RECOVERY);
    }
}

void float_player(bool fly)
{
    if (you.fishtail)
    {
        mprf("Your tail turns into legs as you %s out of the water.",
             fly ? "fly" : "levitate");
        merfolk_stop_swimming();
    }
    else if (you.light_flight())
        mpr("You swoop lightly up into the air.");
    else if (fly)
        mpr("You fly up into the air.");
    else
    {
        mprf("You gently float away from the %s.",
             you.is_wall_clinging() ? "wall" : "floor");
    }

    // Amulet of Controlled Flight can auto-ID.
    if (wearing_amulet(AMU_CONTROLLED_FLIGHT)
        && !extrinsic_amulet_effect(AMU_CONTROLLED_FLIGHT))
    {
        // it's important to do this only if the amulet is not identified yet,
        // or you'd get spammed
        item_def& amu(you.inv[you.equip[EQ_AMULET]]);
        if (!is_artefact(amu) && !item_type_known(amu))
        {
            set_ident_type(amu.base_type, amu.sub_type, ID_KNOWN_TYPE);
            set_ident_flags(amu, ISFLAG_KNOW_PROPERTIES);
            mprf("You are wearing: %s",
                 amu.name(DESC_INVENTORY_EQUIP).c_str());
        }
    }

    burden_change();

    // The player hasn't actuclly taken a step, but in this case, we want
    // neither the message, nor the location effect.
    you.check_clinging(true);
}

void levitate_player(int pow)
{
    bool standing = !you.airborne();
    mprf(MSGCH_DURATION,
         "You feel %s buoyant.", standing ? "very" : "more");

    you.increase_duration(DUR_LEVITATION, 25 + random2(pow), 100);

    if (standing)
        float_player(false);
}

bool land_player()
{
    // there was another source keeping you aloft
    if (you.airborne())
        return false;

    mpr("You float gracefully downwards.");
    burden_change();
    // Landing kills controlled flight.
    you.duration[DUR_CONTROLLED_FLIGHT] = 0;
    you.attribute[ATTR_LEV_UNCANCELLABLE] = 0;
    // Re-enter the terrain.
    move_player_to_grid(you.pos(), false, true);
    return true;
}

int count_worn_ego(int which_ego)
{
    int result = 0;
    for (int slot = EQ_MIN_ARMOUR; slot <= EQ_MAX_ARMOUR; ++slot)
    {
        if (you.equip[slot] != -1 && !you.melded[slot]
            && get_armour_ego_type(you.inv[you.equip[slot]]) == which_ego)
        {
            result++;
        }
    }

    return (result);
}

player::player()
    : kills(0), m_quiver(0)
{
    init();
}

player::player(const player &other)
    : kills(0), m_quiver(0)
{
    init();

    // why doesn't this do a copy_from?
    player_quiver* saved_quiver = m_quiver;
    delete kills;
    *this = other;
    m_quiver = saved_quiver;

    kills = new KillMaster(*(other.kills));
    *m_quiver = *(other.m_quiver);
}

// why is this not called "operator="?
void player::copy_from(const player &other)
{
    if (this == &other)
        return;

    KillMaster *saved_kills = kills;
    player_quiver* saved_quiver = m_quiver;

    *this = other;

    kills  = saved_kills;
    *kills = *(other.kills);
    m_quiver = saved_quiver;
    *m_quiver = *(other.m_quiver);
}


// player struct initialization
void player::init()
{
    // Permanent data:
    your_name.clear();
    species          = SP_UNKNOWN;
    char_class       = JOB_UNKNOWN;
    class_name.clear();
    type             = MONS_PLAYER;
    mid              = MID_PLAYER;
    position.reset();

#ifdef WIZARD
    wizard = (Options.wiz_mode == WIZ_YES) ? true : false;
#else
    wizard = false;
#endif
    birth_time       = time(0);

    // Long-term state:
    elapsed_time     = 0;
    elapsed_time_at_last_input = 0;

    hp               = 0;
    hp_max           = 0;
    base_hp          = 5000;
    base_hp2         = 5000;

    magic_points       = 0;
    max_magic_points   = 0;
    base_magic_points  = 5000;
    base_magic_points2 = 5000;

    stat_loss.init(0);
    base_stats.init(0);
    stat_zero.init(0);
    stat_zero_cause.init("");
    last_chosen        = STAT_RANDOM;

    hunger          = 6000;
    hunger_state    = HS_SATIATED;
    disease         = 0;
    max_level       = 1;
    hit_points_regeneration   = 0;
    magic_points_regeneration = 0;
    experience       = 0;
    experience_level = 1;
    gold             = 0;

    equip.init(-1);
    melded.init(false);
    unrand_reacts   = 0;

    symbol          = MONS_PLAYER;
    form            = TRAN_NONE;

    for (int i = 0; i < ENDOFPACK; i++)
        inv[i].clear();

    burden          = 0;
    burden_state    = BS_UNENCUMBERED;
    spells.init(SPELL_NO_SPELL);
    spell_no        = 0;
    char_direction  = GDT_DESCENDING;
    opened_zot      = false;
    royal_jelly_dead = false;
    transform_uncancellable = false;
    fishtail = false;

    pet_target      = MHITNOT;

    absdepth0       = 0;

    duration.init(0);
    rotting         = 0;
    berserk_penalty = 0;
    attribute.init(0);
    quiver.init(ENDOFPACK);
    sacrifice_value.init(0);

    is_undead       = US_ALIVE;

    friendly_pickup = 0;
#if defined(WIZARD) || defined(DEBUG)
    never_die = false;
#endif
    dead = false;
    lives = 0;
    deaths = 0;
    xray_vision = false;

    skills.init(0);
    // In Zot def we turn off all skills with non-zero skill level later
    practise_skill.init(true);
    autotrain_skill.init(false);
    num_autotrained_skills = 0;
    skill_points.init(0);
    ct_skill_points.init(0);
    skill_order.init(MAX_SKILL_ORDER);

    transfer_from_skill = SK_NONE;
    transfer_to_skill = SK_NONE;
    transfer_skill_points = 0;
    transfer_total_skill_points = 0;

    sage_bonus_skill = NUM_SKILLS;
    sage_bonus_degree = 0;

    skill_cost_level = 1;
    total_skill_points = 0;
    exp_available = 0; // new games get 25 or 80 later

    item_description.init(255);
    unique_items.init(UNIQ_NOT_EXISTS);
    unique_creatures.init(false);

    if (kills)
        delete kills;
    kills = new KillMaster();

    level_type       = LEVEL_DUNGEON;
    level_type_name.clear();
    level_type_ext.clear();
    level_type_name_abbrev.clear();
    level_type_origin.clear();
    level_type_tag.clear();

    where_are_you    = BRANCH_MAIN_DUNGEON;

    branch_stairs.init(0);

    religion         = GOD_NO_GOD;
    jiyva_second_name.clear();
    piety            = 0;
    piety_hysteresis = 0;
    gift_timeout     = 0;
    penance.init(0);
    worshipped.init(0);
    num_current_gifts.init(0);
    num_total_gifts.init(0);
    exp_docked       = 0;
    exp_docked_total = 0;

    mutation.init(0);
    innate_mutations.init(0);
    demonic_traits.clear();

    earth_attunement = 0;
    magic_contamination = 0;

    had_book.init(false);
    seen_spell.init(false);
    seen_weapon.init(0);
    seen_armour.init(0);

    normal_vision    = LOS_RADIUS;
    current_vision   = LOS_RADIUS;

    hell_branch      = NUM_BRANCHES;
    hell_exit        = 0;

    real_time        = 0;
    num_turns        = 0;
    exploration      = 0;

    last_view_update = 0;

    spell_letter_table.init(-1);
    ability_letter_table.init(ABIL_NON_ABILITY);

    uniq_map_tags.clear();
    uniq_map_names.clear();

    global_info = PlaceInfo();
    global_info.make_global();
    global_info.assert_validity();

   if (m_quiver)
        delete m_quiver;
    m_quiver = new player_quiver;

    props.clear();

    beholders.clear();
    fearmongers.clear();
    dactions.clear();

    zotdef_wave_name.clear();
    last_mid = 0;
    last_cast_spell = SPELL_NO_SPELL;


    // Non-saved UI state:
    prev_targ        = MHITNOT;
    prev_grd_targ.reset();
    prev_move.reset();

    travel_x         = 0;
    travel_y         = 0;
    travel_z         = level_id();

    running.clear();
    received_weapon_warning = false;
    bondage_level = 0;

    delay_queue.clear();

    last_keypress_time = time(0);

#if TAG_MAJOR_VERSION == 32
    for (unsigned int i = 0; i < ARRAYSZ(montiers); i++)
        montiers[i] = 0;
#endif


    // Volatile (same-turn) state:
    turn_is_over     = false;
    banished         = false;
    banished_by.clear();

    wield_change     = false;
    redraw_quiver    = false;
    redraw_status_flags = 0;
    redraw_hit_points   = false;
    redraw_magic_points = false;
    redraw_stats.init(false);
    redraw_experience   = false;
    redraw_armour_class = false;
    redraw_evasion      = false;

    flash_colour        = BLACK;

    time_taken          = 0;
    shield_blocks       = 0;

    entry_cause         = EC_SELF_EXPLICIT;
    entry_cause_god     = GOD_NO_GOD;

    old_hunger          = hunger;
    transit_stair       = DNGN_UNSEEN;
    entering_level      = false;

    reset_escaped_death();
    on_current_level    = true;
    walking             = 0;
    seen_portals        = 0;
    frame_no            = 0;

    save                = 0;

    // Protected fields:
    for (int i = 0; i < NUM_BRANCHES; i++)
    {
        branch_info[i].level_type = LEVEL_DUNGEON;
        branch_info[i].branch     = i;
        branch_info[i].assert_validity();
    }

    for (int i = 0; i < (NUM_LEVEL_AREA_TYPES - 1); i++)
    {
        non_branch_info[i].level_type = i + 1;
        non_branch_info[i].branch     = -1;
        non_branch_info[i].assert_validity();
    }
}

player_save_info player_save_info::operator=(const player& rhs)
{
    name             = rhs.your_name;
    experience       = rhs.experience;
    experience_level = rhs.experience_level;
    wizard           = rhs.wizard;
    species          = rhs.species;
    class_name       = rhs.class_name;
    religion         = rhs.religion;
    jiyva_second_name  = rhs.jiyva_second_name;

    // [ds] Perhaps we should move game type to player?
    saved_game_type  = crawl_state.type;

#ifdef USE_TILE
    held_in_net      = false;
#endif

    return (*this);
}

bool player_save_info::operator<(const player_save_info& rhs) const
{
    return experience < rhs.experience
           || (experience == rhs.experience && name < rhs.name);
}

std::string player_save_info::short_desc() const
{
    std::ostringstream desc;

    const std::string qualifier =
        game_state::game_type_name_for(saved_game_type);
    if (!qualifier.empty())
        desc << "[" << qualifier << "] ";

    desc << name << ", a level " << experience_level << ' '
         << species_name(species) << ' ' << class_name;

    if (religion == GOD_JIYVA)
        desc << " of " << god_name_jiyva(true);
    else if (religion != GOD_NO_GOD)
        desc << " of " << god_name(religion);

#ifdef WIZARD
    if (wizard)
        desc << " (WIZ)";
#endif

    return desc.str();
}

player::~player()
{
    delete kills;
    delete m_quiver;
    if (CrawlIsCrashing && save)
    {
        save->abort();
        delete save;
        save = 0;
    }
    ASSERT(!save); // the save file should be closed or deleted
}

bool player::is_levitating() const
{
    return duration[DUR_LEVITATION] || you.attribute[ATTR_PERM_LEVITATION];
}

bool player::in_water() const
{
    return (ground_level() && !beogh_water_walk()
            && feat_is_water(grd(pos())));
}

bool player::can_swim(bool permanently) const
{
    // Transforming could be fatal if it would cause unequipment of
    // stat-boosting boots or heavy armour.
    return (species == SP_MERFOLK
            || body_size(PSIZE_BODY) >= SIZE_GIANT
            || !permanently)
                && form_can_swim();
}

int player::visible_igrd(const coord_def &where) const
{
    if (grd(where) == DNGN_LAVA
        || (grd(where) == DNGN_DEEP_WATER
            && species != SP_MERFOLK && species != SP_GREY_DRACONIAN))
    {
        return (NON_ITEM);
    }

    return (igrd(where));
}

bool player::has_spell(spell_type spell) const
{
    for (int i = 0; i < MAX_KNOWN_SPELLS; i++)
    {
        if (spells[i] == spell)
            return (true);
    }

    return (false);
}

bool player::cannot_speak() const
{
    if (silenced(pos()))
        return (true);

    if (cannot_move()) // we allow talking during sleep ;)
        return (true);

    // No transform that prevents the player from speaking yet.
    return (false);
}

std::string player::shout_verb() const
{
    switch (you.form)
    {
    case TRAN_DRAGON:
        return "roar";
    case TRAN_SPIDER:
        return "hiss";
    case TRAN_BAT:
        return "squeak";
    case TRAN_PIG:
        return "squeal";
    default:
        if (you.species == SP_CAT)
            return coinflip() ? "meow" : "yowl";
        // depends on SCREAM mutation
        int level = player_mutation_level(MUT_SCREAM);
        if (level <= 1)
            return "shout";
        else if (level == 2)
            return "yell";
        else // level == 3
            return "scream";
    }
}

void player::god_conduct(conduct_type thing_done, int level)
{
    ::did_god_conduct(thing_done, level);
}

void player::banish(const std::string &who)
{
    ASSERT(!crawl_state.game_is_arena());

    banished    = true;
    banished_by = who;
}

// For semi-undead species (Vampire!) reduce food cost for spells and abilities
// to 50% (hungry, very hungry) or zero (near starving, starving).
int calc_hunger(int food_cost)
{
    if (you.is_undead == US_SEMI_UNDEAD && you.hunger_state < HS_SATIATED)
    {
        if (you.hunger_state <= HS_NEAR_STARVING)
            return 0;

        return (food_cost/2);
    }
    return (food_cost);
}

int player::warding() const
{
    return (30 * player_warding());
}

bool player::paralysed() const
{
    return (duration[DUR_PARALYSIS]);
}

bool player::cannot_move() const
{
    return (paralysed() || petrified());
}

bool player::confused() const
{
    return (duration[DUR_CONF]);
}

bool player::caught() const
{
    return (attribute[ATTR_HELD]);
}

bool player::petrified() const
{
    return (duration[DUR_PETRIFIED]);
}

int player::shield_block_penalty() const
{
    return (5 * shield_blocks * shield_blocks);
}

int player::shield_bonus() const
{
    const int shield_class = player_shield_class();
    if (shield_class <= 0)
        return (-100);

    return random2avg(shield_class * 2, 2) / 3 - 1;
}

int player::shield_bypass_ability(int tohit) const
{
    return (15 + tohit / 2);
}

void player::shield_block_succeeded(actor *foe)
{
    actor::shield_block_succeeded(foe);

    shield_blocks++;
    practise(EX_SHIELD_BLOCK);
}

int player::skill(skill_type sk) const
{
    if (you.duration[DUR_HEROISM] && sk <= SK_LAST_MUNDANE)
        return std::min(skills[sk] + 5, 27);
//    else if (you.religion == GOD_ASHENZARI)
//        return ash_skill_boost(sk);

    return skills[sk];
}

// only for purposes of detection, not disarming
int player::traps_skill() const
{
    int val = skill(SK_TRAPS_DOORS);

    if (you.religion == GOD_ASHENZARI && !player_under_penance())
        val += you.piety / 15;

    return val;
}

int player_icemail_armour_class()
{
    if (!you.mutation[MUT_ICEMAIL])
        return (0);

    return (ICEMAIL_MAX
               - (you.duration[DUR_ICEMAIL_DEPLETED]
                   * ICEMAIL_MAX / ICEMAIL_TIME));
}

int player::armour_class() const
{
    int AC = 0;

    for (int eq = EQ_MIN_ARMOUR; eq <= EQ_MAX_ARMOUR; ++eq)
    {
        if (eq == EQ_SHIELD)
            continue;

        if (!player_wearing_slot(eq))
            continue;

        const item_def& item   = inv[equip[eq]];
        const int ac_value     = property(item, PARM_AC) * 100;
        const int racial_bonus = _player_armour_racial_bonus(item);

        // [ds] effectively: ac_value * (22 + Arm) / 22, where Arm =
        // Armour Skill + racial_skill_bonus / 2.
        AC += ac_value * (44 + 2 * skill(SK_ARMOUR) + racial_bonus) / 44;
        AC += item.plus * 100;

        // The deformed don't fit into body armour very well.
        // (This includes nagas and centaurs.)
        if (eq == EQ_BODY_ARMOUR && (player_mutation_level(MUT_DEFORMED)
            || player_mutation_level(MUT_PSEUDOPODS)))
            AC -= ac_value / 2;
    }

    AC += player_equip(EQ_RINGS_PLUS, RING_PROTECTION) * 100;

    if (player_equip_ego_type(EQ_WEAPON, SPWPN_PROTECTION))
        AC += 500;

    if (player_equip_ego_type(EQ_SHIELD, SPARM_PROTECTION))
        AC += 300;

    AC += scan_artefacts(ARTP_AC) * 100;

    if (duration[DUR_ICY_ARMOUR])
        AC += 400 + 100 * skill(SK_ICE_MAGIC) / 3;         // max 13

    if (duration[DUR_STONESKIN])
        AC += 200 + 100 * skill(SK_EARTH_MAGIC) / 5;       // max 7

    if (mutation[MUT_ICEMAIL])
        AC += 100 * player_icemail_armour_class();

    if (form == TRAN_NONE
        || form == TRAN_LICH
        || form == TRAN_BLADE_HANDS
        || (form == TRAN_DRAGON
        && player_genus(GENPC_DRACONIAN)))
    {
        // Being a lich doesn't preclude the benefits of hide/scales -- bwr
        //
        // Note: Even though necromutation is a high level spell, it does
        // allow the character full armour (so the bonus is low). -- bwr
        if (form == TRAN_LICH)
            AC += (300 + 100 * skill(SK_NECROMANCY) / 6);   // max 7

        //jmf: only give:
        if (player_genus(GENPC_DRACONIAN))
        {
           AC += 300 + 100 * (you.experience_level / 3);  // max 12

           if (form == TRAN_DRAGON)
               AC += 700;
        }
        else
        {
            switch (species)
            {
            case SP_NAGA:
                AC += 100 * experience_level / 3;              // max 9
                break;

            default:
                break;
            }
        }
    }
    else
    {
        // transformations:
        switch (form)
        {
        case TRAN_NONE:
        case TRAN_BLADE_HANDS:
        case TRAN_LICH:  // can wear normal body armour (small bonus)
            break;

        case TRAN_SPIDER: // low level (small bonus), also gets EV
            AC += (200 + 100 * skill(SK_POISON_MAGIC) / 6); // max 6
            break;

        case TRAN_ICE_BEAST:
            AC += (500 + 100 * (skill(SK_ICE_MAGIC) + 1) / 4); // max 12

            if (duration[DUR_ICY_ARMOUR])
                AC += (100 + 100 * skill(SK_ICE_MAGIC) / 4);   // max +7
            break;

        case TRAN_DRAGON: // Draconians handled above
            AC += (700 + 100 * skill(SK_FIRE_MAGIC) / 3); // max 16
            break;

        case TRAN_STATUE: // main ability is armour (high bonus)
            AC += (1700 + 100 * skill(SK_EARTH_MAGIC) / 2);    // max 30

            if (duration[DUR_STONESKIN])
                AC += (100 + 100 * skill(SK_EARTH_MAGIC) / 4); // max +7
            break;

        default:
            break;
        }
    }

    // Scale mutations, etc.
    AC += player_mutation_level(MUT_TOUGH_SKIN) ? player_mutation_level(MUT_TOUGH_SKIN) * 100 : 0;                          // +1, +2, +3
    AC += player_mutation_level(MUT_SHAGGY_FUR) ? player_mutation_level(MUT_SHAGGY_FUR) * 100 : 0;                          // +1, +2, +3
    AC += player_mutation_level(MUT_IRIDESCENT_SCALES) ? 200 + player_mutation_level(MUT_IRIDESCENT_SCALES) * 200 : 0;      // +4, +6, +8
    AC += player_mutation_level(MUT_LARGE_BONE_PLATES) ? 100 + player_mutation_level(MUT_LARGE_BONE_PLATES) * 100 : 0;      // +2, +3, +4
    AC += player_mutation_level(MUT_ROUGH_BLACK_SCALES) ? 100 + player_mutation_level(MUT_ROUGH_BLACK_SCALES) * 300 : 0;    // +4, +7, +10
    AC += player_mutation_level(MUT_RUGGED_BROWN_SCALES) ? 200 : 0;                                                         // +2, +2, +2
    AC += player_mutation_level(MUT_ICY_BLUE_SCALES) ? player_mutation_level(MUT_ICY_BLUE_SCALES) * 100 : 0;                // +1, +2, +3
    AC += player_mutation_level(MUT_MOLTEN_SCALES) ? player_mutation_level(MUT_MOLTEN_SCALES) * 100 : 0;                    // +1, +2, +3
    AC += player_mutation_level(MUT_SLIMY_GREEN_SCALES) ? player_mutation_level(MUT_SLIMY_GREEN_SCALES) * 100 : 0;          // +1, +2, +3
    AC += player_mutation_level(MUT_THIN_METALLIC_SCALES) ? player_mutation_level(MUT_THIN_METALLIC_SCALES) * 100 : 0;      // +1, +2, +3
    AC += player_mutation_level(MUT_YELLOW_SCALES) ? player_mutation_level(MUT_YELLOW_SCALES) * 100 : 0;                    // +1, +2, +3
    AC += player_mutation_level(MUT_GELATINOUS_BODY) ? (player_mutation_level(MUT_GELATINOUS_BODY) == 3 ? 200 : 100) : 0;   // +1, +1, +2
    return (AC / 100);
}
 /**
  * Guaranteed damage reduction.
  *
  * The percentage of the damage received that is guaranteed to be reduced
  * by the armour. As the AC roll is done before GDR is applied, GDR is only
  * useful when the AC roll is inferior to it. Therefore a higher GDR means
  * more damage reduced, but also more often.
  *
  * \f[ GDR = 14 \times (base\_AC - 2)^\frac{1}{2} \f]
  *
  * \return GDR as a percentage.
  **/
int player::gdr_perc() const
{
    const item_def *body_armour = slot_item(EQ_BODY_ARMOUR, false);

    if (!body_armour)
        return (0);

    const int body_base_AC = property(*body_armour, PARM_AC);
    return (14 * pow(std::max(body_base_AC - 2, 0), 0.5));
}

int player::melee_evasion(const actor *act, ev_ignore_type evit) const
{
    return (player_evasion(evit)
            - ((!act || act->visible_to(this)
                || (evit & EV_IGNORE_HELPLESS)) ? 0 : 10)
            - (you_are_delayed()
               && !(evit & EV_IGNORE_HELPLESS)
               && !delay_is_run(current_delay_action())? 5 : 0));
}

bool player::heal(int amount, bool max_too)
{
    ::inc_hp(amount, max_too);
    return true; /* TODO Check whether the player was healed. */
}

mon_holy_type player::holiness() const
{
    if (is_undead)
        return (MH_UNDEAD);

    if (species == SP_DEMONSPAWN)
        return (MH_DEMONIC);

    return (MH_NATURAL);
}

bool player::undead_or_demonic() const
{
    const mon_holy_type holi = holiness();

    return (holi == MH_UNDEAD || holi == MH_DEMONIC);
}

bool player::is_holy(bool check_spells) const
{
    if (is_good_god(religion) && check_spells)
        return (true);

    return (false);
}

bool player::is_unholy(bool check_spells) const
{
    return (holiness() == MH_DEMONIC);
}

bool player::is_evil(bool check_spells) const
{
    if (holiness() == MH_UNDEAD)
        return (true);

    if (is_evil_god(religion) && check_spells)
        return (true);

    return (false);
}

// This is a stub. Check is used only for silver damage. Worship of chaotic
// gods should probably be checked in the non-existing player::is_unclean,
// which could be used for something Zin-related (such as a priestly monster).
bool player::is_chaotic() const
{
    return (false);
}

bool player::is_artificial() const
{
    return (false);
}

bool player::is_unbreathing() const
{
    switch (you.form)
    {
    case TRAN_LICH:
    case TRAN_STATUE:
        return (true);
    default:
        break;
    }

    return (player_mutation_level(MUT_UNBREATHING));
}

// This is a stub. Makes checking for silver damage a little cleaner.
bool player::is_insubstantial() const
{
    return (false);
}

// Output active level of player mutation.
// Might be lower than real mutation for non-"Alive" Vampires.
int player_mutation_level(mutation_type mut)
{
    const int mlevel = you.mutation[mut];

    if (mutation_is_fully_active(mut))
        return (mlevel);

    // For now, dynamic mutations only apply to semi-undead.
    ASSERT(you.is_undead == US_SEMI_UNDEAD);

    // Assumption: stat mutations are physical, and thus always fully active.
    switch (you.hunger_state)
    {
    case HS_ENGORGED:
        return (mlevel);
    case HS_VERY_FULL:
    case HS_FULL:
        return (std::min(mlevel, 2));
    case HS_SATIATED:
        return (std::min(mlevel, 1));
    }

    return (0);
}

int player::res_acid() const
{
    return (player_res_acid());
}

int player::res_fire() const
{
    return (player_res_fire());
}

int player::res_steam() const
{
    return (player_res_steam());
}

int player::res_cold() const
{
    return (player_res_cold());
}

int player::res_elec() const
{
    return (player_res_electricity() * 2);
}

int player::res_water_drowning() const
{
    return (is_unbreathing()
            || (you.species == SP_MERFOLK && !form_changed_physiology()));
}

int player::res_asphyx() const
{
    // The unbreathing are immune to asphyxiation.
    if (is_unbreathing())
        return 1;

    return 0;
}

int player::res_poison(bool temp) const
{
    return (player_res_poison(true, temp));
}

int player::res_rotting(bool temp) const
{
    switch (is_undead)
    {
        default:
    case US_ALIVE:
        return 0;

    case US_HUNGRY_DEAD:
        return 1; // rottable by Zin, not by necromancy

    case US_SEMI_UNDEAD:
        return temp ? 1 : 0;

    case US_UNDEAD:
        if (!temp && you.form == TRAN_LICH)
            return 0;
        return 3; // full immunity
    }
}

int player::res_sticky_flame() const
{
    return (player_res_sticky_flame());
}

int player::res_holy_energy(const actor *attacker) const
{
    if (undead_or_demonic())
        return (-2);

    if (is_evil())
        return (-1);

    if (is_holy())
        return (1);

    return (0);
}

int player::res_negative_energy() const
{
    return (player_prot_life());
}

int player::res_torment() const
{
    return (player_res_torment());
}

int player::res_wind() const
{
    // Full control of the winds around you can negate a hostile tornado.
    return you.duration[DUR_TORNADO] ? 1 : 0;
}

int player::res_magic() const
{
    return player_res_magic();
}

int player_res_magic(bool calc_unid, bool temp)
{
    int rm = 0;

    switch (you.species)
    {
    default:
        rm = you.experience_level * 3;
        break;
    case SP_HIGH_ELF:
    case SP_SLUDGE_ELF:
    case SP_DEEP_ELF:
    case SP_MOUNTAIN_DWARF:
    case SP_VAMPIRE:
    case SP_DEMIGOD:
    case SP_OGRE:
        rm = you.experience_level * 4;
        break;
    case SP_NAGA:
    case SP_MUMMY:
        rm = you.experience_level * 5;
        break;
    case SP_PURPLE_DRACONIAN:
    case SP_DEEP_DWARF:
    case SP_CAT:
        rm = you.experience_level * 6;
        break;
    case SP_SPRIGGAN:
        rm = you.experience_level * 7;
        break;
    }

    // randarts
    rm += scan_artefacts(ARTP_MAGIC, calc_unid);

    // armour
    rm += 30 * player_equip_ego_type(EQ_ALL_ARMOUR, SPARM_MAGIC_RESISTANCE,
                                     calc_unid);

    // rings of magic resistance
    rm += 40 * player_equip(EQ_RINGS, RING_PROTECTION_FROM_MAGIC, calc_unid);

    // Enchantment skill through staff of enchantment (up to 90).
    if (player_equip(EQ_STAFF, STAFF_ENCHANTMENT, calc_unid))
        rm += 3 * (3 + you.skill(SK_ENCHANTMENTS));

    // Mutations
    rm += 30 * player_mutation_level(MUT_MAGIC_RESISTANCE);

    // transformations
    if (you.form == TRAN_LICH && temp)
        rm += 50;

    // Trog's Hand
    if (you.attribute[ATTR_DIVINE_REGENERATION] && temp)
        rm += 70;

    // Enchantment effect
    if (you.duration[DUR_LOWERED_MR] && temp)
        rm /= 2;

    return (rm);
}

bool player::confusable() const
{
    return (player_mental_clarity() == 0);
}

bool player::slowable() const
{
    return true;
}

flight_type player::flight_mode() const
{
    if (form == TRAN_DRAGON
        || form == TRAN_BAT)
    {
        return (FL_FLY);
    }
    else if (is_levitating())
    {
        return (duration[DUR_CONTROLLED_FLIGHT]
                || wearing_amulet(AMU_CONTROLLED_FLIGHT) ? FL_FLY
                                                         : FL_LEVITATE);
    }
    else
        return (FL_NONE);
}

bool player::permanent_levitation() const
{
    return you.attribute[ATTR_PERM_LEVITATION]
           && player_equip_ego_type(EQ_BOOTS, SPARM_LEVITATION);
}

bool player::permanent_flight() const
{
    return you.attribute[ATTR_PERM_LEVITATION]
           && species == SP_KENKU && experience_level >= 15;
}

bool player::light_flight() const
{
    // Only Kenku get perks for flying light.
    return (species == SP_KENKU
            && flight_mode() == FL_FLY && travelling_light());
}

bool player::travelling_light() const
{
    return (burden < carrying_capacity(BS_UNENCUMBERED) * 70 / 100);
}

int player::mons_species() const
{
    if (player_genus(GENPC_DRACONIAN))
        return (MONS_DRACONIAN);

    switch (species)
    {
    case SP_HILL_ORC:
        return (MONS_ORC);
    case SP_HIGH_ELF: case SP_DEEP_ELF: case SP_SLUDGE_ELF:
        return (MONS_ELF);

    default:
        return (MONS_HUMAN);
    }
}

void player::poison(actor *agent, int amount, bool force)
{
    ::poison_player(amount, agent? agent->name(DESC_NOCAP_A, true) : "");
}

void player::expose_to_element(beam_type element, int st)
{
    ::expose_player_to_element(element, st);
}

void player::blink(bool allow_partial_control)
{
    random_blink(allow_partial_control);
}

void player::teleport(bool now, bool abyss_shift, bool wizard_tele)
{
    ASSERT(!crawl_state.game_is_arena());

    if (now)
        you_teleport_now(true, abyss_shift, wizard_tele);
    else
        you_teleport();
}

int player::hurt(const actor *agent, int amount, beam_type flavour,
                 bool cleanup_dead)
{
    // We ignore cleanup_dead here.
    if (agent->atype() == ACT_MONSTER)
    {
        const monster* mon = agent->as_monster();
        ouch(amount, mon->mindex(),
             KILLED_BY_MONSTER, "", mon->visible_to(&you));
    }
    else
    {
        // Should never happen!
        die("player::hurt() called for self-damage");
    }

    if ((flavour == BEAM_NUKE || flavour == BEAM_DISINTEGRATION) && can_bleed())
        blood_spray(pos(), type, amount / 5);

    return (amount);
}

void player::drain_stat(stat_type s, int amount, actor *attacker)
{
    if (attacker == NULL)
        lose_stat(s, amount, false, "");
    else if (attacker->atype() == ACT_MONSTER)
        lose_stat(s, amount, attacker->as_monster(), false);
    else if (attacker->atype() == ACT_PLAYER)
        lose_stat(s, amount, false, "suicide");
    else
        lose_stat(s, amount, false, "");
}

bool player::rot(actor *who, int amount, int immediate, bool quiet)
{
    ASSERT(!crawl_state.game_is_arena());

    if (amount <= 0 && immediate <= 0)
        return (false);

    if (res_rotting() || you.duration[DUR_DEATHS_DOOR])
    {
        mpr("You feel terrible.");
        return (false);
    }

    // Zin's protection.
    if (religion == GOD_ZIN && x_chance_in_y(piety, MAX_PIETY))
    {
        simple_god_message(" protects your body from decay!");
        return (false);
    }

    if (immediate > 0)
        rot_hp(immediate);

    if (rotting < 40)
    {
        // Either this, or the actual rotting message should probably
        // be changed so that they're easier to tell apart. -- bwr
        mprf(MSGCH_WARN, "You feel your flesh %s away!",
             rotting > 0 ? "rotting" : "start to rot");

        rotting += amount;

        learned_something_new(HINT_YOU_ROTTING);
    }

    if (one_chance_in(4))
        sicken(50 + random2(100));

    return (true);
}

bool player::drain_exp(actor *who, bool quiet, int pow)
{
    return (::drain_exp());
}

void player::confuse(actor *who, int str)
{
    confuse_player(str);
}

/*
 * Paralyse the player for str turns.
 *
 *  Duration is capped at 13.
 *
 * @param who Pointer to the actor who paralysed the player.
 * @param str The number of turns the paralysis will last.
 */
void player::paralyse(actor *who, int str)
{
    ASSERT(!crawl_state.game_is_arena());

    // The shock is too mild to do damage.
    if (stasis_blocks_effect(true, true, "%s gives you a mild electric shock."))
        return;

    if (!(who && who->as_monster() && who->as_monster()->type == MONS_RED_WASP)
        && who && (duration[DUR_PARALYSIS] || duration[DUR_PARALYSIS_IMMUNITY]))
    {
        canned_msg(MSG_YOU_RESIST);
        return;
    }

    int &paralysis(duration[DUR_PARALYSIS]);

    if (!paralysis && who)
    {
        take_note(Note(NOTE_PARALYSIS, str, 0,
                       who->name(DESC_NOCAP_A).c_str()));
    }


    mprf("You %s the ability to move!",
         paralysis ? "still haven't" : "suddenly lose");

    str *= BASELINE_DELAY;
    if (str > paralysis && (paralysis < 3 || one_chance_in(paralysis)))
        paralysis = str;

    if (paralysis > 13 * BASELINE_DELAY)
        paralysis = 13 * BASELINE_DELAY;
}

void player::petrify(actor *who, int str)
{
    ASSERT(!crawl_state.game_is_arena());

    if (stasis_blocks_effect(true, true, "%s gives you a mild electric shock."))
        return;

    str *= BASELINE_DELAY;
    int &petrif(duration[DUR_PETRIFIED]);

    mprf("You %s the ability to move!",
         petrif ? "still haven't" : "suddenly lose");

    if (str > petrif && (petrif < 3 || one_chance_in(petrif)))
        petrif = str;

    petrif = std::min(13 * BASELINE_DELAY, petrif);
}

void player::slow_down(actor *foe, int str)
{
    ::slow_player(str);
}

int player::has_claws(bool allow_tran) const
{
    if (allow_tran)
    {
        // these transformations bring claws with them
        if (form == TRAN_DRAGON)
            return (3);

        // transformations other than these will override claws
        if (form != TRAN_NONE
            && form != TRAN_STATUE
            && form != TRAN_LICH)
        {
            return (0);
        }
    }

    if (const int c = species_has_claws(you.species))
        return (c);

    return (player_mutation_level(MUT_CLAWS));
}

bool player::has_usable_claws(bool allow_tran) const
{
    return (!player_wearing_slot(EQ_GLOVES) && has_claws(allow_tran));
}

int player::has_talons(bool allow_tran) const
{
    if (allow_tran)
    {
        // transformations other than these will override talons
        if (form != TRAN_NONE
            && form != TRAN_BLADE_HANDS
            && form != TRAN_STATUE
            && form != TRAN_LICH)
        {
            return (0);
        }
    }

    // XXX: Do merfolk in water belong under allow_tran?
    if (you.fishtail)
        return (0);

    return (player_mutation_level(MUT_TALONS));
}

bool player::has_usable_talons(bool allow_tran) const
{
    return (!player_wearing_slot(EQ_BOOTS) && has_talons(allow_tran));
}

int player::has_fangs(bool allow_tran) const
{
    if (allow_tran)
    {
        // these transformations bring fangs with them
        if (form == TRAN_DRAGON)
            return (3);

        // transformations other than these will override fangs
        if (form != TRAN_NONE
            && form != TRAN_BLADE_HANDS
            && form != TRAN_STATUE
            && form != TRAN_LICH)
        {
            return (0);
        }
    }

    return (player_mutation_level(MUT_FANGS));
}

int player::has_usable_fangs(bool allow_tran) const
{
    const item_def* helmet = you.slot_item(EQ_HELMET);
    if (helmet && get_helmet_desc(*helmet) == THELM_DESC_VISORED)
        return (0);

    return (has_fangs(allow_tran));
}

int player::has_tail(bool allow_tran) const
{
    if (allow_tran)
    {
        // these transformations bring a tail with them
        if (form == TRAN_DRAGON)
            return (1);

        // transformations other than these will override a tail
        if (form != TRAN_NONE
            && form != TRAN_BLADE_HANDS
            && form != TRAN_STATUE
            && form != TRAN_LICH)
        {
            return (0);
        }
    }

    // XXX: Do merfolk in water belong under allow_tran?
    if (player_genus(GENPC_DRACONIAN)
        || you.fishtail
        || player_mutation_level(MUT_STINGER))
    {
        return (1);
    }

    return (0);
}

int player::has_usable_tail(bool allow_tran) const
{
    // TSO worshippers don't use their stinger in order
    // to avoid poisoning.
    if (you.religion == GOD_SHINING_ONE
        && player_mutation_level(MUT_STINGER) > 0)
    {
        return (0);
    }

    return (has_tail(allow_tran));
}

// Whether the player has a usable offhand for the
// purpose of punching.
// XXX: The weapon check should probably involve HANDS_DOUBLE
//      at some point.
bool player::has_usable_offhand() const
{
    if (player_wearing_slot(EQ_SHIELD))
        return (false);

    const item_def* wp = slot_item(EQ_WEAPON);
    return (!wp
            || hands_reqd(*wp, body_size()) != HANDS_TWO
            || wp->base_type == OBJ_STAVES
            || weapon_skill(*wp) == SK_STAVES);
}

int player::has_pseudopods(bool allow_tran) const
{
    if (allow_tran)
    {
        // transformations other than these will override pseudopods
        if (form != TRAN_NONE
            && form != TRAN_BLADE_HANDS
            && form != TRAN_STATUE
            && form != TRAN_LICH)
        {
            return (0);
        }
    }

    return (player_mutation_level(MUT_PSEUDOPODS));
}

int player::has_usable_pseudopods(bool allow_tran) const
{
    return (has_pseudopods(allow_tran));
}

bool player::sicken(int amount, bool allow_hint)
{
    ASSERT(!crawl_state.game_is_arena());

    if (res_rotting() || amount <= 0)
        return (false);

    // Zin's protection.
    if (religion == GOD_ZIN && x_chance_in_y(piety, MAX_PIETY))
    {
        simple_god_message(" protects your body from disease!");
        return (false);
    }

    mpr("You feel ill.");

    disease += amount * BASELINE_DELAY;
    if (disease > 210 * BASELINE_DELAY)
        disease = 210 * BASELINE_DELAY;

    if (allow_hint)
        learned_something_new(HINT_YOU_SICK);
    return (true);
}

bool player::can_see_invisible(bool calc_unid, bool transient) const
{
    if (crawl_state.game_is_arena())
        return (true);

    int si = 0;

    si += player_equip(EQ_RINGS, RING_SEE_INVISIBLE, calc_unid);

    // armour: (checks head armour only)
    si += player_equip_ego_type(EQ_HELMET, SPARM_SEE_INVISIBLE);

    if (player_mutation_level(MUT_ACUTE_VISION) > 0)
        si += player_mutation_level(MUT_ACUTE_VISION);

    // antennae give sInvis at 3
    if (player_mutation_level(MUT_ANTENNAE) == 3)
        si++;

    if (player_mutation_level(MUT_EYEBALLS) == 3)
        si++;

    //jmf: added see_invisible spell
    if (transient && duration[DUR_SEE_INVISIBLE] > 0)
        si++;

    // randart wpns
    int artefacts = scan_artefacts(ARTP_EYESIGHT, calc_unid);

    if (artefacts > 0)
        si += artefacts;

    if (you.religion == GOD_ASHENZARI && you.piety >= piety_breakpoint(2)
        && !player_under_penance())
    {
        si++;
    }

    if (si > 1)
        si = 1;

    return (si);
}

bool player::can_see_invisible() const
{
    return (can_see_invisible(true));
}

bool player::invisible() const
{
    return (duration[DUR_INVIS] && !backlit());
}

bool player::misled() const
{
    return (duration[DUR_MISLED]);
}

bool player::visible_to(const actor *looker) const
{
    if (crawl_state.game_is_arena())
        return (false);

    if (this == looker)
        return (can_see_invisible() || !invisible());

    const monster* mon = looker->as_monster();
    return (!mon->has_ench(ENCH_BLIND)
            && !invisible()
            || in_water()
            || mon->can_see_invisible()
            || mons_sense_invis(mon)
               && circle_def(pos(), 4, C_ROUND).contains(mon->pos()));
}

bool player::backlit(bool check_haloed, bool self_halo) const
{
    if (get_contamination_level() > 1 || duration[DUR_CORONA]
        || duration[DUR_LIQUID_FLAMES] || duration[DUR_QUAD_DAMAGE])
    {
        return (true);
    }
    if (check_haloed)
        return (haloed() && (self_halo || halo_radius2() == -1));
    return (false);
}

bool player::glows_naturally() const
{
    return (false);
}

// This is the imperative version.
void player::backlight()
{
    if (!duration[DUR_INVIS])
    {
        if (duration[DUR_CORONA] || you.glows_naturally())
            mpr("You glow brighter.");
        else
            mpr("You are outlined in light.");

        you.increase_duration(DUR_CORONA, random_range(15, 35), 250);
    }
    else
    {
        mpr("You feel strangely conspicuous.");

        you.increase_duration(DUR_CORONA, random_range(3, 5), 250);
    }
}

bool player::has_lifeforce() const
{
    const mon_holy_type holi = holiness();

    return (holi == MH_NATURAL || holi == MH_PLANT);
}

bool player::can_mutate() const
{
    return (true);
}

bool player::can_safely_mutate() const
{
    if (!can_mutate())
        return (false);

    return (!is_undead
            || is_undead == US_SEMI_UNDEAD
               && hunger_state == HS_ENGORGED);
}

bool player::can_bleed() const
{
    if (is_undead && (species != SP_VAMPIRE
                          || hunger_state <= HS_SATIATED))
    {
        return (false);
    }

    // The corresponding monsters don't bleed either.
    if (you.form == TRAN_STATUE || you.form == TRAN_ICE_BEAST
        || you.form == TRAN_LICH || you.form == TRAN_SPIDER)
    {
        return (false);
    }
    return (true);
}

bool player::mutate()
{
    ASSERT(!crawl_state.game_is_arena());

    if (!can_mutate())
        return (false);

    if (one_chance_in(5))
    {
        if (::mutate(RANDOM_MUTATION))
        {
            learned_something_new(HINT_YOU_MUTATED);
            return (true);
        }
    }

    return (give_bad_mutation());
}

bool player::is_icy() const
{
    return (form == TRAN_ICE_BEAST);
}

bool player::is_fiery() const
{
    return (false);
}

bool player::is_skeletal() const
{
    return (false);
}

void player::shiftto(const coord_def &c)
{
    crawl_view.shift_player_to(c);
    set_position(c);
}

void player::reset_prev_move()
{
    prev_move.reset();
}

bool player::asleep() const
{
    return (duration[DUR_SLEEP]);
}

bool player::cannot_act() const
{
    return (asleep() || cannot_move());
}

bool player::can_throw_large_rocks() const
{
    return (player_genus(GENPC_OGREISH) || species == SP_TROLL);
}

bool player::can_smell() const
{
    return (species != SP_MUMMY);
}

void player::hibernate(int)
{
    ASSERT(!crawl_state.game_is_arena());

    if (!can_hibernate())
    {
        canned_msg(MSG_YOU_UNAFFECTED);
        return;
    }

    mpr("You fall asleep.");

    stop_delay();
    flash_view(DARKGREY);

    // Do this *after* redrawing the view, or viewwindow() will no-op.
    set_duration(DUR_SLEEP, 3 + random2avg(5, 2));
}

void player::put_to_sleep(actor*, int power)
{
    ASSERT(!crawl_state.game_is_arena());

    if (!can_sleep())
    {
        canned_msg(MSG_YOU_UNAFFECTED);
        return;
    }

    mpr("You fall asleep.");

    stop_delay();
    flash_view(DARKGREY);

    // As above, do this after redraw.
    set_duration(DUR_SLEEP, 5 + random2avg(power/10, 5));
}

void player::awake()
{
    ASSERT(!crawl_state.game_is_arena());

    duration[DUR_SLEEP] = 0;
    mpr("You wake up.");
    flash_view(BLACK);
}

void player::check_awaken(int disturbance)
{
    if (asleep() && x_chance_in_y(disturbance + 1, 50))
        awake();
}

int player::beam_resists(bolt &beam, int hurted, bool doEffects,
                         std::string source)
{
    return check_your_resists(hurted, beam.flavour, source, &beam, doEffects);
}

void player::set_place_info(PlaceInfo place_info)
{
    place_info.assert_validity();

    if (place_info.is_global())
        global_info = place_info;
    else if (place_info.level_type == LEVEL_DUNGEON)
        branch_info[place_info.branch] = place_info;
    else
        non_branch_info[place_info.level_type - 1] = place_info;
}

std::vector<PlaceInfo> player::get_all_place_info(bool visited_only,
                                                  bool dungeon_only) const
{
    std::vector<PlaceInfo> list;

    for (int i = 0; i < NUM_BRANCHES; i++)
    {
        if (visited_only && branch_info[i].num_visits == 0
            || dungeon_only && branch_info[i].level_type != LEVEL_DUNGEON)
        {
            continue;
        }
        list.push_back(branch_info[i]);
    }

    for (int i = 0; i < (NUM_LEVEL_AREA_TYPES - 1); i++)
    {
        if (visited_only && non_branch_info[i].num_visits == 0
            || dungeon_only && non_branch_info[i].level_type != LEVEL_DUNGEON)
        {
            continue;
        }
        list.push_back(non_branch_info[i]);
    }

    return list;
}

bool player::do_shaft()
{
    dungeon_feature_type force_stair = DNGN_UNSEEN;

    if (!is_valid_shaft_level())
        return (false);

    // Handle instances of do_shaft() being invoked magically when
    // the player isn't standing over a shaft.
    if (get_trap_type(pos()) != TRAP_SHAFT)
    {
        switch (grd(pos()))
        {
        case DNGN_FLOOR:
        case DNGN_OPEN_DOOR:
        case DNGN_TRAP_MECHANICAL:
        case DNGN_TRAP_MAGICAL:
        case DNGN_TRAP_NATURAL:
        case DNGN_UNDISCOVERED_TRAP:
        case DNGN_ENTER_SHOP:
            break;

        default:
            return (false);
        }

        handle_items_on_shaft(pos(), false);

        if (!ground_level() || total_weight() == 0)
            return (true);

        force_stair = DNGN_TRAP_NATURAL;
    }

    down_stairs(force_stair);

    return (true);
}

bool player::did_escape_death() const
{
    return (escaped_death_cause != NUM_KILLBY);
}

void player::reset_escaped_death()
{
    escaped_death_cause = NUM_KILLBY;
    escaped_death_aux   = "";
}

void player::add_gold(int delta)
{
    set_gold(gold + delta);
}

void player::del_gold(int delta)
{
    set_gold(gold - delta);
}

void player::set_gold(int amount)
{
    ASSERT(amount >= 0);

    if (amount != gold)
    {
        const int old_gold = gold;
        gold = amount;
        shopping_list.gold_changed(old_gold, gold);
    }
}

void player::increase_duration(duration_type dur, int turns, int cap,
                               const char* msg)
{
    if (msg)
        mpr(msg);
    cap *= BASELINE_DELAY;

    you.duration[dur] += turns * BASELINE_DELAY;
    if (cap && you.duration[dur] > cap)
        you.duration[dur] = cap;
}

void player::set_duration(duration_type dur, int turns,
                          int cap, const char * msg)
{
    you.duration[dur] = 0;
    increase_duration(dur, turns, cap, msg);
}

void player::goto_place(const level_id &lid)
{
    level_type = lid.level_type;
    if (level_type == LEVEL_DUNGEON)
    {
        where_are_you = static_cast<branch_type>(lid.branch);
        absdepth0 = absdungeon_depth(lid.branch, lid.depth);
    }
}

/*
 * Check if the player is about to die from levitation/form expiration.
 *
 * Check whether the player is on a cell which would be deadly if not for some
 * temporary condition, and if such condition is expiring. In that case, we
 * give a strong warning to the player. The actual message printing is done
 * by the caller.
 *
 * @param dur the duration to check for dangerous expiration.
 * @param p the coordinates of the cell to check. Defaults to player position.
 * @return whether the player is in immediate danger.
 */
bool need_expiration_warning(duration_type dur, coord_def p)
{
    if (!is_feat_dangerous(env.grid(p), true) || !dur_expiring(dur))
        return false;

    if (dur == DUR_LEVITATION)
        return true;
    else if (dur == DUR_TRANSFORMATION
             && (!you.airborne() || form_can_fly()))
    {
        return true;
    }
    return false;
}

bool need_expiration_warning(coord_def p)
{
    return need_expiration_warning(DUR_LEVITATION, p)
           || need_expiration_warning(DUR_TRANSFORMATION, p);
}
