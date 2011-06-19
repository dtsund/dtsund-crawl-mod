/**
 * @file
 * @brief Misc function related to player transformations.
**/

#include "AppHdr.h"

#include "transform.h"

#include <stdio.h>
#include <string.h>

#include "externs.h"

#include "artefact.h"
#include "delay.h"
#include "env.h"
#include "godabil.h"
#include "invent.h"
#include "item_use.h"
#include "itemprop.h"
#include "items.h"
#include "output.h"
#include "player.h"
#include "player-equip.h"
#include "player-stats.h"
#include "random.h"
#include "religion.h"
#include "skills2.h"
#include "state.h"
#include "stuff.h"
#include "terrain.h"
#include "traps.h"
#include "xom.h"

static void _extra_hp(int amount_extra);

bool form_can_wield(transformation_type form)
{
    return (form == TRAN_NONE || form == TRAN_STATUE || form == TRAN_LICH);
}

bool form_can_fly(transformation_type form)
{
    if (form == TRAN_LICH
        && you.species == SP_KENKU
        && (you.experience_level >= 15 || you.airborne()))
    {
        return (true);
    }
    return (form == TRAN_DRAGON || form == TRAN_BAT);
}

bool form_can_swim(transformation_type form)
{
    // Ice floats.
    if (form == TRAN_ICE_BEAST)
        return (true);

    if (you.species == SP_MERFOLK && !form_changed_physiology(form))
        return (true);

    size_type size = you.transform_size(form, PSIZE_BODY);
    if (size == SIZE_CHARACTER)
        size = you.body_size(PSIZE_BODY, true);

    return (size >= SIZE_GIANT);
}

bool form_likes_water(transformation_type form)
{
    return (form_can_swim(form) || you.species == SP_GREY_DRACONIAN
                                   && !form_changed_physiology(form));
}

bool form_can_butcher_barehanded(transformation_type form)
{
    return (form == TRAN_BLADE_HANDS || form == TRAN_DRAGON
            || form == TRAN_ICE_BEAST);
}

// Used to mark transformations which override species/mutation intrinsics.
bool form_changed_physiology(transformation_type form)
{
    return (form != TRAN_NONE && form != TRAN_BLADE_HANDS);
}

bool form_can_wear_item(const item_def& item, transformation_type form)
{
    bool rc = true;

    if (item.base_type == OBJ_JEWELLERY)
    {
        // Everything but bats can wear all jewellery; bats and pigs can
        // only wear amulets.
        if ((form == TRAN_BAT || form == TRAN_PIG)
             && !jewellery_is_amulet(item))
        {
            rc = false;
        }
    }
    else
    {
        // It's not jewellery, and it's worn, so it must be armour.
        const equipment_type eqslot = get_armour_slot(item);

        switch (form)
        {
        // Some forms can wear everything.
        case TRAN_NONE:
        case TRAN_LICH:
            rc = true;
            break;

        // Some can't wear anything.
        case TRAN_DRAGON:
        case TRAN_BAT:
        case TRAN_PIG:
        case TRAN_SPIDER:
            rc = false;
            break;

        // And some need more complicated logic.
        case TRAN_BLADE_HANDS:
            rc = (eqslot != EQ_SHIELD && eqslot != EQ_GLOVES);
            break;

        case TRAN_STATUE:
            rc = (eqslot == EQ_CLOAK || eqslot == EQ_HELMET
                  || eqslot == EQ_SHIELD);
            break;

        case TRAN_ICE_BEAST:
            rc = (eqslot == EQ_CLOAK);
            break;

        default:                // Bug-catcher.
            mprf(MSGCH_ERROR, "Unknown transformation type %d in "
                 "form_can_wear_item",
                 you.form);
            break;
        }
    }

    return (rc);
}

