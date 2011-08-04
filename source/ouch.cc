/**
 * @file
 * @brief Functions used when Bad Things happen to the player.
**/

#include "AppHdr.h"

#include <cstring>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cmath>

#ifdef TARGET_OS_DOS
#include <file.h>
#endif

#ifdef UNIX
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "ouch.h"

#ifdef TARGET_COMPILER_MINGW
#include <io.h>
#endif

#include "externs.h"
#include "options.h"

#include "artefact.h"
#include "beam.h"
#include "chardump.h"
#include "coord.h"
#include "delay.h"
#include "dgnevent.h"
#include "effects.h"
#include "env.h"
#include "files.h"
#include "fight.h"
#include "fineff.h"
#include "godabil.h"
#include "hints.h"
#include "hiscores.h"
#include "invent.h"
#include "itemname.h"
#include "itemprop.h"
#include "items.h"
#include "macro.h"
#include "message.h"
#include "mgen_data.h"
#include "misc.h"
#include "mon-util.h"
#include "mon-place.h"
#include "mon-stuff.h"
#include "notes.h"
#include "output.h"
#include "player.h"
#include "player-stats.h"
#include "random.h"
#include "religion.h"
#include "shopping.h"
#include "skills2.h"
#include "spl-selfench.h"
#include "spl-other.h"
#include "state.h"
#include "stuff.h"
#include "transform.h"
#include "travel.h"
#include "tutorial.h"
#include "view.h"
#include "shout.h"
#include "syscalls.h"
#include "xom.h"


static void _end_game(scorefile_entry &se);
static void _item_corrode(int slot);

//This is defined in travel.cc; it clears the default
//autotravel target.  We call it once here, when the player
//ends the game.
extern void reset_level_target();

static void _maybe_melt_player_enchantments(beam_type flavour)
{
    if (flavour == BEAM_FIRE || flavour == BEAM_LAVA
        || flavour == BEAM_HELLFIRE || flavour == BEAM_NAPALM
        || flavour == BEAM_STEAM)
    {
        if (you.duration[DUR_CONDENSATION_SHIELD] > 0)
            remove_condensation_shield();

        if (you.mutation[MUT_ICEMAIL])
        {
            mpr("Your icy envelope dissipates!", MSGCH_DURATION);
            you.duration[DUR_ICEMAIL_DEPLETED] = ICEMAIL_TIME;
            you.redraw_armour_class = true;
        }

        if (you.duration[DUR_ICY_ARMOUR] > 0)
            remove_ice_armour();
    }
}

