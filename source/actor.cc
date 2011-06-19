#include "AppHdr.h"

#include "actor.h"
#include "areas.h"
#include "artefact.h"
#include "coord.h"
#include "coordit.h"
#include "env.h"
#include "itemprop.h"
#include "los.h"
#include "mon-death.h"
#include "player.h"
#include "random.h"
#include "state.h"
#include "stuff.h"
#include "terrain.h"
#include "traps.h"

actor::~actor()
{
}

bool actor::has_equipped(equipment_type eq, int sub_type) const
{
    const item_def *item = slot_item(eq, false);
    return (item && item->sub_type == sub_type);
}

bool actor::will_trigger_shaft() const
{
    return (ground_level() && total_weight() > 0 && is_valid_shaft_level()
            // let's pretend that they always make their saving roll
            && !(atype() == ACT_MONSTER
                 && mons_is_elven_twin(static_cast<const monster* >(this))));
}

level_id actor::shaft_dest(bool known = false) const
{
    return generic_shaft_dest(pos(), known);
}

bool actor::airborne() const
{
    return (is_levitating() || (flight_mode() == FL_FLY && !cannot_move()));
}

/**
 * Check if the actor is on the ground (or in water).
 */
bool actor::ground_level() const
{
    return (!airborne() && !is_wall_clinging());
}

bool actor::stand_on_solid_ground() const
{
    return ground_level() && feat_has_solid_floor(grd(pos()))
           && !feat_is_water(grd(pos()));
}

bool actor::can_wield(const item_def* item, bool ignore_curse,
                      bool ignore_brand, bool ignore_shield,
                      bool ignore_transform) const
{
    if (item == NULL)
    {
        // Unarmed combat.
        item_def fake;
        fake.base_type = OBJ_UNASSIGNED;
        return can_wield(fake, ignore_curse, ignore_brand, ignore_transform);
    }
    else
        return can_wield(*item, ignore_curse, ignore_brand, ignore_transform);
}

bool actor::can_pass_through(int x, int y) const
{
    return can_pass_through_feat(grd[x][y]);
}

bool actor::can_pass_through(const coord_def &c) const
{
    return can_pass_through_feat(grd(c));
}

bool actor::is_habitable(const coord_def &_pos) const
{
    if (can_cling_to(_pos))
        return true;

    return is_habitable_feat(grd(_pos));
}

bool actor::handle_trap()
{
    trap_def* trap = find_trap(pos());
    if (trap)
        trap->trigger(*this);
    return (trap != NULL);
}


int actor::res_holy_fire() const
{
    if (is_evil() || is_unholy())
        return (-1);
    else if (is_holy())
        return (3);
    return (0);
}

int actor::check_res_magic(int power)
{
    const int mrs = res_magic();

    if (mrs == MAG_IMMUNE)
        return (100);

    // Evil, evil hack to make weak one hd monsters easier for first level
    // characters who have resistable 1st level spells. Six is a very special
    // value because mrs = hd * 2 * 3 for most monsters, and the weak, low
    // level monsters have been adjusted so that the "3" is typically a 1.
    // There are some notable one hd monsters that shouldn't fall under this,
    // so we do < 6, instead of <= 6...  or checking mons->hit_dice.  The
    // goal here is to make the first level easier for these classes and give
    // them a better shot at getting to level two or three and spells that can
    // help them out (or building a level or two of their base skill so they
    // aren't resisted as often). - bwr
    if (atype() == ACT_MONSTER && mrs < 6 && coinflip())
        return (-1);

    power = stepdown_value(power, 30, 40, 100, 120);

    const int mrchance = (100 + mrs) - power;
    const int mrch2 = random2(100) + random2(101);

    dprf("Power: %d, MR: %d, target: %d, roll: %d",
         power, mrs, mrchance, mrch2);

    return (mrchance - mrch2);
}

void actor::set_position(const coord_def &c)
{
    const coord_def oldpos = position;
    position = c;
    los_actor_moved(this, oldpos);
    areas_actor_moved(this, oldpos);
}