static std::set<equipment_type>
_init_equipment_removal(transformation_type form)
{
    std::set<equipment_type> result;
    if (!form_can_wield(form) && you.weapon() || you.melded[EQ_WEAPON])
        result.insert(EQ_WEAPON);

    // Liches can't wield holy weapons.
    if (form == TRAN_LICH && you.weapon()
        && get_weapon_brand(*you.weapon()) == SPWPN_HOLY_WRATH)
    {
        result.insert(EQ_WEAPON);
    }

    for (int i = EQ_WEAPON + 1; i < NUM_EQUIP; ++i)
    {
        const equipment_type eq = static_cast<equipment_type>(i);
        const item_def *pitem = you.slot_item(eq, true);
        if (pitem && !form_can_wear_item(*pitem, form))
            result.insert(eq);
    }
    return (result);
}

static void _remove_equipment(const std::set<equipment_type>& removed,
                              bool meld = true, bool mutation = false)
{
    // Meld items into you in (reverse) order. (std::set is a sorted container)
    std::set<equipment_type>::const_iterator iter;
    for (iter = removed.begin(); iter != removed.end(); ++iter)
    {
        const equipment_type e = *iter;
        item_def *equip = you.slot_item(e, true);
        if (equip == NULL)
            continue;

        bool unequip = !meld;
        if (!unequip && e == EQ_WEAPON)
        {
            if (you.form == TRAN_NONE || form_can_wield(you.form))
                unequip = true;
            if (equip->base_type != OBJ_WEAPONS && equip->base_type != OBJ_STAVES)
                unequip = true;
        }

        mprf("%s %s%s %s", equip->name(DESC_CAP_YOUR).c_str(),
             unequip ? "fall" : "meld",
             equip->quantity > 1 ? "" : "s",
             unequip ? "away!" : "into your body.");

        if (unequip)
        {
            if (e == EQ_WEAPON)
            {
                const int slot = you.equip[EQ_WEAPON];
                unwield_item(!you.berserk());
                canned_msg(MSG_EMPTY_HANDED);
                you.attribute[ATTR_WEAPON_SWAP_INTERRUPTED] = slot + 1;
            }
            else
                unequip_item(e);

            if (mutation)
            {
                // A mutation made us not only lose an equipment slot
                // but actually removed a worn item: Funny!
                xom_is_stimulated(is_artefact(*equip) ? 255 : 128);
            }
        }
        else
            meld_slot(e);
    }
}

// FIXME: merge this with you_can_wear(), can_wear_armour(), etc.
static bool _mutations_prevent_wearing(const item_def& item)
{
    const equipment_type eqslot = get_armour_slot(item);

    if (is_hard_helmet(item)
        && (player_mutation_level(MUT_HORNS)
            || player_mutation_level(MUT_ANTENNAE)
            || player_mutation_level(MUT_BEAK)))
    {
        return (true);
    }

    // Barding is excepted here.
    if (item.sub_type == ARM_BOOTS
        && (player_mutation_level(MUT_HOOVES) >= 3
            || player_mutation_level(MUT_TALONS) >= 3))
    {
        return (true);
    }

    if (eqslot == EQ_GLOVES && player_mutation_level(MUT_CLAWS) >= 3)
        return (true);

    if (eqslot == EQ_HELMET && (player_mutation_level(MUT_HORNS) == 3
        || player_mutation_level(MUT_ANTENNAE) == 3))
        return (true);

    return (false);
}

static void _unmeld_equipment_slot(equipment_type e)
{
    item_def& item = you.inv[you.equip[e]];

    if (item.base_type == OBJ_JEWELLERY)
        unmeld_slot(e);
    else if (e == EQ_WEAPON)
    {
        if (you.slot_item(EQ_SHIELD)
            && is_shield_incompatible(item, you.slot_item(EQ_SHIELD)))
        {
            mpr(item.name(DESC_CAP_YOUR) + " is pushed off your body!");
            unequip_item(e);
        }
        else
            unmeld_slot(e);
    }
    else
    {
        // In case the player was mutated during the transformation,
        // check whether the equipment is still wearable.
        bool force_remove = _mutations_prevent_wearing(item);

        // If you switched weapons during the transformation, make
        // sure you can still wear your shield.
        // (This is only possible with Statue Form.)
        if (e == EQ_SHIELD && you.weapon()
            && is_shield_incompatible(*you.weapon(), &item))
        {
            force_remove = true;
        }

        if (force_remove)
        {
            mprf("%s is pushed off your body!",
                 item.name(DESC_CAP_YOUR).c_str());
            unequip_item(e);
        }
        else
            unmeld_slot(e);
    }
}

