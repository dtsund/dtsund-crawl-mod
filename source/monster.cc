/**
 * @file
 * @brief Monsters class methods
**/

#include "AppHdr.h"

#include "areas.h"
#include "artefact.h"
#include "beam.h"
#include "cloud.h"
#include "coord.h"
#include "coordit.h"
#include "delay.h"
#include "dgnevent.h"
#include "dgn-shoals.h"
#include "directn.h"
#include "env.h"
#include "fight.h"
#include "fineff.h"
#include "fprop.h"
#include "ghost.h"
#include "godabil.h"
#include "goditem.h"
#include "itemname.h"
#include "items.h"
#include "kills.h"
#include "libutil.h"
#include "misc.h"
#include "mon-abil.h"
#include "mon-act.h"
#include "mon-behv.h"
#include "mon-death.h"
#include "mon-place.h"
#include "mon-stuff.h"
#include "mon-transit.h"
#include "mon-util.h"
#include "mgen_data.h"
#include "random.h"
#include "religion.h"
#include "shopping.h"
#include "spl-damage.h"
#include "spl-util.h"
#include "state.h"
#include "stuff.h"
#include "terrain.h"
#ifdef USE_TILE
#include "tileview.h"
#endif
#include "traps.h"
#include "hints.h"
#include "view.h"
#include "shout.h"
#include "xom.h"

#include <algorithm>
#include <queue>

// Macro that saves some typing, nothing more.
#define smc get_monster_data(mc)

monster::monster()
    : hit_points(0), max_hit_points(0), hit_dice(0),
      ac(0), ev(0), speed(0), speed_increment(0), target(), firing_pos(),
      patrol_point(), travel_target(MTRAV_NONE), inv(NON_ITEM), spells(),
      attitude(ATT_HOSTILE), behaviour(BEH_WANDER), foe(MHITYOU),
      enchantments(), flags(0L), experience(0), base_monster(MONS_NO_MONSTER),
      number(0), colour(BLACK), foe_memory(0), shield_blocks(0),
      god(GOD_NO_GOD), ghost(), seen_context("")

{
    type = MONS_NO_MONSTER;
    travel_path.clear();
    props.clear();
    if (crawl_state.game_is_arena())
        foe = MHITNOT;
}

// Empty destructor to keep auto_ptr happy with incomplete ghost_demon type.
monster::~monster()
{
}

monster::monster(const monster& mon)
{
    init_with(mon);
}

monster &monster::operator = (const monster& mon)
{
    if (this != &mon)
        init_with(mon);
    return (*this);
}

void monster::reset()
{
    mname.clear();
    enchantments.clear();
    ench_countdown = 0;
    inv.init(NON_ITEM);

    flags           = 0;
    experience      = 0L;
    type            = MONS_NO_MONSTER;
    base_monster    = MONS_NO_MONSTER;
    hit_points      = 0;
    max_hit_points  = 0;
    hit_dice        = 0;
    ac              = 0;
    ev              = 0;
    speed_increment = 0;
    attitude        = ATT_HOSTILE;
    behaviour       = BEH_SLEEP;
    foe             = MHITNOT;
    number          = 0;
    damage_friendly = 0;
    damage_total    = 0;

    mons_remove_from_grid(this);
    position.reset();
    firing_pos.reset();
    patrol_point.reset();
    travel_target = MTRAV_NONE;
    travel_path.clear();
    ghost.reset(NULL);
    seen_context = "";
    props.clear();
}

void monster::init_with(const monster& mon)
{
    reset();

    mid               = mon.mid;
    mname             = mon.mname;
    type              = mon.type;
    base_monster      = mon.base_monster;
    hit_points        = mon.hit_points;
    max_hit_points    = mon.max_hit_points;
    hit_dice          = mon.hit_dice;
    ac                = mon.ac;
    ev                = mon.ev;
    speed             = mon.speed;
    speed_increment   = mon.speed_increment;
    position          = mon.position;
    target            = mon.target;
    firing_pos        = mon.firing_pos;
    patrol_point      = mon.patrol_point;
    travel_target     = mon.travel_target;
    travel_path       = mon.travel_path;
    inv               = mon.inv;
    spells            = mon.spells;
    attitude          = mon.attitude;
    behaviour         = mon.behaviour;
    foe               = mon.foe;
    enchantments      = mon.enchantments;
    flags             = mon.flags;
    experience        = mon.experience;
    number            = mon.number;
    colour            = mon.colour;
    foe_memory        = mon.foe_memory;
    god               = mon.god;
    props             = mon.props;
    damage_friendly   = mon.damage_friendly;
    damage_total      = mon.damage_total;

    if (mon.ghost.get())
        ghost.reset(new ghost_demon(*mon.ghost));
    else
        ghost.reset(NULL);
}

mon_attitude_type monster::temp_attitude() const
{
    if (has_ench(ENCH_CHARM))
        return ATT_FRIENDLY;
    else if (has_ench(ENCH_TEMP_PACIF))
        return ATT_GOOD_NEUTRAL;
    else
        return attitude;
}

bool monster::swimming() const
{
    const dungeon_feature_type grid = grd(pos());
    return (feat_is_watery(grid) && mons_primary_habitat(this) == HT_WATER);
}

bool monster::wants_submerge() const
{
    // Trapdoor spiders only hide themselves under the floor when they
    // can't see their prey.
    if (type == MONS_TRAPDOOR_SPIDER)
    {
        const actor* _foe = get_foe();
        return (_foe == NULL || !can_see(_foe));
    }

    // Don't submerge if we just unsubmerged to shout.
    if (seen_context == "bursts forth shouting")
        return (false);

    if (!mons_is_fleeing(this)
        && mons_can_move_towards_target(this))
    {
        return (false);
    }

    return (!mons_landlubbers_in_reach(this));
}

bool monster::submerged() const
{
    // FIXME, switch to 4.1's MF_SUBMERGED system which is much cleaner.
    // Can't find any reference to MF_SUBMERGED anywhere. Don't know what
    // this means. - abrahamwl
    if (has_ench(ENCH_SUBMERGED))
        return (true);

    if (grd(pos()) == DNGN_DEEP_WATER
        && !monster_habitable_grid(this, DNGN_DEEP_WATER)
        && !can_drown())
    {
        return (true);
    }

    return (false);
}

bool monster::extra_balanced_at(const coord_def p) const
{
    const dungeon_feature_type grid = grd(p);
    return (grid == DNGN_SHALLOW_WATER
            && (mons_genus(type) == MONS_NAGA             // tails, not feet
                || body_size(PSIZE_BODY) > SIZE_MEDIUM));
}

bool monster::extra_balanced() const
{
    return extra_balanced_at(pos());
}

/*
 * Monster floundering conditions.
 *
 * Floundering reduce the movement speed and can cause the monster to fumble
 * its attacks. It can be caused by water or by Leda's liquefaction.
 *
 * @param pos Coordinates of position to check.
 * @return Whether the monster would be floundering at p.
 */
bool monster::floundering_at(const coord_def p) const
{
    const dungeon_feature_type grid = grd(p);
    return liquefied(p)
           || (feat_is_water(grid)
               // Can't use monster_habitable_grid() because that'll return
               // true for non-water monsters in shallow water.
               && mons_primary_habitat(this) != HT_WATER
               // Use real_amphibious to detect giant non-water monsters in
               // deep water, who flounder despite being treated as amphibious.
               && mons_habitat(this, true) != HT_AMPHIBIOUS
               && !extra_balanced_at(p))
           && !cannot_fight()
           && !mons_flies(this)
           && !(can_cling_to_walls() && cell_is_clingable(p));
}

bool monster::floundering() const
{
    // We recheck wall_clinging just to be sure. There might be some cases,
    // where a cell is clingable and the monster is not clinging.
    return floundering_at(pos()) && !is_wall_clinging();
}

bool monster::can_pass_through_feat(dungeon_feature_type grid) const
{
    return mons_class_can_pass(mons_base_type(this), grid);
}

bool monster::is_habitable_feat(dungeon_feature_type actual_grid) const
{
    return monster_habitable_grid(this, actual_grid);
}

bool monster::can_drown() const
{
    // Presumably a shark in lava or a lavafish in deep water could
    // drown, but that should never happen, so this simple check should
    // be enough.
    switch (mons_primary_habitat(this))
    {
    case HT_WATER:
    case HT_LAVA:
        return (false);
    default:
        break;
    }

    // Mummies can fall apart in water or be incinerated in lava.
    // Ghouls and vampires can drown in water or lava.  Others just
    // "sink like a rock", to never be seen again.
    return (!res_water_drowning()
            || mons_genus(type) == MONS_MUMMY
            || mons_genus(type) == MONS_GHOUL
            || mons_genus(type) == MONS_VAMPIRE);
}

size_type monster::body_size(size_part_type /* psize */, bool /* base */) const
{
    monster_info mi(this, MILEV_NAME);
    return mi.body_size();
}

int monster::body_weight(bool /*base*/) const
{
    int mc = mons_base_type(this);

    if (mc == MONS_RAKSHASA_FAKE || mc == MONS_MARA_FAKE)
        return (0);

    int weight = mons_weight(mc);

    // weight == 0 in the monster entry indicates "no corpse".  Can't
    // use CE_NOCORPSE, because the corpse-effect field is used for
    // corpseless monsters to indicate what happens if their blood
    // is sucked.  Grrrr.
    if (weight == 0 && !is_insubstantial())
    {
        weight = actor::body_weight();

        switch (mc)
        {
        case MONS_IRON_IMP:
            weight += 450;
            break;

        case MONS_IRON_DEVIL:
            weight += 550;
            break;

        case MONS_STONE_GOLEM:
        case MONS_EARTH_ELEMENTAL:
        case MONS_CRYSTAL_GOLEM:
            weight *= 2;
            break;

        case MONS_IRON_DRAGON:
        case MONS_IRON_GOLEM:
            weight *= 3;
            break;

        case MONS_QUICKSILVER_DRAGON:
        case MONS_SILVER_STATUE:
        case MONS_STATUE:
            weight *= 4;
            break;

        case MONS_WOOD_GOLEM:
            weight *= 2;
            weight /= 3;
            break;

        case MONS_FLYING_SKULL:
        case MONS_CURSE_SKULL:
        case MONS_BONE_DRAGON:
        case MONS_SKELETAL_WARRIOR:
            weight /= 2;
            break;

        case MONS_SHADOW_FIEND:
        case MONS_SHADOW_IMP:
        case MONS_SHADOW_DEMON:
            weight /= 3;
            break;
        }

        switch (mons_base_char(mc))
        {
        case 'L':
            weight /= 2;
            break;
        }
    }

    if (mc == MONS_SKELETON_SMALL || mc == MONS_SKELETON_LARGE)
        weight /= 2;

    // Slime creature weight is multiplied by the number merged.
    if (mc == MONS_SLIME_CREATURE && number > 1)
        weight *= number;

    return (weight);
}

int monster::total_weight() const
{
    int burden = 0;

    for (int i = 0; i < NUM_MONSTER_SLOTS; i++)
        if (inv[i] != NON_ITEM)
            burden += item_mass(mitm[inv[i]]) * mitm[inv[i]].quantity;

    return (body_weight() + burden);
}

int monster::damage_brand(int which_attack)
{
    const item_def *mweap = weapon(which_attack);

    if (!mweap)
    {
        if (mons_is_ghost_demon(type))
            return (ghost->brand);

        return (SPWPN_NORMAL);
    }

    return (!is_range_weapon(*mweap) ? get_weapon_brand(*mweap) : SPWPN_NORMAL);
}

int monster::damage_type(int which_attack)
{
    const item_def *mweap = weapon(which_attack);

    if (!mweap)
    {
        const mon_attack_def atk = mons_attack_spec(this, which_attack);
        return ((atk.type == AT_CLAW)          ? DVORP_CLAWING :
                (atk.type == AT_TENTACLE_SLAP) ? DVORP_TENTACLE
                                               : DVORP_CRUSHING);
    }

    return (get_vorpal_type(*mweap));
}

item_def *monster::missiles()
{
    return (inv[MSLOT_MISSILE] != NON_ITEM ? &mitm[inv[MSLOT_MISSILE]] : NULL);
}

int monster::missile_count()
{
    if (const item_def *missile = missiles())
        return (missile->quantity);

    return (0);
}

item_def *monster::launcher()
{
    item_def *weap = mslot_item(MSLOT_WEAPON);
    if (weap && is_range_weapon(*weap))
        return (weap);

    weap = mslot_item(MSLOT_ALT_WEAPON);
    return (weap && is_range_weapon(*weap) ? weap : NULL);
}

// Does not check whether the monster can dual-wield - that is the
// caller's responsibility.
static int _mons_offhand_weapon_index(const monster* m)
{
    return (m->inv[MSLOT_ALT_WEAPON]);
}

item_def *monster::weapon(int which_attack)
{
    const mon_attack_def attk = mons_attack_spec(this, which_attack);
    if (attk.type != AT_HIT && attk.type != AT_WEAP_ONLY)
        return (NULL);

    // Even/odd attacks use main/offhand weapon.
    if (which_attack > 1)
        which_attack &= 1;

    // This randomly picks one of the wielded weapons for monsters that can use
    // two weapons. Not ideal, but better than nothing. fight.cc does it right,
    // for various values of right.
    int weap = inv[MSLOT_WEAPON];

    if (which_attack && mons_wields_two_weapons(this))
    {
        const int offhand = _mons_offhand_weapon_index(this);
        if (offhand != NON_ITEM
            && (weap == NON_ITEM || which_attack == 1 || coinflip()))
        {
            weap = offhand;
        }
    }

    return (weap == NON_ITEM ? NULL : &mitm[weap]);
}

bool monster::can_wield(const item_def& item, bool ignore_curse,
                         bool ignore_brand, bool ignore_shield,
                         bool ignore_transform) const
{
    // Monsters can only wield weapons or go unarmed (OBJ_UNASSIGNED
    // means unarmed).
    if (item.base_type != OBJ_WEAPONS && item.base_type != OBJ_UNASSIGNED)
        return (false);

    // These *are* weapons, so they can't wield another weapon or
    // unwield themselves.
    if (type == MONS_DANCING_WEAPON)
        return (false);

    // MF_HARD_RESET means that all items the monster is carrying will
    // disappear when it does, so it can't accept new items or give up
    // the ones it has.
    if (flags & MF_HARD_RESET)
        return (false);

    // Summoned items can only be held by summoned monsters.
    if ((item.flags & ISFLAG_SUMMONED) && !is_summoned())
        return (false);

    item_def* weap1 = NULL;
    if (inv[MSLOT_WEAPON] != NON_ITEM)
        weap1 = &mitm[inv[MSLOT_WEAPON]];

    int       avail_slots = 1;
    item_def* weap2       = NULL;
    if (mons_wields_two_weapons(this))
    {
        if (!weap1 || hands_reqd(*weap1, body_size()) != HANDS_TWO)
            avail_slots = 2;

        const int offhand = _mons_offhand_weapon_index(this);
        if (offhand != NON_ITEM)
            weap2 = &mitm[offhand];
    }

    // If we're already wielding it, then of course we can wield it.
    if (&item == weap1 || &item == weap2)
        return (true);

    // Barehanded needs two hands.
    const bool two_handed = item.base_type == OBJ_UNASSIGNED
                            || hands_reqd(item, body_size()) == HANDS_TWO;

    item_def* _shield = NULL;
    if (inv[MSLOT_SHIELD] != NON_ITEM)
    {
        ASSERT(!(weap1 && weap2));

        if (two_handed && !ignore_shield)
            return (false);

        _shield = &mitm[inv[MSLOT_SHIELD]];
    }

    if (!ignore_curse)
    {
        int num_cursed = 0;
        if (weap1 && weap1->cursed())
            num_cursed++;
        if (weap2 && weap2->cursed())
            num_cursed++;
        if (_shield && _shield->cursed())
            num_cursed++;

        if (two_handed && num_cursed > 0 || num_cursed >= avail_slots)
            return (false);
    }

    return could_wield(item, ignore_brand, ignore_transform);
}

bool monster::could_wield(const item_def &item, bool ignore_brand,
                           bool /* ignore_transform */) const
{
    ASSERT(item.defined());

    // These *are* weapons, so they can't wield another weapon.
    if (type == MONS_DANCING_WEAPON)
        return (false);

    // Monsters can't use unrandarts with special effects.
    if (is_special_unrandom_artefact(item) && !crawl_state.game_is_arena())
        return (false);

    // Wimpy monsters (e.g. kobolds, goblins) can't use halberds, etc.
    if (!check_weapon_wieldable_size(item, body_size()))
        return (false);

    if (!ignore_brand)
    {
        const int brand = get_weapon_brand(item);

        // Draconians won't use dragon slaying weapons.
        if (brand == SPWPN_DRAGON_SLAYING && is_dragonkind(this))
            return (false);

        // Orcs won't use orc slaying weapons.
        if (brand == SPWPN_ORC_SLAYING && is_orckind(this))
            return (false);

        // Undead and demonic monsters and monsters that are
        // gifts/worshippers of Yredelemnul won't use holy weapons.
        if ((undead_or_demonic() || god == GOD_YREDELEMNUL)
            && is_holy_item(item))
        {
            return (false);
        }

        // Holy monsters that aren't gifts/worshippers of chaotic gods
        // and monsters that are gifts/worshippers of good gods won't
        // use potentially unholy weapons.
        if (((is_holy() && !is_chaotic_god(god)) || is_good_god(god))
            && is_potentially_unholy_item(item))
        {
            return (false);
        }

        // Holy monsters and monsters that are gifts/worshippers of good
        // gods won't use unholy weapons.
        if ((is_holy() || is_good_god(god)) && is_unholy_item(item))
            return (false);

        // Holy monsters that aren't gifts/worshippers of chaotic gods
        // and monsters that are gifts/worshippers of good gods won't
        // use potentially evil weapons.
        if (((is_holy() && !is_chaotic_god(god))
                || is_good_god(god))
            && is_potentially_evil_item(item))
        {
            return (false);
        }

        // Holy monsters and monsters that are gifts/worshippers of good
        // gods won't use evil weapons.
        if (((is_holy() || is_good_god(god)))
            && is_evil_item(item))
        {
            return (false);
        }

        // Monsters that are gifts/worshippers of Fedhas won't use
        // corpse-violating weapons.
        if (god == GOD_FEDHAS && is_corpse_violating_item(item))
            return (false);

        // Monsters that are gifts/worshippers of Zin won't use unclean
        // weapons.
        if (god == GOD_ZIN && is_unclean_item(item))
            return (false);

        // Holy monsters that aren't gifts/worshippers of chaotic gods
        // and monsters that are gifts/worshippers of good gods won't
        // use chaotic weapons.
        if (((is_holy() && !is_chaotic_god(god)) || is_good_god(god))
            && is_chaotic_item(item))
        {
            return (false);
        }

        // Monsters that are gifts/worshippers of TSO won't use poisoned
        // weapons.
        if (god == GOD_SHINING_ONE && is_poisoned_item(item))
            return (false);
    }

    return (true);
}

bool monster::can_throw_large_rocks() const
{
    return (type == MONS_STONE_GIANT
            || type == MONS_POLYPHEMUS
            || type == MONS_CHUCK
            || ::mons_species(type) == MONS_CYCLOPS
            || ::mons_species(type) == MONS_OGRE);
}

bool monster::can_speak()
{
    if (has_ench(ENCH_MUTE))
        return (false);

    // Priest and wizard monsters can always speak.
    if (is_priest() || is_actual_spellcaster())
        return (true);

    // Silent or non-sentient monsters can't use the original speech.
    if (mons_intel(this) < I_NORMAL
        || mons_shouts(type) == S_SILENT)
    {
        return (false);
    }

    // Does it have the proper vocal equipment?
    const mon_body_shape shape = get_mon_shape(this);
    return (shape >= MON_SHAPE_HUMANOID && shape <= MON_SHAPE_NAGA);
}

bool monster::has_spell_of_type(unsigned disciplines) const
{
    for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
    {
        if (spells[i] == SPELL_NO_SPELL)
            continue;

        if (spell_typematch(spells[i], disciplines))
            return (true);
    }
    return (false);
}

void monster::bind_melee_flags()
{
    // Bind fighter / dual-wielder / archer flags from the base type.

    // Alas, we don't know if the mon is zombified at the moment, if it
    // is, the flags (other than dual-wielder) will be removed later.
    if (mons_class_flag(type, M_FIGHTER))
        flags |= MF_FIGHTER;
    if (mons_class_flag(type, M_TWO_WEAPONS))
        flags |= MF_TWO_WEAPONS;
    if (mons_class_flag(type, M_ARCHER))
        flags |= MF_ARCHER;
}

void monster::bind_spell_flags()
{
    // Bind spellcaster / priest flags from the base type. These may be
    // overridden by vault defs for individual monsters.

    // Alas, we don't know if the mon is zombified at the moment, if it
    // is, the flags will be removed later.
    if (mons_class_flag(type, M_SPELLCASTER))
        flags |= MF_SPELLCASTER;
    if (mons_class_flag(type, M_ACTUAL_SPELLS))
        flags |= MF_ACTUAL_SPELLS;
    if (mons_class_flag(type, M_PRIEST))
        flags |= MF_PRIEST;
}

static bool _needs_ranged_attack(const monster* mon)
{
    // Prevent monsters that have conjurations from grabbing missiles.
    if (mon->has_spell_of_type(SPTYP_CONJURATION))
        return (false);

    // Same for summonings, but make an exception for friendlies.
    if (!mon->friendly() && mon->has_spell_of_type(SPTYP_SUMMONING))
        return (false);

    // Blademasters don't want to throw stuff.
    if (mon->type == MONS_DEEP_ELF_BLADEMASTER)
        return (false);

    return (true);
}

bool monster::can_use_missile(const item_def &item) const
{
    // Don't allow monsters to pick up missiles without the corresponding
    // launcher. The opposite is okay, and sufficient wandering will
    // hopefully take the monster to a stack of appropriate missiles.

    if (!_needs_ranged_attack(this))
        return (false);

    if (item.base_type == OBJ_WEAPONS
        || item.base_type == OBJ_MISSILES && !has_launcher(item))
    {
        return (is_throwable(this, item));
    }

    // Rods are always non-throwable.
    if (item_is_rod(item) || item_is_staff(item))
        return (false);

    // Stones are allowed even without launcher.
    if (item.sub_type == MI_STONE)
        return (true);

    item_def *launch;
    for (int i = MSLOT_WEAPON; i <= MSLOT_ALT_WEAPON; ++i)
    {
        launch = mslot_item(static_cast<mon_inv_type>(i));
        if (launch && fires_ammo_type(*launch) == item.sub_type)
            return (true);
    }

    // No fitting launcher in inventory.
    return (false);
}

void monster::swap_slots(mon_inv_type a, mon_inv_type b)
{
    const int swap = inv[a];
    inv[a] = inv[b];
    inv[b] = swap;
}