bool actor::can_hibernate(bool holi_only) const
{
    // Undead, nonliving, and plants don't sleep.
    const mon_holy_type holi = holiness();
    if (holi == MH_UNDEAD || holi == MH_NONLIVING || holi == MH_PLANT)
        return (false);

    if (!holi_only)
    {
        // The monster is berserk or already asleep.
        if (!can_sleep())
            return (false);

        // The monster is cold-resistant and can't be hibernated.
        if (res_cold() > 0)
            return (false);

        // The monster has slept recently.
        if (atype() == ACT_MONSTER
            && static_cast<const monster* >(this)->has_ench(ENCH_SLEEP_WARY))
        {
            return (false);
        }
    }

    return (true);
}

bool actor::can_sleep() const
{
    const mon_holy_type holi = holiness();
    if (holi == MH_UNDEAD || holi == MH_NONLIVING || holi == MH_PLANT)
        return (false);
    return !(berserk() || asleep());
}

void actor::shield_block_succeeded(actor *foe)
{
    item_def *sh = shield();
    unrandart_entry *unrand_entry;

    if (sh
        && sh->base_type == OBJ_ARMOUR
        && get_armour_slot(*sh) == EQ_SHIELD
        && is_artefact(*sh)
        && is_unrandom_artefact(*sh)
        && (unrand_entry = get_unrand_entry(sh->special))
        && unrand_entry->fight_func.melee_effects)
    {
       unrand_entry->fight_func.melee_effects(sh, this, foe, false);
    }
}

int actor::body_weight(bool base) const
{
    switch (body_size(PSIZE_BODY, base))
    {
    case SIZE_TINY:
        return (150);
    case SIZE_LITTLE:
        return (300);
    case SIZE_SMALL:
        return (425);
    case SIZE_MEDIUM:
        return (550);
    case SIZE_LARGE:
        return (1300);
    case SIZE_BIG:
        return (1500);
    case SIZE_GIANT:
        return (1800);
    case SIZE_HUGE:
        return (2200);
    default:
        mpr("ERROR: invalid body weight");
        perror("actor::body_weight(): invalid body weight");
        end(0);
        return (0);
    }
}

kill_category actor_kill_alignment(const actor *act)
{
    return (act? act->kill_alignment() : KC_OTHER);
}

bool actor_slime_wall_immune(const actor *act)
{
    return (act->atype() == ACT_PLAYER?
              you.religion == GOD_JIYVA && !you.penance[GOD_JIYVA]
            : act->res_acid() == 3);
}
/*
 * Accessor method to the clinging member.
 *
 * @returns The value of clinging.
 */
bool actor::is_wall_clinging() const
{
    return props.exists("clinging") && props["clinging"].get_bool();
}

/*
 * Check a cell to see if actor can keep clinging if it moves to it.
 *
 * @param p Coordinates of the cell checked.
 * @returns Whether the actor can cling.
 */
bool actor::can_cling_to(const coord_def& p) const
{
    if (!is_wall_clinging() || !can_pass_through_feat(grd(p)))
        return false;

    return cell_can_cling_to(pos(), p);
}

/*
 * Update the clinging status of an actor.
 *
 * It checks adjacent orthogonal walls to see if the actor can cling to them.
 * If actor has fallen from the wall (wall dug or actor changed form), print a
 * message and apply location effects.
 *
 * @param stepped Whether the actor has taken a step.
 * @return the new clinging status.
 */
bool actor::check_clinging(bool stepped, bool door)
{
    bool was_clinging = is_wall_clinging();
    bool clinging = can_cling_to_walls() && cell_is_clingable(pos())
                    && !airborne();

    if (can_cling_to_walls())
        props["clinging"] = clinging;
    else if (props.exists("clinging"))
        props.erase("clinging");

    if (!stepped && was_clinging && !clinging)
    {
        if (you.can_see(this))
        {
            mprf("%s fall%s off the %s.", name(DESC_CAP_THE).c_str(),
                 is_player() ? "" : "s", door ? "door" : "wall");
        }
        apply_location_effects(pos());
    }
    return clinging;
}

void actor::clear_clinging()
{
    if (props.exists("clinging"))
        props["clinging"] = false;
}