// NOTE: DOES NOT check for hellfire!!!
int check_your_resists(int hurted, beam_type flavour, std::string source,
                       bolt *beam, bool doEffects)
{
    int resist;
    int original = hurted;

    dprf("checking resistance: flavour=%d", flavour);

    std::string kaux = "";
    if (beam)
    {
        source = beam->get_source_name();
        kaux = beam->name;
    }

    if (doEffects)
        _maybe_melt_player_enchantments(flavour);

    switch (flavour)
    {
    case BEAM_WATER:
        hurted = resist_adjust_damage(&you, flavour,
                                      you.res_water_drowning(), hurted, true);
        if (!hurted && doEffects)
            mpr("You shrug off the wave.");
        break;

    case BEAM_STEAM:
        hurted = resist_adjust_damage(&you, flavour,
                                      player_res_steam(), hurted, true);
        if (hurted < original && doEffects)
            canned_msg(MSG_YOU_RESIST);
        else if (hurted > original && doEffects)
        {
            mpr("The steam scalds you terribly!");
            xom_is_stimulated(200);
        }
        break;

    case BEAM_FIRE:
        hurted = resist_adjust_damage(&you, flavour,
                                      player_res_fire(), hurted, true);
        if (hurted < original && doEffects)
            canned_msg(MSG_YOU_RESIST);
        else if (hurted > original && doEffects)
        {
            mpr("The fire burns you terribly!");
            xom_is_stimulated(200);
        }
        break;

    case BEAM_COLD:
        hurted = resist_adjust_damage(&you, flavour,
                                      player_res_cold(), hurted, true);
        if (hurted < original && doEffects)
            canned_msg(MSG_YOU_RESIST);
        else if (hurted > original && doEffects)
        {
            mpr("You feel a terrible chill!");
            xom_is_stimulated(200);
        }
        break;

    case BEAM_ELECTRICITY:
        hurted = resist_adjust_damage(&you, flavour,
                                      player_res_electricity(),
                                      hurted, true);

        if (hurted < original && doEffects)
            canned_msg(MSG_YOU_RESIST);
        break;

    case BEAM_POISON:
        resist = player_res_poison();

        if (resist <= 0 && doEffects)
            poison_player(coinflip() ? 2 : 1, source, kaux);

        hurted = resist_adjust_damage(&you, flavour, resist,
                                      hurted, true);
        if (resist > 0 && doEffects)
            canned_msg(MSG_YOU_RESIST);
        break;

    case BEAM_POISON_ARROW:
        // [dshaligram] NOT importing uber-poison arrow from 4.1. Giving no
        // bonus to poison resistant players seems strange and unnecessarily
        // arbitrary.
        resist = player_res_poison();

        if (!resist && doEffects)
            poison_player(4 + random2(3), source, kaux, true);
        else if (!you.is_undead && doEffects)
            poison_player(2 + random2(3), source, kaux, true);

        hurted = resist_adjust_damage(&you, flavour, resist, hurted);
        if (hurted < original && doEffects)
            canned_msg(MSG_YOU_PARTIALLY_RESIST);
        break;

    case BEAM_NEG:
        resist = player_prot_life();

        // TSO's protection.
        if (you.religion == GOD_SHINING_ONE && you.piety > resist * 50)
        {
            int unhurted = std::min(hurted, (you.piety * hurted) / 150);

            if (unhurted > 0)
                hurted -= unhurted;
        }
        else if (resist > 0)
            hurted -= (resist * hurted) / 3;

        if (doEffects)
            drain_exp();
        break;

    case BEAM_ICE:
        hurted = resist_adjust_damage(&you, flavour, player_res_cold(),
                                      hurted, true);

        if (hurted < original && doEffects)
            canned_msg(MSG_YOU_PARTIALLY_RESIST);
        else if (hurted > original && doEffects)
        {
            mpr("You feel a painful chill!");
            xom_is_stimulated(200);
        }
        break;

    case BEAM_LAVA:
        hurted = resist_adjust_damage(&you, flavour, player_res_fire(),
                                      hurted, true);

        if (hurted < original && doEffects)
            canned_msg(MSG_YOU_PARTIALLY_RESIST);
        else if (hurted > original && doEffects)
        {
            mpr("The lava burns you terribly!");
            xom_is_stimulated(200);
        }
        break;

    case BEAM_ACID:
        if (player_res_acid())
        {
            if (doEffects)
                canned_msg(MSG_YOU_RESIST);
            hurted = hurted * player_acid_resist_factor() / 100;
        }
        break;

    case BEAM_MIASMA:
        if (you.res_rotting())
        {
            if (doEffects)
                canned_msg(MSG_YOU_RESIST);
            hurted = 0;
        }
        break;

    case BEAM_HOLY:
    {
        // Cleansing flame.
        const int rhe = you.res_holy_energy(NULL);
        if (rhe > 0)
            hurted = 0;
        else if (rhe == 0)
            hurted /= 2;
        else if (rhe < -1)
            hurted = (hurted * 3) / 2;

        if (hurted == 0 && doEffects)
            canned_msg(MSG_YOU_RESIST);
        break;
    }

    case BEAM_LIGHT:
        if (you.invisible())
            hurted = 0;
        else if (you.species == SP_VAMPIRE)
            hurted += hurted / 2;

        if (original && !hurted && doEffects)
            mpr("The beam of light passes harmlessly through you.");
        else if (hurted > original && doEffects)
        {
            mpr("The light scorches you terribly!");
            xom_is_stimulated(200);
        }
        break;

    case BEAM_AIR:
    {
        // Airstrike.
        if (you.res_wind() > 0)
            hurted = 0;
        else if (you.flight_mode())
            hurted += hurted / 2;
        break;
    }
    default:
        break;
    }                           // end switch

    return (hurted);
}

void splash_with_acid(int acid_strength, bool corrode_items,
                      std::string hurt_message)
{
    int dam = 0;
    const bool wearing_cloak = player_wearing_slot(EQ_CLOAK);

    for (int slot = EQ_MIN_ARMOUR; slot <= EQ_MAX_ARMOUR; slot++)
    {
        const bool cloak_protects = wearing_cloak && coinflip()
                                    && slot != EQ_SHIELD && slot != EQ_CLOAK;

        if (!cloak_protects)
        {
            if (!player_wearing_slot(slot) && slot != EQ_SHIELD)
                dam++;

            if (player_wearing_slot(slot) && corrode_items
                && x_chance_in_y(acid_strength + 1, 20))
            {
                _item_corrode(you.equip[slot]);
            }
        }
    }

    // Without fur, clothed people have dam 0 (+2 later), Sp/Tr/Dr/Og ~1
    // (randomized), Fe 5.  Fur helps only against naked spots.
    const int fur = player_mutation_level(MUT_SHAGGY_FUR);
    dam -= fur * dam / 5;

    // two extra virtual slots so players can't be immune
    dam += 2;
    dam = roll_dice(dam, acid_strength);

    const int post_res_dam = dam * player_acid_resist_factor() / 100;

    if (post_res_dam > 0)
    {
        mpr(hurt_message.empty() ? "The acid burns!" : hurt_message);

        if (post_res_dam < dam)
            canned_msg(MSG_YOU_RESIST);

        ouch(post_res_dam, NON_MONSTER, KILLED_BY_ACID);
    }
}

void weapon_acid(int acid_strength)
{
    int hand_thing = you.equip[EQ_WEAPON];

    if (hand_thing == -1 && !you.melded[EQ_GLOVES])
        hand_thing = you.equip[EQ_GLOVES];

    if (hand_thing == -1)
    {
        msg::stream << "Your " << your_hand(true) << " burn!" << std::endl;
        ouch(roll_dice(1, acid_strength), NON_MONSTER, KILLED_BY_ACID);
    }
    else if (x_chance_in_y(acid_strength + 1, 20))
        _item_corrode(hand_thing);
}