static void _unmeld_equipment(const std::set<equipment_type>& melded)
{
    // Unmeld items in order.
    std::set<equipment_type>::const_iterator iter;
    for (iter = melded.begin(); iter != melded.end(); ++iter)
    {
        const equipment_type e = *iter;
        if (you.equip[e] == -1)
            continue;

        _unmeld_equipment_slot(e);
    }
}

void unmeld_one_equip(equipment_type eq)
{
    std::set<equipment_type> e;
    e.insert(eq);
    _unmeld_equipment(e);
}

void remove_one_equip(equipment_type eq, bool meld, bool mutation)
{
    std::set<equipment_type> r;
    r.insert(eq);
    _remove_equipment(r, meld, mutation);
}

// FIXME: Switch to 4.1 transforms handling.
size_type transform_size(int psize)
{
    return you.transform_size(you.form, psize);
}

size_type player::transform_size(transformation_type tform, int psize) const
{
    switch (tform)
    {
    case TRAN_SPIDER:
    case TRAN_BAT:
        return SIZE_TINY;
    case TRAN_PIG:
        return SIZE_SMALL;
    case TRAN_ICE_BEAST:
        return SIZE_LARGE;
    case TRAN_DRAGON:
        return SIZE_HUGE;
    default:
        return SIZE_CHARACTER;
    }
}

void transformation_expiration_warning()
{
    if (you.duration[DUR_TRANSFORMATION]
            <= get_expiration_threshold(DUR_TRANSFORMATION))
    {
        mpr("You have a feeling this form won't last long.");
    }
}

static bool _abort_or_fizzle(bool just_check)
{
    if (!just_check && you.turn_is_over)
    {
        canned_msg(MSG_SPELL_FIZZLES);
        move_player_to_grid(you.pos(), false, true);
        return (true); // pay the necessary costs
    }
    return (false); // SPRET_ABORT
}

monster_type transform_mons()
{
    switch(you.form)
    {
    case TRAN_SPIDER:
        return MONS_SPIDER;
    case TRAN_STATUE:
        return MONS_STATUE;
    case TRAN_ICE_BEAST:
        return MONS_ICE_BEAST;
    case TRAN_DRAGON:
        return dragon_form_dragon_type();
    case TRAN_LICH:
        return MONS_LICH;
    case TRAN_BAT:
        return you.species == SP_VAMPIRE ? MONS_VAMPIRE_BAT : MONS_BAT;
    case TRAN_PIG:
        return MONS_HOG;
    case TRAN_BLADE_HANDS:
    case TRAN_NONE:
        return MONS_PLAYER;
    }
    die("unknown transformation");
    return MONS_PLAYER;
}

std::string blade_parts(bool terse)
{
    if (you.species == SP_CAT)
        return terse ? "paws" : "front paws";
    return "hands";
}

monster_type dragon_form_dragon_type()
{
    switch(you.species)
    {
        case SP_WHITE_DRACONIAN:
             return MONS_ICE_DRAGON;
        case SP_GREEN_DRACONIAN:
             return MONS_SWAMP_DRAGON;
        case SP_YELLOW_DRACONIAN:
             return MONS_GOLDEN_DRAGON;
        case SP_GREY_DRACONIAN:
             return MONS_IRON_DRAGON;
        case SP_BLACK_DRACONIAN:
             return MONS_STORM_DRAGON;
        case SP_PURPLE_DRACONIAN:
             return MONS_QUICKSILVER_DRAGON;
        case SP_MOTTLED_DRACONIAN:
             return MONS_MOTTLED_DRAGON;
        case SP_PALE_DRACONIAN:
             return MONS_STEAM_DRAGON;
        case SP_RED_DRACONIAN:
        default:
             return MONS_DRAGON;
    }
}

