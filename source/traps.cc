/**
 * @file
 * @brief Traps related functions.
**/

#include "AppHdr.h"

#include "traps.h"
#include "trap_def.h"

#include <algorithm>

#include "artefact.h"
#include "beam.h"
#include "branch.h"
#include "cloud.h"
#include "clua.h"
#include "coord.h"
#include "coordit.h"
#include "delay.h"
#include "describe.h"
#include "directn.h"
#include "exercise.h"
#include "map_knowledge.h"
#include "itemname.h"
#include "itemprop.h"
#include "items.h"
#include "libutil.h"
#include "makeitem.h"
#include "message.h"
#include "misc.h"
#include "mon-util.h"
#include "mon-stuff.h"
#include "mon-transit.h"
#include "ouch.h"
#include "player.h"
#include "skills.h"
#include "spl-miscast.h"
#include "spl-util.h"
#include "state.h"
#include "stuff.h"
#include "travel.h"
#include "env.h"
#include "areas.h"
#include "terrain.h"
#include "transform.h"
#include "hints.h"
#include "view.h"
#include "shout.h"
#include "xom.h"

bool trap_def::active() const
{
    return (this->type != TRAP_UNASSIGNED);
}

bool trap_def::type_has_ammo() const
{
    switch (this->type)
    {
    case TRAP_DART:   case TRAP_ARROW:  case TRAP_BOLT:
    case TRAP_NEEDLE: case TRAP_SPEAR:  case TRAP_AXE:
        return (true);
    default:
        break;
    }
    return (false);
}

void trap_def::message_trap_entry()
{
    if (this->type == TRAP_TELEPORT)
        mpr("You enter a teleport trap!");
}

void trap_def::disarm()
{
    if (this->type_has_ammo() && this->ammo_qty > 0)
    {
        item_def trap_item = this->generate_trap_item();
        trap_item.quantity = this->ammo_qty;
        copy_item_to_grid(trap_item, this->pos);
    }
    this->destroy();
}

void trap_def::destroy()
{
    if (!in_bounds(this->pos))
        die("Trap position out of bounds!");

    grd(this->pos) = DNGN_FLOOR;
    this->ammo_qty = 0;
    this->pos      = coord_def(-1,-1);
    this->type     = TRAP_UNASSIGNED;
}

void trap_def::hide()
{
    grd(this->pos) = DNGN_UNDISCOVERED_TRAP;
}

void trap_def::prepare_ammo()
{
    switch (this->type)
    {
    case TRAP_DART:
    case TRAP_ARROW:
    case TRAP_BOLT:
    case TRAP_NEEDLE:
        this->ammo_qty = 3 + random2avg(9, 3);
        break;
    case TRAP_SPEAR:
    case TRAP_AXE:
        this->ammo_qty = 2 + random2avg(6, 3);
        break;
    case TRAP_ALARM:
        this->ammo_qty = 1 + random2(3);
        // Zotdef: alarm traps have practically unlimited ammo
        if (crawl_state.game_is_zotdef())
            this->ammo_qty = 100000;
        break;
    case TRAP_GOLUBRIA:
        // really, turns until it vanishes
        this->ammo_qty = 30 + random2(20);
        break;
    default:
        this->ammo_qty = 0;
        break;
    }
    // Zot def: traps have 10x as much ammo
    if (crawl_state.game_is_zotdef())
        this->ammo_qty *= 10;
}

void trap_def::reveal()
{
    grd(this->pos) = this->category();
}

std::string trap_def::name(description_level_type desc) const
{
    if (this->type >= NUM_TRAPS)
        return ("buggy");

    const char* basename = trap_name(this->type);
    if (desc == DESC_CAP_A || desc == DESC_NOCAP_A)
    {
        std::string prefix = (desc == DESC_CAP_A ? "A" : "a");
        if (is_vowel(basename[0]))
            prefix += 'n';
        prefix += ' ';
        return (prefix + basename);
    }
    else if (desc == DESC_CAP_THE)
        return (std::string("The ") + basename);
    else if (desc == DESC_NOCAP_THE)
        return (std::string("the ") + basename);
    else                        // everything else
        return (basename);
}

bool trap_def::is_known(const actor* act) const
{
    const bool player_knows = (grd(pos) != DNGN_UNDISCOVERED_TRAP);

    if (act == NULL || act->atype() == ACT_PLAYER)
        return (player_knows);
    else if (act->atype() == ACT_MONSTER)
    {
        const monster* mons = act->as_monster();
        const int intel = mons_intel(mons);

        // Smarter trap handling for intelligent monsters
        // * monsters native to a branch can be assumed to know the trap
        //   locations and thus be able to avoid them
        // * friendlies and good neutrals can be assumed to have been warned
        //   by the player about all traps s/he knows about
        // * very intelligent monsters can be assumed to have a high T&D
        //   skill (or have memorised part of the dungeon layout ;))

        if (this->category() == DNGN_TRAP_NATURAL)
        {
            // Slightly different rules for shafts:
            // * Lower intelligence requirement for native monsters.
            // * Allied zombies won't fall through shafts. (No herding!)
            // * Highly intelligent monsters never fall through shafts.
            return (intel >= I_HIGH
                    || intel > I_PLANT && mons_is_native_in_branch(mons)
                    || player_knows && mons->wont_attack());
        }
        else
        {
            return (intel >= I_NORMAL
                    && (mons_is_native_in_branch(mons)
                        || player_knows && mons->wont_attack()
                        || intel >= I_HIGH && one_chance_in(3)));
        }
    }
    die("invalid actor type");
}


// Returns the number of a net on a given square.
// If trapped, only stationary ones are counted
// otherwise the first net found is returned.
int get_trapping_net(const coord_def& where, bool trapped)
{
    for (stack_iterator si(where); si; ++si)
    {
        if (si->base_type == OBJ_MISSILES
            && si->sub_type == MI_THROWING_NET
            && (!trapped || item_is_stationary(*si)))
        {
            return (si->index());
        }
    }
    return (NON_ITEM);
}

// If there are more than one net on this square
// split off one of them for checking/setting values.
static void _maybe_split_nets(item_def &item, const coord_def& where)
{
    if (item.quantity == 1)
    {
        set_item_stationary(item);
        return;
    }

    item_def it;

    it.base_type = item.base_type;
    it.sub_type  = item.sub_type;
    it.plus      = item.plus;
    it.plus2     = item.plus2;
    it.flags     = item.flags;
    it.special   = item.special;
    it.quantity  = --item.quantity;
    item_colour(it);

    item.quantity = 1;
    set_item_stationary(item);

    copy_item_to_grid(it, where);
}