void monster::equip_weapon(item_def &item, int near, bool msg)
{
    if (msg && !need_message(near))
        msg = false;

    if (msg)
    {
        snprintf(info, INFO_SIZE, " wields %s.",
                 item.name(DESC_NOCAP_A, false, false, true, false,
                           ISFLAG_CURSED).c_str());
        msg = simple_monster_message(this, info);
    }

    const int brand = get_weapon_brand(item);
    if (brand == SPWPN_PROTECTION)
        ac += 5;
    if (brand == SPWPN_EVASION)
        ev += 5;

    if (msg)
    {
        bool message_given = true;
        switch (brand)
        {
        case SPWPN_FLAMING:
            mpr("It bursts into flame!");
            break;
        case SPWPN_FREEZING:
            mpr("It glows with a cold blue light!");
            break;
        case SPWPN_HOLY_WRATH:
            mpr("It softly glows with a divine radiance!");
            break;
        case SPWPN_ELECTROCUTION:
            mpr("You hear the crackle of electricity.", MSGCH_SOUND);
            break;
        case SPWPN_VENOM:
            mpr("It begins to drip with poison!");
            break;
        case SPWPN_DRAINING:
            mpr("You sense an unholy aura.");
            break;
        case SPWPN_FLAME:
            mpr("It bursts into flame!");
            break;
        case SPWPN_FROST:
            mpr("It is covered in frost.");
            break;
        case SPWPN_RETURNING:
            mpr("It wiggles slightly.");
            break;
        case SPWPN_DISTORTION:
            mpr("Its appearance distorts for a moment.");
            break;
        case SPWPN_CHAOS:
            mpr("It is briefly surrounded by a scintillating aura of "
                "random colours.");
            break;
        case SPWPN_PENETRATION:
            mprf("%s %s briefly pass through it before %s manages to get a "
                 "firm grip on it.",
                 pronoun(PRONOUN_CAP_POSSESSIVE).c_str(),
                 hand_name(true).c_str(),
                 pronoun(PRONOUN_NOCAP).c_str());
            break;
        case SPWPN_REAPING:
            mpr("It is briefly surrounded by shifting shadows.");
            break;

        default:
            // A ranged weapon without special message is known to be unbranded.
            if (brand != SPWPN_NORMAL || !is_range_weapon(item))
                message_given = false;
        }

        if (message_given)
        {
            if (is_artefact(item) && !is_special_unrandom_artefact(item))
                artefact_wpn_learn_prop(item, ARTP_BRAND);
            else
                set_ident_flags(item, ISFLAG_KNOW_TYPE);
        }
    }
}

void monster::equip_armour(item_def &item, int near)
{
    if (need_message(near))
    {
        snprintf(info, INFO_SIZE, " wears %s.",
                 item.name(DESC_NOCAP_A).c_str());
        simple_monster_message(this, info);
    }

    const equipment_type eq = get_armour_slot(item);
    if (eq != EQ_SHIELD)
    {
        ac += property(item, PARM_AC);

        const int armour_plus = item.plus;
        ASSERT(abs(armour_plus) < 20);
        if (abs(armour_plus) < 20)
            ac += armour_plus;
    }

    // Shields can affect evasion.
    ev += property(item, PARM_EVASION) / 2;
    if (ev < 1)
        ev = 1;   // This *shouldn't* happen.
}

void monster::equip(item_def &item, int slot, int near)
{
    switch (item.base_type)
    {
    case OBJ_WEAPONS:
    case OBJ_STAVES:
    {
        bool give_msg = (slot == MSLOT_WEAPON || mons_wields_two_weapons(this));
        equip_weapon(item, near, give_msg);
        break;
    }
    case OBJ_ARMOUR:
        equip_armour(item, near);
        break;
    default:
        break;
    }
}

void monster::unequip_weapon(item_def &item, int near, bool msg)
{
    if (msg && !need_message(near))
        msg = false;

    if (msg)
    {
        snprintf(info, INFO_SIZE, " unwields %s.",
                             item.name(DESC_NOCAP_A, false, false, true, false,
                             ISFLAG_CURSED).c_str());
        msg = simple_monster_message(this, info);
    }

    const int brand = get_weapon_brand(item);
    if (brand == SPWPN_PROTECTION)
        ac -= 5;
    if (brand == SPWPN_EVASION)
        ev -= 5;

    if (msg && brand != SPWPN_NORMAL)
    {
        bool message_given = true;
        switch (brand)
        {
        case SPWPN_FLAMING:
            mpr("It stops flaming.");
            break;

        case SPWPN_HOLY_WRATH:
            mpr("It stops glowing.");
            break;

        case SPWPN_ELECTROCUTION:
            mpr("It stops crackling.");
            break;

        case SPWPN_VENOM:
            mpr("It stops dripping with poison.");
            break;

        case SPWPN_DISTORTION:
            mpr("Its appearance distorts for a moment.");
            break;

        default:
            message_given = false;
        }
        if (message_given)
        {
            if (is_artefact(item) && !is_special_unrandom_artefact(item))
                artefact_wpn_learn_prop(item, ARTP_BRAND);
            else
                set_ident_flags(item, ISFLAG_KNOW_TYPE);
        }
    }
}

void monster::unequip_armour(item_def &item, int near)
{
    if (need_message(near))
    {
        snprintf(info, INFO_SIZE, " takes off %s.",
                 item.name(DESC_NOCAP_A).c_str());
        simple_monster_message(this, info);
    }

    const equipment_type eq = get_armour_slot(item);
    if (eq != EQ_SHIELD)
    {
        ac -= property(item, PARM_AC);

        const int armour_plus = item.plus;
        ASSERT(abs(armour_plus) < 20);
        if (abs(armour_plus) < 20)
            ac -= armour_plus;
    }

    ev -= property(item, PARM_EVASION) / 2;
    if (ev < 1)
        ev = 1;   // This *shouldn't* happen.
}

bool monster::unequip(item_def &item, int slot, int near, bool force)
{
    if (!force && item.cursed())
        return (false);

    if (!force && you.can_see(this))
        set_ident_flags(item, ISFLAG_KNOW_CURSE);

    switch (item.base_type)
    {
    case OBJ_WEAPONS:
    {
        bool give_msg = (slot == MSLOT_WEAPON || mons_wields_two_weapons(this));
        unequip_weapon(item, near, give_msg);
        break;
    }
    case OBJ_ARMOUR:
        unequip_armour(item, near);
        break;

    default:
        break;
    }

    return (true);
}

void monster::lose_pickup_energy()
{
    if (const monsterentry* entry = find_monsterentry())
    {
        const int delta = speed * entry->energy_usage.pickup_percent / 100;
        if (speed_increment > 25 && delta < speed_increment)
            speed_increment -= delta;
    }
}

void monster::pickup_message(const item_def &item, int near)
{
    if (need_message(near))
    {
        mprf("%s picks up %s.",
             name(DESC_CAP_THE).c_str(),
             item.base_type == OBJ_GOLD ? "some gold"
                                        : item.name(DESC_NOCAP_A).c_str());
    }
}

bool monster::pickup(item_def &item, int slot, int near, bool force_merge)
{
    ASSERT(item.defined());

    const monster* other_mon = item.holding_monster();

    if (other_mon != NULL)
    {
        if (other_mon == this)
        {
            if (inv[slot] == item.index())
            {
                mprf(MSGCH_DIAGNOSTICS, "Monster %s already holding item %s.",
                     name(DESC_PLAIN, true).c_str(),
                     item.name(DESC_PLAIN, false, true).c_str());
                return (false);
            }
            else
            {
                mprf(MSGCH_DIAGNOSTICS, "Item %s thinks it's already held by "
                                        "monster %s.",
                     item.name(DESC_PLAIN, false, true).c_str(),
                     name(DESC_PLAIN, true).c_str());
            }
        }
        else if (other_mon->type == MONS_NO_MONSTER)
        {
            mprf(MSGCH_DIAGNOSTICS, "Item %s, held by dead monster, being "
                                    "picked up by monster %s.",
                 item.name(DESC_PLAIN, false, true).c_str(),
                 name(DESC_PLAIN, true).c_str());
        }
        else
        {
            mprf(MSGCH_DIAGNOSTICS, "Item %s, held by monster %s, being "
                                    "picked up by monster %s.",
                 item.name(DESC_PLAIN, false, true).c_str(),
                 other_mon->name(DESC_PLAIN, true).c_str(),
                 name(DESC_PLAIN, true).c_str());
        }
    }

    // If a monster chooses a two-handed weapon as main weapon, it will
    // first have to drop any shield it might wear.
    // (Monsters will always favour damage over protection.)
    if ((slot == MSLOT_WEAPON || slot == MSLOT_ALT_WEAPON)
        && inv[MSLOT_SHIELD] != NON_ITEM
        && hands_reqd(item, body_size()) == HANDS_TWO)
    {
        if (!drop_item(MSLOT_SHIELD, near))
            return (false);
    }

    // Similarly, monsters won't pick up shields if they're
    // wielding (or alt-wielding) a two-handed weapon.
    if (slot == MSLOT_SHIELD)
    {
        const item_def* wpn = mslot_item(MSLOT_WEAPON);
        const item_def* alt = mslot_item(MSLOT_ALT_WEAPON);
        if (wpn && hands_reqd(*wpn, body_size()) == HANDS_TWO)
            return (false);
        if (alt && hands_reqd(*alt, body_size()) == HANDS_TWO)
            return (false);
    }

    if (inv[slot] != NON_ITEM)
    {
        item_def &dest(mitm[inv[slot]]);
        if (items_stack(item, dest, force_merge))
        {
            dungeon_events.fire_position_event(
                dgn_event(DET_ITEM_PICKUP, pos(), 0, item.index(),
                          mindex()),
                pos());

            pickup_message(item, near);
            inc_mitm_item_quantity(inv[slot], item.quantity);
            merge_item_stacks(item, dest);
            destroy_item(item.index());
            equip(item, slot, near);
            lose_pickup_energy();
            return (true);
        }
        return (false);
    }

    dungeon_events.fire_position_event(
        dgn_event(DET_ITEM_PICKUP, pos(), 0, item.index(),
                  mindex()),
        pos());

    const int item_index = item.index();
    unlink_item(item_index);

    inv[slot] = item_index;

    item.set_holding_monster(mindex());

    pickup_message(item, near);
    equip(item, slot, near);
    lose_pickup_energy();
    return (true);
}

bool monster::drop_item(int eslot, int near)
{
    if (eslot < 0 || eslot >= NUM_MONSTER_SLOTS)
        return (false);

    int item_index = inv[eslot];
    if (item_index == NON_ITEM)
        return (true);

    item_def* pitem = &mitm[item_index];

    // Unequip equipped items before dropping them; unequip() prevents
    // cursed items from being removed.
    bool was_unequipped = false;
    if (eslot == MSLOT_WEAPON || eslot == MSLOT_ARMOUR
        || eslot == MSLOT_ALT_WEAPON && mons_wields_two_weapons(this))
    {
        if (!unequip(*pitem, eslot, near))
            return (false);
        was_unequipped = true;
    }

    if (pitem->flags & ISFLAG_SUMMONED)
    {
        if (need_message(near))
        {
            mprf("%s %s as %s drops %s!",
                 pitem->name(DESC_CAP_THE).c_str(),
                 summoned_poof_msg(this, *pitem).c_str(),
                 name(DESC_NOCAP_THE).c_str(),
                 pitem->quantity > 1 ? "them" : "it");
        }

        item_was_destroyed(*pitem, mindex());
        destroy_item(item_index);
    }
    else
    {
        if (need_message(near))
        {
            mprf("%s drops %s.", name(DESC_CAP_THE).c_str(),
                 pitem->name(DESC_NOCAP_A).c_str());
        }

        if (!move_item_to_grid(&item_index, pos(), swimming()))
        {
            // Re-equip item if we somehow failed to drop it.
            if (was_unequipped)
                equip(*pitem, eslot, near);

            return (false);
        }

        if (friendly() && item_index != NON_ITEM)
        {
            // move_item_to_grid could change item_index, so
            // update pitem.
            pitem = &mitm[item_index];

            pitem->flags |= ISFLAG_DROPPED_BY_ALLY;
        }
    }

    if (props.exists("wand_known") && near && pitem->base_type == OBJ_WANDS)
        props.erase("wand_known");

    inv[eslot] = NON_ITEM;
    return (true);
}

// We don't want monsters to pick up ammunition that is identical to the
// launcher brand, in hope of another monster wandering by who may want to
// use the ammo in question.
static bool _nonredundant_launcher_ammo_brands(item_def *launcher,
                                             const item_def *ammo)
{
    // If the monster has no ammo then there's no redundancy problems
    // to check.
    if (ammo == NULL)
        return (true);

    const int bow_brand  = get_weapon_brand(*launcher);
    const int ammo_brand = get_ammo_brand(*ammo);

    switch (ammo_brand)
    {
    case SPMSL_FLAME:
        return (bow_brand != SPWPN_FLAME);
    case SPMSL_FROST:
        return (bow_brand != SPWPN_FROST);
    case SPMSL_CHAOS:
        return (bow_brand != SPWPN_CHAOS);
    case SPMSL_REAPING:
        return (bow_brand != SPWPN_REAPING);
    case SPMSL_PENETRATION:
        return (bow_brand != SPWPN_PENETRATION);
    default:
        return (true);
    }
}

bool monster::pickup_launcher(item_def &launch, int near, bool force)
{
    // Don't allow monsters to pick up launchers that would also
    // refuse to pick up the matching ammo.
    if (!force && !_needs_ranged_attack(this))
        return (false);

    // Don't allow monsters to switch to another type of launcher
    // as that would require them to also drop their ammunition
    // and then try to find ammunition for their new launcher.
    // However, they may switch to another launcher if they're
    // out of ammo. (jpeg)
    const int mdam_rating = mons_weapon_damage_rating(launch);
    const missile_type mt = fires_ammo_type(launch);
    int eslot = -1;
    for (int i = MSLOT_WEAPON; i <= MSLOT_ALT_WEAPON; ++i)
    {
        if (const item_def *elaunch = mslot_item(static_cast<mon_inv_type>(i)))
        {
            if (!is_range_weapon(*elaunch))
                continue;

            return ((fires_ammo_type(*elaunch) == mt || !missiles())
                    && (mons_weapon_damage_rating(*elaunch) < mdam_rating
                        || mons_weapon_damage_rating(*elaunch) == mdam_rating
                           && get_weapon_brand(*elaunch) == SPWPN_NORMAL
                           && get_weapon_brand(launch) != SPWPN_NORMAL
                           && _nonredundant_launcher_ammo_brands(&launch,
                                                                 missiles()))
                    && drop_item(i, near) && pickup(launch, i, near));
        }
        else
            eslot = i;
    }

    return (eslot == -1 ? false : pickup(launch, eslot, near));
}

static bool _is_signature_weapon(monster* mons, const item_def &weapon)
{
    if (weapon.base_type != OBJ_WEAPONS)
        return (false);

    if (mons->type == MONS_ANGEL || mons->type == MONS_CHERUB)
        return (weapon.sub_type == WPN_SACRED_SCOURGE);

    if (mons->type == MONS_DAEVA)
        return (weapon.sub_type == WPN_EUDEMON_BLADE);

    // Some other uniques have a signature weapon, usually because they
    // always spawn with it, or because it is referenced in their speech
    // and/or descriptions.
    // Upgrading to a similar type is pretty much always allowed, unless
    // we are more interested in the brand, and the brand is *rare*.
    if (mons_is_unique(mons->type))
    {
        // We might allow Sigmund to pick up a better scythe if he finds
        // one...
        if (mons->type == MONS_SIGMUND)
            return (weapon.sub_type == WPN_SCYTHE);

        // Crazy Yiuf's got MONUSE_STARTING_EQUIPMENT right now, but
        // in case that ever changes we don't want him to switch away
        // from his quarterstaff of chaos.
        if (mons->type == MONS_CRAZY_YIUF)
            return (false);

        // Distortion/chaos is immensely flavourful, and we shouldn't
        // allow Psyche to switch away from it.
        if (mons->type == MONS_PSYCHE)
            return (false);

        // Don't switch Azrael away from the customary scimitar of
        // flaming.
        if (mons->type == MONS_AZRAEL)
            return (false);

        if (mons->type == MONS_AGNES)
            return (weapon.sub_type == WPN_LAJATANG);

        if (mons->type == MONS_EDMUND)
        {
            return (weapon.sub_type == WPN_FLAIL
                    || weapon.sub_type == WPN_SPIKED_FLAIL
                    || weapon.sub_type == WPN_DIRE_FLAIL);
        }

        // Pikel's got MONUSE_STARTING_EQUIPMENT right now, but,
        // in case that ever changes, we don't want him to switch away
        // from a whip.
        if (mons->type == MONS_PIKEL)
            return (get_vorpal_type(weapon) == DVORP_SLASHING);

        if (mons->type == MONS_WIGLAF)
            return (weapon_skill(weapon) == SK_AXES);

        if (mons->type == MONS_NIKOLA)
            return (get_weapon_brand(weapon) == SPWPN_ELECTROCUTION);

        // Technically, this includes knives, but it would have to be
        // a superpowered knife to be an upgrade to a short sword.
        if (mons->type == MONS_DUVESSA)
        {
            return (weapon_skill(weapon) == SK_SHORT_BLADES
                    || weapon_skill(weapon) == SK_LONG_BLADES);
        }
    }

    if (is_unrandom_artefact(weapon))
    {
        switch (weapon.special)
        {
        case UNRAND_ASMODEUS:
            return (mons->type == MONS_ASMODEUS);

        case UNRAND_DISPATER:
            return (mons->type == MONS_DISPATER);

        case UNRAND_CEREBOV:
            return (mons->type == MONS_CEREBOV);

        case UNRAND_MORG:
            return (mons->type == MONS_BORIS);
        }
    }

    return (false);
}

static int _ego_damage_bonus(item_def &item)
{
    switch (get_weapon_brand(item))
    {
    case SPWPN_NORMAL:      return 0;
    case SPWPN_VORPAL:      // deliberate
    case SPWPN_PROTECTION:  // fall through
    case SPWPN_EVASION:     return 1;
    default:                return 2;
    }
}

static bool _item_race_matches_monster(const item_def &item, monster* mons)
{
    if (get_equip_race(item) == ISFLAG_ELVEN)
        return (mons_genus(mons->type) == MONS_ELF);

    if (get_equip_race(item) == ISFLAG_DWARVEN)
        return (mons_genus(mons->type) == MONS_DWARF);

    if (get_equip_race(item) == ISFLAG_ORCISH)
        return (mons_genus(mons->type) == MONS_ORC);

    return (false);
}

bool monster::pickup_melee_weapon(item_def &item, int near)
{
    // Throwable weapons may be picked up as though dual-wielding.
    const bool dual_wielding = (mons_wields_two_weapons(this)
                                || is_throwable(this, item));
    if (dual_wielding && item.quantity == 1)
    {
        // If we have either weapon slot free, pick up the weapon.
        if (inv[MSLOT_WEAPON] == NON_ITEM)
            return pickup(item, MSLOT_WEAPON, near);

        if (inv[MSLOT_ALT_WEAPON] == NON_ITEM)
            return pickup(item, MSLOT_ALT_WEAPON, near);
    }

    const int new_wpn_dam = mons_weapon_damage_rating(item)
                            + _ego_damage_bonus(item);
    int eslot = -1;
    item_def *weap;

    // Monsters have two weapon slots, one of which can be a ranged, and
    // the other a melee weapon. (The exception being dual-wielders who can
    // wield two melee weapons). The weapon in MSLOT_WEAPON is the one
    // currently wielded (can be empty).

    for (int i = MSLOT_WEAPON; i <= MSLOT_ALT_WEAPON; ++i)
    {
        weap = mslot_item(static_cast<mon_inv_type>(i));

        // If the weapon is a stack of throwing weaons, the monster
        // will not use the stack as their primary melee weapon.
        if (item.quantity != 1 && i == MSLOT_WEAPON)
            continue;

        if (!weap)
        {
            // If no weapon in this slot, mark this one.
            if (eslot == -1)
                eslot = i;
        }
        else
        {
            if (is_range_weapon(*weap))
                continue;

            // Don't drop weapons specific to the monster.
            if (_is_signature_weapon(this, *weap) && !dual_wielding)
                return (false);

            // If we get here, the weapon is a melee weapon.
            // If the new weapon is better than the current one and not cursed,
            // replace it. Otherwise, give up.
            const int old_wpn_dam = mons_weapon_damage_rating(*weap)
                                    + _ego_damage_bonus(*weap);

            bool new_wpn_better = (new_wpn_dam > old_wpn_dam);
            if (new_wpn_dam == old_wpn_dam)
            {
                // Use shopping value as a crude estimate of resistances etc.
                // XXX: This is not really logical as many properties don't
                //      apply to monsters (e.g. levitation, blink, berserk).
                // For simplicity, don't apply this check to secondary weapons
                // for dual wielding monsters.
                int oldval = item_value(*weap, true);
                int newval = item_value(item, true);

                // Vastly prefer matching racial type.
                if (_item_race_matches_monster(*weap, this))
                    oldval *= 2;
                if (_item_race_matches_monster(item, this))
                    newval *= 2;

                if (newval > oldval)
                    new_wpn_better = true;
            }

            if (new_wpn_better && !weap->cursed())
            {
                if (!dual_wielding
                    || i == MSLOT_WEAPON
                    || old_wpn_dam
                       < mons_weapon_damage_rating(*mslot_item(MSLOT_WEAPON))
                         + _ego_damage_bonus(*mslot_item(MSLOT_WEAPON)))
                {
                    eslot = i;
                    if (!dual_wielding)
                        break;
                }
            }
            else if (!dual_wielding)
            {
                // We've got a good melee weapon, that's enough.
               return (false);
            }
        }
    }

    // No slot found to place this item.
    if (eslot == -1)
        return (false);

    // Current item cannot be dropped.
    if (inv[eslot] != NON_ITEM && !drop_item(eslot, near))
        return (false);

    return (pickup(item, eslot, near));
}

// Arbitrary damage adjustment for quantity of missiles. So sue me.
static int _q_adj_damage(int damage, int qty)
{
    return (damage * std::min(qty, 8));
}

bool monster::pickup_throwable_weapon(item_def &item, int near)
{
    const mon_inv_type slot = item_to_mslot(item);

    // If it's a melee weapon then pickup_melee_weapon() already rejected
    // it, even though it can also be thrown.
    if (slot == MSLOT_WEAPON)
        return (false);

    ASSERT(slot == MSLOT_MISSILE);

    // Spellcasters shouldn't bother with missiles.
    if (mons_has_ranged_spell(this, true, false))
        return (false);

    // If occupied, don't pick up a throwable weapons if it would just
    // stack with an existing one. (Upgrading is possible.)
    if (mslot_item(slot)
        && (mons_is_wandering(this) || friendly() && foe == MHITYOU)
        && pickup(item, slot, near, true))
    {
        return (true);
    }

    item_def *launch = NULL;
    const int exist_missile = mons_pick_best_missile(this, &launch, true);
    if (exist_missile == NON_ITEM
        || (_q_adj_damage(mons_missile_damage(this, launch,
                                              &mitm[exist_missile]),
                          mitm[exist_missile].quantity)
            < _q_adj_damage(mons_thrown_weapon_damage(&item), item.quantity)))
    {
        if (inv[slot] != NON_ITEM && !drop_item(slot, near))
            return (false);
        return pickup(item, slot, near);
    }
    return (false);
}

bool monster::wants_weapon(const item_def &weap) const
{
    if (!could_wield(weap))
       return (false);

    // Blademasters and master archers like their starting weapon and
    // don't want another, thank you.
    if (type == MONS_DEEP_ELF_BLADEMASTER
        || type == MONS_DEEP_ELF_MASTER_ARCHER)
    {
        return (false);
    }

    // Monsters capable of dual-wielding will always prefer two weapons
    // to a single two-handed one, however strong.
    if (mons_wields_two_weapons(this)
        && hands_reqd(weap, body_size()) == HANDS_TWO)
    {
        return (false);
    }

    // deep dwarf artificers
    if (weap.base_type == OBJ_STAVES
        && type == MONS_DEEP_DWARF_ARTIFICER)
    {
        return (true);
    }

    // Nobody picks up giant clubs. Starting equipment is okay, of course.
    if (weap.sub_type == WPN_GIANT_CLUB
        || weap.sub_type == WPN_GIANT_SPIKED_CLUB)
    {
        return (false);
    }

    return (true);
}

bool monster::wants_armour(const item_def &item) const
{
    // Monsters that are capable of dual wielding won't pick up shields.
    // Neither will monsters that are already wielding a two-hander.
    if (is_shield(item)
        && (mons_wields_two_weapons(this)
            || mslot_item(MSLOT_WEAPON)
               && hands_reqd(*mslot_item(MSLOT_WEAPON), body_size())
                      == HANDS_TWO))
    {
        return (false);
    }

    // Returns whether this armour is the monster's size.
    return (check_armour_size(item, body_size()));
}