static void _item_corrode(int slot)
{
    bool it_resists = false;
    bool suppress_msg = false;
    item_def& item = you.inv[slot];

    // Artefacts don't corrode.
    if (is_artefact(item))
        return;

    // Anti-corrosion items protect against 90% of corrosion.
    if (wearing_amulet(AMU_RESIST_CORROSION) && !one_chance_in(10))
    {
        dprf("Amulet protects.");
        return;
    }

    int how_rusty = ((item.base_type == OBJ_WEAPONS) ? item.plus2 : item.plus);
    // Already very rusty.
    if (how_rusty < -5)
        return;

    // determine possibility of resistance by object type {dlb}:
    switch (item.base_type)
    {
    case OBJ_ARMOUR:
        if ((item.sub_type == ARM_CRYSTAL_PLATE_MAIL
             || get_equip_race(item) == ISFLAG_DWARVEN)
            && !one_chance_in(5))
        {
            it_resists = true;
            suppress_msg = false;
        }
        break;

    case OBJ_WEAPONS:
        if (get_equip_race(item) == ISFLAG_DWARVEN && !one_chance_in(5))
        {
            it_resists = true;
            suppress_msg = false;
        }
        break;

    default:
        // Other items can't corrode.
        return;
    }

    // determine chance of corrosion {dlb}:
    if (!it_resists)
    {
        const int chance = abs(how_rusty);

        // The embedded equation may look funny, but it actually works well
        // to generate a pretty probability ramp {6%, 18%, 34%, 58%, 98%}
        // for values [0,4] which closely matches the original, ugly switch.
        // {dlb}
        if (chance >= 0 && chance <= 4)
            it_resists = x_chance_in_y(2 + (4 << chance) + chance * 8, 100);
        else
            it_resists = true;

        // If the checks get this far, you should hear about it. {dlb}
        suppress_msg = false;
    }

    // handle message output and item damage {dlb}:
    if (!suppress_msg)
    {
        if (it_resists)
            mprf("%s resists.", item.name(DESC_CAP_YOUR).c_str());
        else
            mprf("The acid corrodes %s!", item.name(DESC_NOCAP_YOUR).c_str());
    }

    if (!it_resists)
    {
        how_rusty--;
        xom_is_stimulated(64);

        if (item.base_type == OBJ_WEAPONS)
            item.plus2 = how_rusty;
        else
            item.plus  = how_rusty;

        if (item.base_type == OBJ_ARMOUR)
            you.redraw_armour_class = true;

        if (you.equip[EQ_WEAPON] == slot)
            you.wield_change = true;
    }
}

// Helper function for the expose functions below.
// This currently works because elements only target a single type each.
static int _get_target_class(beam_type flavour)
{
    int target_class = OBJ_UNASSIGNED;

    switch (flavour)
    {
    case BEAM_FIRE:
    case BEAM_LAVA:
    case BEAM_NAPALM:
    case BEAM_HELLFIRE:
        target_class = OBJ_SCROLLS;
        break;

    case BEAM_COLD:
    case BEAM_FRAG:
        target_class = OBJ_POTIONS;
        break;

    case BEAM_SPORE:
    case BEAM_DEVOUR_FOOD:
        target_class = OBJ_FOOD;
        break;

    default:
        break;
    }

    return (target_class);
}

// XXX: These expose functions could use being reworked into a real system...
// the usage and implementation is currently very hacky.
// Handles the destruction of inventory items from the elements.
static bool _expose_invent_to_element(beam_type flavour, int strength)
{
    int num_dest = 0;
    int total_dest = 0;
    int jiyva_block = 0;

    const int target_class = _get_target_class(flavour);
    if (target_class == OBJ_UNASSIGNED)
        return (false);

    // Fedhas worshipers are exempt from the food destruction effect
    // of spores.
    if (flavour == BEAM_SPORE
        && you.religion == GOD_FEDHAS)
    {
        simple_god_message(" protects your food from the spores.",
                           GOD_FEDHAS);
        return (false);
    }

    // Currently we test against each stack (and item in the stack)
    // independently at strength%... perhaps we don't want that either
    // because it makes the system very fair and removes the protection
    // factor of junk (which might be more desirable for game play).
    for (int i = 0; i < ENDOFPACK; ++i)
    {
        if (!you.inv[i].defined())
            continue;

        if (you.inv[i].base_type == target_class
            || target_class == OBJ_FOOD
               && you.inv[i].base_type == OBJ_CORPSES)
        {
            // Conservation doesn't help against harpies' devouring food.
            if (flavour != BEAM_DEVOUR_FOOD
                && player_item_conserve() && !one_chance_in(10))
            {
                continue;
            }

            // These stack with conservation; they're supposed to be good.
            if (target_class == OBJ_SCROLLS
                && you.mutation[MUT_CONSERVE_SCROLLS]
                && !one_chance_in(10))
            {
                continue;
            }

            if (target_class == OBJ_POTIONS
                && you.mutation[MUT_CONSERVE_POTIONS]
                && !one_chance_in(10))
            {
                continue;
            }

            if (you.religion == GOD_JIYVA && !player_under_penance()
                && x_chance_in_y(you.piety, MAX_PIETY))
            {
                ++jiyva_block;
                continue;
            }

            // Get name and quantity before destruction.
            const std::string item_name = you.inv[i].name(DESC_PLAIN);
            const int quantity = you.inv[i].quantity;
            num_dest = 0;

            // Loop through all items in the stack.
            for (int j = 0; j < you.inv[i].quantity; ++j)
            {
                if (x_chance_in_y(strength, 100))
                {
                    num_dest++;

                    if (i == you.equip[EQ_WEAPON])
                        you.wield_change = true;

                    if (dec_inv_item_quantity(i, 1))
                        break;
                    else if (is_blood_potion(you.inv[i]))
                        remove_oldest_blood_potion(you.inv[i]);
                }
            }

            // Name destroyed items.
            // TODO: Combine messages using a vector.
            if (num_dest > 0)
            {
                switch (target_class)
                {
                case OBJ_SCROLLS:
                    mprf("%s %s catch%s fire!",
                         part_stack_string(num_dest, quantity).c_str(),
                         item_name.c_str(),
                         (num_dest == 1) ? "es" : "");
                    break;

                case OBJ_POTIONS:
                    mprf("%s %s freeze%s and shatter%s!",
                         part_stack_string(num_dest, quantity).c_str(),
                         item_name.c_str(),
                         (num_dest == 1) ? "s" : "",
                         (num_dest == 1) ? "s" : "");
                     break;

                case OBJ_FOOD:
                    // Message handled elsewhere.
                    if (flavour == BEAM_DEVOUR_FOOD)
                        break;
                    mprf("%s %s %s covered with spores!",
                         part_stack_string(num_dest, quantity).c_str(),
                         item_name.c_str(),
                         (num_dest == 1) ? "is" : "are");
                     break;

                default:
                    mprf("%s %s %s destroyed!",
                         part_stack_string(num_dest, quantity).c_str(),
                         item_name.c_str(),
                         (num_dest == 1) ? "is" : "are");
                     break;
                }

                total_dest += num_dest;
            }
        }
    }

    if (jiyva_block)
    {
        mprf("%s shields %s delectables from destruction.",
             god_name(GOD_JIYVA).c_str(),
             (total_dest > 0) ? "some of your" : "your");
    }

    if (!total_dest)
        return (false);

    // Message handled elsewhere.
    if (flavour == BEAM_DEVOUR_FOOD)
        return (true);

    xom_is_stimulated((num_dest > 1) ? 32 : 16);

    return (true);
}