static void _mark_net_trapping(const coord_def& where)
{
    int net = get_trapping_net(where);
    if (net == NON_ITEM)
    {
        net = get_trapping_net(where, false);
        if (net != NON_ITEM)
            _maybe_split_nets(mitm[net], where);
    }
}

void monster_caught_in_net(monster* mon, bolt &pbolt)
{
    if (mon->body_size(PSIZE_BODY) >= SIZE_GIANT)
        return;

    if (mon->is_insubstantial())
    {
        if (you.can_see(mon))
        {
            mprf("The net passes right through %s!",
                 mon->name(DESC_NOCAP_THE).c_str());
        }
        return;
    }

    bool mon_flies = mon->flight_mode() == FL_FLY;
    if (mon_flies && (!mons_is_confused(mon) || one_chance_in(3)))
    {
        simple_monster_message(mon, " darts out from under the net!");
        return;
    }

    if (mon->type == MONS_OOZE || mon->type == MONS_PULSATING_LUMP)
    {
        simple_monster_message(mon, " oozes right through the net!");
        return;
    }

    if (!mon->caught() && mon->add_ench(ENCH_HELD))
    {
        if (mons_near(mon) && !mon->visible_to(&you))
            mpr("Something gets caught in the net!");
        else
            simple_monster_message(mon, " is caught in the net!");

        if (mon_flies)
        {
            simple_monster_message(mon, " falls like a stone!");
            mons_check_pool(mon, mon->pos(), pbolt.killer(), pbolt.beam_source);
        }
    }
}

bool player_caught_in_net()
{
    if (you.body_size(PSIZE_BODY) >= SIZE_GIANT)
        return (false);

    if (you.flight_mode() == FL_FLY && (!you.confused() || one_chance_in(3)))
    {
        mpr("You dart out from under the net!");
        return (false);
    }

    if (!you.attribute[ATTR_HELD])
    {
        you.attribute[ATTR_HELD] = 10;
        mpr("You become entangled in the net!");
        stop_running();

        // I guess levitation works differently, keeping both you
        // and the net hovering above the floor
        if (you.flight_mode() == FL_FLY)
        {
            mpr("You fall like a stone!");
            fall_into_a_pool(you.pos(), false, grd(you.pos()));
        }

        stop_delay(true); // even stair delays
        redraw_screen(); // Account for changes in display.
        return (true);
    }
    return (false);
}

void check_net_will_hold_monster(monster* mons)
{
    if (mons->body_size(PSIZE_BODY) >= SIZE_GIANT)
    {
        int net = get_trapping_net(mons->pos());
        if (net != NON_ITEM)
            destroy_item(net);

        if (you.see_cell(mons->pos()))
        {
            if (mons->visible_to(&you))
            {
                mprf("The net rips apart, and %s comes free!",
                     mons->name(DESC_NOCAP_THE).c_str());
            }
            else
                mpr("All of a sudden the net rips apart!");
        }
    }
    else if (mons->is_insubstantial()
             || mons->type == MONS_OOZE
             || mons->type == MONS_PULSATING_LUMP)
    {
        const int net = get_trapping_net(mons->pos());
        if (net != NON_ITEM)
            remove_item_stationary(mitm[net]);

        if (mons->is_insubstantial())
        {
            simple_monster_message(mons,
                                   " drifts right through the net!");
        }
        else
        {
            simple_monster_message(mons,
                                   " oozes right through the net!");
        }
    }
    else
        mons->add_ench(ENCH_HELD);
}

std::vector<coord_def> find_golubria_on_level()
{
    std::vector<coord_def> ret;
    for (rectangle_iterator ri(coord_def(0, 0), coord_def(GXM-1, GYM-1)); ri; ++ri)
    {
        trap_def *trap = find_trap(*ri);
        if (trap && trap->type == TRAP_GOLUBRIA)
            ret.push_back(*ri);
    }
    ASSERT(ret.size() <= 2);
    return ret;
}

static bool _find_other_passage_side(coord_def& to)
{
    std::vector<coord_def> passages = find_golubria_on_level();
    if (passages.size() < 2)
        return false;

    if (to == passages[0])
    {
        to = passages[1];
        return true;
    }
    else if (to == passages[1])
    {
        to = passages[0];
        return true;
    }
    else
        die("Golubria's passage not found");
}

// Returns a direction string from you.pos to the
// specified position. If fuzz is true, may be wrong.
// Returns an empty string if no direction could be
// determined (if fuzz if false, this is only if
// you.pos==pos).
std::string direction_string(coord_def pos, bool fuzz)
{
    int dx = you.pos().x - pos.x;
    if (fuzz)
        dx += random2avg(41,2) - 20;
    int dy = you.pos().y - pos.y;
    if (fuzz)
        dy += random2avg(41,2) - 20;
    const char *ew=((dx > 0) ? "west" : ((dx < 0) ? "east" : ""));
    const char *ns=((dy < 0) ? "south" : ((dy > 0) ? "north" : ""));
    if (abs(dy) > 2 * abs(dx))
        ew="";
    if (abs(dx) > 2 * abs(dy))
        ns="";
    return (std::string(ns) + ew);
}