// Monsters magically know the real properties of all items.
static int _get_monster_armour_value(const monster *mon,
                                     const item_def &item)
{
    // Each resistance/property counts as much as 1 point of AC.
    // Steam has been excluded because of its general uselessness.
    // Well, the same's true for sticky flame but... (jpeg)
    int value  = item.armour_rating();
        value += get_armour_res_fire(item, true);
        value += get_armour_res_cold(item, true);
        value += get_armour_res_elec(item, true);
        value += get_armour_res_sticky_flame(item);

    // Give a simple bonus, no matter the size of the MR bonus.
    if (get_armour_res_magic(item, true) > 0)
        value++;

    // Poison becomes much less valuable if the monster is
    // intrinsically resistant.
    if (get_mons_resists(mon).poison <= 0)
        value += get_armour_res_poison(item, true);

    // Same for life protection.
    if (mon->holiness() != MH_NATURAL)
        value += get_armour_life_protection(item, true);

    // See invisible also is only useful if not already intrinsic.
    if (!mons_class_flag(mon->type, M_SEE_INVIS))
        value += get_armour_see_invisible(item, true);

    // Give a sizable bonus for shields of reflection.
    if (get_armour_ego_type(item) == SPARM_REFLECTION)
        value += 3;

    return value;
}

bool monster::pickup_armour(item_def &item, int near, bool force)
{
    ASSERT(item.base_type == OBJ_ARMOUR);

    if (!force && !wants_armour(item))
        return (false);

    equipment_type eq = EQ_NONE;

    // HACK to allow nagas/centaurs to wear bardings. (jpeg)
    switch (item.sub_type)
    {
    case ARM_NAGA_BARDING:
        if (::mons_genus(type) == MONS_NAGA)
            eq = EQ_BODY_ARMOUR;
        break;
    case ARM_CENTAUR_BARDING:
        if (::mons_species(type) == MONS_CENTAUR
            || ::mons_species(type) == MONS_YAKTAUR)
        {
            eq = EQ_BODY_ARMOUR;
        }
        break;
    // And another hack or two...
    case ARM_WIZARD_HAT:
        if (type == MONS_GASTRONOK)
            eq = EQ_BODY_ARMOUR;
        break;
    case ARM_CLOAK:
        if (type == MONS_MAURICE
            || type == MONS_NIKOLA
            || type == MONS_CRAZY_YIUF)
        {
            eq = EQ_BODY_ARMOUR;
        }
        break;
    case ARM_GLOVES:
        if (type == MONS_NIKOLA)
            eq = EQ_SHIELD;
        break;
    default:
        eq = get_armour_slot(item);
    }

    // Bardings are only wearable by the appropriate monster.
    if (eq == EQ_NONE)
        return (false);

    // XXX: Monsters can only equip body armour and shields (as of 0.4).
    if (!force && eq != EQ_BODY_ARMOUR && eq != EQ_SHIELD)
        return (false);

    const mon_inv_type mslot = equip_slot_to_mslot(eq);
    if (mslot == NUM_MONSTER_SLOTS)
        return (false);

    int value_new = _get_monster_armour_value(this, item);

    // No armour yet -> get this one.
    if (!mslot_item(mslot) && value_new > 0)
        return pickup(item, mslot, near);

    // Simplistic armour evaluation (comparing AC and resistances).
    if (const item_def *existing_armour = slot_item(eq, false))
    {
        if (!force)
        {
            int value_old = _get_monster_armour_value(this,
                                                      *existing_armour);
            if (value_old > value_new)
                return (false);

            if (value_old == value_new)
            {
                // Prefer matching racial type.
                if (_item_race_matches_monster(*existing_armour, this))
                    value_old++;
                if (_item_race_matches_monster(item, this))
                    value_new++;

                if (value_old == value_new)
                {
                    // If items are of the same value, use shopping
                    // value as a further crude estimate.
                    value_old = item_value(*existing_armour, true);
                    value_new = item_value(item, true);
                }
                if (value_old >= value_new)
                    return (false);
            }
        }

        if (!drop_item(mslot, near))
            return (false);
    }

    return (pickup(item, mslot, near));
}

bool monster::pickup_weapon(item_def &item, int near, bool force)
{
    if (!force && !wants_weapon(item))
        return (false);

    // Weapon pickup involves:
    // - If we have no weapons, always pick this up.
    // - If this is a melee weapon and we already have a melee weapon, pick
    //   it up if it is superior to the one we're carrying (and drop the
    //   one we have).
    // - If it is a ranged weapon, and we already have a ranged weapon,
    //   pick it up if it is better than the one we have.
    // - If it is a throwable weapon, and we're carrying no missiles (or our
    //   missiles are the same type), pick it up.

    if (is_range_weapon(item))
        return (pickup_launcher(item, near, force));

    if (pickup_melee_weapon(item, near))
        return (true);

    return (can_use_missile(item) && pickup_throwable_weapon(item, near));
}

bool monster::pickup_missile(item_def &item, int near, bool force)
{
    const item_def *miss = missiles();

    if (!force)
    {
        if (item.sub_type == MI_THROWING_NET)
        {
            // Monster may not pick up trapping net.
            if (caught() && item_is_stationary(item))
                return (false);
        }
        else // None of these exceptions hold for throwing nets.
        {
            // Spellcasters should not waste time with ammunition.
            // Neither summons nor hostile enchantments are counted for
            // this purpose.
            if (!force && mons_has_ranged_spell(this, true, false))
                return (false);

            // Monsters in a fight will only pick up missiles if doing so
            // is worthwhile.
            if (!mons_is_wandering(this)
                && (!friendly() || foe != MHITYOU)
                && (item.quantity < 5 || miss && miss->quantity >= 7))
            {
                return (false);
            }
        }
    }

    if (miss && items_stack(*miss, item))
        return (pickup(item, MSLOT_MISSILE, near));

    if (!force && !can_use_missile(item))
        return (false);

    if (miss)
    {
        item_def *launch;
        for (int i = MSLOT_WEAPON; i <= MSLOT_ALT_WEAPON; ++i)
        {
            launch = mslot_item(static_cast<mon_inv_type>(i));
            if (launch)
            {
                const int item_brand = get_ammo_brand(item);
                // If this ammunition is better, drop the old ones.
                // Don't upgrade to ammunition whose brand cancels the
                // launcher brand or doesn't improve it further.
                if (fires_ammo_type(*launch) == item.sub_type
                    && (fires_ammo_type(*launch) != miss->sub_type
                        || item.plus > miss->plus
                           && get_ammo_brand(*miss) == item_brand
                        || item.plus >= miss->plus
                           && get_ammo_brand(*miss) == SPMSL_NORMAL
                           && item_brand != SPMSL_NORMAL
                           && _nonredundant_launcher_ammo_brands(launch, miss)))
                {
                    if (!drop_item(MSLOT_MISSILE, near))
                        return (false);
                    break;
                }
            }
        }

        // Darts don't absolutely need a launcher - still allow upgrading.
        if (item.sub_type == miss->sub_type
            && item.sub_type == MI_DART
            && (item.plus > miss->plus
                || item.plus == miss->plus
                   && get_ammo_brand(*miss) == SPMSL_NORMAL
                   && get_ammo_brand(item) != SPMSL_NORMAL))
        {
            if (!drop_item(MSLOT_MISSILE, near))
                return (false);
        }
    }

    return pickup(item, MSLOT_MISSILE, near);
}

bool monster::pickup_wand(item_def &item, int near)
{
    // Don't pick up empty wands.
    if (item.plus == 0)
        return (false);

    // Only low-HD monsters bother with wands.
    if (hit_dice >= 14)
        return (false);

    // Holy monsters and worshippers of good gods won't pick up evil
    // wands.
    if ((is_holy() || is_good_god(god)) && is_evil_item(item))
        return (false);

    // If a monster already has a charged wand, don't bother.
    // Otherwise, replace with a charged one.
    if (item_def *wand = mslot_item(MSLOT_WAND))
    {
        if (wand->plus > 0)
            return (false);

        if (!drop_item(MSLOT_WAND, near))
            return (false);
    }

    if (pickup(item, MSLOT_WAND, near))
    {
        if (near)
            props["wand_known"] = item_type_known(item);
        return true;
    }
    else
        return false;
}

bool monster::pickup_scroll(item_def &item, int near)
{
    if (item.sub_type != SCR_TELEPORTATION
        && item.sub_type != SCR_BLINKING
        && item.sub_type != SCR_SUMMONING)
    {
        return (false);
    }

    // Holy monsters and worshippers of good gods won't pick up evil
    // scrolls.
    if ((is_holy() || is_good_god(god)) && is_evil_item(item))
        return (false);

    return (pickup(item, MSLOT_SCROLL, near));
}

bool monster::pickup_potion(item_def &item, int near)
{
    // Only allow monsters to pick up potions if they can actually use
    // them.
    const potion_type ptype = static_cast<potion_type>(item.sub_type);

    if (!can_drink_potion(ptype))
        return (false);

    return (pickup(item, MSLOT_POTION, near));
}

bool monster::pickup_gold(item_def &item, int near)
{
    return (pickup(item, MSLOT_GOLD, near));
}

bool monster::pickup_misc(item_def &item, int near)
{
    // Never pick up runes.
    if (item.base_type == OBJ_MISCELLANY && item.sub_type == MISC_RUNE_OF_ZOT)
        return (false);

    // Holy monsters and worshippers of good gods won't pick up evil
    // miscellaneous items.
    if ((is_holy() || is_good_god(god)) && is_evil_item(item))
        return (false);

    return (pickup(item, MSLOT_MISCELLANY, near));
}

// Eaten items are handled elsewhere, in _handle_pickup() in mon-stuff.cc.
bool monster::pickup_item(item_def &item, int near, bool force)
{
    // Equipping stuff can be forced when initially equipping monsters.
    if (!force)
    {
        // If a monster isn't otherwise occupied (has a foe, is fleeing, etc.)
        // it is considered wandering.
        bool wandering = (mons_is_wandering(this)
                          || friendly() && foe == MHITYOU);
        const int itype = item.base_type;

        // Weak(ened) monsters won't stop to pick up things as long as they
        // feel unsafe.
        if (!wandering && (hit_points * 10 < max_hit_points || hit_points < 10)
            && mon_enemies_around(this))
        {
            return (false);
        }

        if (friendly())
        {
            // Allies are only interested in armour and weaponry.
            // Everything else is likely to only annoy the player
            // because the monster either won't use the object or
            // might use it in ways not helpful to the player.
            if (itype != OBJ_ARMOUR && itype != OBJ_WEAPONS
                && itype != OBJ_MISSILES)
            {
                return (false);
            }

            // Depending on the friendly pickup toggle, your allies may not
            // pick up anything, or only stuff dropped by (other) allies.
            if (you.friendly_pickup == FRIENDLY_PICKUP_NONE
                || you.friendly_pickup == FRIENDLY_PICKUP_FRIEND
                   && !testbits(item.flags, ISFLAG_DROPPED_BY_ALLY)
                || you.friendly_pickup == FRIENDLY_PICKUP_PLAYER
                   && !(item.flags & (ISFLAG_DROPPED | ISFLAG_THROWN
                                        | ISFLAG_DROPPED_BY_ALLY)))
            {
                return (false);
            }
        }

        if (!wandering)
        {
            // These are not important enough for pickup when
            // seeking, fleeing etc.
            if (itype == OBJ_ARMOUR || itype == OBJ_CORPSES
                || itype == OBJ_MISCELLANY || itype == OBJ_GOLD)
            {
                return (false);
            }

            if (itype == OBJ_WEAPONS || itype == OBJ_MISSILES)
            {
                // Fleeing monsters only pick up emergency equipment.
                if (mons_is_fleeing(this))
                    return (false);

                // While occupied, hostile monsters won't pick up items
                // dropped or thrown by you. (You might have done that to
                // distract them.)
                if (!friendly()
                    && (testbits(item.flags, ISFLAG_DROPPED)
                        || testbits(item.flags, ISFLAG_THROWN)))
                {
                    return (false);
                }
            }
        }
    }

    switch (item.base_type)
    {
    // Pickup some stuff only if WANDERING.
    case OBJ_ARMOUR:
        return pickup_armour(item, near, force);
    case OBJ_MISCELLANY:
        return pickup_misc(item, near);
    case OBJ_GOLD:
        return pickup_gold(item, near);
    // Fleeing monsters won't pick up these.
    // Hostiles won't pick them up if they were ever dropped/thrown by you.
    case OBJ_STAVES:
    case OBJ_WEAPONS:
        return pickup_weapon(item, near, force);
    case OBJ_MISSILES:
        return pickup_missile(item, near, force);
    // Other types can always be picked up
    // (barring other checks depending on subtype, of course).
    case OBJ_WANDS:
        return pickup_wand(item, near);
    case OBJ_SCROLLS:
        return pickup_scroll(item, near);
    case OBJ_POTIONS:
        return pickup_potion(item, near);
    case OBJ_BOOKS:
        if (force)
            return pickup_misc(item, near);
        // else fall through
    default:
        return (false);
    }
}

bool monster::need_message(int &near) const
{
    return (near != -1 ? near
                       : (near = observable()));
}

void monster::swap_weapons(int near)
{
    item_def *weap = mslot_item(MSLOT_WEAPON);
    item_def *alt  = mslot_item(MSLOT_ALT_WEAPON);

    if (weap && !unequip(*weap, MSLOT_WEAPON, near))
    {
        // Item was cursed.
        return;
    }

    swap_slots(MSLOT_WEAPON, MSLOT_ALT_WEAPON);

    if (alt)
        equip(*alt, MSLOT_WEAPON, near);

    // Monsters can swap weapons really fast. :-)
    if ((weap || alt) && speed_increment >= 2)
    {
        if (const monsterentry *entry = find_monsterentry())
            speed_increment -= div_rand_round(entry->energy_usage.attack, 5);
    }
}

void monster::wield_melee_weapon(int near)
{
    const item_def *weap = mslot_item(MSLOT_WEAPON);
    if (!weap || (!weap->cursed() && is_range_weapon(*weap)))
    {
        const item_def *alt = mslot_item(MSLOT_ALT_WEAPON);

        // Switch to the alternate weapon if it's not a ranged weapon, too,
        // or switch away from our main weapon if it's a ranged weapon.
        //
        // Don't switch to alt weapon if it's a stack of throwing weapons.
        if (alt && !is_range_weapon(*alt) && alt->quantity == 1
            || weap && !alt && type != MONS_STATUE)
        {
            swap_weapons(near);
        }
    }
}

item_def *monster::slot_item(equipment_type eq, bool include_melded)
{
    return (mslot_item(equip_slot_to_mslot(eq)));
}

item_def *monster::mslot_item(mon_inv_type mslot) const
{
    const int mi = (mslot == NUM_MONSTER_SLOTS) ? NON_ITEM : inv[mslot];
    return (mi == NON_ITEM ? NULL : &mitm[mi]);
}

item_def *monster::shield()
{
    return (mslot_item(MSLOT_SHIELD));
}

bool monster::is_named() const
{
    return (!mname.empty() || mons_is_unique(type));
}

bool monster::has_base_name() const
{
    // Any non-ghost, non-Pandemonium demon that has an explicitly set
    // name has a base name.
    return (!mname.empty() && !ghost.get());
}

static std::string _invalid_monster_str(monster_type type)
{
    std::string str = "INVALID MONSTER ";

    switch (type)
    {
    case NUM_MONSTERS:
        return (str + "NUM_MONSTERS");
    case MONS_NO_MONSTER:
        return (str + "MONS_NO_MONSTER");
    case MONS_PLAYER:
        return (str + "MONS_PLAYER");
    case RANDOM_DRACONIAN:
        return (str + "RANDOM_DRACONIAN");
    case RANDOM_BASE_DRACONIAN:
        return (str + "RANDOM_BASE_DRACONIAN");
    case RANDOM_NONBASE_DRACONIAN:
        return (str + "RANDOM_NONBASE_DRACONIAN");
    case WANDERING_MONSTER:
        return (str + "WANDERING_MONSTER");
    default:
        break;
    }

    str += make_stringf("#%d", (int) type);

    if (type < 0)
        return (str);

    if (type > NUM_MONSTERS)
    {
        str += make_stringf(" (NUM_MONSTERS + %d)",
                            int (NUM_MONSTERS - type));
        return (str);
    }

    int          i;
    monster_type new_type;
    for (i = 0; true; i++)
    {
        new_type = (monster_type) (((int) type) - i);

        if (invalid_monster_type(new_type))
            continue;
        break;
    }
    str += make_stringf(" (%s + %d)",
                        mons_type_name(new_type, DESC_PLAIN).c_str(),
                        i);

    return (str);
}

static std::string _mon_special_name(const monster& mon, description_level_type desc, bool force_seen)
{
    if (desc == DESC_NONE)
        return "";

    monster_type type = mon.type;
    if (!crawl_state.game_is_arena() && you.misled())
        type = mon.get_mislead_type();

    const bool arena_submerged = crawl_state.game_is_arena() && !force_seen
                                     && mon.submerged();

    if (type == MONS_NO_MONSTER)
        return ("DEAD MONSTER");
    else if (invalid_monster_type(type) && type != MONS_PROGRAM_BUG)
        return _invalid_monster_str(type);

    // Handle non-visible case first.
    if (!force_seen && !mon.observable() && !arena_submerged)
    {
        switch (desc)
        {
        case DESC_CAP_THE: case DESC_CAP_A:
            return ("It");
        case DESC_NOCAP_THE: case DESC_NOCAP_A: case DESC_PLAIN:
            return ("it");
        default:
            return ("it (buggy)");
        }
    }

    if (desc == DESC_DBNAME)
    {
        monster_info mi(&mon, MILEV_NAME);
        return mi.db_name();
    }

    return "";
}

std::string monster::name(description_level_type desc, bool force_vis) const
{
    std::string s = _mon_special_name(*this, desc, force_vis);
    if (!s.empty() || desc == DESC_NONE)
        return s;

    monster_info mi(this, MILEV_NAME);
    return mi.proper_name(desc);
}

std::string monster::base_name(description_level_type desc, bool force_vis)
    const
{
    std::string s = _mon_special_name(*this, desc, force_vis);
    if (!s.empty() || desc == DESC_NONE)
        return s;

    monster_info mi(this, MILEV_NAME);
    return mi.common_name(desc);
}

std::string monster::full_name(description_level_type desc,
                                bool use_comma) const
{
    std::string s = _mon_special_name(*this, desc, true);
    if (!s.empty() || desc == DESC_NONE)
        return s;

    monster_info mi(this, MILEV_NAME);
    return mi.full_name(desc);
}

std::string monster::pronoun(pronoun_type pro, bool force_visible) const
{
    return (mons_pronoun(static_cast<monster_type>(type), pro,
                         force_visible || you.can_see(this)));
}

std::string monster::conj_verb(const std::string &verb) const
{
    if (!verb.empty() && verb[0] == '!')
        return (verb.substr(1));

    if (verb == "are")
        return ("is");

    if (verb == "snap closed at")
        return ("snaps closed at");

    if (ends_with(verb, "f") || ends_with(verb, "fe")
        || ends_with(verb, "y"))
    {
        return (verb + "s");
    }

    return (pluralise(verb));
}

std::string monster::hand_name(bool plural, bool *can_plural) const
{
    bool _can_plural;
    if (can_plural == NULL)
        can_plural = &_can_plural;
    *can_plural = true;

    std::string str;
    char        ch = mons_base_char(type);

    const bool rand = (type == MONS_CHAOS_SPAWN);

    switch (get_mon_shape(this))
    {
    case MON_SHAPE_CENTAUR:
    case MON_SHAPE_NAGA:
        // Defaults to "hand"
        break;
    case MON_SHAPE_HUMANOID:
    case MON_SHAPE_HUMANOID_WINGED:
    case MON_SHAPE_HUMANOID_TAILED:
    case MON_SHAPE_HUMANOID_WINGED_TAILED:
        if (ch == 'T' || ch == 'd' || ch == 'n' || mons_is_demon(type))
            str = "claw";
        break;

    case MON_SHAPE_QUADRUPED:
    case MON_SHAPE_QUADRUPED_TAILLESS:
    case MON_SHAPE_QUADRUPED_WINGED:
    case MON_SHAPE_ARACHNID:
        if (mons_genus(type) == MONS_SCORPION || rand && one_chance_in(4))
            str = "pincer";
        else
        {
            str = "front ";
            return (str + foot_name(plural, can_plural));
        }
        break;

    case MON_SHAPE_BLOB:
    case MON_SHAPE_SNAKE:
    case MON_SHAPE_FISH:
        return foot_name(plural, can_plural);

    case MON_SHAPE_BAT:
        str = "wing";
        break;

    case MON_SHAPE_INSECT:
    case MON_SHAPE_INSECT_WINGED:
    case MON_SHAPE_CENTIPEDE:
        str = "antenna";
        break;

    case MON_SHAPE_SNAIL:
        str = "eye-stalk";
        break;

    case MON_SHAPE_PLANT:
        str = "leaf";
        break;

    case MON_SHAPE_MISC:
        if (ch == 'x' || ch == 'X' || rand)
        {
            str = "tentacle";
            break;
        }
        // Deliberate fallthrough.
    case MON_SHAPE_FUNGUS:
        str         = "body";
        *can_plural = false;
        break;

    case MON_SHAPE_ORB:
        switch (type)
        {
            case MONS_GIANT_SPORE:
                str = "rhizome";
                break;

            case MONS_GIANT_EYEBALL:
            case MONS_EYE_OF_DRAINING:
            case MONS_SHINING_EYE:
            case MONS_EYE_OF_DEVASTATION:
            case MONS_GOLDEN_EYE:
                *can_plural = false;
                // Deliberate fallthrough.
            case MONS_GREAT_ORB_OF_EYES:
                str = "pupil";
                break;

            case MONS_GIANT_ORANGE_BRAIN:
            default:
                if (rand)
                    str = "rhizome";
                else
                {
                    str        = "body";
                    *can_plural = false;
                }
                break;
        }
    }

   if (str.empty())
   {
       // Reduce the chance of a random-shaped monster having hands.
       if (rand && coinflip())
           return (hand_name(plural, can_plural));

       str = "hand";
   }

   if (plural && *can_plural)
       str = pluralise(str);

   return (str);
}

std::string monster::foot_name(bool plural, bool *can_plural) const
{
    bool _can_plural;
    if (can_plural == NULL)
        can_plural = &_can_plural;
    *can_plural = true;

    std::string str;
    char        ch = mons_base_char(type);

    const bool rand = (type == MONS_CHAOS_SPAWN);

    switch (get_mon_shape(this))
    {
    case MON_SHAPE_INSECT:
    case MON_SHAPE_INSECT_WINGED:
    case MON_SHAPE_ARACHNID:
    case MON_SHAPE_CENTIPEDE:
        str = "leg";
        break;

    case MON_SHAPE_HUMANOID:
    case MON_SHAPE_HUMANOID_WINGED:
    case MON_SHAPE_HUMANOID_TAILED:
    case MON_SHAPE_HUMANOID_WINGED_TAILED:
        if (type == MONS_MINOTAUR)
            str = "hoof";
        else if (swimming()
                 && (type == MONS_MERFOLK || mons_genus(type) == MONS_MERMAID))
        {
            str         = "tail";
            *can_plural = false;
        }
        break;

    case MON_SHAPE_CENTAUR:
        str = "hoof";
        break;

    case MON_SHAPE_QUADRUPED:
    case MON_SHAPE_QUADRUPED_TAILLESS:
    case MON_SHAPE_QUADRUPED_WINGED:
        if (rand)
        {
            const char* feet[] = {"paw", "talon", "hoof"};
            str = RANDOM_ELEMENT(feet);
        }
        else if (ch == 'h')
            str = "paw";
        else if (ch == 'l' || ch == 'D')
            str = "talon";
        else if (type == MONS_YAK || type == MONS_DEATH_YAK)
            str = "hoof";
        else if (ch == 'H')
        {
            if (type == MONS_MANTICORE || type == MONS_SPHINX)
                str = "paw";
            else
                str = "talon";
        }
        break;

    case MON_SHAPE_BAT:
        str = "claw";
        break;

    case MON_SHAPE_SNAKE:
    case MON_SHAPE_FISH:
        str         = "tail";
        *can_plural = false;
        break;

    case MON_SHAPE_PLANT:
        str = "root";
        break;

    case MON_SHAPE_FUNGUS:
        str         = "stem";
        *can_plural = false;
        break;

    case MON_SHAPE_BLOB:
        str = "pseudopod";
        break;

    case MON_SHAPE_MISC:
        if (ch == 'x' || ch == 'X' || rand)
        {
            str = "tentacle";
            break;
        }
        // Deliberate fallthrough.
    case MON_SHAPE_SNAIL:
    case MON_SHAPE_NAGA:
    case MON_SHAPE_ORB:
        str         = "underside";
        *can_plural = false;
        break;
    }

   if (str.empty())
   {
       // Reduce the chance of a random-shaped monster having feet.
       if (rand && coinflip())
           return (foot_name(plural, can_plural));

       return (plural ? "feet" : "foot");
   }

   if (plural && *can_plural)
       str = pluralise(str);

   return (str);
}

