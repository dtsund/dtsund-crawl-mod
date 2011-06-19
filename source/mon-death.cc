/**
 * @file
 * @brief Contains monster death functionality, including Dowan and Duvessa,
 *        Kirke, Pikel, shedu and spirits.
**/

#include "AppHdr.h"
#include "mon-death.h"

#include "areas.h"
#include "coordit.h"
#include "database.h"
#include "env.h"
#include "items.h"
#include "message.h"
#include "mgen_data.h"
#include "mon-behv.h"
#include "mon-iter.h"
#include "mon-place.h"
#include "mon-speak.h"
#include "mon-stuff.h"
#include "mon-util.h"
#include "random.h"
#include "religion.h"
#include "state.h"
#include "stuff.h"
#include "transform.h"
#include "view.h"

/**
 * Determine if a specified monster is Pikel.
 *
 * Checks both the monster type and the "original_name" prop, thus allowing
 * Pikelness to be transferred through polymorph.
 *
 * @param mons    The monster to be checked.
 * @returns       True if the monster is Pikel, otherwise false.
**/
bool mons_is_pikel (monster* mons)
{
    return (mons->type == MONS_PIKEL
            || (mons->props.exists("original_name")
                && mons->props["original_name"].get_string() == "Pikel"));
}

/**
 * Perform neutralisation for members of Pikel's band upon Pikel's 'death'.
 *
 * This neutralisation occurs in multiple instances: when Pikel is neutralised,
 * enslaved, leaves the level and leaves behind some slaves, when Pikel dies,
 * when Pikel is banished.
 *
 * @param check_tagged    If True, monsters leaving the level are ignored.
**/
void pikel_band_neutralise (bool check_tagged)
{
    bool message_made = false;

    for (monster_iterator mi; mi; ++mi)
    {
        if (mi->type == MONS_SLAVE
            && testbits(mi->flags, MF_BAND_MEMBER)
            && mi->props.exists("pikel_band")
            && mi->mname != "freed slave")
        {
            // Don't neutralise band members that are leaving the level with us.
            if (check_tagged && testbits(mi->flags, MF_TAKING_STAIRS))
                continue;

            if (mi->observable() && !message_made)
            {
                if (check_tagged)
                    mprf("With Pikel's spell partly broken, some of the slaves are set free!");
                else
                    mprf("With Pikel's spell broken, the former slaves thank you for their freedom.");

                message_made = true;
            }
            mi->flags |= MF_NAME_REPLACE | MF_NAME_DESCRIPTOR;
            mi->mname = "freed slave";
            mons_pacify(*mi);
        }
    }
}

/**
 * Determine if a monster is Kirke.
 *
 * As with mons_is_pikel, tracks Kirke via type and original name, thus allowing
 * tracking through polymorph.
 *
 * @param mons    The monster to check.
 * @returns       True if Kirke, false otherwise.
**/
bool mons_is_kirke (monster* mons)
{
    return (mons->type == MONS_KIRKE
            || (mons->props.exists("original_name")
                && mons->props["original_name"].get_string() == "Kirke"));
}