bool expose_items_to_element(beam_type flavour, const coord_def& where,
                             int strength)
{
    int num_dest = 0;

    const int target_class = _get_target_class(flavour);
    if (target_class == OBJ_UNASSIGNED)
        return (false);

    // Beams fly *over* water and lava.
    if (grd(where) == DNGN_LAVA || grd(where) == DNGN_DEEP_WATER)
        return (false);

    for (stack_iterator si(where); si; ++si)
    {
        if (!si->defined())
            continue;

        if (si->base_type == target_class
            || target_class == OBJ_FOOD && si->base_type == OBJ_CORPSES)
        {
            if (x_chance_in_y(strength, 100))
            {
                num_dest++;
                if (!dec_mitm_item_quantity(si->index(), 1)
                    && is_blood_potion(*si))
                {
                    remove_oldest_blood_potion(*si);
                }
            }
        }
    }

    if (!num_dest)
        return (false);

    if (flavour == BEAM_DEVOUR_FOOD)
        return (true);

    if (you.see_cell(where))
    {
        switch (target_class)
        {
        case OBJ_SCROLLS:
            mprf("You see %s of smoke.",
                 (num_dest > 1) ? "some puffs" : "a puff");
            break;

        case OBJ_POTIONS:
            mprf("You see %s shatter.",
                 (num_dest > 1) ? "some glass" : "glass");
            break;

        case OBJ_FOOD:
            mprf("You see %s of spores.",
                 (num_dest > 1) ? "some clouds" : "a cloud");
            break;

        default:
            mprf("%s on the floor %s destroyed!",
                 (num_dest > 1) ? "Some items" : "An item",
                 (num_dest > 1) ? "were" : "was");
            break;
        }
    }

    xom_is_stimulated((num_dest > 1) ? 32 : 16);

    return (true);
}

// Handle side-effects for exposure to element other than damage.  This
// function exists because some code calculates its own damage instead
// of using check_your_resists() and we want to isolate all the special
// code they keep having to do... namely condensation shield checks,
// you really can't expect this function to even be called for much
// else.
//
// This function now calls _expose_invent_to_element() if strength > 0.
//
// XXX: This function is far from perfect and a work in progress.
bool expose_player_to_element(beam_type flavour, int strength)
{
    _maybe_melt_player_enchantments(flavour);

    if (strength <= 0)
        return (false);

    return (_expose_invent_to_element(flavour, strength));
}

void lose_level()
{
    // Because you.experience is unsigned long, if it's going to be
    // negative, must die straightaway.
    if (you.experience_level == 1)
    {
        ouch(INSTANT_DEATH, NON_MONSTER, KILLED_BY_DRAINING);
        // Return in case death was canceled via wizard mode
        return;
    }

    you.experience_level--;

    mprf(MSGCH_WARN,
         "You are now level %d!", you.experience_level);

    // Constant value to avoid grape jelly trick... see level_change() for
    // where these HPs and MPs are given back.  -- bwr
    ouch(4, NON_MONSTER, KILLED_BY_DRAINING);
    dec_max_hp(4);

    dec_mp(1);
    dec_max_mp(1);

    calc_hp();
    calc_mp();

    char buf[200];
    sprintf(buf, "HP: %d/%d MP: %d/%d",
            you.hp, you.hp_max, you.magic_points, you.max_magic_points);
    take_note(Note(NOTE_XP_LEVEL_CHANGE, you.experience_level, 0, buf));

    redraw_skill(you.your_name, player_title());
    you.redraw_experience = true;

    xom_is_stimulated(255);

    // Kill the player if maxhp <= 0.  We can't just move the ouch() call past
    // dec_max_hp() since it would decrease hp twice, so here's another one.
    ouch(0, NON_MONSTER, KILLED_BY_DRAINING);
}