std::string monster::arm_name(bool plural, bool *can_plural) const
{
    mon_body_shape shape = get_mon_shape(this);

    if (shape > MON_SHAPE_NAGA)
        return hand_name(plural, can_plural);

    if (can_plural != NULL)
        *can_plural = true;

    std::string str;
    switch (mons_genus(type))
    {
    case MONS_NAGA:
    case MONS_DRACONIAN: str = "scaled arm"; break;

    case MONS_MUMMY: str = "bandaged wrapped arm"; break;

    case MONS_SKELETAL_WARRIOR:
    case MONS_LICH:  str = "bony arm"; break;

    default: str = "arm"; break;
    }

   if (plural)
       str = pluralise(str);

   return (str);
}

int monster::mindex() const
{
    return (this - menv.buffer());
}

int monster::get_experience_level() const
{
    return (hit_dice);
}

void monster::moveto(const coord_def& c, bool clear_net)
{
    if (clear_net && c != pos() && in_bounds(pos()))
        mons_clear_trapping_net(this);

    if (mons_is_projectile(type))
    {
        // Assume some means of displacement, normal moves will overwrite this.
        props["iood_x"].get_float() += c.x - pos().x;
        props["iood_y"].get_float() += c.y - pos().y;
    }

    set_position(c);
}

bool monster::fumbles_attack(bool verbose)
{
    if (floundering() && one_chance_in(4))
    {
        if (verbose)
        {
            if (you.can_see(this))
            {
                mpr(name(DESC_CAP_THE)
                    + (liquefied(pos())
                       ? " becomes momentarily stuck in the liquid earth."
                       : " splashes around in the water."));
            }
            else if (player_can_hear(pos(), LOS_RADIUS))
                mpr("You hear a splashing noise.", MSGCH_SOUND);
        }

        return (true);
    }

    if (submerged())
        return (true);

    return (false);
}

bool monster::cannot_fight() const
{
    return (mons_class_flag(type, M_NO_EXP_GAIN)
            || mons_is_statue(type));
}

void monster::attacking(actor * /* other */)
{
}

// Sends a monster into a frenzy.
void monster::go_frenzy()
{
    if (!can_go_berserk())
        return;

    if (has_ench(ENCH_SLOW))
    {
        del_ench(ENCH_SLOW, true); // Give no additional message.
        simple_monster_message(this,
            make_stringf(" shakes off %s lethargy.",
                         pronoun(PRONOUN_NOCAP_POSSESSIVE).c_str()).c_str());
    }
    del_ench(ENCH_HASTE, true);
    del_ench(ENCH_FATIGUE, true); // Give no additional message.

    const int duration = 16 + random2avg(13, 2);

    // store the attitude for later retrieval
    props["old_attitude"] = short(attitude);

    attitude = ATT_NEUTRAL;
    add_ench(mon_enchant(ENCH_INSANE, 0, 0, duration * 10));
    add_ench(mon_enchant(ENCH_HASTE, 0, 0, duration * 10));
    add_ench(mon_enchant(ENCH_MIGHT, 0, 0, duration * 10));
    mons_att_changed(this);

    if (simple_monster_message(this, " flies into a frenzy!"))
        // Xom likes monsters going insane.
        xom_is_stimulated(friendly() ? 32 : 128);
}

void monster::go_berserk(bool /* intentional */, bool /* potion */)
{
    if (!can_go_berserk())
        return;

    if (has_ench(ENCH_SLOW))
    {
        del_ench(ENCH_SLOW, true); // Give no additional message.
        simple_monster_message(this,
            make_stringf(" shakes off %s lethargy.",
                         pronoun(PRONOUN_NOCAP_POSSESSIVE).c_str()).c_str());
    }
    del_ench(ENCH_FATIGUE, true); // Give no additional message.

    const int duration = 16 + random2avg(13, 2);
    add_ench(mon_enchant(ENCH_BERSERK, 0, 0, duration * 10));
    if (simple_monster_message(this, " goes berserk!"))
        // Xom likes monsters going berserk.
        xom_is_stimulated(friendly() ? 32 : 128);
}

void monster::expose_to_element(beam_type flavour, int strength)
{
    switch (flavour)
    {
    case BEAM_COLD:
        if (mons_class_flag(type, M_COLD_BLOOD) && res_cold() <= 0
            && coinflip())
        {
            slow_down(this, strength);
        }
        break;
    default:
        break;
    }
}

void monster::banish(const std::string &)
{
    coord_def old_pos = pos();

    if (mons_is_projectile(type))
        return;
    simple_monster_message(this, " is devoured by a tear in reality.",
                           MSGCH_BANISHMENT);
    monster_die(this, KILL_BANISHED, NON_MONSTER);

    place_cloud(CLOUD_TLOC_ENERGY, old_pos, 5 + random2(8), 0);
    for (adjacent_iterator ai(old_pos); ai; ++ai)
        if (!feat_is_solid(grd(*ai)) && env.cgrid(*ai) == EMPTY_CLOUD
            && coinflip())
        {
            place_cloud(CLOUD_TLOC_ENERGY, *ai, 1 + random2(8), 0);
        }
}

bool monster::has_spells() const
{
    for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
        if (spells[i] != SPELL_NO_SPELL)
            return (true);

    return (false);
}

bool monster::has_spell(spell_type spell) const
{
    for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
        if (spells[i] == spell)
            return (true);

    return (false);
}

bool monster::has_holy_spell() const
{
    for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
        if (is_holy_spell(spells[i]))
            return (true);

    return (false);
}

bool monster::has_unholy_spell() const
{
    for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
        if (is_unholy_spell(spells[i]))
            return (true);

    return (false);
}

bool monster::has_evil_spell() const
{
    for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
        if (is_evil_spell(spells[i]))
            return (true);

    return (false);
}

bool monster::has_unclean_spell() const
{
    for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
        if (is_unclean_spell(spells[i]))
            return (true);

    return (false);
}

bool monster::has_chaotic_spell() const
{
    for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
        if (is_chaotic_spell(spells[i]))
            return (true);

    return (false);
}

bool monster::has_attack_flavour(int flavour) const
{
    for (int i = 0; i < 4; ++i)
    {
        const int attk_flavour = mons_attack_spec(this, i).flavour;
        if (attk_flavour == flavour)
            return (true);
    }

    return (false);
}

bool monster::has_damage_type(int dam_type)
{
    for (int i = 0; i < 4; ++i)
    {
        const int dmg_type = damage_type(i);
        if (dmg_type == dam_type)
            return (true);
    }

    return (false);
}

// Whether the monster is temporarily confused.
// False for butterflies, vapours etc.
bool monster::confused() const
{
    return (mons_is_confused(this));
}

bool monster::confused_by_you() const
{
    if (mons_class_flag(type, M_CONFUSED))
        return false;

    const mon_enchant me = get_ench(ENCH_CONFUSION);
    const mon_enchant me2 = get_ench(ENCH_MAD);

    return ((me.ench == ENCH_CONFUSION && me.who == KC_YOU) ||
            (me2.ench == ENCH_MAD && me2.who == KC_YOU));
}

bool monster::paralysed() const
{
    return (has_ench(ENCH_PARALYSIS) || has_ench(ENCH_DUMB));
}

bool monster::cannot_act() const
{
    return (paralysed()
            || petrified() && !petrifying()
            || has_ench(ENCH_PREPARING_RESURRECT));
}

bool monster::cannot_move() const
{
    return (cannot_act() || petrifying());
}

bool monster::asleep() const
{
    return (behaviour == BEH_SLEEP);
}

bool monster::backlit(bool check_haloed, bool self_halo) const
{
    if (has_ench(ENCH_CORONA) || has_ench(ENCH_STICKY_FLAME) || has_ench(ENCH_SILVER_CORONA))
        return (true);
    if (check_haloed)
        return (haloed() && (self_halo || halo_radius2() == -1));
    return (false);
}

bool monster::glows_naturally() const
{
    return (mons_class_flag(type, M_GLOWS_LIGHT)
            || mons_class_flag(type, M_GLOWS_RADIATION));
}

bool monster::caught() const
{
    return (has_ench(ENCH_HELD));
}

bool monster::petrified() const
{
    return (has_ench(ENCH_PETRIFIED));
}

bool monster::petrifying() const
{
    return (has_ench(ENCH_PETRIFYING));
}

bool monster::friendly() const
{
    return (attitude == ATT_FRIENDLY || has_ench(ENCH_CHARM));
}

bool monster::neutral() const
{
    return (attitude == ATT_NEUTRAL || has_ench(ENCH_TEMP_PACIF)
            || attitude == ATT_GOOD_NEUTRAL
            || attitude == ATT_STRICT_NEUTRAL);
}

bool monster::good_neutral() const
{
    return (attitude == ATT_GOOD_NEUTRAL || has_ench(ENCH_TEMP_PACIF));
}

bool monster::strict_neutral() const
{
    return (attitude == ATT_STRICT_NEUTRAL);
}

bool monster::wont_attack() const
{
    return (friendly() || good_neutral() || strict_neutral());
}

bool monster::pacified() const
{
    return (attitude == ATT_NEUTRAL && testbits(flags, MF_GOT_HALF_XP));
}

int monster::shield_bonus() const
{
    const item_def *shld = const_cast<monster* >(this)->shield();
    if (shld && get_armour_slot(*shld) == EQ_SHIELD)
    {
        // Note that 0 is not quite no-blocking.
        if (incapacitated())
            return (0);

        int shld_c = property(*shld, PARM_AC) + shld->plus;
        return (random2avg(shld_c + hit_dice * 2 / 3, 2));
    }
    return (-100);
}

int monster::shield_block_penalty() const
{
    return (4 * shield_blocks * shield_blocks);
}

void monster::shield_block_succeeded(actor *attacker)
{
    actor::shield_block_succeeded(attacker);

    ++shield_blocks;
}

int monster::shield_bypass_ability(int) const
{
    return (15 + hit_dice * 2 / 3);
}

int monster::armour_class() const
{
    // Extra AC for snails/turtles drawn into their shells.
    return (ac + (has_ench(ENCH_WITHDRAWN) ? 10 : 0));
}

int monster::melee_evasion(const actor *act, ev_ignore_type evit) const
{
    int evasion = ev;

    // Phase Shift EV is already included.
    if (evit & EV_IGNORE_PHASESHIFT && mons_class_flag(type, M_PHASE_SHIFT))
        evasion -= 8;

    if (evit & EV_IGNORE_HELPLESS)
        return (evasion);

    if (paralysed() || asleep())
        evasion = 0;
    else if (caught())
        evasion /= (body_size(PSIZE_BODY) + 2);
    else if (confused())
        evasion /= 2;
    return (evasion);
}

bool monster::heal(int amount, bool max_too)
{
    if (mons_is_statue(type))
        return (false);

    if (amount < 1)
        return (false);
    else if (!max_too && hit_points == max_hit_points)
        return (false);

    hit_points += amount;

    bool success = true;

    if (hit_points > max_hit_points)
    {
        if (max_too)
        {
            const monsterentry* m = get_monster_data(type);
            const int maxhp =
                m->hpdice[0] * (m->hpdice[1] + m->hpdice[2]) + m->hpdice[3];

            // Limit HP growth.
            if (random2(3 * maxhp) > 2 * max_hit_points)
                max_hit_points = std::min(max_hit_points + 1, MAX_MONSTER_HP);
            else
                success = false;
        }

        hit_points = max_hit_points;
    }

    if (hit_points == max_hit_points)
    {
        // Clear the damage blame if it goes away completely.
        damage_friendly = 0;
        damage_total = 0;
    }

    return (success);
}

void monster::blame_damage(const actor* attacker, int amount)
{
    ASSERT(amount >= 0);
    damage_total = std::min<int>(MAX_DAMAGE_COUNTER, damage_total + amount);
    if (attacker)
        damage_friendly = std::min<int>(MAX_DAMAGE_COUNTER * 2,
                      damage_friendly + amount * exp_rate(attacker->mindex()));
}

void monster::suicide(int hp)
{
    if (hit_points > 0)
        blame_damage(NULL, hit_points);
    hit_points = hp;
}

mon_holy_type monster::holiness() const
{
    if (testbits(flags, MF_FAKE_UNDEAD))
        return (MH_UNDEAD);

    return (mons_class_holiness(type));
}

bool monster::undead_or_demonic() const
{
    const mon_holy_type holi = holiness();

    return (holi == MH_UNDEAD || holi == MH_DEMONIC);
}

bool monster::is_holy(bool check_spells) const
{
    if (holiness() == MH_HOLY)
        return (true);

    // Assume that all unknown gods (GOD_NAMELESS) are not holy.
    if (is_priest() && is_good_god(god) && check_spells)
        return (true);

    if (has_holy_spell() && (check_spells || !is_actual_spellcaster()))
        return (true);

    return (false);
}

bool monster::is_unholy(bool check_spells) const
{
    if (type == MONS_SILVER_STATUE)
        return (true);

    if (holiness() == MH_DEMONIC)
        return (true);

    if (has_unholy_spell() && check_spells)
        return (true);

    return (false);
}

bool monster::is_evil(bool check_spells) const
{
    if (holiness() == MH_UNDEAD)
        return (true);

    // Assume that all unknown gods (GOD_NAMELESS) are evil.
    if (is_priest() && (is_evil_god(god) || god == GOD_NAMELESS)
        && check_spells)
    {
        return (true);
    }

    if (has_evil_spell() && check_spells)
        return (true);

    if (has_attack_flavour(AF_DRAIN_XP)
        || has_attack_flavour(AF_VAMPIRIC))
    {
        return (true);
    }

    return (false);
}

bool monster::is_unclean(bool check_spells) const
{
    if (has_unclean_spell() && check_spells)
        return (true);

    if (has_attack_flavour(AF_DISEASE)
        || has_attack_flavour(AF_HUNGER)
        || has_attack_flavour(AF_ROT)
        || has_attack_flavour(AF_STEAL)
        || has_attack_flavour(AF_STEAL_FOOD))
    {
        return (true);
    }

    // Zin considers insanity unclean.  And slugs that speak.
    if (type == MONS_CRAZY_YIUF
        || type == MONS_PSYCHE
        || type == MONS_GASTRONOK)
    {
        return (true);
    }

    // Assume that all unknown gods (GOD_NAMELESS) are not chaotic.
    // Being a worshipper of a chaotic god doesn't yet make you
    // physically/essentially chaotic (so you don't get hurt by silver),
    // but Zin does mind.
    if (is_priest() && is_chaotic_god(god) && check_spells)
        return (true);

    if (has_chaotic_spell() && is_actual_spellcaster() && check_spells)
        return (true);

    corpse_effect_type ce = mons_corpse_effect(type);
    if ((ce == CE_HCL || ce == CE_MUTAGEN_RANDOM || ce == CE_MUTAGEN_GOOD
         || ce == CE_MUTAGEN_BAD || ce == CE_RANDOM) && !is_chaotic())
    {
        return (true);
    }

    return (false);
}

bool monster::is_known_chaotic() const
{
    if (type == MONS_UGLY_THING
        || type == MONS_VERY_UGLY_THING
        || type == MONS_ABOMINATION_SMALL
        || type == MONS_ABOMINATION_LARGE
        || type == MONS_KILLER_KLOWN // For their random attacks.
        || type == MONS_TIAMAT)      // For her colour-changing.
    {
        return (true);
    }

    if (is_shapeshifter() && (flags & MF_KNOWN_MIMIC))
        return (true);

    // Knowing chaotic spells is not enough to make you "essentially"
    // chaotic (i.e., silver doesn't hurt you), it's just unclean for
    // Zin.  Having chaotic abilities (not actual spells) does mean
    // you're truly changed by chaos.
    if (has_chaotic_spell() && !is_actual_spellcaster())
        return (true);

    if (has_attack_flavour(AF_MUTATE)
        || has_attack_flavour(AF_CHAOS))
    {
        return (true);
    }

    return (false);
}

bool monster::is_chaotic() const
{
    return (is_shapeshifter() || is_known_chaotic());
}

bool monster::is_artificial() const
{
    return (mons_class_flag(type, M_ARTIFICIAL));
}

bool monster::is_unbreathing() const
{
    const mon_holy_type holi = holiness();

    if (holi == MH_UNDEAD
        || holi == MH_NONLIVING
        || holi == MH_PLANT)
    {
        return (true);
    }

    return (mons_class_flag(type, M_UNBREATHING));
}

bool monster::is_insubstantial() const
{
    return (mons_class_flag(type, M_INSUBSTANTIAL));
}

int monster::res_hellfire() const
{
    int res = get_mons_resists(this).hellfire;

    return res;
}

int monster::res_fire() const
{
    const mon_resist_def res = get_mons_resists(this);

    int u = std::min(res.fire + res.hellfire * 3, 3);

    if (mons_itemuse(this) >= MONUSE_STARTING_EQUIPMENT)
    {
        u += scan_mon_inv_randarts(this, ARTP_FIRE);

        const int armour = inv[MSLOT_ARMOUR];
        const int shld   = inv[MSLOT_SHIELD];

        if (armour != NON_ITEM && mitm[armour].base_type == OBJ_ARMOUR)
            u += get_armour_res_fire(mitm[armour], false);

        if (shld != NON_ITEM && mitm[shld].base_type == OBJ_ARMOUR)
            u += get_armour_res_fire(mitm[shld], false);
    }

    if (u < -3)
        u = -3;
    else if (u > 3)
        u = 3;

    return (u);
}

int monster::res_steam() const
{
    int res = get_mons_resists(this).steam;
    if (has_equipped(EQ_BODY_ARMOUR, ARM_STEAM_DRAGON_ARMOUR))
        res += 3;
    return (res + res_fire() / 2);
}

int monster::res_cold() const
{
    int u = get_mons_resists(this).cold;

    if (mons_itemuse(this) >= MONUSE_STARTING_EQUIPMENT)
    {
        u += scan_mon_inv_randarts(this, ARTP_COLD);

        const int armour = inv[MSLOT_ARMOUR];
        const int shld   = inv[MSLOT_SHIELD];

        if (armour != NON_ITEM && mitm[armour].base_type == OBJ_ARMOUR)
            u += get_armour_res_cold(mitm[armour], false);

        if (shld != NON_ITEM && mitm[shld].base_type == OBJ_ARMOUR)
            u += get_armour_res_cold(mitm[shld], false);
    }

    if (u < -3)
        u = -3;
    else if (u > 3)
        u = 3;

    return (u);
}

int monster::res_elec() const
{
    // This is a variable, not a player_xx() function, so can be above 1.
    int u = 0;

    u += get_mons_resists(this).elec;

    // Don't bother checking equipment if the monster can't use it.
    if (mons_itemuse(this) >= MONUSE_STARTING_EQUIPMENT)
    {
        u += scan_mon_inv_randarts(this, ARTP_ELECTRICITY);

        // No ego armour, but storm dragon.
        const int armour = inv[MSLOT_ARMOUR];
        if (armour != NON_ITEM && mitm[armour].base_type == OBJ_ARMOUR)
            u += get_armour_res_elec(mitm[armour], false);
    }

    // Monsters can legitimately get multiple levels of electricity resistance.

    return (u);
}

int monster::res_asphyx() const
{
    int res = get_mons_resists(this).asphyx;
    if (is_unbreathing())
        res += 1;
    return (res);
}

int monster::res_water_drowning() const
{
    if (is_unbreathing())
        return (1);

    switch (mons_habitat(this))
    {
    case HT_WATER:
    case HT_AMPHIBIOUS:
        return (1);
    default:
        return (0);
    }
}

int monster::res_poison(bool temp) const
{
    UNUSED(temp);

    int u = get_mons_resists(this).poison;

    if (mons_itemuse(this) >= MONUSE_STARTING_EQUIPMENT)
    {
        u += scan_mon_inv_randarts(this, ARTP_POISON);

        const int armour = inv[MSLOT_ARMOUR];
        const int shld   = inv[MSLOT_SHIELD];

        if (armour != NON_ITEM && mitm[armour].base_type == OBJ_ARMOUR)
            u += get_armour_res_poison(mitm[armour], false);

        if (shld != NON_ITEM && mitm[shld].base_type == OBJ_ARMOUR)
            u += get_armour_res_poison(mitm[shld], false);
    }

    // Monsters can legitimately get multiple levels of poison resistance.

    return (u);
}

int monster::res_sticky_flame() const
{
    int res = get_mons_resists(this).sticky_flame;
    if (is_insubstantial())
        res += 1;
    if (has_equipped(EQ_BODY_ARMOUR, ARM_MOTTLED_DRAGON_ARMOUR))
        res += 1;
    return (res);
}

int monster::res_rotting(bool temp) const
{
    UNUSED(temp);

    int res = get_mons_resists(this).rotting;
    if (res)
        return (1);
    switch(holiness())
    {
    case MH_NATURAL:
    case MH_PLANT: // was 1 before.  Gardening shows it should be -1 instead...
        res = 0;
        break;
    case MH_UNDEAD:
        if (mons_base_char(type) == 'n'
            || type == MONS_ZOMBIE_SMALL
            || type == MONS_ZOMBIE_LARGE)
        {
            res = 1;
        }
        else
            res = 3;
        break;
    case MH_DEMONIC:
    case MH_HOLY:
        res = 1;
        break;
    case MH_NONLIVING:
        res = 3;
        break;
    }
    if (is_insubstantial())
        res = 3;
    return (res);
}

int monster::res_holy_energy(const actor *attacker) const
{
    if (undead_or_demonic())
        return (-2);

    if (is_evil())
        return (-1);

    if (is_holy()
        || is_good_god(god)
        || neutral()
        || is_unchivalric_attack(attacker, this)
        || is_good_god(you.religion) && is_follower(this))
    {
        return (1);
    }

    return (0);
}

int monster::res_negative_energy() const
{
    if (holiness() != MH_NATURAL
        || type == MONS_SHADOW_DRAGON)
    {
        return (3);
    }

    int u = 0;

    if (mons_itemuse(this) >= MONUSE_STARTING_EQUIPMENT)
    {
        u += scan_mon_inv_randarts(this, ARTP_NEGATIVE_ENERGY);

        const int armour = inv[MSLOT_ARMOUR];
        const int shld   = inv[MSLOT_SHIELD];

        if (armour != NON_ITEM && mitm[armour].base_type == OBJ_ARMOUR)
            u += get_armour_life_protection(mitm[armour], false);

        if (shld != NON_ITEM && mitm[shld].base_type == OBJ_ARMOUR)
            u += get_armour_life_protection(mitm[shld], false);
    }

    if (u > 3)
        u = 3;

    return (u);
}