/**
 * Convert hogs to neutral humans.
 *
 * Called upon Kirke's death. This does not track members of her band that
 * have been transferred across levels. Non-hogs are ignored. If a monster has
 * an ORIG_MONSTER_KEY prop, it will be returned to its previous state,
 * otherwise it will be converted to a neutral human.
**/
void hogs_to_humans()
{
    // Simplification: if, in a rare event, another hog which was not created
    // as a part of Kirke's band happens to be on the level, the player can't
    // tell them apart anyway.
    // On the other hand, hogs which left the level are too far away to be
    // affected by the magic of Kirke's death.
    int any = 0, human = 0;

    for (monster_iterator mi; mi; ++mi)
    {
        if (mi->type != MONS_HOG)
            continue;

        // Shapeshifters will stop being a hog when they feel like it.
        if (mi->is_shapeshifter())
            continue;

        const bool could_see = you.can_see(*mi);

        monster orig;

        if (mi->props.exists(ORIG_MONSTER_KEY))
            // Copy it, since the instance in props will get deleted
            // as soon a **mi is assigned to.
            orig = mi->props[ORIG_MONSTER_KEY].get_monster();
        else
        {
            orig.type     = MONS_HUMAN;
            orig.attitude = mi->attitude;
            define_monster(&orig);
        }
        orig.mid = mi->mid;

        // Keep at same spot.
        const coord_def pos = mi->pos();
        // Preserve relative HP.
        const float hp
            = (float) mi->hit_points / (float) mi->max_hit_points;
        // Preserve some flags.
        const uint64_t preserve_flags =
            mi->flags & ~(MF_JUST_SUMMONED | MF_WAS_IN_VIEW);
        // Preserve enchantments.
        mon_enchant_list enchantments = mi->enchantments;

        // Restore original monster.
        **mi = orig;

        mi->move_to_pos(pos);
        mi->enchantments = enchantments;
        mi->hit_points   = std::max(1, (int) (mi->max_hit_points * hp));
        mi->flags        = mi->flags | preserve_flags;

        const bool can_see = you.can_see(*mi);

        // A monster changing factions while in the arena messes up
        // arena book-keeping.
        if (!crawl_state.game_is_arena())
        {
            // * A monster's attitude shouldn't downgrade from friendly
            //   or good-neutral because you helped it.  It'd suck to
            //   lose a permanent ally that way.
            //
            // * A monster has to be smart enough to realize that you
            //   helped it.
            if (mi->attitude == ATT_HOSTILE
                && mons_intel(*mi) >= I_NORMAL)
            {
                mi->attitude = ATT_GOOD_NEUTRAL;
                mi->flags   |= MF_WAS_NEUTRAL;
                mons_att_changed(*mi);
            }
        }

        behaviour_event(*mi, ME_EVAL);

        if (could_see && can_see)
        {
            any++;
            if (mi->type == MONS_HUMAN)
                human++;
        }
        else if (could_see && !can_see)
            mpr("The hog vanishes!");
        else if (!could_see && can_see)
            mprf("%s appears from out of thin air!",
                 mi->name(DESC_CAP_A).c_str());
    }

    if (any == 1)
    {
        if (any == human)
            mpr("No longer under Kirke's spell, the hog turns into a human!");
        else
            mpr("No longer under Kirke's spell, the hog returns to its "
                "original form!");
    }
    else if (any > 1)
    {
        if (any == human)
            mpr("No longer under Kirke's spell, all hogs revert to their "
                "human forms!");
        else
            mpr("No longer under Kirke's spell, all hogs revert to their "
                "original forms!");
    }

    // Revert the player as well.
    if (you.form == TRAN_PIG)
        untransform();
}

/**
 * Determine if a monster is Dowan.
 *
 * Tracks through type and original_name, thus tracking through polymorph.
 *
 * @param mons    The monster to check.
 * @returns       True if Dowan, otherwise false.
**/
bool mons_is_dowan(const monster* mons)
{
    return (mons->type == MONS_DOWAN
            || (mons->props.exists("original_name")
                && mons->props["original_name"].get_string() == "Dowan"));
}

/**
 * Determine if a monster is Duvessa.
 *
 * Tracks through type and original_name, thus tracking through polymorph.
 *
 * @param mons    The monster to check.
 * @returns       True if Duvessa, otherwise false.
**/
bool mons_is_duvessa(const monster* mons)
{
    return (mons->type == MONS_DUVESSA
            || (mons->props.exists("original_name")
                && mons->props["original_name"].get_string() == "Duvessa"));
}

/**
 * Determine if a monster is either Dowan or Duvessa.
 *
 * Tracks through type and original_name, thus tracking through polymorph. A
 * wrapper around mons_is_dowan and mons_is_duvessa. Used to determine if a
 * death function should be called for the monster in question.
 *
 * @param mons    The monster to check.
 * @returns       True if either Dowan or Duvessa, otherwise false.
**/
bool mons_is_elven_twin(const monster* mons)
{
    return (mons_is_dowan(mons) || mons_is_duvessa(mons));
}