void trap_def::trigger(actor& triggerer, bool flat_footed)
{
    const bool you_know = this->is_known();
    const bool trig_knows = !flat_footed && this->is_known(&triggerer);

    const bool you_trigger = (triggerer.atype() == ACT_PLAYER);
    const bool in_sight = you.see_cell(this->pos);

    // Zot def - player never sets off known traps
    if (crawl_state.game_is_zotdef() && you_trigger && you_know)
    {
        mpr("You step safely past the trap.");
        return;
    }

    // If set, the trap will be removed at the end of the
    // triggering process.
    bool trap_destroyed = false, know_trap_destroyed = false;;

    monster* m = triggerer.as_monster();

    // Smarter monsters and those native to the level will simply
    // side-step known shafts. Unless they are already looking for
    // an exit, of course.
    if (this->type == TRAP_SHAFT && m)
    {
        if (!m->will_trigger_shaft()
            || trig_knows && !mons_is_fleeing(m) && !m->pacified())
        {
            // No message for flying monsters to avoid message spam.
            if (you_know && triggerer.ground_level())
                simple_monster_message(m, " carefully avoids the shaft.");
            return;
        }
    }

    // Zot def - friendly monsters never set off known traps
    if (crawl_state.game_is_zotdef() && m && m->friendly() && trig_knows)
    {
        simple_monster_message(m," carefully avoids a trap.");
        return;
    }
    // Only magical traps affect flying critters.
    if (!triggerer.ground_level() && this->category() != DNGN_TRAP_MAGICAL)
    {
        if (you_know && m && triggerer.airborne())
            simple_monster_message(m, " flies safely over a trap.");
        return;
    }

    // Anything stepping onto a trap almost always reveals it.
    // (We can rehide it later for the exceptions.)
    if (in_sight)
        this->reveal();

    // OK, something is going to happen.
    if (you_trigger)
        this->message_trap_entry();

    // Store the position now in case it gets cleared inbetween.
    const coord_def p(this->pos);

    if (this->type_has_ammo())
        this->shoot_ammo(triggerer, trig_knows);
    else switch (this->type)
    {
    case TRAP_GOLUBRIA: {
        coord_def to = p;
        if (_find_other_passage_side(to))
        {
            if (you_trigger)
                mpr("You enter the passage of Golubria.");
            else
                simple_monster_message(m, " enters the passage of Golubria.");

            if (triggerer.move_to_pos(to))
            {
                if (you_trigger)
                    place_cloud(CLOUD_TLOC_ENERGY, p, 1 + random2(3), &you);
                else
                    place_cloud(CLOUD_TLOC_ENERGY, p, 1 + random2(3), m);
                trap_destroyed = true;
                know_trap_destroyed = you_trigger;
            }
            else
            {
                mpr("But it is blocked!");
            }
        }
        break;
    }
    case TRAP_TELEPORT:
        // Never revealed by monsters.
        // except when it's in sight, it's pretty obvious what happened. -doy
        if (!you_trigger && !you_know && !in_sight)
            this->hide();
        triggerer.teleport(true);
        break;

    case TRAP_ALARM:
        if (!ammo_qty--)
        {
            if (you_trigger)
                mpr("You trigger an alarm trap, but it seems broken.");
            else if (in_sight && you_know)
                mpr("The alarm trap gives no sound.");
            trap_destroyed = true;
        }
        else if (silenced(this->pos))
        {
            if (you_know && in_sight)
                mpr("The alarm trap is silent.");

            // If it's silent, you don't know about it.
            if (!you_know)
                this->hide();
        }
        else if (!(m && m->friendly()))
        {
            // Alarm traps aren't set off by hostile monsters, because
            // that would be way too nasty for the player.
            std::string msg;
            if (you_trigger)
                msg = "An alarm trap emits a blaring wail!";
            else
            {
                std::string dir=direction_string(this->pos, !in_sight);
                msg = std::string("You hear a ") +
                    ((in_sight) ? "" : "distant ")
                    + "blaring wail "
                    + (!dir.empty()? ("to the " + dir + ".") : "behind you!");
            }
            // Monsters of normal or greater intelligence will realize that
            // they were the one to set off the trap.
            int source = !m ? you.mindex() :
                         mons_intel(m) >= I_NORMAL ? m->mindex() : -1;

            // Zotdef - Made alarm traps noisier and more noticeable
            int noiselevel = crawl_state.game_is_zotdef() ? 30 : 12;
            noisy(noiselevel, this->pos, msg.c_str(), source, false);
            if (crawl_state.game_is_zotdef())
                more();
        }
        break;

    case TRAP_BLADE:
        if (you_trigger)
        {
            if (trig_knows && one_chance_in(3))
                mpr("You avoid triggering a blade trap.");
            else if (random2limit(player_evasion(), 40)
                     + (random2(you.dex()) / 3) + (trig_knows ? 3 : 0) > 8)
            {
                mpr("A huge blade swings just past you!");
            }
            else
            {
                mpr("A huge blade swings out and slices into you!");
                const int damage = (you.absdepth0 * 2) + random2avg(29, 2)
                    - random2(1 + you.armour_class());
                std::string n = name(DESC_NOCAP_A) + " trap";
                ouch(damage, NON_MONSTER, KILLED_BY_TRAP, n.c_str());
                bleed_onto_floor(you.pos(), MONS_PLAYER, damage, true);
            }
        }
        else if (m)
        {
            if (one_chance_in(5) || (trig_knows && coinflip()))
            {
                // Trap doesn't trigger. Don't reveal it.
                if (you_know)
                {
                    simple_monster_message(m,
                                           " fails to trigger a blade trap.");
                }
                else
                    this->hide();
            }
            else if (random2(m->ev) > 8 || (trig_knows && random2(m->ev) > 8))
            {
                if (in_sight
                    && !simple_monster_message(m,
                                            " avoids a huge, swinging blade."))
                {
                    mpr("A huge blade swings out!");
                }
            }
            else
            {
                if (in_sight)
                {
                    std::string msg = "A huge blade swings out";
                    if (m->visible_to(&you))
                    {
                        msg += " and slices into ";
                        msg += m->name(DESC_NOCAP_THE);
                    }
                    msg += "!";
                    mpr(msg.c_str());
                }

                int damage_taken = 10 + random2avg(29, 2) - random2(1 + m->ac);

                if (damage_taken < 0)
                    damage_taken = 0;

                if (!m->is_summoned())
                    bleed_onto_floor(m->pos(), m->type, damage_taken, true);

                m->hurt(NULL, damage_taken);
                if (in_sight && m->alive())
                    print_wounds(m);

                // zotdef: blade traps break eventually
                if (crawl_state.game_is_zotdef() && one_chance_in(200))
                {
                    if (in_sight)
                        mpr("The blade breaks!");
                    disarm();
                }
            }
        }
        break;

    case TRAP_NET:
        if (you_trigger)
        {
            if (trig_knows && one_chance_in(3))
                mpr("A net swings high above you.");
            else
            {
                if (random2limit(player_evasion(), 40)
                    + (random2(you.dex()) / 3) + (trig_knows ? 3 : 0) > 12)
                {
                    mpr("A net drops to the ground!");
                }
                else
                {
                    mpr("A large net falls onto you!");
                    if (player_caught_in_net() && player_in_a_dangerous_place())
                        xom_is_stimulated(64);
                }

                item_def item = this->generate_trap_item();
                copy_item_to_grid(item, triggerer.pos());

                if (you.attribute[ATTR_HELD])
                    _mark_net_trapping(you.pos());

                trap_destroyed = true;
            }
        }
        else if (m)
        {
            bool triggered = false;
            if (one_chance_in(3) || (trig_knows && coinflip()))
            {
                // Not triggered, trap stays.
                triggered = false;
                if (you_know)
                    simple_monster_message(m, " fails to trigger a net trap.");
                else
                    this->hide();
            }
            else if (random2(m->ev) > 8 || (trig_knows && random2(m->ev) > 8))
            {
                // Triggered but evaded.
                triggered = true;

                if (in_sight)
                {
                    if (!simple_monster_message(m,
                                                " nimbly jumps out of the way "
                                                "of a falling net."))
                    {
                        mpr("A large net falls down!");
                    }
                }
            }
            else
            {
                // Triggered and hit.
                triggered = true;

                if (in_sight)
                {
                    msg::stream << "A large net falls down";
                    if (m->visible_to(&you))
                        msg::stream << " onto " << m->name(DESC_NOCAP_THE);
                    msg::stream << "!" << std::endl;
                }
                // FIXME: Fake a beam for monster_caught_in_net().
                bolt beam;
                beam.flavour = BEAM_MISSILE;
                beam.thrower = KILL_MISC;
                beam.beam_source = NON_MONSTER;
                monster_caught_in_net(m, beam);
            }

            if (triggered)
            {
                item_def item = this->generate_trap_item();
                copy_item_to_grid(item, triggerer.pos());

                if (m->caught())
                    _mark_net_trapping(m->pos());

                trap_destroyed = true;
            }
        }
        break;

    case TRAP_ZOT:
        if (you_trigger)
        {
            mpr((trig_knows) ? "You enter the Zot trap."
                             : "Oh no! You have blundered into a Zot trap!");
            if (!trig_knows)
                xom_is_stimulated(32);

            MiscastEffect(&you, ZOT_TRAP_MISCAST, SPTYP_RANDOM,
                           3, "a Zot trap");
        }
        else if (m)
        {
            // Zot traps are out to get *the player*! Hostile monsters
            // benefit and friendly monsters suffer. Such is life.

            // dtsund - Hostile monsters triggering these against the player
            // is one of the most retarded mechanics ever to appear in any
            // game.  As such, it has been removed.

            // The old code rehid the trap, but that's pure interface screw
            // in 99% of cases - a player can just watch who stepped where
            // and mark the trap on an external paper map.  Not good.

            actor* targ = NULL;
            if (m->wont_attack() || crawl_state.game_is_arena())
                targ = m;

            // Give the player a chance to figure out what happened
            // to their friend.
            if (player_can_hear(this->pos) && (!targ || !in_sight))
            {
                mprf(MSGCH_SOUND, "You hear a %s \"Zot\"!",
                     in_sight ? "loud" : "distant");
            }

            if (targ)
            {
                if (in_sight)
                {
                    mprf("The power of Zot is invoked against %s!",
                         targ->name(DESC_NOCAP_THE).c_str());
                }
                MiscastEffect(targ, ZOT_TRAP_MISCAST, SPTYP_RANDOM,
                              3, "the power of Zot");
            }
        }
        break;

    case TRAP_SHAFT:
        // Unknown shafts are traps triggered by walking onto them.
        // Known shafts are used as escape hatches

        // Paranoia
        if (!is_valid_shaft_level())
        {
            if (you_know && in_sight)
                mpr("The shaft disappears in a puff of logic!");

            trap_destroyed = true;
            break;
        }

        // If the shaft isn't known, don't reveal it.
        // The shafting code in downstairs() needs to know
        // whether it's undiscovered.
        if (!you_know)
            this->hide();

        // Known shafts don't trigger as traps.
        if (trig_knows)
            break;

        // Depending on total (body + equipment) weight, give monsters
        // and player a chance to escape a shaft.
        if (x_chance_in_y(200, triggerer.total_weight()))
            break;

        // Fire away!
        triggerer.do_shaft();

        // Player-used shafts are destroyed
        // after one use in down_stairs(), misc.cc
        if (!you_trigger)
        {
            if (in_sight)
                mpr("The shaft crumbles and collapses.");
            trap_destroyed = true;
        }
        break;

    case TRAP_PLATE:
        dungeon_events.fire_position_event(DET_PRESSURE_PLATE, pos);
        break;

    default:
        break;
    }

    if (you_trigger)
    {
        learned_something_new(HINT_SEEN_TRAP, p);

        // Exercise T&D if the trap revealed itself, but not if it ran
        // out of ammo.
        if (!you_know && this->type != TRAP_UNASSIGNED && this->is_known())
            practise(EX_TRAP_TRIGGER);
    }

    if (trap_destroyed)
    {
        if (know_trap_destroyed)
        {
            env.map_knowledge(this->pos).set_feature(DNGN_FLOOR);
        }
        this->destroy();
    }
}

