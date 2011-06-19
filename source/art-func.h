/**
 * @file
 * @brief Functions non-standard unrandarts uses.
**/

/*
 * util/art-data.pl scans through this file to grab the functions
 * non-standard unrandarts use and put them into the unranddata structs
 * in art-data.h, so the function names must have the form of
 * _UNRAND_ENUM_func_name() in order to be recognised.
 */

#ifdef ART_FUNC_H
#error "art-func.h included twice!"
#endif

#ifdef ART_DATA_H
#error "art-func.h must be included before art-data.h"
#endif

#define ART_FUNC_H

#include "cloud.h"         // For storm bow's and robe of clouds' rain
#include "effects.h"       // For Sceptre of Torment tormenting
#include "env.h"           // For storm bow env.cgrid
#include "food.h"          // For evokes
#include "godconduct.h"    // did_god_conduct.
#include "coord.h"
#include "misc.h"
#include "mgen_data.h"     // For Sceptre of Asmodeus evoke
#include "mon-info.h"
#include "mon-place.h"     // For Sceptre of Asmodeus evoke
#include "mon-stuff.h"     // For Scythe of Curses cursing items
#include "player.h"
#include "spl-cast.h"      // For evokes
#include "spl-miscast.h"   // For Staff of Wucad Mu miscasts
#include "spl-summoning.h" // For Zonguldrok animating dead
#include "terrain.h"       // For storm bow.

/*******************
 * Helper functions.
 *******************/

static void _equip_mpr(bool* show_msgs, const char* msg,
                       msg_channel_type chan = MSGCH_PLAIN)
{
    bool def_show = true;

    if (show_msgs == NULL)
        show_msgs = &def_show;

    if (*show_msgs)
        mpr(msg, chan);

    // Caller shouldn't give any more messages.
    *show_msgs = false;
}

/*******************
 * Unrand functions.
 *******************/

static void _ASMODEUS_melee_effect(item_def* weapon, actor* attacker,
                                   actor* defender, bool mondied)
{
    if (attacker->atype() == ACT_PLAYER)
    {
        did_god_conduct(DID_UNHOLY, 3);
    }
}

static bool _evoke_sceptre_of_asmodeus()
{
    bool rc = true;
    if (one_chance_in(21))
        rc = false;
    else if (one_chance_in(20))
    {
        // Summon devils, maybe a Fiend.
        const monster_type mon = (one_chance_in(4) ? MONS_FIEND :
                                     summon_any_demon(DEMON_COMMON));
        const bool good_summon = create_monster(
                                     mgen_data::hostile_at(mon,
                                         "the Sceptre of Asmodeus",
                                         true, 6, 0, you.pos())) != -1;

        if (good_summon)
        {
            if (mon == MONS_FIEND)
                mpr("\"Your arrogance condemns you, mortal!\"");
            else
                mpr("The Sceptre summons one of its servants.");
        }
        else
            mpr("The air shimmers briefly.");
    }
    else
    {
        // Cast a destructive spell.
        const spell_type spl = static_cast<spell_type>(
            random_choose_weighted(114, SPELL_BOLT_OF_FIRE,
                                   57,  SPELL_LIGHTNING_BOLT,
                                   57,  SPELL_BOLT_OF_DRAINING,
                                   12,  SPELL_HELLFIRE,
                                   0));
        your_spells(spl, you.skill(SK_EVOCATIONS) * 8, false);
    }

    return (rc);
}


static bool _ASMODEUS_evoke(item_def *item, int* pract, bool* did_work,
                            bool* unevokable)
{
    if (_evoke_sceptre_of_asmodeus())
    {
        make_hungry(200, false, true);
        *did_work = true;
        *pract    = 1;
    }

    return (false);
}

////////////////////////////////////////////////////

// XXX: Defender's resistance to fire being temporarily downgraded is
// hardcoded in melee_attack::fire_res_apply_cerebov_downgrade()

static void _CEREBOV_melee_effect(item_def* weapon, actor* attacker,
                                  actor* defender, bool mondied)
{
    if (attacker->atype() == ACT_PLAYER)
    {
        did_god_conduct(DID_UNHOLY, 3);
    }
}

////////////////////////////////////////////////////

static void _CURSES_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    _equip_mpr(show_msgs, "A shiver runs down your spine.");
}