int monster::res_torment() const
{
    const mon_holy_type holy = holiness();
    if (holy == MH_UNDEAD
        || holy == MH_DEMONIC
        || holy == MH_NONLIVING)
    {
        return (1);
    }

    return (0);
}

int monster::res_wind() const
{
    if (has_ench(ENCH_PERM_TORNADO))
        return 1;
    return mons_class_res_wind(type);
}

int monster::res_acid() const
{
    return (get_mons_resists(this).acid);
}

int monster::res_magic() const
{
    if (mons_immune_magic(this))
        return (MAG_IMMUNE);

    int u = (get_monster_data(type))->resist_magic;

    // Negative values get multiplied with monster hit dice.
    if (u < 0)
        u = hit_dice * -u * 4 / 3;

    // Randarts have a multiplicative effect.
    u *= (scan_mon_inv_randarts(this, ARTP_MAGIC) + 100);
    u /= 100;

    // ego armour resistance
    const int armour = inv[MSLOT_ARMOUR];
    const int shld   = inv[MSLOT_SHIELD];

    if (armour != NON_ITEM && mitm[armour].base_type == OBJ_ARMOUR)
        u += get_armour_res_magic(mitm[armour], false);

    if (shld != NON_ITEM && mitm[shld].base_type == OBJ_ARMOUR)
        u += get_armour_res_magic(mitm[shld], false);

    if (has_ench(ENCH_LOWERED_MR))
        u /= 2;

    if (has_ench(ENCH_RAISED_MR)) //trog's hand
        u += 70;

    return (u);
}

flight_type monster::flight_mode() const
{
    return (mons_flies(this));
}

bool monster::is_levitating() const
{
    // Checking class flags is not enough - see mons_flies().
    return (flight_mode() == FL_LEVITATE);
}

int monster::mons_species() const
{
    return ::mons_species(type);
}

void monster::poison(actor *agent, int amount, bool force)
{
    if (amount <= 0)
        return;

    // Scale poison down for monsters.
    if (!(amount /= 2))
        amount = 1;

    poison_monster(this, agent, amount, force);
}

int monster::skill(skill_type sk) const
{
    switch (sk)
    {
    case SK_EVOCATIONS:
        return (type == MONS_DEEP_DWARF_ARTIFICER ? hit_dice * 2 : hit_dice);

    case SK_NECROMANCY:
        return ((holiness() == MH_UNDEAD || holiness() == MH_DEMONIC) ? hit_dice : hit_dice / 2);

    default:
        return (0);
    }
}

void monster::blink(bool)
{
    monster_blink(this);
}

void monster::teleport(bool now, bool, bool)
{
    monster_teleport(this, now, false);
}

bool monster::alive() const
{
    return (hit_points > 0 && type != MONS_NO_MONSTER);
}

god_type monster::deity() const
{
    return (god);
}

bool monster::drain_exp(actor *agent, bool quiet, int pow)
{
    if (x_chance_in_y(res_negative_energy(), 3))
        return (false);

    if (!quiet && you.can_see(this))
        mprf("%s is drained!", name(DESC_CAP_THE).c_str());

    // If quiet, don't clean up the monster in order to credit properly.
    hurt(agent, 2 + random2(pow), BEAM_NEG, !quiet);

    if (alive())
    {
        if (x_chance_in_y(pow, 15))
        {
            hit_dice--;
            experience = 0;
        }

        max_hit_points -= 2 + random2(pow);
        hit_points = std::min(max_hit_points, hit_points);
    }

    return (true);
}

bool monster::rot(actor *agent, int amount, int immediate, bool quiet)
{
    if (res_rotting() || amount <= 0)
        return (false);

    if (!quiet && you.can_see(this))
    {
        mprf("%s %s!", name(DESC_CAP_THE).c_str(),
             amount > 0 ? "rots" : "looks less resilient");
    }

    // Apply immediate damage because we can't handle rotting for
    // monsters yet.
    if (immediate > 0)
    {
        // If quiet, don't clean up the monster in order to credit
        // properly.
        hurt(agent, immediate, BEAM_MISSILE, !quiet);

        if (alive())
        {
            max_hit_points -= immediate * 2;
            hit_points = std::min(max_hit_points, hit_points);
        }
    }

    add_ench(mon_enchant(ENCH_ROT, std::min(amount, 4), agent));

    return (true);
}

int monster::hurt(const actor *agent, int amount, beam_type flavour,
                   bool cleanup_dead)
{
    const int initial_damage = amount;
    if (mons_is_projectile(type) || mindex() == ANON_FRIENDLY_MONSTER)
        return (0);

    if (alive())
    {
        if (amount != INSTANT_DEATH
            && ::mons_species(mons_base_type(this)) == MONS_DEEP_DWARF)
        {
            // Deep Dwarves get to shave _any_ hp loss. Player version:
            int shave = 1 + random2(2 + random2(1 + hit_dice / 3));
            dprf("(mon) HP shaved: %d.", shave);
            amount -= shave;
            if (amount <= 0)
                return (0);
        }

        if (amount == INSTANT_DEATH)
            amount = hit_points;
        else if (hit_dice <= 0)
            amount = hit_points;
        else if (amount <= 0 && hit_points <= max_hit_points)
            return (0);

        if (agent == &you && you.duration[DUR_QUAD_DAMAGE])
        {
            amount *= 4;
            if (amount > hit_points + 50)
                flags |= MF_EXPLODE_KILL;
        }

        amount = std::min(amount, hit_points);
        hit_points -= amount;

        if (hit_points > max_hit_points)
        {
            amount    += hit_points - max_hit_points;
            hit_points = max_hit_points;
        }

        if (flavour == BEAM_NUKE || flavour == BEAM_DISINTEGRATION)
        {
            if (can_bleed())
                blood_spray(pos(), type, amount / 5);

            if (!alive())
                flags |= MF_EXPLODE_KILL;
        }

        // Allow the victim to exhibit passive damage behaviour (e.g.
        // the royal jelly).
        react_to_damage(agent, amount, flavour);

        if (has_ench(ENCH_MIRROR_DAMAGE))
            add_final_effect(FINEFF_MIRROR_DAMAGE, agent, this,
                             coord_def(0, 0), initial_damage);

        blame_damage(agent, amount);
    }

    if (cleanup_dead && (hit_points <= 0 || hit_dice <= 0) && type != -1)
    {
        if (agent == NULL)
            monster_die(this, KILL_MISC, NON_MONSTER);
        else if (agent->atype() == ACT_PLAYER)
            monster_die(this, KILL_YOU, NON_MONSTER);
        else
            monster_die(this, KILL_MON, agent->mindex());
    }

    return (amount);
}

void monster::confuse(actor *atk, int strength)
{
    enchant_monster_with_flavour(this, atk, BEAM_CONFUSION, strength);
}

void monster::paralyse(actor *atk, int strength)
{
    enchant_monster_with_flavour(this, atk, BEAM_PARALYSIS, strength);
}

void monster::petrify(actor *atk, int strength)
{
    if (is_insubstantial())
        return;

    enchant_monster_with_flavour(this, atk, BEAM_PETRIFY, strength);
}

void monster::slow_down(actor *atk, int strength)
{
    enchant_monster_with_flavour(this, atk, BEAM_SLOW, strength);
}

void monster::set_ghost(const ghost_demon &g, bool has_name)
{
    ghost.reset(new ghost_demon(g));

    if (has_name)
        mname = ghost->name;
}

void monster::pandemon_init()
{
    hit_dice        = ghost->xl;
    max_hit_points  = std::min<int>(ghost->max_hp, MAX_MONSTER_HP);
    hit_points      = max_hit_points;
    ac              = ghost->ac;
    ev              = ghost->ev;
    flags           = MF_INTERESTING;
    // Don't make greased-lightning Pandemonium demons in the dungeon
    // max speed = 17). Demons in Pandemonium can be up to speed 20,
    // possibly with haste. Non-caster demons are likely to be fast.
    if (you.level_type == LEVEL_DUNGEON)
        speed = (!ghost->spellcaster ? 11 + roll_dice(2, 3) :
                 one_chance_in(3) ? 10 :
                 7 + roll_dice(2, 5));
    else
        speed = (!ghost->spellcaster ? 12 + roll_dice(2, 4) :
                 one_chance_in(3) ? 10 :
                 8 + roll_dice(2, 6));

    speed_increment = 70;

    colour = ghost->colour;

    load_ghost_spells();
}

void monster::set_new_monster_id()
{
    mid = ++you.last_mid;
}

void monster::ghost_init(bool need_pos)
{
    set_new_monster_id();
    type            = MONS_PLAYER_GHOST;
    god             = ghost->religion;
    hit_dice        = ghost->xl;
    max_hit_points  = std::min<int>(ghost->max_hp, MAX_MONSTER_HP);
    hit_points      = max_hit_points;
    ac              = ghost->ac;
    ev              = ghost->ev;
    speed           = ghost->speed;
    speed_increment = 70;
    attitude        = ATT_HOSTILE;
    behaviour       = BEH_WANDER;
    flags           = MF_INTERESTING;
    foe             = MHITNOT;
    foe_memory      = 0;
    colour          = ghost->colour;
    number          = MONS_NO_MONSTER;
    load_ghost_spells();

    inv.init(NON_ITEM);
    enchantments.clear();
    ench_countdown = 0;

    // Summoned player ghosts are already given a position; calling this
    // in those instances will cause a segfault. Instead, check to see
    // if we have a home first. {due}
    if (need_pos && !in_bounds(pos()))
        find_place_to_live();
}

void monster::uglything_init(bool only_mutate)
{
    // If we're mutating an ugly thing, leave its experience level, hit
    // dice and maximum and current hit points as they are.
    if (!only_mutate)
    {
        hit_dice        = ghost->xl;
        max_hit_points  = ghost->max_hp;
        hit_points      = max_hit_points;
    }

    ac              = ghost->ac;
    ev              = ghost->ev;
    speed           = ghost->speed;
    speed_increment = 70;
    colour          = ghost->colour;
}

void monster::dancing_weapon_init()
{
    hit_dice        = ghost->xl;
    max_hit_points  = ghost->max_hp;
    hit_points      = max_hit_points;
    ac              = ghost->ac;
    ev              = ghost->ev;
    speed           = ghost->speed;
    speed_increment = 70;
    colour          = ghost->colour;
}

void monster::labrat_init ()
{
    hit_dice        = ghost->xl;
    max_hit_points  = ghost->max_hp;
    hit_points      = max_hit_points;
    ac              = ghost->ac;
    ev              = ghost->ev;
    speed           = ghost->speed;
    speed_increment = 70;
    colour          = ghost->colour;

    load_ghost_spells();
}

void monster::uglything_mutate(uint8_t force_colour)
{
    ghost->init_ugly_thing(type == MONS_VERY_UGLY_THING, true, force_colour);
    uglything_init(true);
}

void monster::uglything_upgrade()
{
    ghost->ugly_thing_to_very_ugly_thing();
    uglything_init();
}

bool monster::check_set_valid_home(const coord_def &place,
                                    coord_def &chosen,
                                    int &nvalid) const
{
    if (!in_bounds(place))
        return (false);

    if (actor_at(place))
        return (false);

    if (!monster_habitable_grid(this, grd(place)))
        return (false);

    if (one_chance_in(++nvalid))
        chosen = place;

    return (true);
}

#define MAX_PLACE_NEAR_DIST 8

bool monster::find_home_near_place(const coord_def &c)
{
    int last_dist = -1;
    coord_def place(-1, -1);
    int nvalid = 0;
    SquareArray<int, MAX_PLACE_NEAR_DIST> dist(-1);
    std::queue<coord_def> q;

    q.push(c);
    dist(c - c) = 0;
    while (!q.empty())
    {
        coord_def p = q.front();
        q.pop();
        if (dist(p - c) >= last_dist && nvalid)
        {
            // already found a valid closer destination
            return (move_to_pos(place));
        }
        else if (dist(p - c) >= MAX_PLACE_NEAR_DIST)
            break;

        for (adjacent_iterator ai(p); ai; ++ai)
        {
            if (dist(*ai - c) > -1)
                continue;
            dist(*ai - c) = last_dist = dist(p - c) + 1;

            if (!monster_habitable_grid(this, grd(*ai)))
                continue;

            q.push(*ai);
            check_set_valid_home(*ai, place, nvalid);
        }
    }

    return (false);
}

bool monster::find_home_near_player()
{
    return (find_home_near_place(you.pos()));
}

bool monster::find_home_anywhere()
{
    coord_def place(-1, -1);
    int nvalid = 0;
    for (int tries = 0; tries < 600; ++tries)
        if (check_set_valid_home(random_in_bounds(), place, nvalid))
            return (move_to_pos(place));
    return (false);
}

bool monster::find_place_to_live(bool near_player)
{
    return (near_player && find_home_near_player()
            || find_home_anywhere());
}

void monster::destroy_inventory()
{
    for (int j = 0; j < NUM_MONSTER_SLOTS; j++)
    {
        if (inv[j] != NON_ITEM)
        {
            destroy_item(inv[j]);
            inv[j] = NON_ITEM;
        }
    }
}

bool monster::is_travelling() const
{
    return (!travel_path.empty());
}

bool monster::is_patrolling() const
{
    return (!patrol_point.origin());
}

bool monster::needs_abyss_transit() const
{
    return ((mons_is_unique(type)
                || (flags & MF_BANISHED)
                || you.level_type == LEVEL_DUNGEON
                   && hit_dice > 8 + random2(25)
                   && mons_can_use_stairs(this))
            && !has_ench(ENCH_ABJ));
}

void monster::set_transit(const level_id &dest)
{
    add_monster_to_transit(dest, *this);
}

void monster::load_ghost_spells()
{
    if (!ghost.get())
    {
        spells.init(SPELL_NO_SPELL);
        return;
    }

    spells = ghost->spells;

#ifdef DEBUG_DIAGNOSTICS
    dprf("Ghost spells:");
    for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; i++)
    {
        mprf(MSGCH_DIAGNOSTICS, "Spell #%d: %d (%s)",
             i, spells[i], spell_title(spells[i]));
    }
#endif
}

bool monster::has_hydra_multi_attack() const
{
    return (mons_genus(mons_base_type(this)) == MONS_HYDRA);
}

bool monster::has_multitargeting() const
{
    if (mons_wields_two_weapons(this))
        return (true);

    // Hacky little list for now. evk
    return ((has_hydra_multi_attack() && !mons_is_zombified(this))
            || type == MONS_TENTACLED_MONSTROSITY
            || type == MONS_ELECTRIC_GOLEM);
}

bool monster::can_use_spells() const
{
    return (flags & MF_SPELLCASTER);
}

bool monster::is_priest() const
{
    return (flags & MF_PRIEST);
}

bool monster::is_actual_spellcaster() const
{
    return (flags & MF_ACTUAL_SPELLS);
}

bool monster::is_shapeshifter() const
{
    return (has_ench(ENCH_GLOWING_SHAPESHIFTER, ENCH_SHAPESHIFTER));
}

bool monster::has_ench(enchant_type ench) const
{
    return (enchantments.find(ench) != enchantments.end());
}

bool monster::has_ench(enchant_type ench, enchant_type ench2) const
{
    if (ench2 == ENCH_NONE)
        ench2 = ench;

    for (int i = ench; i <= ench2; ++i)
        if (has_ench(static_cast<enchant_type>(i)))
            return (true);

    return (false);
}

mon_enchant monster::get_ench(enchant_type ench1,
                               enchant_type ench2) const
{
    if (ench2 == ENCH_NONE)
        ench2 = ench1;

    for (int e = ench1; e <= ench2; ++e)
    {
        mon_enchant_list::const_iterator i =
            enchantments.find(static_cast<enchant_type>(e));

        if (i != enchantments.end())
            return (i->second);
    }

    return mon_enchant();
}

void monster::update_ench(const mon_enchant &ench)
{
    if (ench.ench != ENCH_NONE)
    {
        mon_enchant_list::iterator i = enchantments.find(ench.ench);
        if (i != enchantments.end())
            i->second = ench;
    }
}

bool monster::add_ench(const mon_enchant &ench)
{
    // silliness
    if (ench.ench == ENCH_NONE)
        return (false);

    if (ench.ench == ENCH_FEAR
        && (holiness() == MH_NONLIVING || berserk()))
    {
        return (false);
    }

    if (ench.ench == ENCH_LEVITATION && has_ench(ENCH_LIQUEFYING))
    {
        del_ench(ENCH_LIQUEFYING);
        invalidate_agrid();
    }

    mon_enchant_list::iterator i = enchantments.find(ench.ench);
    bool new_enchantment = false;
    mon_enchant *added = NULL;
    if (i == enchantments.end())
    {
        new_enchantment = true;
        added = &(enchantments[ench.ench] = ench);
    }
    else
    {
        i->second += ench;
        added = &i->second;
    }

    // If the duration is not set, we must calculate it (depending on the
    // enchantment).
    if (!ench.duration)
        added->set_duration(this, new_enchantment ? NULL : &ench);

    if (new_enchantment)
        add_enchantment_effect(ench);

    return (true);
}

void monster::forget_random_spell()
{
    int which_spell = -1;
    int count = 0;
    for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
        if (spells[i] != SPELL_NO_SPELL && one_chance_in(++count))
            which_spell = i;
    if (which_spell != -1)
        spells[which_spell] = SPELL_NO_SPELL;
}

void monster::add_enchantment_effect(const mon_enchant &ench, bool quiet)
{
    // Check for slow/haste.
    switch (ench.ench)
    {
    case ENCH_INSANE:
    case ENCH_BERSERK:
        // Inflate hp.
        scale_hp(3, 2);

        if (has_ench(ENCH_SUBMERGED))
            del_ench(ENCH_SUBMERGED);

        if (mons_is_lurking(this))
        {
            behaviour = BEH_WANDER;
            behaviour_event(this, ME_EVAL);
        }
        calc_speed();
        break;

    case ENCH_HASTE:
        calc_speed();
        break;

    case ENCH_SLOW:
        calc_speed();
        break;

    case ENCH_STONESKIN:
        {
            // player gets 2+earth/5
            const int ac_bonus = hit_dice / 2;

            ac += ac_bonus;
            // the monster may get drained or level up, we need to store the bonus
            props["stoneskin_ac"].get_byte() = ac_bonus;
        }
        break;

    case ENCH_SUBMERGED:
        mons_clear_trapping_net(this);

        // Don't worry about invisibility.  You should be able to see if
        // something has submerged.
        if (!quiet && mons_near(this))
        {
            if (type == MONS_AIR_ELEMENTAL)
            {
                mprf("%s merges itself into the air.",
                     name(DESC_CAP_A, true).c_str());
            }
            else if (type == MONS_TRAPDOOR_SPIDER)
            {
                mprf("%s hides itself under the floor.",
                     name(DESC_CAP_A, true).c_str());
            }
            else if (seen_context == "surfaces"
                     || seen_context == "bursts forth"
                     || seen_context == "emerges")
            {
                // The monster surfaced and submerged in the same turn
                // without doing anything else.
                interrupt_activity(AI_SEE_MONSTER,
                                   activity_interrupt_data(this,
                                                           "surfaced"));
            }
            else if (crawl_state.game_is_arena())
                mprf("%s submerges.", name(DESC_CAP_A, true).c_str());
        }

        // Pacified monsters leave the level when they submerge.
        if (pacified())
            make_mons_leave_level(this);
        break;

    case ENCH_CONFUSION:
    case ENCH_MAD:
        if (type == MONS_TRAPDOOR_SPIDER && has_ench(ENCH_SUBMERGED))
            del_ench(ENCH_SUBMERGED);

        if (mons_is_lurking(this))
        {
            behaviour = BEH_WANDER;
            behaviour_event(this, ME_EVAL);
        }
        break;

    case ENCH_CHARM:
        behaviour = BEH_SEEK;
        target    = you.pos();
        foe       = MHITYOU;

        if (is_patrolling())
        {
            // Enslaved monsters stop patrolling and forget their patrol
            // point; they're supposed to follow you now.
            patrol_point.reset();
            firing_pos.reset();
        }
        mons_att_changed(this);
        if (you.can_see(this))
            learned_something_new(HINT_MONSTER_FRIENDLY, pos());
        break;

    case ENCH_LIQUEFYING:
    case ENCH_SILENCE:
        invalidate_agrid(true);
        break;

    default:
        break;
    }
}

static bool _prepare_del_ench(monster* mon, const mon_enchant &me)
{
    if (me.ench != ENCH_SUBMERGED)
        return (true);

    // Lurking monsters only unsubmerge when their foe is in sight if the foe
    // is right next to them.
    if (mons_is_lurking(mon))
    {
        const actor* foe = mon->get_foe();
        if (foe != NULL && mon->can_see(foe)
            && !adjacent(mon->pos(), foe->pos()))
        {
            return (false);
        }
    }

    int midx = mon->mindex();

    if (!monster_at(mon->pos()))
        mgrd(mon->pos()) = midx;

    if (mon->pos() != you.pos() && midx == mgrd(mon->pos()))
        return (true);

    if (midx != mgrd(mon->pos()))
    {
        monster* other_mon = &menv[mgrd(mon->pos())];

        if (other_mon->type == MONS_NO_MONSTER
            || other_mon->type == MONS_PROGRAM_BUG)
        {
            mgrd(mon->pos()) = midx;

            mprf(MSGCH_ERROR, "mgrd(%d,%d) points to %s monster, even "
                 "though it contains submerged monster %s (see bug 2293518)",
                 mon->pos().x, mon->pos().y,
                 other_mon->type == MONS_NO_MONSTER ? "dead" : "buggy",
                 mon->name(DESC_PLAIN, true).c_str());

            if (mon->pos() != you.pos())
                return (true);
        }
        else
            mprf(MSGCH_ERROR, "%s tried to unsubmerge while on same square as "
                 "%s (see bug 2293518)", mon->name(DESC_CAP_THE, true).c_str(),
                 mon->name(DESC_NOCAP_A, true).c_str());
    }

    // Monster un-submerging while under player or another monster.  Try to
    // move to an adjacent square in which the monster could have been
    // submerged and have it unsubmerge from there.
    coord_def target_square;
    int       okay_squares = 0;

    for (adjacent_iterator ai(you.pos()); ai; ++ai)
        if (!actor_at(*ai)
            && monster_can_submerge(mon, grd(*ai))
            && one_chance_in(++okay_squares))
        {
            target_square = *ai;
        }

    if (okay_squares > 0)
        return (mon->move_to_pos(target_square));

    // No available adjacent squares from which the monster could also
    // have unsubmerged.  Can it just stay submerged where it is?
    if (monster_can_submerge(mon, grd(mon->pos())))
        return (false);

    // The terrain changed and the monster can't remain submerged.
    // Try to move to an adjacent square where it would be happy.
    for (adjacent_iterator ai(you.pos()); ai; ++ai)
    {
        if (!monster_at(*ai)
            && monster_habitable_grid(mon, grd(*ai))
            && !find_trap(*ai))
        {
            if (one_chance_in(++okay_squares))
                target_square = *ai;
        }
    }

    if (okay_squares > 0)
        return (mon->move_to_pos(target_square));

    return (true);
}

bool monster::del_ench(enchant_type ench, bool quiet, bool effect)
{
    mon_enchant_list::iterator i = enchantments.find(ench);
    if (i == enchantments.end())
        return (false);

    const mon_enchant me = i->second;
    const enchant_type et = i->first;

    if (!_prepare_del_ench(this, me))
        return (false);

    enchantments.erase(et);
    if (effect)
        remove_enchantment_effect(me, quiet);
    return (true);
}