int trap_def::max_damage(const actor& act)
{
    int level = you.absdepth0;

    // Trap damage to monsters is not a function of level, because
    // they are fairly stupid and tend to have fewer hp than
    // players -- this choice prevents traps from easily killing
    // large monsters fairly deep within the dungeon.
    if (act.atype() == ACT_MONSTER)
        level = 0;

    switch (this->type)
    {
        case TRAP_NEEDLE: return  0;
        case TRAP_DART:   return  4 + level/2;
        case TRAP_ARROW:  return  7 + level;
        case TRAP_SPEAR:  return 10 + level;
        case TRAP_BOLT:   return 13 + level;
        case TRAP_AXE:    return 15 + level;
        case TRAP_BLADE:  return (level ? 2*level : 10) + 28;
        default:          return  0;
    }

    return (0);
}

int trap_def::shot_damage(actor& act)
{
    const int dam = max_damage(act);

    if (!dam)
        return 0;
    return random2(dam) + 1;
}

int reveal_traps(const int range)
{
    int traps_found = 0;

    for (int i = 0; i < MAX_TRAPS; i++)
    {
        trap_def& trap = env.trap[i];

        if (!trap.active())
            continue;

        if (distance(you.pos(), trap.pos) < dist_range(range) && !trap.is_known())
        {
            traps_found++;
            trap.reveal();
            env.map_knowledge(trap.pos).set_feature(grd(trap.pos));
            set_terrain_mapped(trap.pos);
        }
    }

    return (traps_found);
}

void destroy_trap(const coord_def& pos)
{
    if (trap_def* ptrap = find_trap(pos))
        ptrap->destroy();
}