/**
 * Perform functional changes Dowan or Duvessa upon the other's death.
 *
 * This functional is called when either Dowan or Duvessa are killed or
 * banished. It performs a variety of changes in both attitude, spells, flavour,
 * speech, etc.
 *
 * @param twin          The monster who died.
 * @param in_transit    True if banished, otherwise false.
 * @param killer        The kill-type related to twin.
 * @param killer_index  The index of the actor who killed twin.
**/
void elven_twin_died(monster* twin, bool in_transit, killer_type killer, int killer_index)
{
    // Sometimes, if you pacify one twin near a staircase, they leave
    // in the same turn. Convert, in those instances.
    if (twin->neutral())
    {
        elven_twins_pacify(twin);
        return;
    }

    bool found_duvessa = false;
    bool found_dowan = false;
    monster* mons;

    for (monster_iterator mi; mi; ++mi)
    {
        if (*mi == twin)
            continue;

        // Don't consider already neutralised monsters.
        if ((*mi)->good_neutral())
            continue;

        if (mons_is_duvessa(*mi))
        {
            mons = *mi;
            found_duvessa = true;
            break;
        }
        else if (mons_is_dowan(*mi))
        {
            mons = *mi;
            found_dowan = true;
            break;
        }
    }

    if (!found_duvessa && !found_dowan)
        return;

    // Okay, let them climb stairs now.
    mons->props["can_climb"] = "yes";
    if (!in_transit)
        mons->props["speech_prefix"] = "twin_died";
    else
        mons->props["speech_prefix"] = "twin_banished";

    // If you've stabbed one of them, the other one is likely asleep still.
    if (mons->asleep())
        behaviour_event(mons, ME_DISTURB, MHITNOT, mons->pos());

    // Will generate strings such as 'Duvessa_Duvessa_dies' or, alternately
    // 'Dowan_Dowan_dies', but as neither will match, these can safely be
    // ignored.
    std::string key = "_" + mons->name(DESC_CAP_THE, true) + "_"
                          + twin->name(DESC_CAP_THE) + "_dies_";

    if (mons_near(mons) && !mons->observable())
        key += "invisible_";
    else if (!mons_near(mons))
        key += "distance_";

    bool i_killed = ((killer == KILL_MON || killer == KILL_MON_MISSILE)
                      && mons->mindex() == killer_index);

    if (i_killed)
    {
        key += "bytwin_";
        mons->props["speech_prefix"] = "twin_ikilled";
    }

    std::string death_message = getSpeakString(key);

    // Check if they can speak or not: they may have been polymorphed.
    if (mons_near(mons) && !death_message.empty() && mons->can_speak())
        mons_speaks_msg(mons, death_message, MSGCH_TALK, silenced(you.pos()));
    else if (mons->can_speak())
        mprf("%s", death_message.c_str());

    if (found_duvessa)
    {
        if (mons_near(mons))
            // Provides its own flavour message.
            mons->go_berserk(true);
        else
            // She'll go berserk the next time she sees you
            mons->props["duvessa_berserk"] = bool(true);
    }
    else if (found_dowan)
    {
        if (mons->observable())
        {
            mons->add_ench(ENCH_HASTE);
            simple_monster_message(mons, " seems to find hidden reserves of power!");
        }
        else
            mons->props["dowan_upgrade"] = bool(true);

        mons->spells[0] = SPELL_THROW_ICICLE;
        mons->spells[1] = SPELL_BLINK;
        mons->spells[3] = SPELL_STONE_ARROW;
        mons->spells[4] = SPELL_HASTE;
        // Nothing with 6.
    }
}

/**
 * Pacification effects for Dowan and Duvessa.
 *
 * As twins, pacifying one pacifies the other.
 *
 * @param twin    The orignial monster pacified.
**/
void elven_twins_pacify (monster* twin)
{
    bool found_duvessa = false;
    bool found_dowan = false;
    monster* mons;

    for (monster_iterator mi; mi; ++mi)
    {
        if (*mi == twin)
            continue;

        // Don't consider already neutralised monsters.
        if ((*mi)->neutral())
            continue;

        if (mons_is_duvessa(*mi))
        {
            mons = *mi;
            found_duvessa = true;
            break;
        }
        else if (mons_is_dowan(*mi))
        {
            mons = *mi;
            found_dowan = true;
            break;
        }
    }

    if (!found_duvessa && !found_dowan)
        return;

    // This shouldn't happen, but sometimes it does.
    if (mons->neutral())
        return;

    if (you.religion == GOD_ELYVILON)
        gain_piety(random2(mons->max_hit_points / (2 + you.piety / 20)), 2);

    if (mons_near(mons))
        simple_monster_message(mons, " likewise turns neutral.");

    mons_pacify(mons, ATT_NEUTRAL);
}