void monster::remove_enchantment_effect(const mon_enchant &me, bool quiet)
{
    switch (me.ench)
    {
    case ENCH_TIDE:
        shoals_release_tide(this);
        break;

    case ENCH_INSANE:
        attitude = static_cast<mon_attitude_type>(props["old_attitude"].get_short());
        mons_att_changed(this);
        // deliberate fall through

    case ENCH_BERSERK:
        scale_hp(2, 3);
        calc_speed();
        break;

    case ENCH_HASTE:
        calc_speed();
        if (!quiet)
            simple_monster_message(this, " is no longer moving quickly.");
        break;

    case ENCH_SWIFT:
        if (!quiet)
            if (type == MONS_ALLIGATOR)
                simple_monster_message(this, " slows down.");
            else
                simple_monster_message(this, " is no longer moving somewhat quickly.");
        break;

    case ENCH_SILENCE:
        invalidate_agrid();
        if (!quiet && !silenced(pos()))
            if (alive())
                simple_monster_message(this, " becomes audible again.");
            else
                mprf("As %s dies, the sound returns.", name(DESC_NOCAP_THE).c_str());
        break;

    case ENCH_MIGHT:
        if (!quiet)
            simple_monster_message(this, " no longer looks unusually strong.");
        break;

    case ENCH_SLOW:
        if (!quiet && !(liquefied(pos()) && ground_level() && !is_insubstantial()))
            simple_monster_message(this, " is no longer moving slowly.");
        calc_speed();
        break;

    case ENCH_STONESKIN:
        if (props.exists("stoneskin_ac"))
            ac -= props["stoneskin_ac"].get_byte();
        break;

    case ENCH_PARALYSIS:
        if (!quiet)
            simple_monster_message(this, " is no longer paralysed.");

        behaviour_event(this, ME_EVAL);
        break;

    case ENCH_TEMP_PACIF:
        if (!quiet)
            simple_monster_message(this, (" seems to come to "
                + pronoun(PRONOUN_NOCAP_POSSESSIVE) + " senses.").c_str());
        // Yeah, this _is_ offensive to Zin, but hey, he deserves it (1KB).

        behaviour_event(this, ME_EVAL);
        break;

    case ENCH_PETRIFIED:
        if (!quiet)
            simple_monster_message(this, " is no longer petrified.");
        del_ench(ENCH_PETRIFYING);

        behaviour_event(this, ME_EVAL);
        break;

    case ENCH_PETRIFYING:
        if (!petrified())
            break;

        if (!quiet)
            simple_monster_message(this, " stops moving altogether!");

        behaviour_event(this, ME_EVAL);
        break;

    case ENCH_FEAR:
        if (holiness() == MH_NONLIVING || berserk())
        {
            // This should only happen because of fleeing sanctuary
            snprintf(info, INFO_SIZE, " stops retreating.");
        }
        else if (!mons_is_tentacle(type))
        {
            snprintf(info, INFO_SIZE, " seems to regain %s courage.",
                     pronoun(PRONOUN_NOCAP_POSSESSIVE, true).c_str());
        }

        if (!quiet)
            simple_monster_message(this, info);

        // Reevaluate behaviour.
        behaviour_event(this, ME_EVAL);
        break;

    case ENCH_CONFUSION:
        if (!quiet)
            simple_monster_message(this, " seems less confused.");

        // Reevaluate behaviour.
        behaviour_event(this, ME_EVAL);
        break;

    case ENCH_INVIS:
        // Invisible monsters stay invisible.
        if (mons_class_flag(type, M_INVIS))
            add_ench(mon_enchant(ENCH_INVIS));
        else if (mons_near(this) && !you.can_see_invisible()
                 && !has_ench(ENCH_SUBMERGED))
        {
            if (!quiet)
            {
                mprf("%s appears from thin air!",
                     name(DESC_CAP_A, true).c_str());
                autotoggle_autopickup(false);
            }

            handle_seen_interrupt(this);
        }
        break;

    case ENCH_CHARM:
        if (!quiet)
            simple_monster_message(this, " is no longer charmed.");

        if (you.can_see(this))
        {
            // and fire activity interrupts
            interrupt_activity(AI_SEE_MONSTER,
                               activity_interrupt_data(this, "uncharm"));
        }

        if (is_patrolling())
        {
            // Enslaved monsters stop patrolling and forget their patrol point,
            // in case they were on order to wait.
            patrol_point.reset();
        }
        mons_att_changed(this);

        // Reevaluate behaviour.
        behaviour_event(this, ME_EVAL);
        break;

    case ENCH_CORONA:
    case ENCH_SILVER_CORONA:
    if (!quiet)
        {
            if (visible_to(&you))
                simple_monster_message(this, " stops glowing.");
            else if (has_ench(ENCH_INVIS) && mons_near(this))
            {
                mprf("%s stops glowing and disappears.",
                     name(DESC_CAP_THE, true).c_str());
            }
        }
        break;

    case ENCH_STICKY_FLAME:
        if (!quiet)
            simple_monster_message(this, " stops burning.");
        break;

    case ENCH_POISON:
        if (!quiet)
            simple_monster_message(this, " looks more healthy.");
        break;

    case ENCH_ROT:
        if (!quiet)
        {
            if (this->type == MONS_BOG_MUMMY)
                simple_monster_message(this, "'s decay slows.");
            else
                simple_monster_message(this, " is no longer rotting.");
        }
        break;

    case ENCH_HELD:
    {
        int net = get_trapping_net(pos());
        if (net != NON_ITEM)
            remove_item_stationary(mitm[net]);

        if (!quiet)
            simple_monster_message(this, " breaks free.");
        break;
    }
    case ENCH_FAKE_ABJURATION:
    case ENCH_ABJ:
    case ENCH_SHORT_LIVED:
        // Set duration to -1 so that monster_die() and any of its
        // callees can tell that the monster ran out of time or was
        // abjured.
        add_ench(mon_enchant(
            (me.ench != ENCH_FAKE_ABJURATION) ?
                ENCH_ABJ : ENCH_FAKE_ABJURATION, 0, 0, -1));

        if (berserk())
            simple_monster_message(this, " is no longer berserk.");

        monster_die(this, (me.ench == ENCH_FAKE_ABJURATION) ? KILL_MISC :
                            (quiet) ? KILL_DISMISSED : KILL_RESET, NON_MONSTER);
        break;

    case ENCH_SUBMERGED:
        if (mons_is_wandering(this))
        {
            behaviour = BEH_SEEK;
            behaviour_event(this, ME_EVAL);
        }

        if (you.pos() == pos())
        {
            mprf(MSGCH_ERROR, "%s is on the same square as you!",
                 name(DESC_CAP_A).c_str());
        }

        if (you.can_see(this))
        {
            if (!mons_is_safe(this) && delay_is_run(current_delay_action()))
            {
                // Already set somewhere else.
                if (!seen_context.empty())
                    return;

                if (type == MONS_AIR_ELEMENTAL)
                    seen_context = "thin air";
                else if (type == MONS_TRAPDOOR_SPIDER)
                    seen_context = "leaps out";
                else if (!monster_habitable_grid(this, DNGN_FLOOR))
                    seen_context = "bursts forth";
                else
                    seen_context = "surfaces";
            }
            else if (!quiet)
            {
                if (type == MONS_AIR_ELEMENTAL)
                {
                    mprf("%s forms itself from the air!",
                         name(DESC_CAP_A, true).c_str());
                }
                else if (type == MONS_TRAPDOOR_SPIDER)
                {
                    mprf("%s leaps out from its hiding place under the floor!",
                         name(DESC_CAP_A, true).c_str());
                }
                else if (crawl_state.game_is_arena())
                    mprf("%s surfaces.", name(DESC_CAP_A, true).c_str());
            }
        }
        else if (mons_near(this)
                 && feat_compatible(grd(pos()), DNGN_DEEP_WATER))
        {
            mpr("Something invisible bursts forth from the water.");
            interrupt_activity(AI_FORCE_INTERRUPT);
        }
        break;

    case ENCH_SOUL_RIPE:
        if (!quiet)
        {
            simple_monster_message(this,
                                   "'s soul is no longer ripe for the taking.");
        }
        break;

    case ENCH_AWAKEN_FOREST:
        env.forest_awoken_until = 0;
        if (!quiet)
            forest_message(pos(), "The forest calms down.");
        break;

    case ENCH_BLEED:
        if (!quiet)
            simple_monster_message(this, " is no longer bleeding.");
        break;

    case ENCH_WITHDRAWN:
        if (!quiet)
            simple_monster_message(this, " emerges from its shell.");
        break;

    case ENCH_LIQUEFYING:
        invalidate_agrid();

        if (!quiet)
            simple_monster_message(this, " is no longer liquefying the ground.");
        break;

    case ENCH_LEVITATION:
        apply_location_effects(pos(), me.killer(), me.kill_agent());
        break;

    case ENCH_DAZED:
        if (!quiet && alive())
                simple_monster_message(this, " is no longer dazed.");
        break;

    //The following should never happen, but just in case...

    case ENCH_MUTE:
        if (!quiet && alive())
                simple_monster_message(this, " is no longer mute.");
        break;

    case ENCH_BLIND:
        if (!quiet && alive())
            simple_monster_message(this, " is no longer blind.");

        // Reevaluate behaviour.
        behaviour_event(this, ME_EVAL);
        break;

    case ENCH_DUMB:
        if (!quiet && alive())
            simple_monster_message(this, " is no longer stupefied.");

        // Reevaluate behaviour.
        behaviour_event(this, ME_EVAL);
        break;

    case ENCH_MAD:
        if (!quiet && alive())
            simple_monster_message(this, " is no longer mad.");

        // Reevaluate behaviour.
        behaviour_event(this, ME_EVAL);
        break;

    default:
        break;
    }
}

bool monster::lose_ench_levels(const mon_enchant &e, int lev)
{
    if (!lev)
        return (false);

    if (e.degree <= lev)
    {
        del_ench(e.ench);
        return (true);
    }
    else
    {
        mon_enchant newe(e);
        newe.degree -= lev;
        update_ench(newe);
        return (false);
    }
}

bool monster::lose_ench_duration(const mon_enchant &e, int dur)
{
    if (!dur)
        return (false);

    if (e.duration <= dur)
    {
        del_ench(e.ench);
        return (true);
    }
    else
    {
        mon_enchant newe(e);
        newe.duration -= dur;
        update_ench(newe);
        return (false);
    }
}

//---------------------------------------------------------------
//
// timeout_enchantments
//
// Update a monster's enchantments when the player returns
// to the level.
//
// Management for enchantments... problems with this are the oddities
// (monster dying from poison several thousands of turns later), and
// game balance.
//
// Consider: Poison/Sticky Flame a monster at range and leave, monster
// dies but can't leave level to get to player (implied game balance of
// the delayed damage is that the monster could be a danger before
// it dies).  This could be fixed by keeping some monsters active
// off level and allowing them to take stairs (a very serious change).
//
// Compare this to the current abuse where the player gets
// effectively extended duration of these effects (although only
// the actual effects only occur on level, the player can leave
// and heal up without having the effect disappear).
//
// This is a simple compromise between the two... the enchantments
// go away, but the effects don't happen off level.  -- bwr
//
//---------------------------------------------------------------
void monster::timeout_enchantments(int levels)
{
    if (enchantments.empty())
        return;

    const mon_enchant_list ec = enchantments;
    for (mon_enchant_list::const_iterator i = ec.begin();
         i != ec.end(); ++i)
    {
        switch (i->first)
        {
        case ENCH_WITHDRAWN:
            if (hit_points >= (max_hit_points - max_hit_points / 4)
                && !one_chance_in(3))
            {
                del_ench(i->first);
                break;
            }
            // Deliberate fall-through

        case ENCH_POISON: case ENCH_ROT: case ENCH_CORONA:
        case ENCH_STICKY_FLAME: case ENCH_ABJ: case ENCH_SHORT_LIVED:
        case ENCH_SLOW: case ENCH_HASTE: case ENCH_MIGHT: case ENCH_FEAR:
        case ENCH_INVIS: case ENCH_CHARM:  case ENCH_SLEEP_WARY:
        case ENCH_SICK: case ENCH_SLEEPY: case ENCH_PARALYSIS:
        case ENCH_PETRIFYING: case ENCH_PETRIFIED: case ENCH_SWIFT:
        case ENCH_BATTLE_FRENZY: case ENCH_TEMP_PACIF: case ENCH_SILENCE:
        case ENCH_LOWERED_MR: case ENCH_SOUL_RIPE: case ENCH_BLEED:
        case ENCH_ANTIMAGIC: case ENCH_FEAR_INSPIRING:
        case ENCH_REGENERATION: case ENCH_RAISED_MR: case ENCH_MIRROR_DAMAGE:
        case ENCH_STONESKIN: case ENCH_LIQUEFYING:
        case ENCH_SILVER_CORONA: case ENCH_DAZED: case ENCH_FAKE_ABJURATION:
            lose_ench_levels(i->second, levels);
            break;

        case ENCH_INSANE:
        case ENCH_BERSERK:
            del_ench(i->first);
            break;

        case ENCH_FATIGUE:
            del_ench(i->first);
            del_ench(ENCH_SLOW);
            break;

        case ENCH_TP:
            teleport(true);
            del_ench(i->first);
            break;

        case ENCH_CONFUSION:
            if (!mons_class_flag(type, M_CONFUSED))
                del_ench(i->first);
            monster_blink(this, true);
            break;

        case ENCH_HELD:
            del_ench(i->first);
            break;

        case ENCH_TIDE:
        {
            const int actdur = speed_to_duration(speed) * levels;
            lose_ench_duration(i->first, actdur);
            break;
        }

        case ENCH_SLOWLY_DYING:
        {
            const int actdur = speed_to_duration(speed) * levels;
            if (lose_ench_duration(i->first, actdur))
                monster_die(this, KILL_MISC, NON_MONSTER, true);
            break;
        }

        case ENCH_FADING_AWAY:
        {
            const int actdur = speed_to_duration(speed) * levels;
            if (lose_ench_duration(i->first, actdur))
                spirit_fades(this);
            break;
        }

        case ENCH_PREPARING_RESURRECT:
        {
            const int actdur = speed_to_duration(speed) * levels;
            if (lose_ench_duration(i->first, actdur))
                shedu_do_actual_resurrection(this);
            break;
        }

        default:
            break;
        }

        if (!alive())
            break;
    }
}

std::string monster::describe_enchantments() const
{
    std::ostringstream oss;
    for (mon_enchant_list::const_iterator i = enchantments.begin();
         i != enchantments.end(); ++i)
    {
        if (i != enchantments.begin())
            oss << ", ";
        oss << std::string(i->second);
    }
    return (oss.str());
}

// Used to adjust time durations in calc_duration() for monster speed.
static inline int _mod_speed(int val, int speed)
{
    if (!speed)
        speed = 10;
    const int modded = (speed ? (val * 10) / speed : val);
    return (modded? modded : 1);
}

bool monster::decay_enchantment(const mon_enchant &me, bool decay_degree)
{
    // Faster monsters can wiggle out of the net more quickly.
    const int spd = (me.ench == ENCH_HELD) ? speed :
                                             10;
    const int actdur = speed_to_duration(spd);
    if (lose_ench_duration(me, actdur))
        return (true);

    if (!decay_degree)
        return (false);

    // Decay degree so that higher degrees decay faster than lower
    // degrees, and a degree of 1 does not decay (it expires when the
    // duration runs out).
    const int level = me.degree;
    if (level <= 1)
        return (false);

    const int decay_factor = level * (level + 1) / 2;
    if (me.duration < me.maxduration * (decay_factor - 1) / decay_factor)
    {
        mon_enchant newme = me;
        --newme.degree;
        newme.maxduration = newme.duration;

        if (newme.degree <= 0)
        {
            del_ench(me.ench);
            return (true);
        }
        else
            update_ench(newme);
    }
    return (false);
}