trap_def* find_trap(const coord_def& pos)
{
    if (!feat_is_trap(grd(pos), true))
        return (NULL);

    unsigned short t = env.tgrid(pos);
    ASSERT(t != NON_ENTITY && t < MAX_TRAPS);
    ASSERT(env.trap[t].pos == pos && env.trap[t].type != TRAP_UNASSIGNED);

    return (&env.trap[t]);
}

trap_type get_trap_type(const coord_def& pos)
{
    if (trap_def* ptrap = find_trap(pos))
        return (ptrap->type);

    if (feature_mimic_at(pos))
    {
        monster *mimic = monster_at(pos);
        if (mimic->props.exists("trap_type"))
            return static_cast<trap_type>(mimic->props["trap_type"].get_short());
    }

    return (TRAP_UNASSIGNED);
}

// Returns the unqualified name ("blade", "dart") of the trap at the
// given position. Does not check if the trap has been discovered, and
// will faithfully report the names of unknown traps.
//
// If there is no trap at the given position, returns an empty string.
const char *trap_name_at(const coord_def& c)
{
    const trap_type trap = get_trap_type(c);
    return trap != TRAP_UNASSIGNED? trap_name(trap) : "";
}

static bool _disarm_is_deadly(trap_def& trap)
{
    int dam = trap.max_damage(you);
    if (trap.type == TRAP_NEEDLE && you.res_poison() <= 0)
        dam += 15; // arbitrary

    return (you.hp <= dam);
}

// where *must* point to a valid, discovered trap.
void disarm_trap(const coord_def& where)
{
    if (you.berserk())
    {
        canned_msg(MSG_TOO_BERSERK);
        return;
    }

    trap_def& trap = *find_trap(where);

    switch (trap.category())
    {
    case DNGN_TRAP_MAGICAL:
        // Zotdef - allow alarm traps to be disarmed
        if (!crawl_state.game_is_zotdef() || trap.type != TRAP_ALARM)
        {
            mpr("You can't disarm that trap.");
            return;
        }
        break;
    case DNGN_TRAP_NATURAL:
        // Only shafts for now.
        mpr("You can't disarm a shaft.");
        return;
    default:
        break;
    }

    // Prompt for any trap for which you might not survive setting it off.
    if (_disarm_is_deadly(trap))
    {
        std::string prompt = make_stringf(
                               "Really try disarming that %s?",
                               feature_description(trap.category(),
                                                   get_trap_type(where),
                                                   "", DESC_BASENAME,
                                                   false).c_str());

        if (!yesno(prompt.c_str(), true, 'n'))
        {
            canned_msg(MSG_OK);
            return;
        }
    }

    // Make the actual attempt
    you.turn_is_over = true;
    if (random2(you.skill(SK_TRAPS_DOORS) + 2) <= random2(you.absdepth0 + 5))
    {
        mpr("You failed to disarm the trap.");
        if (random2(you.dex()) > 5 + random2(5 + you.absdepth0))
            practise(EX_TRAP_DISARM_FAIL, you.absdepth0);
        else
        {
            if (trap.type == TRAP_NET && trap.pos != you.pos())
            {
                if (coinflip())
                {
                    mpr("You stumble into the trap!");
                    move_player_to_grid(trap.pos, true, false);
                }
            }
            else
                trap.trigger(you, true);

            practise(EX_TRAP_DISARM_TRIGGER);
        }
    }
    else
    {
        mpr("You have disarmed the trap.");
        trap.disarm();
        practise(EX_TRAP_DISARM, you.absdepth0);
    }
}

// Attempts to take a net off a given monster.
// This doesn't actually have any effect (yet).
// Do not expect gratitude for this!
// ----------------------------------
void remove_net_from(monster* mon)
{
    you.turn_is_over = true;

    int net = get_trapping_net(mon->pos());

    if (net == NON_ITEM)
    {
        mon->del_ench(ENCH_HELD, true);
        return;
    }

    // factor in whether monster is paralysed or invisible
    int paralys = 0;
    if (mon->paralysed()) // makes this easier
        paralys = random2(5);

    int invis = 0;
    if (!mon->visible_to(&you)) // makes this harder
        invis = 3 + random2(5);

    bool net_destroyed = false;
    if (random2(you.skill(SK_TRAPS_DOORS) + 2) + paralys
           <= random2(2*mon->body_size(PSIZE_BODY) + 3) + invis)
    {
        if (one_chance_in(you.skill(SK_TRAPS_DOORS) + you.dex()/2))
        {
            mitm[net].plus--;
            mpr("You tear at the net.");
            if (mitm[net].plus < -7)
            {
                mprf("Whoops! The net comes apart in your %s!",
                     your_hand(true).c_str());
                mon->del_ench(ENCH_HELD, true);
                destroy_item(net);
                net_destroyed = true;
            }
        }

        if (!net_destroyed)
        {
            if (mon->visible_to(&you))
            {
                mprf("You fail to remove the net from %s.",
                     mon->name(DESC_NOCAP_THE).c_str());
            }
            else
                mpr("You fail to remove the net.");
        }

        practise(EX_REMOVE_NET);
        return;
    }

    mon->del_ench(ENCH_HELD, true);
    remove_item_stationary(mitm[net]);

    if (mon->visible_to(&you))
        mprf("You free %s.", mon->name(DESC_NOCAP_THE).c_str());
    else
        mpr("You loosen the net.");

}