static void _CURSES_world_reacts(item_def *item)
{
    if (one_chance_in(30))
        curse_an_item(false);
}

static void _CURSES_melee_effect(item_def* weapon, actor* attacker,
                                 actor* defender, bool mondied)
{
    if (attacker->atype() == ACT_PLAYER)
    {
        did_god_conduct(DID_NECROMANCY, 3);
    }
}

/////////////////////////////////////////////////////

static void _DISPATER_melee_effect(item_def* weapon, actor* attacker,
                                   actor* defender, bool mondied)
{
    if (attacker->atype() == ACT_PLAYER)
    {
        did_god_conduct(DID_UNHOLY, 3);
    }
}

static bool _DISPATER_evoke(item_def *item, int* pract, bool* did_work,
                            bool* unevokable)
{
    if (you.duration[DUR_DEATHS_DOOR] || !enough_hp(11, true)
        || !enough_mp(5, true))
    {
        return (false);
    }

    mpr("You feel the staff feeding on your energy!");

    dec_hp(5 + random2avg(19, 2), false, "Staff of Dispater");
    dec_mp(2 + random2avg(5, 2));
    make_hungry(100, false, true);

    int power = you.skill(SK_EVOCATIONS) * 8;
    your_spells(SPELL_HELLFIRE, power, false);

    *pract    = (coinflip() ? 2 : 1);
    *did_work = true;

    return (false);
}

////////////////////////////////////////////////////

// XXX: Staff giving a boost to poison spells is hardcoded in
// player_spec_poison()

static void _olgreb_pluses(item_def *item)
{
    // Giving Olgreb's staff a little lift since staves of poison have
    // been made better. -- bwr
    item->plus  = you.skill(SK_POISON_MAGIC) / 3;
    item->plus2 = item->plus;
}

static void _OLGREB_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    if (you.can_smell())
        _equip_mpr(show_msgs, "You smell chlorine.");
    else
        _equip_mpr(show_msgs, "The staff glows a sickly green.");

    _olgreb_pluses(item);
}

static void _OLGREB_unequip(const item_def *item, bool *show_msgs)
{
    if (you.can_smell())
        _equip_mpr(show_msgs, "The smell of chlorine vanishes.");
    else
        _equip_mpr(show_msgs, "The staff's sickly green glow vanishes.");
}

static void _OLGREB_world_reacts(item_def *item)
{
    _olgreb_pluses(item);
}

static bool _OLGREB_evoke(item_def *item, int* pract, bool* did_work,
                          bool* unevokable)
{
    if (!enough_mp(4, true) || you.skill(SK_EVOCATIONS) < random2(6))
        return (false);

    dec_mp(4);
    make_hungry(50, false, true);
    *pract    = 1;
    *did_work = true;

    int power = 10 + you.skill(SK_EVOCATIONS) * 8;

    your_spells(SPELL_OLGREBS_TOXIC_RADIANCE, power, false);

    if (x_chance_in_y(you.skill(SK_EVOCATIONS) + 1, 10))
        your_spells(SPELL_VENOM_BOLT, power, false);

    return (false);
}

static void _OLGREB_melee_effect(item_def* weapon, actor* attacker,
                                 actor* defender, bool mondied)
{
    if (defender->alive()
        && (coinflip() || x_chance_in_y(you.skill(SK_POISON_MAGIC), 8)))
    {
        defender->poison(attacker, 2, defender->has_lifeforce()
                                      && x_chance_in_y(you.skill(SK_POISON_MAGIC), 8));
        if (attacker->atype() == ACT_PLAYER)
            did_god_conduct(DID_POISON, 3);
    }
}

////////////////////////////////////////////////////

static void _power_pluses(item_def *item)
{
    item->plus  = stepdown_value(-4 + (you.hp / 5), 4, 4, 4, 20);
    item->plus2 = item->plus;
}

static void _POWER_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    _equip_mpr(show_msgs, "You sense an aura of extreme power.");
    _power_pluses(item);
}

static void _POWER_world_reacts(item_def *item)
{
    _power_pluses(item);
}

////////////////////////////////////////////////////