void monster::apply_enchantment(const mon_enchant &me)
{
    const int spd = 10;
    switch (me.ench)
    {
    case ENCH_INSANE:
        if (decay_enchantment(me))
        {
            simple_monster_message(this, " is no longer in an insane frenzy.");
            const int duration = random_range(70, 130);
            add_ench(mon_enchant(ENCH_FATIGUE, 0, 0, duration));
            add_ench(mon_enchant(ENCH_SLOW, 0, 0, duration));
        }
        break;

    case ENCH_BERSERK:
        if (decay_enchantment(me))
        {
            simple_monster_message(this, " is no longer berserk.");
            const int duration = random_range(70, 130);
            add_ench(mon_enchant(ENCH_FATIGUE, 0, 0, duration));
            add_ench(mon_enchant(ENCH_SLOW, 0, 0, duration));
        }
        break;

    case ENCH_FATIGUE:
        if (decay_enchantment(me))
        {
            simple_monster_message(this, " looks more energetic.");
            del_ench(ENCH_SLOW, true);
        }
        break;

    case ENCH_WITHDRAWN:
        if (hit_points >= (max_hit_points - max_hit_points / 4)
                && !one_chance_in(3))
        {
            del_ench(ENCH_WITHDRAWN);
            break;
        }

        // Deliberate fall through.

    case ENCH_SLOW:
    case ENCH_HASTE:
    case ENCH_SWIFT:
    case ENCH_MIGHT:
    case ENCH_FEAR:
    case ENCH_PARALYSIS:
    case ENCH_TEMP_PACIF:
    case ENCH_PETRIFYING:
    case ENCH_PETRIFIED:
    case ENCH_SICK:
    case ENCH_CORONA:
    case ENCH_ABJ:
    case ENCH_CHARM:
    case ENCH_SLEEP_WARY:
    case ENCH_LOWERED_MR:
    case ENCH_SOUL_RIPE:
    case ENCH_TIDE:
    case ENCH_ANTIMAGIC:
    case ENCH_REGENERATION:
    case ENCH_RAISED_MR:
    case ENCH_STONESKIN:
    case ENCH_FEAR_INSPIRING:
    case ENCH_LIFE_TIMER:
    case ENCH_LEVITATION:
    case ENCH_DAZED:
    case ENCH_LIQUEFYING:
    case ENCH_FAKE_ABJURATION:
    case ENCH_RECITE_TIMER:
        decay_enchantment(me);
        break;

    case ENCH_MIRROR_DAMAGE:
        if (decay_enchantment(me))
            simple_monster_message(this, "'s dark mirror aura disappears.");
        break;

    case ENCH_SILENCE:
        decay_enchantment(me);
        invalidate_agrid();
        break;

    case ENCH_BATTLE_FRENZY:
        decay_enchantment(me, false);
        break;

    case ENCH_AQUATIC_LAND:
        // Aquatic monsters lose hit points every turn they spend on dry land.
        ASSERT(mons_habitat(this) == HT_WATER);
        if (feat_is_watery(grd(pos())))
        {
            // The tide, water card or Fedhas gave us water.
            del_ench(ENCH_AQUATIC_LAND);
            break;
        }

        // Zombies don't take damage from flopping about on land.
        if (mons_is_zombified(this))
            break;

        hurt(me.agent(), 1 + random2(5), BEAM_NONE);
        break;

    case ENCH_HELD:
    {
        if (mons_is_stationary(this) || cannot_act() || asleep())
            break;

        int net = get_trapping_net(pos(), true);

        if (net == NON_ITEM) // Really shouldn't happen!
        {
            del_ench(ENCH_HELD);
            break;
        }

        // Handled in handle_pickup().
        if (mons_eats_items(this))
            break;

        // The enchantment doubles as the durability of a net
        // the more corroded it gets, the more easily it will break.
        const int hold = mitm[net].plus; // This will usually be negative.
        const int mon_size = body_size(PSIZE_BODY);

        // Smaller monsters can escape more quickly.
        if (mon_size < random2(SIZE_BIG)  // BIG = 5
            && !berserk() && type != MONS_DANCING_WEAPON)
        {
            if (mons_near(this) && !visible_to(&you))
                mpr("Something wriggles in the net.");
            else
                simple_monster_message(this, " struggles to escape the net.");

            // Confused monsters have trouble finding the exit.
            if (has_ench(ENCH_CONFUSION) && !one_chance_in(5))
                break;

            decay_enchantment(me, 2*(NUM_SIZE_LEVELS - mon_size) - hold);

            // Frayed nets are easier to escape.
            if (mon_size <= -(hold-1)/2)
                decay_enchantment(me, (NUM_SIZE_LEVELS - mon_size));
        }
        else // Large (and above) monsters always thrash the net and destroy it
        {    // e.g. ogre, large zombie (large); centaur, naga, hydra (big).

            if (mons_near(this) && !visible_to(&you))
                mpr("Something wriggles in the net.");
            else
                simple_monster_message(this, " struggles against the net.");

            // Confused monsters more likely to struggle without result.
            if (has_ench(ENCH_CONFUSION) && one_chance_in(3))
                break;

            // Nets get destroyed more quickly for larger monsters
            // and if already strongly frayed.
            int damage = 0;

            // tiny: 1/6, little: 2/5, small: 3/4, medium and above: always
            if (x_chance_in_y(mon_size + 1, SIZE_GIANT - mon_size))
                damage++;

            // Handled specially to make up for its small size.
            if (type == MONS_DANCING_WEAPON)
            {
                damage += one_chance_in(3);

                if (can_cut_meat(mitm[inv[MSLOT_WEAPON]]))
                    damage++;
            }


            // Extra damage for large (50%) and big (always).
            if (mon_size == SIZE_BIG || mon_size == SIZE_LARGE && coinflip())
                damage++;

            // overall damage per struggle:
            // tiny   -> 1/6
            // little -> 2/5
            // small  -> 3/4
            // medium -> 1
            // large  -> 1,5
            // big    -> 2

            // extra damage if already damaged
            if (random2(body_size(PSIZE_BODY) - hold + 1) >= 4)
                damage++;

            // Berserking doubles damage dealt.
            if (berserk())
                damage *= 2;

            // Faster monsters can damage the net more often per
            // time period.
            if (speed != 0)
                damage = div_rand_round(damage * speed, spd);

            mitm[net].plus -= damage;

            if (mitm[net].plus < -7)
            {
                if (mons_near(this))
                {
                    if (visible_to(&you))
                    {
                        mprf("The net rips apart, and %s comes free!",
                             name(DESC_NOCAP_THE).c_str());
                    }
                    else
                    {
                        mpr("All of a sudden the net rips apart!");
                    }
                }
                destroy_item(net);

                del_ench(ENCH_HELD, true);
            }
        }
        break;
    }
    case ENCH_CONFUSION:
        if (!mons_class_flag(type, M_CONFUSED))
            decay_enchantment(me);
        break;

    case ENCH_INVIS:
        if (!mons_class_flag(type, M_INVIS))
            decay_enchantment(me);
        break;

    case ENCH_SUBMERGED:
    {
        // Don't unsubmerge into a harmful cloud
        if (!is_harmless_cloud(cloud_type_at(pos())))
            break;

        // Air elementals are a special case, as their submerging in air
        // isn't up to choice. - bwr
        if (type == MONS_AIR_ELEMENTAL)
        {
            heal(1, one_chance_in(5));

            if (one_chance_in(5))
                del_ench(ENCH_SUBMERGED);

            break;
        }

        // Now we handle the others:
        const dungeon_feature_type grid = grd(pos());

        if (!monster_can_submerge(this, grid))
            del_ench(ENCH_SUBMERGED); // forced to surface
        else if (mons_landlubbers_in_reach(this))
            del_ench(ENCH_SUBMERGED);
        break;
    }
    case ENCH_POISON:
    {
        const int poisonval = me.degree;
        int dam = (poisonval >= 4) ? 1 : 0;

        if (coinflip())
            dam += roll_dice(1, poisonval + 1);

        if (res_poison() < 0)
            dam += roll_dice(2, poisonval) - 1;

        if (dam > 0)
        {
#ifdef DEBUG_DIAGNOSTICS
            // For debugging, we don't have this silent.
            simple_monster_message(this, " takes poison damage.",
                                   MSGCH_DIAGNOSTICS);
            mprf(MSGCH_DIAGNOSTICS, "poison damage: %d", dam);
#endif

            hurt(me.agent(), dam, BEAM_POISON);
        }

        decay_enchantment(me, true);
        break;
    }
    case ENCH_ROT:
    {
        if (hit_points > 1 && one_chance_in(3))
        {
            hurt(me.agent(), 1);
            if (hit_points < max_hit_points && coinflip())
                --max_hit_points;
        }

        decay_enchantment(me, true);
        break;
    }

    // Assumption: monster::res_fire has already been checked.
    case ENCH_STICKY_FLAME:
    {
        if (feat_is_watery(grd(pos())) && ground_level())
        {
            if (mons_near(this) && visible_to(&you))
            {
                mprf("The flames covering %s go out.",
                     name(DESC_NOCAP_THE, false).c_str());
            }
            del_ench(ENCH_STICKY_FLAME);
            break;
        }
        int dam = resist_adjust_damage(this, BEAM_FIRE, res_fire(),
                                       roll_dice(2, 4) - 1);

        if (dam > 0)
        {
            simple_monster_message(this, " burns!");
            dprf("sticky flame damage: %d", dam);

            if (type == MONS_SHEEP)
            {
                for (adjacent_iterator ai(pos()); ai; ++ai)
                {
                    monster *mon = monster_at(*ai);
                    if (mon && mon->type == MONS_SHEEP
                        && !mon->has_ench(ENCH_STICKY_FLAME)
                        && coinflip())
                    {
                        mprf("%s catches fire!", mon->name(DESC_CAP_A).c_str());
                        const int dur = me.degree/2 + 1 + random2(me.degree);
                        mon->add_ench(mon_enchant(ENCH_STICKY_FLAME, dur,
                                                  me.agent()));
                        mon->add_ench(mon_enchant(ENCH_FEAR, dur + random2(20),
                                                  me.agent()));
                        behaviour_event(mon, ME_SCARE, me.who);
                        xom_is_stimulated(128);
                    }
                }
            }

            hurt(me.agent(), dam, BEAM_NAPALM);
        }

        decay_enchantment(me, true);
        break;
    }

    case ENCH_SHORT_LIVED:
        // This should only be used for ball lightning -- bwr
        if (decay_enchantment(me))
            suicide();
        break;

    case ENCH_SLOWLY_DYING:
        // If you are no longer dying, you must be dead.
        if (decay_enchantment(me))
        {
            if (you.see_cell(position))

            {
                if (type == MONS_SALT_PILLAR)
                     mprf("The %s crumbles away.",
                     name(DESC_PLAIN, false).c_str());
                else
                     mprf("A nearby %s withers and dies.",
                     name(DESC_PLAIN, false).c_str());
            }

            monster_die(this, KILL_MISC, NON_MONSTER, true);
        }
        break;

    case ENCH_FADING_AWAY:
        // Summon a nasty!
        if (decay_enchantment(me))
            spirit_fades(this);
        break;

    case ENCH_PREPARING_RESURRECT:
        if (decay_enchantment(me))
            shedu_do_actual_resurrection(this);
        break;

    case ENCH_SPORE_PRODUCTION:
        // Reduce the timer, if that means we lose the enchantment then
        // spawn a spore and re-add the enchantment.
        if (decay_enchantment(me))
        {
            // Search for an open adjacent square to place a spore on
            int idx[] = {0, 1, 2, 3, 4, 5, 6, 7};
            std::random_shuffle(idx, idx + 8);

            bool re_add = true;

            for (unsigned i = 0; i < 8; ++i)
            {
                coord_def adjacent = pos() + Compass[idx[i]];

                if (mons_class_can_pass(MONS_GIANT_SPORE, env.grid(adjacent))
                                        && !actor_at(adjacent))
                {
                    beh_type created_behavior = SAME_ATTITUDE(this);

                    int rc = create_monster(mgen_data(MONS_GIANT_SPORE,
                                                      created_behavior,
                                                      NULL,
                                                      0,
                                                      0,
                                                      adjacent,
                                                      MHITNOT,
                                                      MG_FORCE_PLACE));

                    if (rc != -1)
                    {
                        env.mons[rc].behaviour = BEH_WANDER;
                        env.mons[rc].number = 20;

                        if (you.see_cell(adjacent) && you.see_cell(pos()))
                            mpr("A ballistomycete spawns a giant spore.");

                        // Decrease the count and maybe become inactive
                        // again.
                        if (number)
                        {
                            number--;
                            if (number == 0)
                            {
                                colour = MAGENTA;
                                del_ench(ENCH_SPORE_PRODUCTION);
                                re_add = false;
                            }
                        }

                    }
                    break;
                }
            }
            // Re-add the enchantment (this resets the spore production
            // timer).
            if (re_add)
                add_ench(ENCH_SPORE_PRODUCTION);
        }

        break;

    case ENCH_EXPLODING:
    {
        // Reduce the timer, if that means we lose the enchantment then
        // spawn a spore and re-add the enchantment
        if (decay_enchantment(me))
        {
            monster_type mtype = type;
            bolt beam;

            setup_spore_explosion(beam, *this);

            beam.explode();

            // The ballisto dying, then a spore being created in its slot
            // env.mons means we can appear to be alive, but in fact be
            // an entirely different monster.
            if (alive() && type == mtype)
            {
                add_ench(ENCH_EXPLODING);
            }
        }

    }
    break;

    case ENCH_PORTAL_TIMER:
    {
        if (decay_enchantment(me))
        {
            coord_def base_position = this->props["base_position"].get_coord();
            // Do a thing.
            if (you.see_cell(base_position))
            {
                mprf("The portal closes; %s is severed.", name(DESC_NOCAP_THE).c_str());
            }

            if (env.grid(base_position) == DNGN_TEMP_PORTAL)
            {
                env.grid(base_position) = DNGN_FLOOR;
            }

            env.pgrid(base_position) |= FPROP_BLOODY;
            add_ench(ENCH_SEVERED);

            // Severed tentacles immediately become "hostile" to everyone (or insane)
            this->attitude = ATT_NEUTRAL;
            behaviour_event(this, ME_ALERT, MHITNOT);
        }
    }
    break;

    case ENCH_PORTAL_PACIFIED:
    {
        if (decay_enchantment(me))
        {
            if (this->has_ench(ENCH_SEVERED))
                break;

            if (!this->friendly())
                break;

            if (!silenced(you.pos()))
            {
                if (you.can_see(this))
                    simple_monster_message(this, " suddenly becomes enraged!");
                else
                    mpr("You hear a distant and violent thrashing sound.");
            }

            this->attitude = ATT_HOSTILE;
            behaviour_event(this, ME_ALERT, MHITYOU);
        }
    }
    break;


    case ENCH_SEVERED:
    {
        simple_monster_message(this, " writhes!");
        coord_def base_position = props["base_position"].get_coord();
        dungeon_feature_type ftype = env.grid(base_position);
        if (feat_has_solid_floor(ftype)
            && !feat_is_watery(ftype))
        {
            env.pgrid(base_position) |= FPROP_BLOODY;
        }
        hurt(me.agent(), 20);
    }

    break;

    case ENCH_GLOWING_SHAPESHIFTER: // This ench never runs out!
        // Number of actions is fine for shapeshifters.  Don't change
        // shape while taking the stairs because monster_polymorph() has
        // an assert about it. -cao
        if (!(flags & MF_TAKING_STAIRS) && !asleep()
            && (type == MONS_GLOWING_SHAPESHIFTER
                || one_chance_in(4)))
        {
            monster_polymorph(this, RANDOM_MONSTER);
        }
        break;

    case ENCH_SHAPESHIFTER:         // This ench never runs out!
        if (!(flags & MF_TAKING_STAIRS) && !asleep()
            && (type == MONS_SHAPESHIFTER
                || x_chance_in_y(1000 / (15 * hit_dice / 5), 1000)))
        {
            monster_polymorph(this, RANDOM_MONSTER);
        }
        break;

    case ENCH_TP:
        if (decay_enchantment(me, true))
            monster_teleport(this, true);
        break;

    case ENCH_SLEEPY:
        // deleted separately at end of monster turn; otherwise
        // other monsters can wake up this one on their moves
        break;

    case ENCH_EAT_ITEMS:
        break;

    case ENCH_AWAKEN_FOREST:
        forest_damage(this);
        decay_enchantment(me);
        break;

    case ENCH_PERM_TORNADO:
        tornado_damage(this, speed_to_duration(speed));
        break;

    case ENCH_BLEED:
    {
        // 3, 6, 9% of current hp
        int dam = div_rand_round(random2((1 + hit_points)*(me.degree * 3)),100);

        if (dam < hit_points)
        {
            hurt(me.agent(), dam);

            dprf("hit_points: %d ; bleed damage: %d ; degree: %d",
                 hit_points, dam, me.degree);
        }

        decay_enchantment(me, true);
        break;
    }

    //This is like Corona, but if silver harms them, it sticky flame levels of damage.
    case ENCH_SILVER_CORONA:

        if ((this->holiness() == MH_UNDEAD && !this->is_insubstantial()) || this->is_chaotic())
        {
            bolt beam;
            beam.flavour = BEAM_LIGHT;
            int dam = roll_dice(2, 4) - 1;

            //Double damage to vampires - it's silver and it's light!

            int newdam = mons_adjust_flavoured(this, beam, dam, false);

            if (newdam > 0)
            {
                std::string msg = mons_has_flesh(this) ? "'s flesh" : "";
                msg += (dam < newdam) ? " is horribly charred!"
                                      : " is seared.";
                simple_monster_message(this, msg.c_str());
            }

            dprf("Zin's Corona damage: %d", dam);
            hurt(me.agent(), dam, BEAM_LIGHT);
        }

        decay_enchantment(me, true);
        break;

    default:
        break;
    }
}

void monster::mark_summoned(int longevity, bool mark_items, int summon_type)
{
    add_ench(mon_enchant(ENCH_ABJ, longevity));
    if (summon_type != 0)
        add_ench(mon_enchant(ENCH_SUMMON, summon_type, 0, INT_MAX));

    if (mark_items)
    {
        for (int i = 0; i < NUM_MONSTER_SLOTS; ++i)
        {
            const int item = inv[i];
            if (item != NON_ITEM)
                mitm[item].flags |= ISFLAG_SUMMONED;
        }
    }
}

bool monster::is_summoned(int* duration, int* summon_type) const
{
    const mon_enchant abj = get_ench(ENCH_ABJ);
    if (abj.ench == ENCH_NONE)
    {
        if (duration != NULL)
            *duration = -1;
        if (summon_type != NULL)
            *summon_type = 0;

        return (false);
    }
    if (duration != NULL)
        *duration = abj.duration;

    const mon_enchant summ = get_ench(ENCH_SUMMON);
    if (summ.ench == ENCH_NONE)
    {
        if (summon_type != NULL)
            *summon_type = 0;

        return (true);
    }
    if (summon_type != NULL)
        *summon_type = summ.degree;

    if (mons_is_conjured(type))
        return (false);

    switch (summ.degree)
    {
    // Temporarily dancing weapons are really there.
    case SPELL_TUKIMAS_DANCE:

    // A corpse/skeleton which was temporarily animated.
    case SPELL_ANIMATE_DEAD:
    case SPELL_ANIMATE_SKELETON:

    // Conjured stuff (fire vortices, ball lightning, IOOD) is handled above.

    // Clones aren't really summoned (though their equipment might be).
    case MON_SUMM_CLONE:

    // Nor are body parts.
    case SPELL_KRAKEN_TENTACLES:

    // Some object which was animated, and thus not really summoned.
    case MON_SUMM_ANIMATE:
        return (false);
    }

    return (true);
}

void monster::apply_enchantments()
{
    if (enchantments.empty())
        return;

    // The ordering in enchant_type makes sure that "super-enchantments"
    // like berserk time out before their parts.
    const mon_enchant_list ec = enchantments;
    for (mon_enchant_list::const_iterator i = ec.begin(); i != ec.end(); ++i)
    {
        apply_enchantment(i->second);
        if (!alive())
            break;
    }
}

void monster::scale_hp(int num, int den)
{
    // Without the +1, we lose maxhp on every berserk (the only use) if the
    // maxhp is odd.  This version does preserve the value correctly, but only
    // if it is first inflated then deflated.
    hit_points     = (hit_points * num + 1) / den;
    max_hit_points = (max_hit_points * num + 1) / den;

    if (hit_points < 1)
        hit_points = 1;
    if (max_hit_points < 1)
        max_hit_points = 1;
    if (hit_points > max_hit_points)
        hit_points = max_hit_points;
}

kill_category monster::kill_alignment() const
{
    return (friendly() ? KC_FRIENDLY : KC_OTHER);
}

bool monster::sicken(int amount, bool unused)
{
    UNUSED(unused);

    if (res_rotting() || (amount /= 2) < 1)
        return (false);

    if (!has_ench(ENCH_SICK) && you.can_see(this))
    {
        // Yes, could be confused with poisoning.
        mprf("%s looks sick.", name(DESC_CAP_THE).c_str());
    }

    add_ench(mon_enchant(ENCH_SICK, 0, 0, amount * 10));

    return (true);
}

bool monster::bleed(const actor* agent, int amount, int degree)
{
    if (!has_ench(ENCH_BLEED) && you.can_see(this))
    {
        mprf("%s begins to bleed from %s wounds!", name(DESC_CAP_THE).c_str(),
             pronoun(PRONOUN_NOCAP_POSSESSIVE).c_str());
    }

    add_ench(mon_enchant(ENCH_BLEED, degree, agent, amount * 10));

    return (true);
}

// Recalculate movement speed.
void monster::calc_speed()
{
    speed = mons_real_base_speed(type);

    bool is_liquefied = (liquefied(pos()) && ground_level()
                         && !is_insubstantial());

    // Going berserk on liquid ground doesn't speed you up any.
    if (!is_liquefied && (has_ench(ENCH_BERSERK) || has_ench(ENCH_INSANE)))
        speed *= 2;
    else if (has_ench(ENCH_HASTE))
        speed = haste_mul(speed);
    if (has_ench(ENCH_SLOW))
        speed = haste_div(speed);
}

// Check speed and speed_increment sanity.
void monster::check_speed()
{
    // FIXME: If speed is borked, recalculate.  Need to figure out how
    // speed is getting borked.
    if (speed < 0 || speed > 130)
    {
#ifdef DEBUG_DIAGNOSTICS
        mprf(MSGCH_DIAGNOSTICS,
             "Bad speed: %s, spd: %d, spi: %d, hd: %d, ench: %s",
             name(DESC_PLAIN).c_str(),
             speed, speed_increment, hit_dice,
             describe_enchantments().c_str());
#endif

        calc_speed();

#ifdef DEBUG_DIAGNOSTICS
        mprf(MSGCH_DIAGNOSTICS, "Fixed speed for %s to %d",
             name(DESC_PLAIN).c_str(), speed);
#endif
    }

    if (speed_increment < 0)
        speed_increment = 0;

    if (speed_increment > 200)
    {
#ifdef DEBUG_DIAGNOSTICS
        mprf(MSGCH_DIAGNOSTICS,
             "Clamping speed increment on %s: %d",
             name(DESC_PLAIN).c_str(), speed_increment);
#endif
        speed_increment = 140;
    }
}

actor *monster::get_foe() const
{
    if (foe == MHITNOT)
        return (NULL);
    else if (foe == MHITYOU)
        return (friendly() ? NULL : &you);

    // Must be a monster!
    monster* my_foe = &menv[foe];
    return (my_foe->alive()? my_foe : NULL);
}

int monster::foe_distance() const
{
    const actor *afoe = get_foe();
    return (afoe ? pos().distance_from(afoe->pos())
                 : INFINITE_DISTANCE);
}

bool monster::can_go_berserk() const
{
    if (holiness() != MH_NATURAL || mons_is_tentacle(type))
        return (false);

    if (mons_intel(this) == I_PLANT)
        return (false);

    if (berserk() || has_ench(ENCH_FATIGUE))
        return (false);

    // If we have no melee attack, going berserk is pointless.
    const mon_attack_def attk = mons_attack_spec(this, 0);
    if (attk.type == AT_NONE || attk.damage == 0)
        return (false);

    return (true);
}

bool monster::frenzied() const
{
    return (has_ench(ENCH_INSANE));
}

bool monster::berserk() const
{
    return (has_ench(ENCH_BERSERK) || has_ench(ENCH_INSANE));
}

bool monster::needs_berserk(bool check_spells) const
{
    if (!can_go_berserk())
        return (false);

    if (has_ench(ENCH_HASTE) || has_ench(ENCH_TP))
        return (false);

    if (foe_distance() > 3)
        return (false);

    if (check_spells)
    {
        for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
        {
            const int spell = spells[i];
            if (spell != SPELL_NO_SPELL && spell != SPELL_BERSERKER_RAGE)
                return (false);
        }
    }

    return (true);
}

bool monster::can_see_invisible() const
{
    if (mons_is_ghost_demon(type))
        return (ghost->see_invis);
    else if (mons_class_flag(type, M_SEE_INVIS))
        return (true);
    else if (scan_mon_inv_randarts(this, ARTP_EYESIGHT) > 0)
        return (true);
    return (false);
}

bool monster::invisible() const
{
    return (has_ench(ENCH_INVIS) && !backlit());
}

bool monster::visible_to(const actor *looker) const
{
    bool sense_invis = looker->atype() == ACT_MONSTER
                       && mons_sense_invis(looker->as_monster());

    bool blind = looker->atype() == ACT_MONSTER
                 && looker->as_monster()->has_ench(ENCH_BLIND);

    bool vis = !blind && !invisible() || looker->can_see_invisible()
               || sense_invis && adjacent(pos(), looker->pos());

    return (vis && (this == looker || !submerged()));
}

bool monster::near_foe() const
{
    const actor *afoe = get_foe();
    return (afoe && see_cell(afoe->pos()));
}

bool monster::has_lifeforce() const
{
    const mon_holy_type holi = holiness();

    return (holi == MH_NATURAL || holi == MH_PLANT);
}

bool monster::can_mutate() const
{
    return (has_lifeforce());
}

bool monster::can_safely_mutate() const
{
    return (can_mutate());
}

bool monster::can_bleed() const
{
    return (mons_has_blood(type));
}

bool monster::mutate()
{
    if (!can_mutate())
        return (false);

    // Polymorphing a (very) ugly thing will mutate it into a different
    // (very) ugly thing.
    if (type == MONS_UGLY_THING || type == MONS_VERY_UGLY_THING)
    {
        ugly_thing_mutate(this);
        return (true);
    }

    // Polymorphing a shapeshifter will make it revert to its original
    // form.
    if (has_ench(ENCH_GLOWING_SHAPESHIFTER))
        return (monster_polymorph(this, MONS_GLOWING_SHAPESHIFTER));
    if (has_ench(ENCH_SHAPESHIFTER))
        return (monster_polymorph(this, MONS_SHAPESHIFTER));

    // Polymorphing a slime creature will usually split it first
    // and polymorph each part separately.
    if (type == MONS_SLIME_CREATURE)
    {
        slime_creature_mutate(this);
        return (true);
    }

    return (monster_polymorph(this, RANDOM_MONSTER));
}

static bool _mons_is_icy(int mc)
{
    return (mc == MONS_ICE_BEAST
            || mc == MONS_SIMULACRUM_SMALL
            || mc == MONS_SIMULACRUM_LARGE
            || mc == MONS_ICE_STATUE);
}

bool monster::is_icy() const
{
    return (_mons_is_icy(type));
}

static bool _mons_is_fiery(int mc)
{
    return (mc == MONS_FIRE_VORTEX
            || mc == MONS_FIRE_ELEMENTAL
            || mc == MONS_FLAMING_CORPSE
            || mc == MONS_EFREET
            || mc == MONS_AZRAEL
            || mc == MONS_LAVA_WORM
            || mc == MONS_LAVA_FISH
            || mc == MONS_LAVA_SNAKE
            || mc == MONS_SALAMANDER
            || mc == MONS_MOLTEN_GARGOYLE
            || mc == MONS_ORB_OF_FIRE);
}

bool monster::is_fiery() const
{
    return (_mons_is_fiery(type));
}

static bool _mons_is_skeletal(int mc)
{
    return (mc == MONS_SKELETON_SMALL
            || mc == MONS_SKELETON_LARGE
            || mc == MONS_BONE_DRAGON
            || mc == MONS_SKELETAL_WARRIOR
            || mc == MONS_FLYING_SKULL);
}

bool monster::is_skeletal() const
{
    return (_mons_is_skeletal(type));
}

bool monster::has_action_energy() const
{
    return (speed_increment >= 80);
}

void monster::check_redraw(const coord_def &old, bool clear_tiles) const
{
    if (!crawl_state.io_inited)
        return;

    const bool see_new = you.see_cell(pos());
    const bool see_old = you.see_cell(old);
    if ((see_new || see_old) && !view_update())
    {
        if (see_new)
            view_update_at(pos());

        // Don't leave a trail if we can see the monster move in.
        if (see_old || (pos() - old).rdist() <= 1)
        {
            view_update_at(old);
#ifdef USE_TILE
            if (clear_tiles && !see_old)
                tile_clear_monster(old);
#endif
        }
        update_screen();
    }
}

void monster::apply_location_effects(const coord_def &oldpos,
                                     killer_type killer,
                                     int killernum)
{
    if (oldpos != pos())
        dungeon_events.fire_position_event(DET_MONSTER_MOVED, pos());

    if (alive() && mons_habitat(this) == HT_WATER
        && !feat_is_watery(grd(pos())) && !has_ench(ENCH_AQUATIC_LAND))
    {
        add_ench(ENCH_AQUATIC_LAND);
    }

    if (alive() && has_ench(ENCH_AQUATIC_LAND))
    {
        if (!feat_is_watery(grd(pos())))
            simple_monster_message(this, " flops around on dry land!");
        else if (!feat_is_watery(grd(oldpos)))
        {
            simple_monster_message(this, " dives back into the water!");
            del_ench(ENCH_AQUATIC_LAND);
        }
        // This may have been called via dungeon_terrain_changed instead
        // of by the monster moving move, in that case grd(oldpos) will
        // be the current position that became watery.
        else
            del_ench(ENCH_AQUATIC_LAND);
    }

    // Monsters stepping on traps:
    trap_def* ptrap = find_trap(pos());
    if (ptrap)
        ptrap->trigger(*this);

    if (alive())
        mons_check_pool(this, pos(), killer, killernum);

    if (alive() && has_ench(ENCH_SUBMERGED)
        && (!monster_can_submerge(this, grd(pos()))
            || type == MONS_TRAPDOOR_SPIDER))
    {
        del_ench(ENCH_SUBMERGED);

        if (type == MONS_TRAPDOOR_SPIDER)
            behaviour_event(this, ME_EVAL);
    }

    terrain_property_t &prop = env.pgrid(pos());

    if (prop & FPROP_BLOODY)
    {
        monster_type genus = mons_genus(type);

        if (genus == MONS_JELLY || genus == MONS_GIANT_SLUG)
        {
            prop &= ~FPROP_BLOODY;
            if (you.see_cell(pos()) && !visible_to(&you))
            {
               std::string desc =
                   feature_description(pos(), false, DESC_NOCAP_THE, false);
               mprf("The bloodstain on %s disappears!", desc.c_str());
            }
        }
    }
}

bool monster::move_to_pos(const coord_def &newpos, bool clear_net)
{
    const actor* a = actor_at(newpos);
    if (a && (a != &you || !fedhas_passthrough(this)))
        return (false);

    const int index = mindex();

    // Clear old cell pointer.
    if (in_bounds(pos()) && mgrd(pos()) == index)
        mgrd(pos()) = NON_MONSTER;

    // Set monster x,y to new value.
    moveto(newpos, clear_net);

    // Set new monster grid pointer to this monster.
    mgrd(newpos) = index;

    return (true);
}