// Decides whether you will try to tear the net (result <= 0)
// or try to slip out of it (result > 0).
// Both damage and escape could be 9 (more likely for damage)
// but are capped at 5 (damage) and 4 (escape).
static int damage_or_escape_net(int hold)
{
    // Spriggan: little (+2)
    // Halfling, Kobold: small (+1)
    // Human, Elf, ...: medium (0)
    // Ogre, Troll, Centaur, Naga: large (-1)
    // transformations: spider, bat: tiny (+3); ice beast: large (-1)
    int escape = SIZE_MEDIUM - you.body_size(PSIZE_BODY);

    int damage = -escape;

    // your weapon may damage the net, max. bonus of 2
    if (you.weapon())
    {
        if (can_cut_meat(*you.weapon()))
            damage++;

        int brand = get_weapon_brand(*you.weapon());
        if (brand == SPWPN_FLAMING || brand == SPWPN_VORPAL)
            damage++;
    }
    else if (you.form == TRAN_BLADE_HANDS)
        damage += 2;
    else if (you.has_usable_claws())
    {
        int level = you.has_claws();
        if (level == 1)
            damage += coinflip();
        else
            damage += level - 1;
    }

    // Berserkers get a fighting bonus.
    if (you.berserk())
        damage += 2;

    // Check stats.
    if (x_chance_in_y(you.strength(), 18))
        damage++;
    if (x_chance_in_y(you.dex(), 12))
        escape++;
    if (x_chance_in_y(player_evasion(), 20))
        escape++;

    // Dangerous monsters around you add urgency.
    if (there_are_monsters_nearby(true))
    {
        damage++;
        escape++;
    }

    // Confusion makes the whole thing somewhat harder
    // (less so for trying to escape).
    if (you.confused())
    {
        if (escape > 1)
            escape--;
        else if (damage >= 2)
            damage -= 2;
    }

    // Damaged nets are easier to destroy.
    if (hold < 0)
    {
        damage += random2(-hold/3 + 1);

        // ... and easier to slip out of (but only if escape looks feasible).
        if (you.attribute[ATTR_HELD] < 5 || escape >= damage)
            escape += random2(-hold/2) + 1;
    }

    // If undecided, choose damaging approach (it's quicker).
    if (damage >= escape)
        return (-damage); // negate value

    return (escape);
}

// Calls the above function to decide on how to get free.
// Note that usually the net will be damaged until trying to slip out
// becomes feasible (for size etc.), so it may take even longer.
void free_self_from_net()
{
    int net = get_trapping_net(you.pos());

    if (net == NON_ITEM) // really shouldn't happen!
    {
        you.attribute[ATTR_HELD] = 0;
        you.redraw_quiver = true;
        return;
    }

    int hold = mitm[net].plus;
    int do_what = damage_or_escape_net(hold);
    dprf("net.plus: %d, ATTR_HELD: %d, do_what: %d",
         hold, you.attribute[ATTR_HELD], do_what);

    if (do_what <= 0) // You try to destroy the net
    {
        // For previously undamaged nets this takes at least 2 and at most
        // 8 turns.
        bool can_slice =
            (you.form == TRAN_BLADE_HANDS)
            || (you.weapon() && can_cut_meat(*you.weapon()));

        int damage = -do_what;

        if (damage < 1)
            damage = 1;

        if (you.berserk())
            damage *= 2;

        // Medium sized characters are at a disadvantage and sometimes
        // get a bonus.
        if (you.body_size(PSIZE_BODY) == SIZE_MEDIUM)
            damage += coinflip();

        if (damage > 5)
            damage = 5;

        hold -= damage;
        mitm[net].plus = hold;

        if (hold < -7)
        {
            mprf("You %s the net and break free!",
                 can_slice ? (damage >= 4? "slice" : "cut") :
                             (damage >= 4? "shred" : "rip"));

            destroy_item(net);

            you.attribute[ATTR_HELD] = 0;
            you.redraw_quiver = true;
            return;
        }

        if (damage >= 4)
        {
            mprf("You %s into the net.",
                 can_slice? "slice" : "tear a large gash");
        }
        else
            mpr("You struggle against the net.");

        // Occasionally decrease duration a bit
        // (this is so switching from damage to escape does not hurt as much).
        if (you.attribute[ATTR_HELD] > 1 && coinflip())
        {
            you.attribute[ATTR_HELD]--;

            if (you.attribute[ATTR_HELD] > 1 && hold < -random2(5))
                you.attribute[ATTR_HELD]--;
        }
   }
   else
   {
        // You try to escape (takes at least 3 turns, and at most 10).
        int escape = do_what;

        if (you.duration[DUR_HASTE] || you.duration[DUR_BERSERK]) // extra bonus
            escape++;

        // Medium sized characters are at a disadvantage and sometimes
        // get a bonus.
        if (you.body_size(PSIZE_BODY) == SIZE_MEDIUM)
            escape += coinflip();

        if (escape > 4)
            escape = 4;

        if (escape >= you.attribute[ATTR_HELD])
        {
            if (escape >= 3)
                mpr("You slip out of the net!");
            else
                mpr("You break free from the net!");

            you.attribute[ATTR_HELD] = 0;
            you.redraw_quiver = true;
            remove_item_stationary(mitm[net]);
            return;
        }

        if (escape >= 3)
            mpr("You try to slip out of the net.");
        else
            mpr("You struggle to escape the net.");

        you.attribute[ATTR_HELD] -= escape;
   }
}

void clear_trapping_net()
{
    if (!you.attribute[ATTR_HELD])
        return;

    if (!in_bounds(you.pos()))
        return;

    const int net = get_trapping_net(you.pos());
    if (net != NON_ITEM)
        remove_item_stationary(mitm[net]);

    you.attribute[ATTR_HELD] = 0;
    you.redraw_quiver = true;
}

item_def trap_def::generate_trap_item()
{
    item_def item;
    object_class_type base;
    int sub;

    switch (this->type)
    {
    case TRAP_DART:   base = OBJ_MISSILES; sub = MI_DART;         break;
    case TRAP_ARROW:  base = OBJ_MISSILES; sub = MI_ARROW;        break;
    case TRAP_BOLT:   base = OBJ_MISSILES; sub = MI_BOLT;         break;
    case TRAP_SPEAR:  base = OBJ_WEAPONS;  sub = WPN_SPEAR;       break;
    case TRAP_AXE:    base = OBJ_WEAPONS;  sub = WPN_HAND_AXE;    break;
    case TRAP_NEEDLE: base = OBJ_MISSILES; sub = MI_NEEDLE;       break;
    case TRAP_NET:    base = OBJ_MISSILES; sub = MI_THROWING_NET; break;
    default:          return item;
    }

    item.base_type = base;
    item.sub_type  = sub;
    item.quantity  = 1;

    if (base == OBJ_MISSILES)
    {
        set_item_ego_type(item, base,
                          (sub == MI_NEEDLE) ? SPMSL_POISONED : SPMSL_NORMAL);
    }
    else
    {
        set_item_ego_type(item, base, SPWPN_NORMAL);
    }

    item_colour(item);
    return item;
}