bool feat_dangerous_for_form(transformation_type which_trans,
                             dungeon_feature_type feat)
{
    // Everything is okay if we can fly.
    if (form_can_fly(which_trans) || you.is_levitating())
        return (false);

    // We can only cling for safety if we're already doing so.
    if (which_trans == TRAN_SPIDER && you.is_wall_clinging())
        return (false);

    if (feat == DNGN_LAVA)
        return (true);

    if (feat == DNGN_DEEP_WATER)
        return (!form_likes_water(which_trans) && !beogh_water_walk());

    return (false);
}

static bool _transformation_is_safe(transformation_type which_trans,
                                    dungeon_feature_type feat, bool quiet)
{
    if (!feat_dangerous_for_form(which_trans, feat))
        return (true);

    if (!quiet)
    {
        mprf("You would %s in your new form.",
             feat == DNGN_DEEP_WATER ? "drown" : "burn");
    }
    return (false);
}

// Transforms you into the specified form. If force is true, checks for
// inscription warnings are skipped, and the transformation fails silently
// (if it fails). If just_check is true the transformation doesn't actually
// happen, but the method returns whether it would be successful.
bool transform(int pow, transformation_type which_trans, bool force,
               bool just_check)
{
    transformation_type previous_trans = you.form;
    bool was_in_water = you.in_water();
    const flight_type was_flying = you.flight_mode();

    // Zin's protection.
    if (!just_check && you.religion == GOD_ZIN
        && x_chance_in_y(you.piety, MAX_PIETY) && which_trans != TRAN_NONE)
    {
        simple_god_message(" protects your body from unnatural transformation!");
        return (false);
    }

    if (!force && crawl_state.is_god_acting())
        force = true;

    if (!force && you.transform_uncancellable)
    {
        // Jiyva's wrath-induced transformation is blocking the attempt.
        // May need to be updated if transform_uncancellable is used for
        // other uses.
        return (false);
    }

    if (!_transformation_is_safe(which_trans, env.grid(you.pos()), force))
        return (false);

    // This must occur before the untransform() and the is_undead check.
    if (previous_trans == which_trans)
    {
        if (you.duration[DUR_TRANSFORMATION] < 100 * BASELINE_DELAY)
        {
            if (just_check)
                return (true);

            if (which_trans == TRAN_PIG)
                mpr("You feel you'll be a pig longer.");
            else
                mpr("You extend your transformation's duration.");
            you.increase_duration(DUR_TRANSFORMATION, random2(pow), 100);

            return (true);
        }
        else
        {
            if (!force && which_trans != TRAN_PIG)
                mpr("You cannot extend your transformation any further!");
            return (false);
        }
    }

    // The actual transformation may still fail later (e.g. due to cursed
    // equipment). Ideally, untransforming should cost a turn but nothing
    // else (as does the "End Transformation" ability). As it is, you
    // pay with mana and hunger if you already untransformed.
    if (!just_check && previous_trans != TRAN_NONE)
    {
        bool skip_wielding = false;
        switch (which_trans)
        {
        case TRAN_STATUE:
        case TRAN_LICH:
            break;
        default:
            skip_wielding = true;
            break;
        }
        // Skip wielding weapon if it gets unwielded again right away.
        untransform(skip_wielding, true);
    }

    // Catch some conditions which prevent transformation.
    if (you.is_undead
        && (you.species != SP_VAMPIRE
            || which_trans != TRAN_BAT && you.hunger_state <= HS_SATIATED))
    {
        if (!force)
            mpr("Your unliving flesh cannot be transformed in this way.");
        return (_abort_or_fizzle(just_check));
    }

    if (which_trans == TRAN_LICH && you.duration[DUR_DEATHS_DOOR])
    {
        if (!force)
        {
            mpr("The transformation conflicts with an enchantment "
                "already in effect.");
        }
        return (_abort_or_fizzle(just_check));
    }

    std::set<equipment_type> rem_stuff = _init_equipment_removal(which_trans);

    int str = 0, dex = 0, xhp = 0, dur = 0;
    const char* tran_name = "buggy";
    std::string msg;

    if (was_in_water && form_can_fly(which_trans))
        msg = "You fly out of the water as you turn into ";
    else if (form_can_fly(previous_trans)
             && form_can_swim(which_trans)
             && feat_is_water(grd(you.pos())))
        msg = "As you dive into the water, you turn into ";
    else
        msg = "You turn into ";

    switch (which_trans)
    {
    case TRAN_SPIDER:
        tran_name = "spider";
        dex       = 5;
        dur       = std::min(10 + random2(pow) + random2(pow), 60);
        msg      += "a venomous arachnid creature.";
        break;

    case TRAN_BLADE_HANDS:
        tran_name = "Blade Hands";
        if (you.species == SP_CAT)
            tran_name = "Blade Paws";
        dur       = std::min(10 + random2(pow), 100);
        msg       = "Your " + blade_parts()
                    + " turn into razor-sharp scythe blades.";
        break;

    case TRAN_STATUE:
        tran_name = "statue";
        str       = 2;
        dex       = -2;
        xhp       = 15;
        dur       = std::min(20 + random2(pow) + random2(pow), 100);
        if (player_genus(GENPC_DWARVEN) && one_chance_in(10))
            msg = "You inwardly fear your resemblance to a lawn ornament.";
        else
            msg += "a living statue of rough stone.";
        break;

    case TRAN_ICE_BEAST:
        tran_name = "ice beast";
        xhp       = 12;
        dur       = std::min(30 + random2(pow) + random2(pow), 100);
        msg      += "a creature of crystalline ice.";
        break;

    case TRAN_DRAGON:
        tran_name = "dragon";
        str       = 10;
        xhp       = 16;
        dur       = std::min(20 + random2(pow) + random2(pow), 100);
        msg      += "a fearsome dragon!";
        break;

    case TRAN_LICH:
        tran_name = "lich";
        str       = 3;
        dur       = std::min(20 + random2(pow) + random2(pow), 100);
        msg       = "Your body is suffused with negative energy!";
        break;

    case TRAN_BAT:
        tran_name = "bat";
        str       = -5;
        dex       = 5;
        dur       = std::min(20 + random2(pow) + random2(pow), 100);
        if (you.species == SP_VAMPIRE)
            msg += "a vampire bat.";
        else
            msg += "a bat.";
        break;

    case TRAN_PIG:
        tran_name = "pig";
        dur       = pow;
        msg       = "You have been turned into a pig!";
        you.transform_uncancellable = true;
        break;

    case TRAN_NONE:
        break;
    default:
        msg += "something buggy!";
    }

    // If we're just pretending return now.
    if (just_check)
        return (true);

    // All checks done, transformation will take place now.
    you.redraw_evasion      = true;
    you.redraw_armour_class = true;
    you.wield_change        = true;
    if (form_changed_physiology(which_trans))
        you.fishtail = false;

    // Most transformations conflict with stone skin.
    if (which_trans != TRAN_NONE
        && which_trans != TRAN_BLADE_HANDS
        && which_trans != TRAN_STATUE)
    {
        you.duration[DUR_STONESKIN] = 0;
    }

    // Give the transformation message.
    mpr(msg);

    // Update your status.
    you.form = which_trans;
    you.set_duration(DUR_TRANSFORMATION, dur);
    update_player_symbol();

    _remove_equipment(rem_stuff);
    burden_change();

    if (str)
    {
        notify_stat_change(STAT_STR, str, true,
                    make_stringf("gaining the %s transformation",
                                 tran_name).c_str());
    }

    if (dex)
    {
        notify_stat_change(STAT_DEX, dex, true,
                    make_stringf("gaining the %s transformation",
                                 tran_name).c_str());
    }

    if (xhp)
        _extra_hp(xhp);

    // Extra effects
    switch (which_trans)
    {
    case TRAN_STATUE:
        if (you.duration[DUR_STONESKIN])
            mpr("Your new body merges with your stone armour.");
        break;

    case TRAN_ICE_BEAST:
        if (you.duration[DUR_ICY_ARMOUR])
            mpr("Your new body merges with your icy armour.");
        break;

    case TRAN_DRAGON:
        if (you.attribute[ATTR_HELD])
        {
            mpr("The net rips apart!");
            you.attribute[ATTR_HELD] = 0;
            you.redraw_quiver = true;
            int net = get_trapping_net(you.pos());
            if (net != NON_ITEM)
                destroy_item(net);
        }
        break;

    case TRAN_LICH:
        // undead cannot regenerate -- bwr
        if (you.duration[DUR_REGENERATION])
        {
            mpr("You stop regenerating.", MSGCH_DURATION);
            you.duration[DUR_REGENERATION] = 0;
        }

        // silently removed since undead automatically resist poison -- bwr
        you.duration[DUR_RESIST_POISON] = 0;

        you.is_undead = US_UNDEAD;
        you.hunger_state = HS_SATIATED;  // no hunger effects while transformed
        set_redraw_status(REDRAW_HUNGER);
        break;

    default:
        break;
    }

    you.check_clinging(false);

    // This only has an effect if the transformation happens passively,
    // for example if Xom decides to transform you while you're busy
    // running around or butchering corpses.
    stop_delay();

    if (you.species != SP_VAMPIRE || which_trans != TRAN_BAT)
        transformation_expiration_warning();

    // Re-check terrain now that be may no longer be swimming or flying.
    if (was_flying && you.flight_mode() == FL_NONE
                   || feat_is_water(grd(you.pos()))
                      && which_trans == TRAN_BLADE_HANDS
                      && you.species == SP_MERFOLK)
    {
        move_player_to_grid(you.pos(), false, true);
    }

    return (true);
}