// Returns true if the trap should be revealed to the player.
bool monster::do_shaft()
{
    if (!is_valid_shaft_level())
        return (false);

    // Handle instances of do_shaft() being invoked magically when
    // the monster isn't standing over a shaft.
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

        if (!ground_level() || total_weight() == 0)
        {
            if (mons_near(this))
            {
                if (visible_to(&you))
                {
                    mprf("A shaft briefly opens up underneath %s!",
                         name(DESC_NOCAP_THE).c_str());
                }
                else
                    mpr("A shaft briefly opens up in the floor!");
            }

            handle_items_on_shaft(pos(), false);
            return (false);
        }
    }

    level_id lev = shaft_dest(false);

    if (lev == level_id::current())
        return (false);

    // If a pacified monster is leaving the level via a shaft trap, and
    // has reached its goal, handle it here.
    if (!pacified())
        set_transit(lev);

    const bool reveal =
        simple_monster_message(this, " falls through a shaft!");

    handle_items_on_shaft(pos(), false);

    // Monster is no longer on this level.
    destroy_inventory();
    monster_cleanup(this);

    return (reveal);
}

void monster::hibernate(int)
{
    if (!can_hibernate())
        return;

    behaviour = BEH_SLEEP;
    add_ench(ENCH_SLEEPY);
    add_ench(ENCH_SLEEP_WARY);
}

void monster::put_to_sleep(actor *attacker, int strength)
{
    if (!can_sleep())
        return;

    behaviour = BEH_SLEEP;
    add_ench(ENCH_SLEEPY);
}

void monster::check_awaken(int)
{
    // XXX
}

int monster::beam_resists(bolt &beam, int hurted, bool doEffects,
                          std::string source)
{
    return mons_adjust_flavoured(this, beam, hurted, doEffects);
}

const monsterentry *monster::find_monsterentry() const
{
    return (type == MONS_NO_MONSTER || type == MONS_PROGRAM_BUG) ? NULL
                                                    : get_monster_data(type);
}

monster_type monster::get_mislead_type() const
{
    if (props.exists("mislead_as"))
        return static_cast<monster_type>(props["mislead_as"].get_short());
    else
        return type;
}

int monster::action_energy(energy_use_type et) const
{
    if (const monsterentry *me = find_monsterentry())
    {
        const mon_energy_usage &mu = me->energy_usage;
        int move_cost = 0;
        switch (et)
        {
        case EUT_MOVE:    move_cost = mu.move; break;
        // Amphibious monster speed boni are now dealt with using SWIM_ENERGY,
        // rather than here.
        case EUT_SWIM:    move_cost = mu.swim; break;
        case EUT_MISSILE: return mu.missile;
        case EUT_ITEM:    return mu.item;
        case EUT_SPECIAL: return mu.special;
        case EUT_SPELL:   return mu.spell;
        case EUT_ATTACK:  return mu.attack;
        case EUT_PICKUP:  return mu.pickup_percent;
        }

        if (has_ench(ENCH_SWIFT))
            move_cost -= 2;

        // Floundering monsters get the same penalty as the player, except that
        // player get penalty on entering water, while monster get the penalty
        // when leaving it.
        if (floundering())
            move_cost += 3 + random2(8);

        // If the monster cast it, it has more control and is there not
        // as slow as when the player casts it.
        if (has_ench(ENCH_LIQUEFYING))
            move_cost -= 2;

        return move_cost;
    }
    return 10;
}

void monster::lose_energy(energy_use_type et, int div, int mult)
{
    int energy_loss  = div_round_up(mult * action_energy(et), div);
    if (has_ench(ENCH_PETRIFYING))
    {
        energy_loss *= 3;
        energy_loss /= 2;
    }

    // Randomize movement cost slightly, to make it less predictable,
    // and make pillar-dancing not entirely safe.
    if (et == EUT_MOVE || et == EUT_SWIM)
        energy_loss += random2(3) - 1;

    speed_increment -= energy_loss;
}

bool monster::can_drink_potion(potion_type ptype) const
{
    if (mons_class_is_stationary(type))
        return (false);

    if (mons_itemuse(this) < MONUSE_STARTING_EQUIPMENT)
        return (false);

    // These monsters cannot drink.
    if (is_skeletal() || is_insubstantial()
        || mons_species() == MONS_LICH || mons_genus(type) == MONS_MUMMY
        || type == MONS_GASTRONOK)
    {
        return (false);
    }

    switch (ptype)
    {
        case POT_HEALING:
        case POT_HEAL_WOUNDS:
            return (holiness() != MH_NONLIVING
                    && holiness() != MH_PLANT);
        case POT_BLOOD:
        case POT_BLOOD_COAGULATED:
            return (mons_species() == MONS_VAMPIRE);
        case POT_SPEED:
        case POT_MIGHT:
        case POT_BERSERK_RAGE:
        case POT_INVISIBILITY:
            // If there are any item using monsters that are permanently
            // invisible, this might have to be restricted.
            return (true);
        default:
            break;
    }

    return (false);
}

bool monster::should_drink_potion(potion_type ptype) const
{
    switch (ptype)
    {
    case POT_HEALING:
        return (hit_points <= max_hit_points / 2)
                || has_ench(ENCH_POISON)
                || has_ench(ENCH_SICK)
                || has_ench(ENCH_CONFUSION)
                || has_ench(ENCH_ROT);
    case POT_HEAL_WOUNDS:
        return (hit_points <= max_hit_points / 2);
    case POT_BLOOD:
    case POT_BLOOD_COAGULATED:
        return (hit_points <= max_hit_points / 2);
    case POT_SPEED:
        return (!has_ench(ENCH_HASTE));
    case POT_MIGHT:
        return (!has_ench(ENCH_MIGHT) && foe_distance() <= 2);
    case POT_BERSERK_RAGE:
        // this implies !berserk()
        return (!has_ench(ENCH_MIGHT) && !has_ench(ENCH_HASTE)
                && needs_berserk());
    case POT_INVISIBILITY:
        // We're being nice: friendlies won't go invisible if the player
        // won't be able to see them.
        return (!has_ench(ENCH_INVIS)
                && (you.can_see_invisible(false) || !friendly()));
    default:
        break;
    }

    return (false);
}

// Return the ID status gained.
item_type_id_state_type monster::drink_potion_effect(potion_type pot_eff)
{
    simple_monster_message(this, " drinks a potion.");

    item_type_id_state_type ident = ID_MON_TRIED_TYPE;

    switch (pot_eff)
    {
    case POT_HEALING:
    {
        heal(5 + random2(7));
        simple_monster_message(this, " is healed!");

        const enchant_type cured_enchants[] = {
            ENCH_POISON, ENCH_SICK, ENCH_CONFUSION, ENCH_ROT
        };

        // We can differentiate healing and heal wounds (and blood,
        // for vampires) by seeing if any status ailments are cured.
        for (unsigned int i = 0; i < ARRAYSZ(cured_enchants); ++i)
            if (del_ench(cured_enchants[i]))
                ident = ID_KNOWN_TYPE;
    }
    break;

    case POT_HEAL_WOUNDS:
        heal(10 + random2avg(28, 3));
        simple_monster_message(this, " is healed!");
        break;

    case POT_BLOOD:
    case POT_BLOOD_COAGULATED:
        if (mons_species() == MONS_VAMPIRE)
        {
            heal(10 + random2avg(28, 3));
            simple_monster_message(this, " is healed!");
        }
        break;

    case POT_SPEED:
        if (enchant_monster_with_flavour(this, this, BEAM_HASTE))
            ident = ID_KNOWN_TYPE;
        break;

    case POT_INVISIBILITY:
        if (enchant_monster_with_flavour(this, this, BEAM_INVISIBILITY))
            ident = ID_KNOWN_TYPE;
        break;

    case POT_MIGHT:
        if (enchant_monster_with_flavour(this, this, BEAM_MIGHT))
            ident = ID_KNOWN_TYPE;
        break;

    case POT_BERSERK_RAGE:
        if (enchant_monster_with_flavour(this, this, BEAM_BERSERK))
            ident = ID_KNOWN_TYPE;
        break;

    default:
        break;
    }

    return (ident);
}

void monster::react_to_damage(const actor *oppressor, int damage,
                               beam_type flavour)
{
    if (type == MONS_SIXFIRHY && flavour == BEAM_ELECTRICITY)
    {
        if (!alive()) // overcharging is deadly
            simple_monster_message(this,
                                   " explodes in a shower of sparks!");
        else if (heal(damage*2, false))
            simple_monster_message(this, " seems to be charged up!");
        return;
    }

    if (!alive())
        return;

    // The royal jelly objects to taking damage and will SULK. :-)
    if (type == MONS_ROYAL_JELLY)
    {
        int lobes = hit_points / 12;
        int oldlobes = (hit_points + damage) / 12;

        if (lobes == oldlobes)
            return;

        mon_acting mact(this);

        const int tospawn = oldlobes - lobes;
#ifdef DEBUG_DIAGNOSTICS
        mprf(MSGCH_DIAGNOSTICS, "Trying to spawn %d jellies.", tospawn);
#endif
        const beh_type beha = SAME_ATTITUDE(this);
        int spawned = 0;
        for (int i = 0; i < tospawn; ++i)
        {
            const monster_type jelly = royal_jelly_ejectable_monster();
            coord_def jpos = find_newmons_square_contiguous(jelly, pos());
            if (!in_bounds(jpos))
                continue;

            const int nmons = mons_place(
                                  mgen_data(jelly, beha, this, 0, 0,
                                            jpos, foe, 0, god));

            if (nmons != -1 && nmons != NON_MONSTER)
            {
                // Don't allow milking the royal jelly.
                menv[nmons].flags |= MF_NO_REWARD;
                spawned++;
            }
        }

        const bool needs_message = spawned && mons_near(this)
                                   && visible_to(&you);

        if (needs_message)
        {
            const std::string monnam = name(DESC_CAP_THE);
            mprf("%s shudders%s.", monnam.c_str(),
                 spawned >= 5 ? " alarmingly" :
                 spawned >= 3 ? " violently" :
                 spawned > 1 ? " vigorously" : "");

            if (spawned == 1)
                mprf("%s spits out another jelly.", monnam.c_str());
            else
            {
                mprf("%s spits out %s more jellies.",
                     monnam.c_str(),
                     number_in_words(spawned).c_str());
            }
        }
    }
    else if (type == MONS_KRAKEN_TENTACLE && flavour != BEAM_TORMENT_DAMAGE)
    {
        if (!invalid_monster_index(number)
            && mons_base_type(&menv[number]) == MONS_KRAKEN)
        {
            menv[number].hurt(oppressor, damage, flavour);

            // We could be removed, undo this or certain post-hit
            // effects will cry.
            if (invalid_monster(this))
            {
                type = MONS_KRAKEN_TENTACLE;
                hit_points = -1;
            }
        }
    }
    else if (type == MONS_KRAKEN_TENTACLE_SEGMENT)
    {
        if (!invalid_monster_index(number)
            && mons_base_type(&menv[number]) == MONS_KRAKEN_TENTACLE)
        {

            // If we are going to die, monster_die hook will handle
            // purging the tentacle.
            if (hit_points < menv[number].hit_points
                && this->hit_points > 0)
            {
                int pass_damage = menv[number].hit_points -  hit_points;
                menv[number].hurt(oppressor, pass_damage, flavour);

                // We could be removed, undo this or certain post-hit
                // effects will cry.
                if (invalid_monster(this))
                {
                    type = MONS_KRAKEN_TENTACLE_SEGMENT;
                    hit_points = -1;
                }
            }
        }
    }
    else if (type == MONS_ELDRITCH_TENTACLE_SEGMENT)
    {
        //mprf("in tentacle damage pass");
        if (!invalid_monster_index(number)
            && mons_base_type(&menv[number]) == MONS_ELDRITCH_TENTACLE)
        {
            if (hit_points < menv[number].hit_points)
            {
                int pass_damage = menv[number].hit_points -  hit_points;
                menv[number].hurt(oppressor, pass_damage, flavour);

                // We could be removed, undo this or certain post-hit
                // effects will cry.
                if (invalid_monster(this))
                {
                    type = MONS_ELDRITCH_TENTACLE_SEGMENT;
                    hit_points = -1;
                }
            }
        }
    }
    else if (type == MONS_BUSH && flavour == BEAM_FIRE
             && damage>8 && x_chance_in_y(damage, 20))
    {
        place_cloud(CLOUD_FIRE, pos(), 20+random2(15), oppressor, 5);
    }
    else if (type == MONS_SPRIGGAN_RIDER)
    {
        if (hit_points + damage > max_hit_points / 2)
            damage = max_hit_points / 2 - hit_points;
        if (damage > 0 && x_chance_in_y(damage, damage + hit_points))
        {
            bool fly_died = coinflip();
            int old_hp                = hit_points;
            uint64_t old_flags        = flags;
            mon_enchant_list old_ench = enchantments;
            int8_t old_ench_countdown = ench_countdown;

            if (!fly_died)
                monster_drop_things(this, mons_aligned(oppressor, &you));

            type = fly_died ? MONS_SPRIGGAN : MONS_FIREFLY;
            define_monster(this);
            hit_points = std::min(old_hp, hit_points);
            flags          = old_flags;
            enchantments   = old_ench;
            ench_countdown = old_ench_countdown;

            mounted_kill(this, fly_died ? MONS_FIREFLY : MONS_SPRIGGAN,
                !oppressor ? KILL_MISC
                : (oppressor->atype() == ACT_PLAYER)
                  ? KILL_YOU : KILL_MON,
                (oppressor && oppressor->atype() == ACT_MONSTER)
                  ? oppressor->mindex() : NON_MONSTER);

            if (fly_died && !is_habitable(pos()))
            {
                hit_points = 0;
                if (observable())
                    mprf("As %s mount dies, %s plunges down into %s!",
                         pronoun(PRONOUN_NOCAP_POSSESSIVE).c_str(),
                         name(DESC_NOCAP_THE).c_str(),
                         grd(pos()) == DNGN_LAVA ?
                             "lava and is incinerated" :
                             "deep water and drowns");
            }
            else if (fly_died && observable())
            {
                mprf("%s jumps down from %s now dead mount.",
                     name(DESC_CAP_THE).c_str(),
                     pronoun(PRONOUN_NOCAP_POSSESSIVE).c_str());
            }
        }
    }
}

reach_type monster::reach_range() const
{
    const item_def *wpn = primary_weapon();
    const mon_attack_def attk(mons_attack_spec(this, 0));

    if (wpn && get_weapon_brand(*wpn) == SPWPN_REACHING)
        return (REACH_TWO);
    if (attk.flavour == AF_REACH && attk.damage)
        return (REACH_KNIGHT);
    return (REACH_NONE);
}

bool monster::can_cling_to_walls() const
{
    return (mons_genus(type) == MONS_SPIDER || type == MONS_GIANT_GECKO
            || type == MONS_GIANT_COCKROACH || type == MONS_GIANT_MITE
            || type == MONS_DEMONIC_CRAWLER);
}

/////////////////////////////////////////////////////////////////////////
// mon_enchant

static const char *enchant_names[] =
{
    "none", "berserk", "haste", "might", "fatigue", "slow", "fear",
    "confusion", "invis", "poison", "rot", "summon", "abj", "corona",
    "charm", "sticky_flame", "glowing_shapeshifter", "shapeshifter", "tp",
    "sleep_wary", "submerged", "short_lived", "paralysis", "sick",
    "sleepy", "held", "battle_frenzy", "temp_pacif", "petrifying",
    "petrified", "lowered_mr", "soul_ripe", "slowly_dying", "eat_items",
    "aquatic_land", "spore_production", "slouch", "swift", "tide",
    "insane", "silenced", "awaken_forest", "exploding", "bleeding",
    "tethered", "severed", "antimagic", "fading_away", "preparing_resurrect", "regen",
    "magic_res", "mirror_dam", "stoneskin", "fear inspiring", "temporarily pacified",
    "withdrawn", "attached", "guardian_timer", "levitation",
    "helpless", "liquefying", "perm_tornado", "fake_abjuration",
    "dazed", "mute", "blind", "dumb", "mad", "silver_corona", "recite timer",
    "buggy",
};

static const char *_mons_enchantment_name(enchant_type ench)
{
    COMPILE_CHECK(ARRAYSZ(enchant_names) == NUM_ENCHANTMENTS+1, c1);

    if (ench > NUM_ENCHANTMENTS)
        ench = NUM_ENCHANTMENTS;

    return (enchant_names[ench]);
}

enchant_type name_to_ench(const char *name)
{
    for (unsigned int i = ENCH_NONE; i < ARRAYSZ(enchant_names); i++)
        if (!strcmp(name, enchant_names[i]))
            return (enchant_type)i;
    return ENCH_NONE;
}

mon_enchant::mon_enchant(enchant_type e, int deg, const actor* a,
                         int dur)
    : ench(e), degree(deg), duration(dur), maxduration(0)
{
    if (a)
    {
        who = a->kill_alignment();
        source = a->mid;
    }
    else
    {
        who = KC_OTHER;
        source = 0;
    }
}

mon_enchant::operator std::string () const
{
    const actor *a = agent();
    return make_stringf("%s (%d:%d%s %s)",
                        _mons_enchantment_name(ench),
                        degree,
                        duration,
                        kill_category_desc(who),
                        source == MID_ANON_FRIEND ? "anon friend" :
                            a ? a->name(DESC_PLAIN, true).c_str() : "N/A");
}

const char *mon_enchant::kill_category_desc(kill_category k) const
{
    return (k == KC_YOU?      " you" :
            k == KC_FRIENDLY? " pet" : "");
}

void mon_enchant::merge_killer(kill_category k, mid_t m)
{
    if (who >= k) // prefer the new one
        who = k, source = m;
}

void mon_enchant::cap_degree()
{
    // Sickness is not capped.
    if (ench == ENCH_SICK)
        return;

    // Hard cap to simulate old enum behaviour, we should really throw this
    // out entirely.
    const int max = (ench == ENCH_ABJ || ench == ENCH_FAKE_ABJURATION) ? 6 : 4;
    if (degree > max)
        degree = max;
}

mon_enchant &mon_enchant::operator += (const mon_enchant &other)
{
    if (ench == other.ench)
    {
        degree   += other.degree;
        cap_degree();
        duration += other.duration;
        merge_killer(other.who, other.source);
    }
    return (*this);
}

mon_enchant mon_enchant::operator + (const mon_enchant &other) const
{
    mon_enchant tmp(*this);
    tmp += other;
    return (tmp);
}

killer_type mon_enchant::killer() const
{
    return (who == KC_YOU      ? KILL_YOU :
            who == KC_FRIENDLY ? KILL_MON
                               : KILL_MISC);
}

int mon_enchant::kill_agent() const
{
    return (who == KC_FRIENDLY? ANON_FRIENDLY_MONSTER : 0);
}

actor* mon_enchant::agent() const
{
    return find_agent(source, who);
}

int mon_enchant::modded_speed(const monster* mons, int hdplus) const
{
    return (_mod_speed(mons->hit_dice + hdplus, mons->speed));
}

int mon_enchant::calc_duration(const monster* mons,
                               const mon_enchant *added) const
{
    int cturn = 0;

    const int newdegree = added ? added->degree : degree;
    const int deg = newdegree ? newdegree : 1;

    // Beneficial enchantments (like Haste) should not be throttled by
    // monster HD via modded_speed(). Use mod_speed instead!
    switch (ench)
    {
    case ENCH_WITHDRAWN:
        cturn = 5000 / _mod_speed(25, mons->speed);
        break;

    case ENCH_SWIFT:
        cturn = 1000 / _mod_speed(25, mons->speed);
        break;
    case ENCH_HASTE:
    case ENCH_MIGHT:
    case ENCH_INVIS:
    case ENCH_FEAR_INSPIRING:
    case ENCH_STONESKIN:
        cturn = 1000 / _mod_speed(25, mons->speed);
        break;
    case ENCH_LIQUEFYING:
    case ENCH_SILENCE:
    case ENCH_REGENERATION:
    case ENCH_RAISED_MR:
    case ENCH_MIRROR_DAMAGE:
        cturn = 300 / _mod_speed(25, mons->speed);
        break;
    case ENCH_SLOW:
        cturn = 250 / (1 + modded_speed(mons, 10));
        break;
    case ENCH_FEAR:
        cturn = 150 / (1 + modded_speed(mons, 5));
        break;
    case ENCH_PARALYSIS:
        cturn = std::max(90 / modded_speed(mons, 5), 3);
        break;
    case ENCH_PETRIFIED:
        cturn = std::max(8, 150 / (1 + modded_speed(mons, 5)));
        break;
    case ENCH_DAZED:
    case ENCH_PETRIFYING:
        cturn = 50 / _mod_speed(10, mons->speed);
        break;
    case ENCH_CONFUSION:
        cturn = std::max(100 / modded_speed(mons, 5), 3);
        break;
    case ENCH_HELD:
        cturn = 120 / _mod_speed(25, mons->speed);
        break;
    case ENCH_POISON:
        cturn = 1000 * deg / _mod_speed(125, mons->speed);
        break;
    case ENCH_STICKY_FLAME:
        cturn = 1000 * deg / _mod_speed(200, mons->speed);
        break;
    case ENCH_ROT:
        if (deg > 1)
            cturn = 1000 * (deg - 1) / _mod_speed(333, mons->speed);
        cturn += 1000 / _mod_speed(250, mons->speed);
        break;
    case ENCH_CORONA:
    case ENCH_SILVER_CORONA:
        if (deg > 1)
            cturn = 1000 * (deg - 1) / _mod_speed(200, mons->speed);
        cturn += 1000 / _mod_speed(100, mons->speed);
        break;
    case ENCH_SHORT_LIVED:
        cturn = 1000 / _mod_speed(200, mons->speed);
        break;
    case ENCH_SLOWLY_DYING:
        // This may be a little too direct but the randomization at the end
        // of this function is excessive for toadstools. -cao
        return (2 * FRESHEST_CORPSE + random2(10))
                  * speed_to_duration(mons->speed) * mons->speed / 10;
    case ENCH_SPORE_PRODUCTION:
        // This is used as a simple timer, when the enchantment runs out
        // the monster will create a giant spore.
        return (random_range(475, 525) * 10);

    case ENCH_FADING_AWAY:
        // Also used as a simple timer. When it runs out, it will summon a
        // greater holy being.
        return (random_range(180, 230) * 10);

    case ENCH_PREPARING_RESURRECT:
        // A timer. When it runs out, the creature will cast resurrect.
        return (random_range(4, 7) * 10);

    case ENCH_EXPLODING:
        return (random_range(3,7) * 10);

    case ENCH_PORTAL_PACIFIED:
        // Must be set by spell.
        return (0);

    case ENCH_PORTAL_TIMER:
        cturn = 30 * 10 / _mod_speed(10, mons->speed);
        break;

    case ENCH_FAKE_ABJURATION:
    case ENCH_ABJ:
        if (deg >= 6)
            cturn = 1000 / _mod_speed(10, mons->speed);
        if (deg >= 5)
            cturn += 1000 / _mod_speed(20, mons->speed);
        cturn += 1000 * std::min(4, deg) / _mod_speed(100, mons->speed);
        break;
    case ENCH_CHARM:
        cturn = 500 / modded_speed(mons, 10);
        break;
    case ENCH_TP:
        cturn = 1000 * deg / _mod_speed(1000, mons->speed);
        break;
    case ENCH_SLEEP_WARY:
        cturn = 1000 / _mod_speed(50, mons->speed);
        break;
    case ENCH_LIFE_TIMER:
        cturn = 10 * (4 + random2(4)) / _mod_speed(10, mons->speed);
    default:
        break;
    }

    cturn = std::max(2, cturn);

    int raw_duration = (cturn * speed_to_duration(mons->speed));
    raw_duration = std::max(15, fuzz_value(raw_duration, 60, 40));

    dprf("cturn: %d, raw_duration: %d", cturn, raw_duration);

    return (raw_duration);
}

// Calculate the effective duration (in terms of normal player time - 10
// duration units being one normal player action) of this enchantment.
void mon_enchant::set_duration(const monster* mons, const mon_enchant *added)
{
    if (duration && !added)
        return;

    if (added && added->duration)
        duration += added->duration;
    else
        duration += calc_duration(mons, added);

    if (duration > maxduration)
        maxduration = duration;
}