static void _SINGING_SWORD_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    bool def_show = true;

    if (show_msgs == NULL)
        show_msgs = &def_show;

    if (!*show_msgs)
        return;

    if (!item_type_known(*item))
    {
        mprf(MSGCH_TALK, "%s says, \"Hi!  I'm the Singing Sword!\"",
             item->name(DESC_CAP_THE).c_str());
    }
    else
        mpr("The Singing Sword hums in delight!", MSGCH_TALK);

    *show_msgs = false;
}

static void _SINGING_SWORD_unequip(const item_def *item, bool *show_msgs)
{
    _equip_mpr(show_msgs, "The Singing Sword sighs.", MSGCH_TALK);
}

////////////////////////////////////////////////////

static void _PRUNE_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    _equip_mpr(show_msgs, "You feel pruney.");
}

////////////////////////////////////////////////////

static void _TORMENT_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    _equip_mpr(show_msgs, "A terribly searing pain shoots up your arm!");
}

static void _TORMENT_world_reacts(item_def *item)
{
    if (one_chance_in(200))
    {
        torment(TORMENT_SPWLD, you.pos());
        did_god_conduct(DID_UNHOLY, 1);
    }
}

static void _TORMENT_melee_effect(item_def* weapon, actor* attacker,
                                  actor* defender, bool mondied)
{
    if (attacker->atype() == ACT_PLAYER && coinflip())
    {
        torment(TORMENT_SPWLD, you.pos());
        did_god_conduct(DID_UNHOLY, 5);
    }
}

/////////////////////////////////////////////////////

static void _TROG_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    _equip_mpr(show_msgs, "You feel bloodthirsty!");
}

static void _TROG_unequip(const item_def *item, bool *show_msgs)
{
    _equip_mpr(show_msgs, "You feel less violent.");
}

static void _TROG_melee_effect(item_def* weapon, actor* attacker,
                               actor* defender, bool mondied)
{
    if (coinflip())
        attacker->go_berserk(false);
}

////////////////////////////////////////////////////

static void _wucad_miscast(actor* victim, int power,int fail)
{
    MiscastEffect(victim, WIELD_MISCAST, SPTYP_DIVINATION, power, fail,
                  "the Staff of Wucad Mu", NH_NEVER);
}

static void _wucad_pluses(item_def *item)
{
    item->plus  = std::min(you.intel() - 3, 22);
    item->plus2 = std::min(you.intel() / 2, 13);
}

static void _WUCAD_MU_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    _wucad_pluses(item);
}

static void _WUCAD_MU_world_reacts(item_def *item)
{
    _wucad_pluses(item);
}

static bool _WUCAD_MU_evoke(item_def *item, int* pract, bool* did_work,
                            bool* unevokable)
{
    if (you.magic_points == you.max_magic_points
        || you.skill(SK_EVOCATIONS) < random2(25))
    {
        return (false);
    }

    mpr("Magical energy flows into your mind!");

    inc_mp(3 + random2(5) + you.skill(SK_EVOCATIONS) / 3, false);
    make_hungry(50, false, true);

    *pract    = 1;
    *did_work = true;

    if (one_chance_in(3))
        _wucad_miscast(&you, random2(9), random2(70));

    return (false);
}

///////////////////////////////////////////////////

// XXX: Always getting maximal vampiric drain is hardcoded in
// melee_attack::apply_damage_brand()

static void _VAMPIRES_TOOTH_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    if (you.is_undead != US_UNDEAD)
    {
        _equip_mpr(show_msgs,
                   "You feel a strange hunger, and smell blood in the air...");
        make_hungry(4500, false, false);
    }
    else
        _equip_mpr(show_msgs, "You feel strangely empty.");
}

///////////////////////////////////////////////////

// XXX: Pluses at creation time are hardcoded in make_item_unrandart()

static void _VARIABILITY_world_reacts(item_def *item)
{
    do_uncurse_item(*item);

    if (x_chance_in_y(2, 5))
        item->plus  += (coinflip() ? +1 : -1);

    if (x_chance_in_y(2, 5))
        item->plus2 += (coinflip() ? +1 : -1);

    if (item->plus < -4)
        item->plus = -4;
    else if (item->plus > 16)
        item->plus = 16;

    if (item->plus2 < -4)
        item->plus2 = -4;
    else if (item->plus2 > 16)
        item->plus2 = 16;
}

///////////////////////////////////////////////////

static void _ZONGULDROK_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    _equip_mpr(show_msgs, "You sense an extremely unholy aura.");
}