/**
 * Unpacification effects for Dowan and Duvessa.
 *
 * If they are both pacified and you attack one, the other will not remain
 * neutral. This is both for flavour (they do things together), and
 * functionality (so Dowan does not begin beating on Duvessa, etc).
 *
 * @param twin    The monster attacked.
**/
void elven_twins_unpacify (monster* twin)
{
    bool found_duvessa = false;
    bool found_dowan = false;
    monster* mons;

    for (monster_iterator mi; mi; ++mi)
    {
        if (*mi == twin)
            continue;

        // Don't consider already neutralised monsters.
        if (!(*mi)->neutral())
            continue;

        if (mons_is_duvessa(*mi))
        {
            mons = *mi;
            found_duvessa = true;
            break;
        }
        else if (mons_is_dowan(*mi))
        {
            mons = *mi;
            found_dowan = true;
            break;
        }
    }

    if (!found_duvessa && !found_dowan)
        return;

    behaviour_event(mons, ME_WHACK, MHITYOU, you.pos(), false);
}

/**
 * Spirit death effects.
 *
 * When a spirit dies of "nautral" causes, ie, it's FADING_AWAY timer runs out,
 * it summons a high-level monster. This function is only called for spirits
 * that have died as a result of the FADING_AWAY timer enchantment running out.
 *
 * @param spirit    The monster that died.
**/
void spirit_fades (monster *spirit)
{

    if (mons_near(spirit))
        simple_monster_message(spirit, " fades away with a wail!", MSGCH_TALK);
    else
        mprf("You hear a distant wailing.", MSGCH_TALK);

    const coord_def c = spirit->pos();

    mgen_data mon = mgen_data(static_cast<monster_type>(random_choose_weighted(
                        10, MONS_SILVER_STAR, 10, MONS_PHOENIX,
                        10, MONS_APIS,        5,  MONS_DAEVA,
                        2,  MONS_PEARL_DRAGON,
                      0)), SAME_ATTITUDE(spirit),
                      NULL, 0, 0, c,
                      spirit->foe, 0);

    if (spirit->alive())
        monster_die(spirit, KILL_MISC, NON_MONSTER, true);

    int mon_id = create_monster(mon);

    if (mon_id == -1)
        return;

    monster *new_mon = &menv[mon_id];

    if (mons_near(new_mon))
        simple_monster_message(new_mon, " seeks to avenge the fallen spirit!", MSGCH_TALK);
    else
        mprf("A powerful presence appears to avenge a fallen spirit!", MSGCH_TALK);

}

/**
 * Determine a shedu's pair by index.
 *
 * The index of a shedu's pair is stored as mons->number. This function attempts
 * to return a pointer to that monster. If that monster doesn't exist, or is
 * dead, returns NULL.
 *
 * @param mons    The monster whose pair we're searching for. Assumed to be a
 *                 shedu.
 * @returns        Either a monster* or NULL if a monster was not found.
**/
monster* get_shedu_pair (const monster* mons)
{
    monster* pair = monster_by_mid(mons->number);
    if (pair)
        return (pair);

    return (NULL);
}

/**
 * Determine if a shedu's pair is alive.
 *
 * A simple function that checks the return value of get_shedu_pair is not null.
 *
 * @param mons    The monster whose pair we are searching for.
 * @returns        True if the pair is alive, False otherwise.
**/
bool shedu_pair_alive (const monster* mons)
{
    if (get_shedu_pair(mons) == NULL)
        return (false);

    return (true);
}

/**
 * Determine if a monster is or was a shedu.
 *
 * @param mons    The monster to check.
 * @returns        Either True if the monster is or was a shedu, otherwise
 *                 False.
**/
bool mons_is_shedu(const monster* mons)
{
    return (mons->type == MONS_SHEDU
            || (mons->props.exists("original_name")
                && mons->props["original_name"].get_string() == "shedu"));
}