// Shoot a single piece of ammo at the relevant actor.
void trap_def::shoot_ammo(actor& act, bool was_known)
{
    if (this->ammo_qty <= 0)
    {
        if (was_known && act.atype() == ACT_PLAYER)
            mpr("The trap is out of ammunition!");
        else if (player_can_hear(this->pos) && you.see_cell(this->pos))
            mpr("You hear a soft click.");

        this->disarm();
    }
    else
    {
        // Record position now, in case it's a monster and dies (thus
        // resetting its position) before the ammo can be dropped.
        const coord_def apos = act.pos();

        item_def shot = this->generate_trap_item();

        bool force_poison = (env.markers.property_at(pos, MAT_ANY,
                                "poisoned_needle_trap") == "true");

        bool force_hit = (env.markers.property_at(pos, MAT_ANY,
                                "force_hit") == "true");

        bool poison = (this->type == TRAP_NEEDLE
                       && !act.res_poison()
                       && (x_chance_in_y(50 - (3*act.armour_class()) / 2, 100)
                            || force_poison));

        int damage_taken =
            std::max(this->shot_damage(act) - random2(act.armour_class()+1),0);

        int trap_hit = (20 + (you.absdepth0*2)) * random2(200) / 100;

        if (act.atype() == ACT_PLAYER)
        {
            if (!force_hit && (one_chance_in(5) || was_known && !one_chance_in(4)))
            {
                mprf("You avoid triggering %s trap.",
                      this->name(DESC_NOCAP_A).c_str());

                return;         // no ammo generated either
            }

            // Start constructing the message.
            std::string msg = shot.name(DESC_CAP_A) + " shoots out and ";

            // Check for shield blocking.
            // Exercise only if the trap was unknown (to prevent scumming.)
            if (!was_known && player_shield_class())
                practise(EX_SHIELD_TRAP);

            const int con_block = random2(20 + you.shield_block_penalty());
            const int pro_block = you.shield_bonus();
            if (pro_block >= con_block && !force_hit)
            {
                // Note that we don't call shield_block_succeeded()
                // because that can exercise Shields skill.
                you.shield_blocks++;
                msg += "hits your shield.";
                mpr(msg.c_str());
            }
            else
            {
                int repel_turns = you.duration[DUR_REPEL_MISSILES]
                                               / BASELINE_DELAY;
                // Note that this uses full (not random2limit(foo,40))
                // player_evasion.
                int your_dodge = you.melee_evasion(NULL) - 2
                    + (random2(you.dex()) / 3)
                    + (repel_turns * 10);

                // Check if it got past dodging. Deflect Missiles provides
                // immunity to such traps.
                if (trap_hit >= your_dodge
                    && you.duration[DUR_DEFLECT_MISSILES] == 0
                    || force_hit)
                {
                    // OK, we've been hit.
                    msg += "hits you!";
                    mpr(msg.c_str());

                    std::string n = name(DESC_NOCAP_A) + " trap";

                    // Needle traps can poison.
                    if (poison)
                        poison_player(1 + random2(3), "", n);

                    ouch(damage_taken, NON_MONSTER, KILLED_BY_TRAP, n.c_str());
                }
                else            // trap dodged
                {
                    msg += "misses you.";
                    mpr(msg.c_str());
                }

                // Exercise only if the trap was unknown (to prevent scumming.)
                if (!was_known)
                    practise(EX_DODGE_TRAP);
            }
        }
        else if (act.atype() == ACT_MONSTER)
        {
            // Determine whether projectile hits.
            bool hit = (trap_hit >= act.melee_evasion(NULL));

            if (you.see_cell(act.pos()))
            {
                mprf("%s %s %s%s!",
                     shot.name(DESC_CAP_A).c_str(),
                     hit ? "hits" : "misses",
                     act.name(DESC_NOCAP_THE).c_str(),
                     (hit && damage_taken == 0
                         && !poison) ? ", but does no damage" : "");
            }

            // Apply damage.
            if (hit)
            {
                if (poison)
                    act.poison(NULL, 1 + random2(3));
                act.hurt(NULL, damage_taken);
            }
        }

        // Drop the item (sometimes.)
        if (coinflip())
            copy_item_to_grid(shot, apos);

        this->ammo_qty--;
    }
}

// returns appropriate trap symbol
dungeon_feature_type trap_def::category() const
{
    return trap_category(type);
}

dungeon_feature_type trap_category(trap_type type)
{
    switch (type)
    {
    case TRAP_SHAFT:
        return (DNGN_TRAP_NATURAL);

    case TRAP_TELEPORT:
    case TRAP_ALARM:
    case TRAP_ZOT:
    case TRAP_GOLUBRIA:
        return (DNGN_TRAP_MAGICAL);

    case TRAP_DART:
    case TRAP_ARROW:
    case TRAP_SPEAR:
    case TRAP_AXE:
    case TRAP_BLADE:
    case TRAP_BOLT:
    case TRAP_NEEDLE:
    case TRAP_NET:
    case TRAP_PLATE:
    default:                    // what *would* be the default? {dlb}
        return (DNGN_TRAP_MECHANICAL);
    }
}

trap_type random_trap()
{
    return (static_cast<trap_type>(random2(TRAP_MAX_REGULAR+1)));
}

trap_type random_trap(dungeon_feature_type feat)
{
    ASSERT(feat_is_trap(feat, false));
    trap_type trap = NUM_TRAPS;
    do
        trap = random_trap();
    while (trap_category(trap) != feat);
    return trap;
}

bool is_valid_shaft_level(const level_id &place)
{
    if (crawl_state.test
        || crawl_state.game_is_sprint()
        || crawl_state.game_is_zotdef())
    {
        return (false);
    }

    if (place.level_type != LEVEL_DUNGEON)
        return (false);

    // Shafts are now allowed on the first two levels, as they have a
    // good chance of being detected. You'll also fall less deep.
    /* if (place == BRANCH_MAIN_DUNGEON && you.absdepth0 < 2)
        return (false); */

    // Don't generate shafts in branches where teleport control
    // is prevented.  Prevents player from going down levels without
    // reaching stairs, and also keeps player from getting stuck
    // on lower levels with the innability to use teleport control to
    // get back up.
    if (testbits(get_branch_flags(place.branch), BFLAG_NO_TELE_CONTROL))
        return (false);

    const Branch &branch = branches[place.branch];

    // When generating levels, don't place a shaft on the level
    // immediately above the bottom of a branch if that branch is
    // significantly more dangerous than normal.
    int min_delta = 1;
    if (env.turns_on_level == -1 && branch.dangerous_bottom_level)
        min_delta = 2;

    return ((branch.depth - place.depth) >= min_delta);
}