bool drain_exp(bool announce_full)
{
    const int protection = player_prot_life();

    if (protection == 3)
    {
        if (announce_full)
            canned_msg(MSG_YOU_RESIST);

        return (false);
    }

    if (you.experience == 0)
    {
        ouch(INSTANT_DEATH, NON_MONSTER, KILLED_BY_DRAINING);

        // Return in case death was escaped via wizard mode.
        return (true);
    }

    if (you.experience_level == 1)
    {
        you.experience = 0;

        return (true);
    }

    unsigned int total_exp = exp_needed(you.experience_level + 1)
                                  - exp_needed(you.experience_level);
    unsigned int exp_drained = (total_exp * (5 + random2(11))) / 100;
    unsigned int pool_drained = std::min(exp_drained,
                                     (unsigned int)you.exp_available);

    // TSO's protection.
    if (you.religion == GOD_SHINING_ONE && you.piety > protection * 50)
    {
        unsigned int undrained = std::min(exp_drained,
                                      (you.piety * exp_drained) / 150);
        unsigned int pool_undrained = std::min(pool_drained,
                                           (you.piety * pool_drained) / 150);

        if (undrained > 0 || pool_undrained > 0)
        {
            simple_god_message(" protects your life force!");
            if (undrained > 0)
                exp_drained -= undrained;
            if (pool_undrained > 0)
                pool_drained -= pool_undrained;
        }
    }
    else if (protection > 0)
    {
        canned_msg(MSG_YOU_PARTIALLY_RESIST);
        exp_drained -= (protection * exp_drained) / 3;
        pool_drained -= (protection * pool_drained) / 3;
    }

    if (exp_drained > 0)
    {
        mpr("You feel drained.");
        xom_is_stimulated(20);
        you.experience -= exp_drained;
        you.exp_available -= pool_drained;

        you.exp_available = std::max(0, you.exp_available);

        dprf("You lose %d experience points, %d from pool.",
             exp_drained, pool_drained);

        level_change();

        return (true);
    }

    return (false);
}

static void _xom_checks_damage(kill_method_type death_type,
                               int dam, int death_source)
{
    if (you.religion == GOD_XOM)
    {
        if (death_type == KILLED_BY_TARGETING
            || death_type == KILLED_BY_BOUNCE
            || death_type == KILLED_BY_REFLECTION
            || death_type == KILLED_BY_SELF_AIMED
               && player_in_a_dangerous_place())
        {
            // Xom thinks the player accidentally hurting him/herself is funny.
            // Deliberate damage is only amusing if it's dangerous.
            int amusement = 255 * dam / (dam + you.hp);
            if (death_type == KILLED_BY_SELF_AIMED)
                amusement /= 5;
            xom_is_stimulated(amusement);
            return;
        }
        else if (death_type == KILLED_BY_FALLING_DOWN_STAIRS
                 || death_type == KILLED_BY_FALLING_THROUGH_GATE)
        {
            // Xom thinks falling down the stairs is hilarious.
            xom_is_stimulated(255);
            return;
        }
        else if (death_type == KILLED_BY_DISINT)
        {
            // flying chunks...
            xom_is_stimulated(128);
            return;
        }
        else if (death_type != KILLED_BY_MONSTER
                    && death_type != KILLED_BY_BEAM
                    && death_type != KILLED_BY_DISINT
                 || invalid_monster_index(death_source))
        {
            return;
        }

        int amusementvalue = 1;
        const monster* mons = &menv[death_source];

        if (!mons->alive())
            return;

        if (mons->wont_attack())
        {
            // Xom thinks collateral damage is funny.
            xom_is_stimulated(255 * dam / (dam + you.hp));
            return;
        }

        int leveldif = mons->hit_dice - you.experience_level;
        if (leveldif == 0)
            leveldif = 1;

        // Note that Xom is amused when you are significantly hurt by a
        // creature of higher level than yourself, as well as by a
        // creature of lower level than yourself.
        amusementvalue += leveldif * leveldif * dam;

        if (!mons->visible_to(&you))
            amusementvalue += 10;

        if (mons->speed < 100/player_movement_speed())
            amusementvalue += 8;

        if (death_type != KILLED_BY_BEAM
            && you.skill(SK_THROWING) <= (you.experience_level / 4))
        {
            amusementvalue += 2;
        }
        else if (you.skill(SK_FIGHTING) <= (you.experience_level / 4))
            amusementvalue += 2;

        if (player_in_a_dangerous_place())
            amusementvalue += 2;

        amusementvalue /= (you.hp > 0) ? you.hp : 1;

        xom_is_stimulated(amusementvalue);
    }
}

static void _yred_mirrors_injury(int dam, int death_source)
{
    if (yred_injury_mirror())
    {
        if (dam <= 0 || invalid_monster_index(death_source))
            return;

        add_final_effect(FINEFF_MIRROR_DAMAGE, &menv[death_source], &you,
                         coord_def(0, 0), dam);
    }
}