void untransform(bool skip_wielding, bool skip_move)
{
    const flight_type old_flight = you.flight_mode();

    you.redraw_evasion      = true;
    you.redraw_armour_class = true;
    you.wield_change        = true;

    // Must be unset first or else infinite loops might result. -- bwr
    const transformation_type old_form = you.form;

    // We may have to unmeld a couple of equipment types.
    std::set<equipment_type> melded = _init_equipment_removal(old_form);

    you.form = TRAN_NONE;
    you.duration[DUR_TRANSFORMATION]   = 0;
    update_player_symbol();

    burden_change();

    int hp_downscale = 10;

    switch (old_form)
    {
    case TRAN_SPIDER:
        mpr("Your transformation has ended.", MSGCH_DURATION);
        notify_stat_change(STAT_DEX, -5, true,
                     "losing the spider transformation");
        if (!skip_move)
            you.check_clinging(false);
        break;

    case TRAN_BAT:
        mpr("Your transformation has ended.", MSGCH_DURATION);
        notify_stat_change(STAT_DEX, -5, true,
                     "losing the bat transformation");
        notify_stat_change(STAT_STR, 5, true,
                     "losing the bat transformation");
        break;

    case TRAN_BLADE_HANDS:
        mprf(MSGCH_DURATION, "Your %s revert to their normal proportions.",
             blade_parts().c_str());
        you.wield_change = true;
        break;

    case TRAN_STATUE:
        mpr("You revert to your normal fleshy form.", MSGCH_DURATION);
        notify_stat_change(STAT_DEX, 2, true,
                     "losing the statue transformation");
        notify_stat_change(STAT_STR, -2, true,
                     "losing the statue transformation");

        // Note: if the core goes down, the combined effect soon disappears,
        // but the reverse isn't true. -- bwr
        if (you.duration[DUR_STONESKIN])
            you.duration[DUR_STONESKIN] = 1;

        hp_downscale = 15;
        break;

    case TRAN_ICE_BEAST:
        mpr("You warm up again.", MSGCH_DURATION);

        // Note: if the core goes down, the combined effect soon disappears,
        // but the reverse isn't true. -- bwr
        if (you.duration[DUR_ICY_ARMOUR])
            you.duration[DUR_ICY_ARMOUR] = 1;

        hp_downscale = 12;
        break;

    case TRAN_DRAGON:
        mpr("Your transformation has ended.", MSGCH_DURATION);
        notify_stat_change(STAT_STR, -10, true,
                    "losing the dragon transformation");
        hp_downscale = 16;
        break;

    case TRAN_LICH:
        mpr("You feel yourself come back to life.", MSGCH_DURATION);
        notify_stat_change(STAT_STR, -3, true,
                    "losing the lich transformation");
        you.is_undead = US_ALIVE;
        break;

    case TRAN_PIG:
        mpr("Your transformation has ended.", MSGCH_DURATION);
        break;

    default:
        break;
    }

    _unmeld_equipment(melded);

    // Re-check terrain now that be may no longer be swimming or flying.
    if (!skip_move && (old_flight && you.flight_mode() == FL_NONE
                       || (feat_is_water(grd(you.pos()))
                           && (old_form == TRAN_ICE_BEAST
                               || you.species == SP_MERFOLK))))
    {
        move_player_to_grid(you.pos(), false, true);
    }

    if (form_can_butcher_barehanded(old_form))
        stop_butcher_delay();

    // If nagas wear boots while transformed, they fall off again afterwards:
    // I don't believe this is currently possible, and if it is we
    // probably need something better to cover all possibilities.  -bwr

    // Removed barding check, no transformed creatures can wear barding
    // anyway.
    // *coughs* Ahem, blade hands... -- jpeg
    if (you.species == SP_NAGA || you.species == SP_CENTAUR)
    {
        const int arm = you.equip[EQ_BOOTS];

        if (arm != -1 && you.inv[arm].sub_type == ARM_BOOTS)
            remove_one_equip(EQ_BOOTS);
    }

    // End Ozocubu's Icy Armour if you unmelded wearing heavy armour
    if (you.duration[DUR_ICY_ARMOUR]
        && !player_effectively_in_light_armour())
    {
        you.duration[DUR_ICY_ARMOUR] = 1;

        const item_def *armour = you.slot_item(EQ_BODY_ARMOUR, false);
        mprf(MSGCH_DURATION, "%s cracks your icy armour.",
             armour->name(DESC_CAP_YOUR).c_str());
    }

    if (hp_downscale != 10 && you.hp != you.hp_max)
    {
        you.hp = you.hp * 10 / hp_downscale;
        if (you.hp < 1)
            you.hp = 1;
        else if (you.hp > you.hp_max)
            you.hp = you.hp_max;
    }
    calc_hp();

    if (!skip_wielding)
        handle_interrupted_swap(true, false, true);

    you.turn_is_over = true;
    if (you.transform_uncancellable)
        you.transform_uncancellable = false;
}