/**
 * Initial resurrection functionality for Shedu.
 *
 * This function is called when a shedu dies. It attempt to find that shedu's
 * pair, wake them if necessary, and then begin the resurrection process by
 * giving them the ENCH_PREPARING_RESURRECT enchantment timer. If a pair does
 * not exist (ie, this is the second shedu to have died), nothing happens.
 *
 * @param mons    The shedu who died.
**/
void shedu_do_resurrection (const monster* mons)
{
    if (!mons_is_shedu(mons))
        return;

    if (mons->number == 0)
        return;

    monster* my_pair = get_shedu_pair(mons);
    if (!my_pair)
        return;

    // Wake the other one up if it's asleep.
    if (my_pair->asleep())
        behaviour_event(my_pair, ME_DISTURB, MHITNOT, my_pair->pos());

    if (you.can_see(my_pair))
        simple_monster_message(my_pair, " ceases action and prepares to resurrect its fallen mate.");

    my_pair->add_ench(ENCH_PREPARING_RESURRECT);
}

/**
 * Perform resurrection of a shedu.
 *
 * This function is called when the ENCH_PREPARING_RESURRECT timer runs out. It
 * determines if there is a viable corpse (of which there will always be one),
 * where that corpse is (if it is not in line of sight, no resurrection occurs;
 * if it is in your pack, it is resurrected "from" your pack, etc), and then
 * perform the actual resurrection by creating a new shedu monster.
 *
 * @param mons    The shedu who is to perform the resurrection.
**/
void shedu_do_actual_resurrection (monster* mons)
{
    // Here is where we actually recreate the dead
    // shedu.
    bool found_body = false;
    coord_def place_at;
    bool from_inventory = false;

    // Our pair might already be irretrievably dead.
    if (mons->number == 0)
        return;

    for (radius_iterator ri(mons->pos(), LOS_RADIUS, C_ROUND, mons->get_los()); ri; ++ri)
    {
        for (stack_iterator si(*ri); si; ++si)
            if (si->base_type == OBJ_CORPSES && si->sub_type == CORPSE_BODY
                && si->props.exists(MONSTER_MID)
                && static_cast<unsigned int>(si->props[MONSTER_MID].get_int()) == mons->number)
            {
                place_at = *ri;
                destroy_item(si->index());
                found_body = true;
                break;
            }
    }

    if (!found_body)
    {
        for (unsigned slot = 0; slot < ENDOFPACK; ++slot)
        {
            if (!you.inv[slot].defined())
                continue;

            item_def* si = &you.inv[slot];
            if (si->base_type == OBJ_CORPSES && si->sub_type == CORPSE_BODY
                && si->props.exists(MONSTER_MID)
                && static_cast<unsigned int>(si->props[MONSTER_MID].get_int()) == mons->number)
            {
                // it was in the player's inventory
                place_at = coord_def(-1, -1);
                dec_inv_item_quantity(slot, 1, false);
                found_body = true;
                from_inventory = true;
                break;
            }
        }

        if (found_body)
            place_at = you.pos();
    }

    if (!found_body)
    {
        mons->number = 0;
        return;
    }

    mgen_data new_shedu;
    new_shedu.cls = MONS_SHEDU;
    new_shedu.behaviour = mons->behaviour;
    ASSERT(!place_at.origin());
    new_shedu.foe = mons->foe;
    new_shedu.god = mons->god;

    int id = -1;
    for (distance_iterator di(place_at, true, false); di; ++di)
    {
        if (monster_at(*di) || !monster_habitable_grid(mons, grd(*di)))
            continue;

        new_shedu.pos = *di;
        if ((id = place_monster(new_shedu, true)) != -1)
            break;
    }

    // give up
    if (id == -1)
    {
        dprf("Couldn't place new shedu!");
        return;
    }

    monster* my_pair = &menv[id];
    my_pair->number = mons->mid;
    mons->number = my_pair->mid;
    my_pair->flags |= MF_BAND_MEMBER;

    if (from_inventory)
        simple_monster_message(mons, " resurrects its mate from your pack!");
    else if (you.can_see(mons))
        simple_monster_message(mons, " resurrects its mate from the grave!");
    else if (you.can_see(my_pair))
        simple_monster_message(mons, " rises from the grave!");
}