static void _maybe_spawn_jellies(int dam, const char* aux,
                                  kill_method_type death_type, int death_source)
{
    // We need to exclude acid damage and similar things or this function
    // will crash later.
    if (death_source == NON_MONSTER)
        return;

    monster_type mon = royal_jelly_ejectable_monster();

    // Exclude torment damage.
    const char *ptr = strstr(aux, "torment");
    if (you.religion == GOD_JIYVA && you.piety > 160 && ptr == NULL)
    {
        int how_many = 0;
        if (dam >= you.hp_max * 3 / 4)
            how_many = random2(4) + 2;
        else if (dam >= you.hp_max / 2)
            how_many = random2(2) + 2;
        else if (dam >= you.hp_max / 4)
            how_many = 1;

        if (how_many > 0)
        {
            if (x_chance_in_y(how_many, 8)
                && !lose_stat(STAT_STR, 1, true, "spawning slimes"))
            {
                canned_msg(MSG_NOTHING_HAPPENS);
                return;
            }

            int count_created = 0;
            for (int i = 0; i < how_many; ++i)
            {
                int foe = death_source;
                if (invalid_monster_index(foe))
                    foe = MHITNOT;
                mgen_data mg(mon, BEH_FRIENDLY, &you, 2, 0, you.pos(),
                             foe, 0, GOD_JIYVA);

                if (create_monster(mg) != -1)
                    count_created++;
            }

            if (count_created > 0)
            {
                mprf("You shudder from the %s and a %s!",
                     death_type == KILLED_BY_MONSTER ? "blow" : "blast",
                     count_created > 1 ? "flood of jellies pours out from you"
                                       : "jelly pops out");
            }
        }
    }
}

static void _pain_recover_mp(int dam)
{
    if (you.mutation[MUT_POWERED_BY_PAIN]
        && (you.magic_points < you.max_magic_points))
    {
        if (random2(dam) > 2 + 3 * player_mutation_level(MUT_POWERED_BY_PAIN)
            || dam >= you.hp_max / 2)
        {
            int gain_mp = roll_dice(3, 2 + 3 * player_mutation_level(MUT_POWERED_BY_PAIN));

            mpr("You focus.");
            inc_mp(gain_mp, false);
        }
    }
}

static void _place_player_corpse(bool explode)
{
    if (!in_bounds(you.pos()))
        return;

    item_def corpse;
    if (fill_out_corpse(0, player_mons(), corpse) == MONS_NO_MONSTER)
        return;

    if (explode && explode_corpse(corpse, you.pos()))
        return;

    int o = get_item_slot();
    if (o == NON_ITEM)
    {
        item_was_destroyed(corpse);
        return;
    }

    corpse.props[MONSTER_HIT_DICE].get_short() = you.experience_level;
    corpse.props[CORPSE_NAME_KEY] = you.your_name;
    corpse.props[CORPSE_NAME_TYPE_KEY].get_int64() = 0;
    corpse.props["ev"].get_int() = player_evasion(static_cast<ev_ignore_type>(
                                   EV_IGNORE_HELPLESS | EV_IGNORE_PHASESHIFT));
    // mostly mutations here.  At least there's no need to handle armour.
    corpse.props["ac"].get_int() = you.armour_class();
    mitm[o] = corpse;

    move_item_to_grid(&o, you.pos(), !you.in_water());
}


#if defined(WIZARD) || defined(DEBUG)
static void _wizard_restore_life()
{
    if (you.hp <= 0)
        set_hp(you.hp_max, false);
    for (int i = 0; i < NUM_STATS; ++i)
    {
        if (you.stat(static_cast<stat_type>(i)) <= 0)
        {
            you.stat_loss[i] = 0;
            you.redraw_stats[i] = true;
        }
    }
}
#endif

void reset_damage_counters()
{
    you.turn_damage = 0;
    you.damage_source = NON_MONSTER;
    you.source_damage = 0;
}