// XXX: This whole system is a mess as it still relies on special
// cases to handle a large number of things (see wear_armour()) -- bwr
bool can_equip(equipment_type use_which, bool ignore_temporary)
{
    if (use_which == EQ_HELMET
        && (player_mutation_level(MUT_HORNS)
            || player_mutation_level(MUT_BEAK)))
    {
        return (false);
    }

    if (use_which == EQ_BOOTS && !player_has_feet())
        return (false);

    if (use_which == EQ_GLOVES && you.has_claws(false) >= 3)
        return (false);

    if (!ignore_temporary)
    {
        switch (you.form)
        {
        case TRAN_NONE:
        case TRAN_LICH:
            return (true);

        case TRAN_BLADE_HANDS:
            return (use_which != EQ_WEAPON
                    && use_which != EQ_GLOVES
                    && use_which != EQ_SHIELD);

        case TRAN_STATUE:
            return (use_which == EQ_WEAPON
                    || use_which == EQ_SHIELD
                    || use_which == EQ_CLOAK
                    || use_which == EQ_HELMET);

        case TRAN_ICE_BEAST:
            return (use_which == EQ_CLOAK);

        default:
            return (false);
        }
    }

    return (true);
}

static void _extra_hp(int amount_extra) // must also set in calc_hp
{
    calc_hp();

    you.hp *= amount_extra;
    you.hp /= 10;

    deflate_hp(you.hp_max, false);
}