static void _ZONGULDROK_world_reacts(item_def *item)
{
    if (one_chance_in(5))
    {
        animate_dead(&you, 1 + random2(3), BEH_HOSTILE, MHITYOU, 0,
                     "the Sword of Zonguldrok");
        did_god_conduct(DID_NECROMANCY, 1);
        did_god_conduct(DID_CORPSE_VIOLATION, 1);
    }
}

static void _ZONGULDROK_melee_effect(item_def* weapon, actor* attacker,
                                     actor* defender, bool mondied)
{
    if (attacker->atype() == ACT_PLAYER)
    {
        did_god_conduct(DID_NECROMANCY, 3);
        did_god_conduct(DID_CORPSE_VIOLATION, 3);
    }
}

///////////////////////////////////////////////////

static void _STORM_BOW_world_reacts(item_def *item)
{
    if (!one_chance_in(300))
        return;

    for (radius_iterator ri(you.pos(), 2); ri; ++ri)
        if (!cell_is_solid(*ri) && env.cgrid(*ri) == EMPTY_CLOUD && one_chance_in(5))
            place_cloud(CLOUD_RAIN, *ri, random2(20), &you, 3);
}

///////////////////////////////////////////////////

static void _GONG_melee_effect(item_def* item, actor* wearer,
                               actor* attacker, bool dummy)
{
    if (silenced(wearer->pos()))
        return;

    std::string msg = getSpeakString("shield of the gong");
    if (msg.empty())
        msg = "You hear a strange loud sound.";
    mpr(msg.c_str(), MSGCH_SOUND);

    noisy(40, wearer->pos());
}

///////////////////////////////////////////////////

static void _RCLOUDS_world_reacts(item_def *item)
{
    cloud_type cloud;
    if (one_chance_in(4))
        cloud = CLOUD_RAIN;
    else
        cloud = CLOUD_MIST;

    for (radius_iterator ri(you.pos(), 2); ri; ++ri)
        if (!cell_is_solid(*ri) && env.cgrid(*ri) == EMPTY_CLOUD
                && one_chance_in(20))
        {
            place_cloud(cloud, *ri, random2(10), &you, 1);
        }
}

static void _RCLOUDS_equip(item_def *item, bool *show_msgs, bool unmeld)
{
    _equip_mpr(show_msgs, "A thin mist springs up around you!");
}

///////////////////////////////////////////////////

static void _DEMON_AXE_melee_effect(item_def* item, actor* attacker,
                                    actor* defender, bool mondied)
{
    if (one_chance_in(10))
        cast_summon_demon(50+random2(100), you.religion);

    did_god_conduct(DID_UNHOLY, 3);
}

static void _DEMON_AXE_world_reacts(item_def *item)
{
    std::vector<monster_info> targets;
    get_monster_info(targets);

    int dist = LOS_RADIUS + 1;

    if (targets.empty())
        return;

    monster* closest = NULL;

    std::vector<monster_info>::const_iterator mi;

    for (mi = targets.begin(); mi != targets.end(); ++mi)
    {
        if (grid_distance(you.pos(), mi->mon()->pos()) < dist
            && you.possible_beholder(mi->mon()))
        {
            dist = grid_distance(you.pos(), mi->mon()->pos());
            closest = mi->mon();
        }
    }

    if (!closest)
        return;

    if (!you.beheld_by(closest))
    {
         mprf("Visions of slaying %s flood into your mind.",
              closest->name(DESC_NOCAP_THE).c_str());

         // The monsters (if any) currently mesmerising the player do not include
         // this monster. To avoid trapping the player, all other beholders
         // are removed.

         you.clear_beholders();
    }

    if (you.confused())
    {
        mpr("Your confusion fades away as the thirst for blood takes over your mind.");
        you.duration[DUR_CONF] = 0;
    }

    you.add_beholder(closest, true);
}

static void _DEMON_AXE_unequip(const item_def *item, bool *show_msgs)
{
    if (you.beheld())
    {
        // This shouldn't clear mermaids and sirens, but we lack the information
        // why they behold us -- usually, it's due to the axe.  Since unwielding
        // it costs scrolls of rem curse, we might say getting the demon away is
        // enough of a shock to get you back to senses.
        you.clear_beholders();
        mpr("Your thirst for blood fades away.");
    }
}