// death_source should be set to NON_MONSTER for non-monsters. {dlb}
void ouch(int dam, int death_source, kill_method_type death_type,
          const char *aux, bool see_source, const char *death_source_name)
{
    ASSERT(!crawl_state.game_is_arena());
    if (you.duration[DUR_TIME_STEP])
        return;

    if (you.dead) // ... but eligible for revival
        return;

    if (dam != INSTANT_DEATH && you.species == SP_DEEP_DWARF)
    {
        // Deep Dwarves get to shave any hp loss.
        int shave = 1 + random2(2 + random2(1 + you.experience_level / 3));
        dprf("HP shaved: %d.", shave);
        dam -= shave;
        if (dam <= 0)
        {
            // Rotting and costs may lower hp directly.
            if (you.hp > 0)
                return;
            dam = 0;
        }
    }

    ait_hp_loss hpl(dam, death_type);
    interrupt_activity(AI_HP_LOSS, &hpl);

    if (dam > 0)
        you.check_awaken(500);

    const bool non_death = death_type == KILLED_BY_QUITTING
        || death_type == KILLED_BY_WINNING
        || death_type == KILLED_BY_LEAVING;

    if (you.duration[DUR_DEATHS_DOOR] && death_type != KILLED_BY_LAVA
        && death_type != KILLED_BY_WATER && !non_death && you.hp_max > 0)
    {
        return;
    }

    if (dam != INSTANT_DEATH)
    {
        if (player_spirit_shield() && death_type != KILLED_BY_POISON)
        {
            if (dam <= you.magic_points)
            {
                dec_mp(dam);
                return;
            }
            dam -= you.magic_points;
            dec_mp(you.magic_points);
        }

        if (dam >= you.hp && god_protects_from_harm())
        {
            simple_god_message(" protects you from harm!");
            return;
        }

        you.turn_damage += dam;
        if (you.damage_source != death_source)
        {
            you.damage_source = death_source;
            you.source_damage = 0;
        }
        you.source_damage += dam;

        dec_hp(dam, true);

        // Even if we have low HP messages off, we'll still give a
        // big hit warning (in this case, a hit for half our HPs) -- bwr
        if (dam > 0 && you.hp_max <= dam * 2)
            mpr("Ouch! That really hurt!", MSGCH_DANGER);

        if (you.hp > 0)
        {
            if (Options.hp_warning
                && you.hp <= (you.hp_max * Options.hp_warning) / 100)
            {
                mpr("* * * LOW HITPOINT WARNING * * *", MSGCH_DANGER);
                dungeon_events.fire_event(DET_HP_WARNING);
            }

            hints_healing_check();

            _xom_checks_damage(death_type, dam, death_source);

            // for note taking
            std::string damage_desc;
            if (!see_source)
            {
                damage_desc = make_stringf("something (%d)", dam);
            }
            else
            {
                damage_desc = scorefile_entry(dam, death_source,
                                              death_type, aux, true)
                    .death_description(scorefile_entry::DDV_TERSE);
            }

            take_note(
                      Note(NOTE_HP_CHANGE, you.hp, you.hp_max, damage_desc.c_str()));

            _yred_mirrors_injury(dam, death_source);
            _maybe_spawn_jellies(dam, aux, death_type, death_source);
            _pain_recover_mp(dam);

            return;
        } // else hp <= 0
    }

    // Is the player being killed by a direct act of Xom?
    if (crawl_state.is_god_acting()
        && crawl_state.which_god_acting() == GOD_XOM
        && crawl_state.other_gods_acting().size() == 0)
    {
        you.escaped_death_cause = death_type;
        you.escaped_death_aux   = aux == NULL ? "" : aux;

        // Xom should only kill his worshippers if they're under penance
        // or Xom is bored.
        if (you.religion == GOD_XOM && !you.penance[GOD_XOM]
            && you.gift_timeout > 0)
        {
            return;
        }

        // Also don't kill wizards testing Xom acts.
        if ((crawl_state.repeat_cmd == CMD_WIZARD
                || crawl_state.prev_cmd == CMD_WIZARD)
            && you.religion != GOD_XOM)
        {
            return;
        }

        // Okay, you *didn't* escape death.
        you.reset_escaped_death();

        // Ensure some minimal information about Xom's involvement.
        if (aux == NULL || !*aux)
        {
            if (death_type != KILLED_BY_XOM)
                aux = "Xom";
        }
        else if (strstr(aux, "Xom") == NULL)
            death_type = KILLED_BY_XOM;
    }
    // Xom may still try to save your life.
    else if (xom_saves_your_life(dam, death_source, death_type, aux,
                                 see_source))
    {
        return;
    }

#if defined(WIZARD) || defined(DEBUG)
    if (you.never_die)
    {
        _wizard_restore_life();
        return;
    }
#endif

    crawl_state.cancel_cmd_all();

    // Construct scorefile entry.
    scorefile_entry se(dam, death_source, death_type, aux, false,
                       death_source_name);

#ifdef WIZARD
    if (!non_death)
    {
        if (crawl_state.test || you.wizard)
        {
            const std::string death_desc
                = se.death_description(scorefile_entry::DDV_VERBOSE);

            dprf("Damage: %d; Hit points: %d", dam, you.hp);

            if (crawl_state.test || !yesno("Die?", false, 'n'))
            {
                take_note(Note(NOTE_DEATH, you.hp, you.hp_max,
                                death_desc.c_str()), true);
                _wizard_restore_life();
                return;
            }
        }
    }
#endif  // WIZARD

    if (crawl_state.game_is_tutorial())
    {
        if (!non_death)
            tutorial_death_message();

        screen_end_game("");
    }

    // Okay, so you're dead.
    take_note(Note(NOTE_DEATH, you.hp, you.hp_max,
                    se.death_description(scorefile_entry::DDV_NORMAL).c_str()),
              true);
    if (you.lives && !non_death)
    {
        mark_milestone("death", lowercase_first(se.long_kill_message()).c_str());

        you.deaths++;
        you.lives--;
        you.dead = true;

        stop_delay(true);

        mprnojoin("You die...");
        xom_death_message((kill_method_type) se.get_death_type());
        more();

        _place_player_corpse(death_type == KILLED_BY_DISINT);
        return;
    }

    // The game's over.
    crawl_state.need_save       = false;
    crawl_state.updating_scores = true;

#if TAG_MAJOR_VERSION == 32
    note_montiers();
#endif

    // Prevent bogus notes.
    activate_notes(false);

#ifdef SCORE_WIZARD_CHARACTERS
    // Add this highscore to the score file.
    hiscores_new_entry(se);
    logfile_new_entry(se);
#else

    // Only add non-wizards to the score file.
    // Never generate bones files of wizard or tutorial characters -- bwr
    if (!you.wizard)
    {
        hiscores_new_entry(se);
        logfile_new_entry(se);

        if (!non_death && !crawl_state.game_is_tutorial())
            save_ghost();
    }
#endif

    _end_game(se);
}