// Shafts can be generated visible.
//
// Starts about 50% of the time and approaches 0% for randomly
// placed traps, and starts at 100% and approaches 50% for
// others (e.g. at end of corridor).
bool shaft_known(int depth, bool randomly_placed)
{
    if (randomly_placed)
        return (coinflip() && x_chance_in_y(3, depth));
    else
        return (coinflip() || x_chance_in_y(3, depth));
}

level_id generic_shaft_dest(level_pos lpos, bool known = false)
{
    level_id  lid   = lpos.id;

    if (lid.level_type != LEVEL_DUNGEON)
        return lid;

    int      curr_depth = lid.depth;
    Branch   &branch    = branches[lid.branch];

    // Shaft traps' behavior depends on whether it is entered intentionally.
    // Knowingly entering one is more likely to drop you 1 level.
    // Falling in unknowingly can drop you 1/2/3 levels with equal chance.

    if (known)
    {
        // Chances are 5/8s for 1 level, 2/8s for 2 levels, 1/8 for 3 levels
        int s = random2(8) + 1;
        if (s == 1)
            lid.depth += 3;
        else if (s <= 3)
            lid.depth += 2;
        else
            lid.depth += 1;
    }
    else
    {
        // 33.3% for 1, 2, 3 from D:3, less before
        lid.depth += 1 + random2(std::min(lid.depth, 3));
    }

    if (lid.depth > branch.depth)
        lid.depth = branch.depth;

    if (lid.depth == curr_depth)
        return lid;

    // Only shafts on the level immediately above a dangerous branch
    // bottom will take you to that dangerous bottom, and shafts can't
    // be created during level generation time.
    // Include level 27 of the main dungeon here, but don't restrict
    // shaft creation (so don't set branch.dangerous_bottom_level).
    if (branch.dangerous_bottom_level
        && lid.depth == branch.depth
        && (branch.depth - curr_depth) > 1)
    {
        lid.depth--;
    }

    return lid;
}

level_id generic_shaft_dest(coord_def pos, bool known = false)
{
    return generic_shaft_dest(level_pos(level_id::current(), pos));
}

void handle_items_on_shaft(const coord_def& pos, bool open_shaft)
{
    if (!is_valid_shaft_level())
        return;

    level_id dest = generic_shaft_dest(pos);

    if (dest == level_id::current())
        return;

    int o = igrd(pos);

    if (o == NON_ITEM)
        return;

    igrd(pos) = NON_ITEM;

    if (env.map_knowledge(pos).seen() && open_shaft)
    {
        mpr("A shaft opens up in the floor!");
        grd(pos) = DNGN_TRAP_NATURAL;
    }

    while (o != NON_ITEM)
    {
        int next = mitm[o].link;

        if (mitm[o].defined())
        {
            if (env.map_knowledge(pos).seen())
            {
                mprf("%s falls through the shaft.",
                     mitm[o].name(DESC_INVENTORY).c_str());
            }
            add_item_to_transit(dest, mitm[o]);

            mitm[o].base_type = OBJ_UNASSIGNED;
            mitm[o].quantity = 0;
            mitm[o].props.clear();
        }

        o = next;
    }
}

static int _num_traps_default(int level_number, const level_id &place)
{
    return random2avg(9, 2);
}

int num_traps_for_place(int level_number, const level_id &place)
{
    if (level_number == -1)
        level_number = place.absdepth();

    switch (place.level_type)
    {
    case LEVEL_DUNGEON:
        if (branches[place.branch].num_traps_function != NULL)
            return branches[place.branch].num_traps_function(level_number);
        else
            return _num_traps_default(level_number, place);
    case LEVEL_PANDEMONIUM:
        return _num_traps_default(level_number, place);
    case LEVEL_LABYRINTH:
    case LEVEL_PORTAL_VAULT:
        die("invalid place for traps");
        break;
    case LEVEL_ABYSS:
    default:
        return 0;
    }

    return 0;
}

trap_type random_trap_slime(int level_number)
{
    trap_type type = NUM_TRAPS;

    if (random2(1 + level_number) > 14 && one_chance_in(3))
    {
        type = TRAP_ZOT;
    }

    if (one_chance_in(5) && is_valid_shaft_level(level_id::current()))
        type = TRAP_SHAFT;
    if (one_chance_in(5) && !crawl_state.game_is_sprint())
        type = TRAP_TELEPORT;
    if (one_chance_in(10))
        type = TRAP_ALARM;

    return (type);
}

static trap_type _random_trap_default(int level_number, const level_id &place)
{
    trap_type type = TRAP_DART;

    if ((random2(1 + level_number) > 1) && one_chance_in(4))
        type = TRAP_NEEDLE;
    if (random2(1 + level_number) > 3)
        type = TRAP_SPEAR;
    if (random2(1 + level_number) > 5)
        type = TRAP_AXE;

    // Note we're boosting arrow trap numbers by moving it
    // down the list, and making spear and axe traps rarer.
    if (type == TRAP_DART ? random2(1 + level_number) > 2
                          : one_chance_in(7))
    {
        type = TRAP_ARROW;
    }

    if ((type == TRAP_DART || type == TRAP_ARROW) && one_chance_in(15))
        type = TRAP_NET;

    if (random2(1 + level_number) > 7)
        type = TRAP_BOLT;
    if (random2(1 + level_number) > 11)
        type = TRAP_BLADE;

    if (random2(1 + level_number) > 14 && one_chance_in(3)
        || (place == BRANCH_HALL_OF_ZOT && coinflip()))
    {
        type = TRAP_ZOT;
    }

    if (one_chance_in(20) && is_valid_shaft_level(place))
        type = TRAP_SHAFT;
    if (one_chance_in(20) && !crawl_state.game_is_sprint())
        type = TRAP_TELEPORT;
    if (one_chance_in(40))
        type = TRAP_ALARM;

    return (type);
}

trap_type random_trap_for_place(int level_number, const level_id &place)
{
    if (level_number == -1)
        level_number = place.absdepth();

    if (place.level_type == LEVEL_DUNGEON)
        if (branches[place.branch].rand_trap_function != NULL)
            return branches[place.branch].rand_trap_function(level_number);

    return _random_trap_default(level_number, place);
}

int traps_zero_number(int level_number)
{
    return 0;
}

int count_traps(trap_type ttyp)
{
    int num = 0;
    for (int tcount = 0; tcount < MAX_TRAPS; tcount++)
        if (env.trap[tcount].type == ttyp)
            num++;
    return num;
}