static std::string _morgue_name(time_t when_crawl_got_even)
{
#ifdef SHORT_FILE_NAMES
    return "morgue";
#else  // !SHORT_FILE_NAMES
    std::string name = "morgue-" + you.your_name;

    std::string time = make_file_time(when_crawl_got_even);
    if (!time.empty())
        name += "-" + time;

    return (name);
#endif // SHORT_FILE_NAMES
}

// Delete save files on game end.
static void _delete_files()
{
    you.save->unlink();
    delete you.save;
    you.save = 0;
}

void screen_end_game(std::string text)
{
    crawl_state.cancel_cmd_all();
    _delete_files();

    if (!text.empty())
    {
        clrscr();
        linebreak_string2(text, get_number_of_cols());
        display_tagged_block(text);

        if (!crawl_state.seen_hups)
            get_ch();
    }

    game_ended();
}

void _end_game(scorefile_entry &se)
{
    //Make sure the game doesn't crash on future plays!
    reset_level_target();
    
    for (int i = 0; i < ENDOFPACK; i++)
        if (item_type_unknown(you.inv[i]))
            add_inscription(you.inv[i], "unknown");

    for (int i = 0; i < ENDOFPACK; i++)
    {
        if (!you.inv[i].defined())
            continue;
        set_ident_flags(you.inv[i], ISFLAG_IDENT_MASK);
        set_ident_type(you.inv[i], ID_KNOWN_TYPE);
        if (Options.autoinscribe_artefacts && is_artefact(you.inv[i]))
        {
            std::string inscr = artefact_auto_inscription(you.inv[i]);
            if (inscr != "")
                add_autoinscription(you.inv[i], inscr);
        }
    }

    _delete_files();

    // death message
    if (se.get_death_type() != KILLED_BY_LEAVING
        && se.get_death_type() != KILLED_BY_QUITTING
        && se.get_death_type() != KILLED_BY_WINNING)
    {
        mprnojoin("You die...");      // insert player name here? {dlb}
        xom_death_message((kill_method_type) se.get_death_type());

        switch (you.religion)
        {
        case GOD_FEDHAS:
            simple_god_message(" appreciates your contribution to the "
                               "ecosystem.");
            break;

        case GOD_NEMELEX_XOBEH:
            nemelex_death_message();
            break;

        case GOD_KIKUBAAQUDGHA:
            if (you.is_undead
                && you.form != TRAN_LICH)
            {
                simple_god_message(" rasps: \"You have failed me! "
                                   "Welcome... oblivion!\"");
            }
            else
            {
                simple_god_message(" rasps: \"You have failed me! "
                                   "Welcome... death!\"");
            }
            break;

        case GOD_YREDELEMNUL:
            if (you.is_undead)
                simple_god_message(" claims you as an undead slave.");
            else if (se.get_death_type() != KILLED_BY_DISINT
                     && se.get_death_type() != KILLED_BY_LAVA)
            {
                mpr("Your body rises from the dead as a mindless zombie.",
                    MSGCH_GOD);
            }
            // No message if you're not undead and your corpse is lost.
            break;

        default:
            break;
        }

        flush_prev_message();
        viewwindow(); // don't do for leaving/winning characters

        if (crawl_state.game_is_hints())
            hints_death_screen();
    }

    if (!dump_char(_morgue_name(se.get_death_time()), false, true, &se))
    {
        mpr("Char dump unsuccessful! Sorry about that.");
        if (!crawl_state.seen_hups)
            more();
        clrscr();
    }

#ifdef DGL_WHEREIS
    whereis_record(se.get_death_type() == KILLED_BY_QUITTING? "quit" :
                   se.get_death_type() == KILLED_BY_WINNING ? "won"  :
                   se.get_death_type() == KILLED_BY_LEAVING ? "bailed out"
                                                            : "dead");
#endif

    if (!crawl_state.seen_hups)
        more();

    browse_inventory(true);
    textcolor(LIGHTGREY);

    // Prompt for saving macros.
    if (crawl_state.unsaved_macros && yesno("Save macros?", true, 'n'))
        macro_save();

    clrscr();
    cprintf("Goodbye, %s.", you.your_name.c_str());
    cprintf("\n\n    "); // Space padding where # would go in list format

    std::string hiscore = hiscores_format_single_long(se, true);

    const int lines = count_occurrences(hiscore, "\n") + 1;

    cprintf("%s", hiscore.c_str());

    cprintf("\nBest Crawlers - %s\n",
            crawl_state.game_type_name().c_str());

    // "- 5" gives us an extra line in case the description wraps on a line.
    hiscores_print_list(get_number_of_lines() - lines - 5);

#ifndef DGAMELAUNCH
    cprintf("\nYou can find your morgue file in the '%s' directory.",
            morgue_directory().c_str());
#endif

    // just to pause, actual value returned does not matter {dlb}
    if (!crawl_state.seen_hups)
        get_ch();

    if (se.get_death_type() == KILLED_BY_WINNING)
        crawl_state.last_game_won = true;

    game_ended();
}

int actor_to_death_source(const actor* agent)
{
    if (agent->atype() == ACT_PLAYER)
        return (NON_MONSTER);
    else if (agent->atype() == ACT_MONSTER)
        return (agent->as_monster()->mindex());
    else
        return (NON_MONSTER);
}
