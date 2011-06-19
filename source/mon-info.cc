/**
 * @file
 * @brief Monster information that may be passed to the user.
 *
 * Used to fill the monster pane and to pass monster info to Lua.
**/

#include "AppHdr.h"

#include "mon-info.h"

#include "areas.h"
#include "artefact.h"
#include "coord.h"
#include "env.h"
#include "fight.h"
#include "ghost.h"
#include "itemname.h"
#include "libutil.h"
#include "message.h"
#include "misc.h"
#include "mon-iter.h"
#include "mon-util.h"
#include "monster.h"
#include "options.h"
#include "religion.h"
#include "showsymb.h"
#include "skills2.h"
#include "state.h"
#include "stuff.h"
#include "tagstring.h"

#include <algorithm>
#include <sstream>

#define ULL1 ((uint64_t)1)

static uint64_t ench_to_mb(const monster& mons, enchant_type ench)
{
    // Suppress silly-looking combinations, even if they're
    // internally valid.
    if (mons.paralysed() && (ench == ENCH_SLOW || ench == ENCH_HASTE
                      || ench == ENCH_SWIFT
                      || ench == ENCH_PETRIFIED
                      || ench == ENCH_PETRIFYING))
        return 0;

    if (ench == ENCH_HASTE && mons.has_ench(ENCH_SLOW))
        return 0;

    if (ench == ENCH_SLOW && mons.has_ench(ENCH_HASTE))
        return 0;

    if (ench == ENCH_PETRIFIED && mons.has_ench(ENCH_PETRIFYING))
        return 0;

    switch (ench)
    {
    case ENCH_BERSERK:
        return ULL1 << MB_BERSERK;
    case ENCH_POISON:
        return ULL1 << MB_POISONED;
    case ENCH_SICK:
        return ULL1 << MB_SICK;
    case ENCH_ROT:
        return ULL1 << MB_ROTTING;
    case ENCH_CORONA:
    case ENCH_SILVER_CORONA:
        return ULL1 << MB_GLOWING;
    case ENCH_SLOW:
        return ULL1 << MB_SLOWED;
    case ENCH_INSANE:
        return ULL1 << MB_INSANE;
    case ENCH_BATTLE_FRENZY:
        return ULL1 << MB_FRENZIED;
    case ENCH_HASTE:
        return ULL1 << MB_HASTED;
    case ENCH_MIGHT:
        return ULL1 << MB_STRONG;
    case ENCH_CONFUSION:
        return ULL1 << MB_CONFUSED;
    case ENCH_INVIS:
        return ULL1 << MB_INVISIBLE;
    case ENCH_CHARM:
        return ULL1 << MB_CHARMED;
    case ENCH_STICKY_FLAME:
        return ULL1 << MB_BURNING;
    case ENCH_HELD:
        return ULL1 << MB_CAUGHT;
    case ENCH_PETRIFIED:
        return ULL1 << MB_PETRIFIED;
    case ENCH_PETRIFYING:
        return ULL1 << MB_PETRIFYING;
    case ENCH_LOWERED_MR:
        return ULL1 << MB_VULN_MAGIC;
    case ENCH_SWIFT:
        return ULL1 << MB_SWIFT;
    case ENCH_SILENCE:
        return ULL1 << MB_SILENCING;
    case ENCH_PARALYSIS:
        return ULL1 << MB_PARALYSED;
    case ENCH_SOUL_RIPE:
        return ULL1 << MB_POSSESSABLE;
    case ENCH_PREPARING_RESURRECT:
        return ULL1 << MB_PREP_RESURRECT;
    case ENCH_FADING_AWAY:
        if ((mons.get_ench(ENCH_FADING_AWAY)).duration < 400) // min dur is 180*20, max dur 230*10
            return ULL1 << MB_MOSTLY_FADED;

        return ULL1 << MB_FADING_AWAY;
    case ENCH_REGENERATION:
        return ULL1 << MB_REGENERATION;
    case ENCH_RAISED_MR:
        return ULL1 << MB_RAISED_MR;
    case ENCH_MIRROR_DAMAGE:
        return ULL1 << MB_MIRROR_DAMAGE;
    case ENCH_FEAR_INSPIRING:
        return ULL1 << MB_FEAR_INSPIRING;
    case ENCH_WITHDRAWN:
        return ULL1 << MB_WITHDRAWN;
    case ENCH_ATTACHED:
        return ULL1 << MB_ATTACHED;
    case ENCH_BLEED:
        return ULL1 << MB_BLEEDING;
    case ENCH_DAZED:
        return ULL1 << MB_DAZED;
    case ENCH_MUTE:
        return ULL1 << MB_MUTE;
    case ENCH_BLIND:
        return ULL1 << MB_BLIND;
    case ENCH_DUMB:
        return ULL1 << MB_DUMB;
    case ENCH_MAD:
        return ULL1 << MB_MAD;
    default:
        return 0;
    }
}

static bool _blocked_ray(const coord_def &where,
                         dungeon_feature_type* feat = NULL)
{
    if (exists_ray(you.pos(), where) || !exists_ray(you.pos(), where, opc_default))
        return (false);
    if (feat == NULL)
        return (true);
    *feat = ray_blocker(you.pos(), where);
    return (true);
}

static bool _is_public_key(std::string key)
{
    if (key == "helpless" || key == "wand_known")
        return true;
    else
        return false;
}

monster_info::monster_info(monster_type p_type, monster_type p_base_type)
{
    mb = 0;
    attitude = ATT_HOSTILE;
    pos = coord_def(0, 0);

    mimic_feature = DNGN_UNSEEN;

    type = p_type;
    base_type = p_base_type;

    draco_type = mons_genus(type) == MONS_DRACONIAN ? MONS_DRACONIAN : type;

    number = 0;
    colour = mons_class_colour(type);

    holi = mons_class_holiness(type);

    mintel = mons_class_intel(type);

    mresists = get_mons_class_resists(type);

    mitemuse = mons_class_itemuse(type);

    mbase_speed = mons_class_base_speed(type);

    fly = mons_class_flies(type);
    if (fly == FL_NONE)
        fly = mons_class_flies(base_type);

    two_weapons = mons_class_wields_two_weapons(type);
    if (!two_weapons)
        two_weapons = mons_class_wields_two_weapons(base_type);

    no_regen = !mons_class_can_regenerate(type);
    if (!no_regen)
        no_regen = !mons_class_can_regenerate(base_type);

    dam = MDAM_OKAY;

    fire_blocker = DNGN_UNSEEN;

    if (mons_is_pghost(type))
    {
        u.ghost.species = SP_HUMAN;
        u.ghost.job = JOB_WANDERER;
        u.ghost.religion = GOD_NO_GOD;
        u.ghost.best_skill = SK_FIGHTING;
        u.ghost.best_skill_rank = 2;
        u.ghost.xl_rank = 3;
    }

    if (base_type == MONS_NO_MONSTER)
        base_type = type;

    props.clear();
}

monster_info::monster_info(const monster* m, int milev)
{
    mb = 0;
    attitude = ATT_HOSTILE;
    pos = grid2player(m->pos());

    mimic_feature = DNGN_UNSEEN;

    // XXX: this doesn't take into account ENCH_TEMP_PACIF, but that's probably
    // a bug for mons_attitude, not this.
    // XXX: also, mons_attitude_type should be sorted hostile/neutral/friendly;
    // will break saves a little bit though.
    attitude = mons_attitude(m);
    if (m->has_ench(ENCH_CHARM))
        attitude = ATT_FRIENDLY;
    else if (m->has_ench(ENCH_TEMP_PACIF))
        attitude = ATT_GOOD_NEUTRAL;

    bool type_known = false;
    bool nomsg_wounds = false;

    // CHANGE: now friendly fake Rakshasas/Maras are known (before you could still tell by equipment)
    if (m->props.exists("mislead_as") && you.misled())
        type = m->get_mislead_type();
    else if (attitude != ATT_FRIENDLY && (m->type == MONS_RAKSHASA || m->type == MONS_RAKSHASA_FAKE))
        type = MONS_RAKSHASA;
    else if (attitude != ATT_FRIENDLY && (m->type == MONS_MARA || m->type == MONS_MARA_FAKE))
        type = MONS_MARA;
    else
    {
        type_known = true;
        type = m->type;
    }

    props.clear();
    if (!m->props.empty())
    {
        CrawlHashTable::hash_map_type::const_iterator i = m->props.begin();
        for (; i != m->props.end(); i++)
            if (_is_public_key(i->first))
                props[i->first] = i->second;
    }

    if (type_known)
    {
        draco_type =
            mons_genus(type) == MONS_DRACONIAN ? ::draco_subspecies(m) : type;

        if (!mons_can_display_wounds(m)
            || !mons_class_can_display_wounds(type))
        {
            nomsg_wounds = true;
        }

        base_type = m->base_monster;
        if (base_type == MONS_NO_MONSTER)
            base_type = type;

        // these use number for internal information
        if (type == MONS_MANTICORE
            || type == MONS_KRAKEN_TENTACLE
            || type == MONS_KRAKEN_TENTACLE_SEGMENT
            || type == MONS_ELDRITCH_TENTACLE_SEGMENT)
        {
            number = 0;
        }
        else
            number = m->number;
        colour = m->colour;

        if (m->is_summoned())
            mb |= ULL1 << MB_SUMMONED;

        if (testbits(m->flags, MF_HARD_RESET) && testbits(m->flags, MF_NO_REWARD))
            mb |= ULL1 << MB_PERM_SUMMON;

        if (mons_is_known_mimic(m) && mons_genus(type) == MONS_DOOR_MIMIC)
            mimic_feature = get_mimic_feat(m);
    }
    else
    {
        base_type = type;
        number = 0;
        colour = mons_class_colour(type);
    }

    if (mons_is_unique(type))
    {
        if (type == MONS_LERNAEAN_HYDRA
            || type == MONS_ROYAL_JELLY
            || type == MONS_SERPENT_OF_HELL)
        {
            mb |= ULL1 << MB_NAME_THE;
        }
        else
            mb |= (ULL1 << MB_NAME_UNQUALIFIED) | (ULL1 << MB_NAME_THE);
    }

    mname = m->mname;

    if ((m->flags & MF_NAME_MASK) == MF_NAME_SUFFIX)
        mb |= ULL1 << MB_NAME_SUFFIX;
    else if ((m->flags & MF_NAME_MASK) == MF_NAME_ADJECTIVE)
        mb |= ULL1 << MB_NAME_ADJECTIVE;
    else if ((m->flags & MF_NAME_MASK) == MF_NAME_REPLACE)
        mb |= ULL1 << MB_NAME_REPLACE;

    const bool need_name_desc =
        (m->flags & MF_NAME_SUFFIX)
            || (m->flags & MF_NAME_ADJECTIVE)
            || (m->flags & MF_NAME_DEFINITE);

    if (!mname.empty()
        && !(m->flags & MF_NAME_DESCRIPTOR)
        && !need_name_desc)
    {
        mb |= (ULL1 << MB_NAME_UNQUALIFIED) | (ULL1 << MB_NAME_THE);
    }
    else if (m->flags & MF_NAME_DEFINITE)
        mb |= ULL1 << MB_NAME_THE;
    if (m->flags & MF_NAME_ZOMBIE)
        mb |= ULL1 << MB_NAME_ZOMBIE;

    if (milev <= MILEV_NAME)
    {
        if (type_known && type == MONS_DANCING_WEAPON
            && m->inv[MSLOT_WEAPON] != NON_ITEM)
        {
            inv[MSLOT_WEAPON].reset(
                new item_def(get_item_info(mitm[m->inv[MSLOT_WEAPON]])));
        }
        return;
    }

    holi = m->holiness();

    // don't give away mindlessness of monsters you're misled about
    if (type_known)
        mintel = mons_intel(m);
    else
        mintel = mons_class_intel(type);

    // don't give away resistances of monsters you're misled about
    if (type_known)
        mresists = get_mons_resists(m);
    else
        mresists = get_mons_class_resists(type);

    mitemuse = mons_itemuse(m);

    // don't give away base speed of monsters you're misled about
    if (type_known)
        mbase_speed = mons_base_speed(m);
    else
        mbase_speed = mons_class_base_speed(type);

    // consider randarts too, since flight should be visually obvious
    if (type_known)
        fly = mons_flies(m);
    else
        fly = mons_class_flies(type);

    two_weapons = mons_wields_two_weapons(m);

    // don't give away regeneration of monsters you're misled about
    if (type_known)
        no_regen = !mons_can_regenerate(m);
    else
        no_regen = !mons_class_can_regenerate(type);

    if (m->haloed())
        mb |= ULL1 << MB_HALOED;
    if (mons_looks_stabbable(m))
        mb |= ULL1 << MB_STABBABLE;
    if (mons_looks_distracted(m))
        mb |= ULL1 << MB_DISTRACTED;
    if (liquefied(m->pos()) && m->ground_level() && !m->is_insubstantial())
        mb |= ULL1 << MB_SLOWED;
    if (m->is_wall_clinging())
        mb |= ULL1 << MB_CLINGING;

    dam = mons_get_damage_level(m);

    // If no messages about wounds, don't display damage level either.
    if (nomsg_wounds)
        dam = MDAM_OKAY;

    if (mons_behaviour_perceptible(m))
    {
        if (m->asleep())
        {
            if (!m->can_hibernate(true))
                mb |= ULL1 << MB_DORMANT;
            else
                mb |= ULL1 << MB_SLEEPING;
        }
        // Applies to both friendlies and hostiles
        else if (mons_is_fleeing(m))
        {
            mb |= ULL1 << MB_FLEEING;
        }
        else if (mons_is_wandering(m) && !mons_is_batty(m))
        {
            if (mons_is_stationary(m))
                mb |= ULL1 << MB_UNAWARE;
            else
                mb |= ULL1 << MB_WANDERING;
        }
        // TODO: is this ever needed?
        else if (m->foe == MHITNOT && !mons_is_batty(m)
                 && m->attitude == ATT_HOSTILE)
        {
            mb |= ULL1 << MB_UNAWARE;
        }
    }

    for (mon_enchant_list::const_iterator e = m->enchantments.begin();
         e != m->enchantments.end(); ++e)
    {
        mb |= ench_to_mb(*m, e->first);
    }

    // fake enchantment (permanent)
    if (mons_class_flag(m->type, M_DEFLECT_MISSILES))
        mb |= ULL1 << MB_DEFLECT_MSL;

    if (m->type == MONS_SILENT_SPECTRE)
        mb |= ULL1 << MB_SILENCING;

    if (you.beheld_by(m))
        mb |= ULL1 << MB_MESMERIZING;

    // Evilness of attacking
    switch (attitude)
    {
    case ATT_NEUTRAL:
    case ATT_HOSTILE:
        if (you.religion == GOD_SHINING_ONE
            && !tso_unchivalric_attack_safe_monster(m)
            && is_unchivalric_attack(&you, m))
        {
            mb |= ULL1 << MB_EVIL_ATTACK;
        }
        break;
    default:
        break;
    }

    if (testbits(m->flags, MF_ENSLAVED_SOUL))
        mb |= ULL1 << MB_ENSLAVED;

    if (m->is_shapeshifter() && (m->flags & MF_KNOWN_MIMIC))
        mb |= ULL1 << MB_SHAPESHIFTER;

    if (m->is_known_chaotic())
        mb |= ULL1 << MB_CHAOTIC;

    if (m->submerged())
        mb |= ULL1 << MB_SUBMERGED;

    // these are excluded as mislead types
    if (mons_is_pghost(type))
    {
        ASSERT(m->ghost.get());
        ghost_demon& ghost = *m->ghost;
        u.ghost.species = ghost.species;
        if (species_genus(u.ghost.species) == GENPC_DRACONIAN && ghost.xl < 7)
            u.ghost.species = SP_BASE_DRACONIAN;
        u.ghost.job = ghost.job;
        u.ghost.religion = ghost.religion;
        u.ghost.best_skill = ghost.best_skill;
        u.ghost.best_skill_rank = get_skill_rank(ghost.best_skill_level);
        u.ghost.xl_rank = ghost_level_to_rank(ghost.xl);
    }

    for (unsigned i = 0; i <= MSLOT_LAST_VISIBLE_SLOT; ++i)
    {
        bool ok;
        if (m->inv[i] == NON_ITEM)
            ok = false;
        else if (i == MSLOT_MISCELLANY)
            ok = mons_is_mimic(type);
        else if (attitude == ATT_FRIENDLY)
            ok = true;
        else if (i == MSLOT_WAND)
            ok = props.exists("wand_known") && props["wand_known"];
        else if (m->props.exists("ash_id") && item_type_known(mitm[m->inv[i]]))
            ok = true;
        else if (i == MSLOT_ALT_WEAPON)
            ok = two_weapons;
        else if (i == MSLOT_MISSILE)
            ok = false;
        else
            ok = true;
        if (ok)
            inv[i].reset(new item_def(get_item_info(mitm[m->inv[i]])));
    }

    fire_blocker = DNGN_UNSEEN;
    if (!crawl_state.arena_suspended
        && m->pos() != you.pos())
    {
        _blocked_ray(m->pos(), &fire_blocker);
    }

    if (m->props.exists("quote"))
        quote = m->props["quote"].get_string();

    if (m->props.exists("description"))
        description = m->props["description"].get_string();

    // this must be last because it provides this structure to Lua code
    if (milev > MILEV_SKIP_SAFE)
    {
        if (mons_is_safe(m))
            mb |= ULL1 << MB_SAFE;
        else
            mb |= ULL1 << MB_UNSAFE;
    }
}

monster* monster_info::mon() const
{
    int m = env.mgrid(player2grid(pos));
    ASSERT(m >= 0);
#ifdef USE_TILE
    if (m == NON_MONSTER)
        return NULL;
#endif
    return &env.mons[m];
}

std::string monster_info::db_name() const
{
    if (type == MONS_DANCING_WEAPON && inv[MSLOT_WEAPON].get())
    {
        iflags_t ignore_flags = ISFLAG_KNOW_CURSE | ISFLAG_KNOW_PLUSES;
        bool     use_inscrip  = false;
        return (inv[MSLOT_WEAPON]->name(DESC_DBNAME, false, false, use_inscrip, false,
                          ignore_flags));
    }

    return get_monster_data(type)->name;
}

std::string monster_info::_core_name() const
{
    monster_type nametype = type;

    switch (type)
    {
    case MONS_ZOMBIE_SMALL:     case MONS_ZOMBIE_LARGE:
    case MONS_SKELETON_SMALL:   case MONS_SKELETON_LARGE:
    case MONS_SIMULACRUM_SMALL: case MONS_SIMULACRUM_LARGE:
    case MONS_SPECTRAL_THING:   case MONS_SALT_PILLAR:
        nametype = base_type;
        break;

    default:
        break;
    }

    std::string s;

    if (is(MB_NAME_REPLACE))
        s = mname;
    else if (nametype == MONS_LERNAEAN_HYDRA)
        s = "Lernaean hydra"; // TODO: put this into mon-data.h
    else if (nametype == MONS_ROYAL_JELLY)
        s = "royal jelly";
    else if (nametype == MONS_SERPENT_OF_HELL)
        s = "Serpent of Hell";
    else if (invalid_monster_type(nametype) && nametype != MONS_PROGRAM_BUG)
        s = "INVALID MONSTER";
    else
    {
        const char* slime_sizes[] = {"buggy ", "", "large ", "very large ",
                                               "enormous ", "titanic "};
        s = get_monster_data(nametype)->name;

        switch (type)
        {
        case MONS_SLIME_CREATURE:
            ASSERT(number <= 5);
            s = slime_sizes[number] + s;
            break;
        case MONS_UGLY_THING:
        case MONS_VERY_UGLY_THING:
            s = ugly_thing_colour_name(colour) + " " + s;
            break;

        case MONS_LABORATORY_RAT:
            s = adjective_for_labrat_colour(colour) + " " + s;
            break;

        case MONS_DRACONIAN_CALLER:
        case MONS_DRACONIAN_MONK:
        case MONS_DRACONIAN_ZEALOT:
        case MONS_DRACONIAN_SHIFTER:
        case MONS_DRACONIAN_ANNIHILATOR:
        case MONS_DRACONIAN_KNIGHT:
        case MONS_DRACONIAN_SCORCHER:
            if (base_type != MONS_NO_MONSTER)
                s = draconian_colour_name(base_type) + " " + s;
            break;

        case MONS_DANCING_WEAPON:
            if (inv[MSLOT_WEAPON].get())
            {
                iflags_t ignore_flags = ISFLAG_KNOW_CURSE | ISFLAG_KNOW_PLUSES;
                bool     use_inscrip  = true;
                const item_def& item = *inv[MSLOT_WEAPON];
                s = (item.name(DESC_PLAIN, false, false, use_inscrip, false,
                                  ignore_flags));
            }
            break;

        case MONS_PLAYER_GHOST:
            s = apostrophise(mname) + " ghost";
            break;
        case MONS_PLAYER_ILLUSION:
            s = apostrophise(mname) + " illusion";
            break;
        case MONS_PANDEMONIUM_DEMON:
            s = mname;
            break;
        default:
            break;
        }
    }

    if (is(MB_NAME_SUFFIX))
        s += " " + mname;
    else if (is(MB_NAME_ADJECTIVE))
        s = mname + " " + s;

    return s;
}

std::string monster_info::_apply_adjusted_description(description_level_type desc, const std::string& s) const
{
    if (desc == DESC_NOCAP_ITS)
        desc = DESC_NOCAP_THE;
    if (is(MB_NAME_THE))
    {
        if (desc == DESC_CAP_A)
            desc = DESC_CAP_THE;
        if (desc == DESC_NOCAP_A)
            desc = DESC_NOCAP_THE;
    }
    if (attitude == ATT_FRIENDLY)
    {
        if (desc == DESC_CAP_THE)
            desc = DESC_CAP_YOUR;
        if (desc == DESC_NOCAP_THE)
            desc = DESC_NOCAP_YOUR;
    }
    return apply_description(desc, s);
}

std::string monster_info::common_name(description_level_type desc) const
{
    std::ostringstream ss;

    if (props.exists("helpless"))
        ss << "helpless ";

    if (is(MB_SUBMERGED))
        ss << "submerged ";

    if (type == MONS_SPECTRAL_THING && !is(MB_NAME_ZOMBIE))
        ss << "spectral ";

    if (type == MONS_BALLISTOMYCETE)
        ss << (number ? "active " : "");

    if ((mons_genus(type) == MONS_HYDRA
            || mons_genus(base_type) == MONS_HYDRA)
        && number > 0)
    {
        if (number < 11)
        {
            const char* cardinals[] = {"one", "two", "three", "four", "five",
                                       "six", "seven", "eight", "nine", "ten"};
            ss << cardinals[number - 1];
        }
        else
            ss << make_stringf("%d", number);

        ss << "-headed ";
    }

    std::string core = _core_name();
    ss << core;

    // Add suffixes.
    switch (type)
    {
    case MONS_ZOMBIE_SMALL:
    case MONS_ZOMBIE_LARGE:
        if (!is(MB_NAME_ZOMBIE))
            ss << " zombie";
        break;
    case MONS_SKELETON_SMALL:
    case MONS_SKELETON_LARGE:
        if (!is(MB_NAME_ZOMBIE))
            ss << " skeleton";
        break;
    case MONS_SIMULACRUM_SMALL:
    case MONS_SIMULACRUM_LARGE:
        if (!is(MB_NAME_ZOMBIE))
            ss << " simulacrum";
        break;
    case MONS_SALT_PILLAR:
        ss << " shaped pillar of salt";
        break;
    default:
        break;
    }

    if (is(MB_SHAPESHIFTER))
    {
        // If momentarily in original form, don't display "shaped
        // shifter".
        if (mons_genus(type) != MONS_SHAPESHIFTER)
            ss << " shaped shifter";
    }

    std::string s;
    // only respect unqualified if nothing was added ("Sigmund" or "The spectral Sigmund")
    if (!is(MB_NAME_UNQUALIFIED) || has_proper_name() || ss.str() != core)
        s = _apply_adjusted_description(desc, ss.str());
    else
        s = ss.str();

    if (desc == DESC_NOCAP_ITS)
        s = apostrophise(s);

    return (s);
}

bool monster_info::has_proper_name() const
{
    return !mname.empty() && !mons_is_ghost_demon(type)
            && !is(MB_NAME_REPLACE) && !is(MB_NAME_ADJECTIVE) && !is(MB_NAME_SUFFIX);
}

std::string monster_info::proper_name(description_level_type desc) const
{
    if (has_proper_name())
    {
        if (desc == DESC_NOCAP_ITS)
            return apostrophise(mname);
        else
            return mname;
    }
    else
        return common_name(desc);
}

std::string monster_info::full_name(description_level_type desc, bool use_comma) const
{
    if (desc == DESC_NONE)
        return ("");

    if (has_proper_name())
    {
        std::string s = mname + (use_comma ? ", the " : " the ") + common_name();
        if (desc == DESC_NOCAP_ITS)
            s = apostrophise(s);
        return s;
    }
    else
        return common_name(desc);
}

// Needed because gcc 4.3 sort does not like comparison functions that take
// more than 2 arguments.
bool monster_info::less_than_wrapper(const monster_info& m1,
                                     const monster_info& m2)
{
    return monster_info::less_than(m1, m2, true);
}

// Sort monsters by (in that order):    attitude, difficulty, type, brand
bool monster_info::less_than(const monster_info& m1, const monster_info& m2,
                             bool zombified, bool fullname)
{
    if (m1.attitude < m2.attitude)
        return (true);
    else if (m1.attitude > m2.attitude)
        return (false);

    // Force plain but different coloured draconians to be treated like the
    // same sub-type.
    if (!zombified && m1.type >= MONS_DRACONIAN
        && m1.type <= MONS_PALE_DRACONIAN
        && m2.type >= MONS_DRACONIAN
        && m2.type <= MONS_PALE_DRACONIAN)
    {
        return (false);
    }

    int diff_delta = mons_difficulty(m1.type) - mons_difficulty(m2.type);

    // By descending difficulty
    if (diff_delta > 0)
        return (true);
    else if (diff_delta < 0)
        return (false);

    // Force mimics of different types to be treated like the same one.
    if (mons_is_mimic(m1.type) && mons_is_mimic(m2.type))
        return (false);

    if (m1.type < m2.type)
        return (true);
    else if (m1.type > m2.type)
        return (false);

    // Never distinguish between dancing weapons.
    // The above checks guarantee that *both* monsters are of this type.
    if (m1.type == MONS_DANCING_WEAPON)
        return (false);

    if (m1.type == MONS_SLIME_CREATURE)
        return (m1.number > m2.number);

    if (m1.type == MONS_BALLISTOMYCETE)
        return ((m1.number > 0) > (m2.number > 0));

    if (zombified)
    {
        if (mons_class_is_zombified(m1.type))
        {
            // Because of the type checks above, if one of the two is zombified, so
            // is the other, and of the same type.
            if (m1.base_type < m2.base_type)
                return (true);
            else if (m1.base_type > m2.base_type)
                return (false);
        }

        // Both monsters are hydras or hydra zombies, sort by number of heads.
        if (mons_genus(m1.type) == MONS_HYDRA || mons_genus(m1.base_type) == MONS_HYDRA)
        {
            if (m1.number > m2.number)
                return (true);
            else if (m1.number < m2.number)
                return (false);
        }
    }

    if (fullname || mons_is_pghost(m1.type))
        return (m1.mname < m2.mname);

#if 0 // for now, sort mb together.
    // By descending mb, so no mb sorts to the end
    if (m1.mb > m2.mb)
        return (true);
    else if (m1.mb < m2.mb)
        return (false);
#endif

    return (false);
}

static std::string _verbose_info0(const monster_info& mi)
{
    if (mi.is(MB_CAUGHT))
        return ("caught");

    if (mi.is(MB_PETRIFIED))
        return ("petrified");
    if (mi.is(MB_DUMB))
        return ("dumb");
    if (mi.is(MB_PARALYSED))
        return ("paralysed");
    if (mi.is(MB_PETRIFYING))
        return ("petrifying");
    if (mi.is(MB_MAD))
        return ("mad");
    if (mi.is(MB_CONFUSED))
        return ("confused");
    if (mi.is(MB_FLEEING))
        return ("fleeing");
    if (mi.is(MB_DORMANT))
        return ("dormant");
    if (mi.is(MB_SLEEPING))
        return ("sleeping");
    if (mi.is(MB_UNAWARE))
        return ("unaware");
    // avoid jelly (wandering) (fellow slime)
    if (mi.is(MB_WANDERING) && mi.attitude != ATT_STRICT_NEUTRAL)
        return ("wandering");
    if (mi.is(MB_BURNING))
        return ("burning");
    if (mi.is(MB_ROTTING))
        return ("rotting");
    if (mi.is(MB_BLEEDING))
        return ("bleeding");
    if (mi.is(MB_INVISIBLE))
        return ("invisible");
    if (mi.is(MB_DAZED))
        return ("dazed");
    if (mi.is(MB_MUTE))
        return ("mute");
    if (mi.is(MB_BLIND))
        return ("blind");

    return ("");
}

static std::string _verbose_info(const monster_info& mi)
{
    std::string inf = _verbose_info0(mi);
    if (!inf.empty())
        inf = " (" + inf + ")";
    return inf;
}

void monster_info::to_string(int count, std::string& desc,
                                  int& desc_color, bool fullname) const
{
    std::ostringstream out;

    if (count == 1)
    {
        if (mons_is_mimic(type))
            out << mons_type_name(type, DESC_PLAIN);
        else
            out << full_name();
    }
    else
    {
        // TODO: this should be done in a much cleaner way, with code to merge multiple monster_infos into a single common structure
        out << count << " ";

        // Don't pluralise uniques, ever.  Multiple copies of the same unique
        // are unlikely in the dungeon currently, but quite common in the
        // arena.  This prevens "4 Gra", etc. {due}
        // Unless it's Mara, who summons illusions of himself.
        if (mons_is_unique(type) && type != MONS_MARA)
        {
            out << common_name();
        }
        // Specialcase mimics, so they don't get described as piles of gold
        // when that would be inappropriate. (HACK)
        else if (mons_is_mimic(type))
        {
            out << "mimics";
        }
        else if (mons_genus(type) == MONS_DRACONIAN)
        {
            out << pluralise(mons_type_name(MONS_DRACONIAN, DESC_PLAIN));
        }
        else if (type == MONS_UGLY_THING || type == MONS_VERY_UGLY_THING
                || type == MONS_DANCING_WEAPON || type == MONS_LABORATORY_RAT
                || !fullname)
        {
            out << pluralise(mons_type_name(type, DESC_PLAIN));
        }
        else
        {
            out << pluralise(common_name());
        }
    }

#ifdef DEBUG_DIAGNOSTICS
    out << " av" << mons_difficulty(type);
#endif

    if (count == 1)
    {
        if (is(MB_FRENZIED))
            out << " (frenzied)";
        else if (is(MB_BERSERK))
            out << " (berserk)";
        else if (Options.verbose_monster_pane)
            out << _verbose_info(*this);
        else if (is(MB_STABBABLE))
            out << " (resting)";
        else if (is(MB_DISTRACTED))
            out << " (distracted)";
        else if (is(MB_INVISIBLE))
            out << " (invisible)";
        else if (is(MB_WITHDRAWN))
            out << " (withdrawn)";
    }

    // Friendliness
    switch (attitude)
    {
    case ATT_FRIENDLY:
        //out << " (friendly)";
        desc_color = GREEN;
        break;
    case ATT_GOOD_NEUTRAL:
    case ATT_NEUTRAL:
        //out << " (neutral)";
        desc_color = BROWN;
        break;
    case ATT_STRICT_NEUTRAL:
         out << " (fellow slime)";
         desc_color = BROWN;
         break;
    case ATT_HOSTILE:
        // out << " (hostile)";
        desc_color = LIGHTGREY;
        break;
    }

    if (count == 1 && is(MB_EVIL_ATTACK))
        desc_color = Options.evil_colour;

    desc = out.str();
}

std::vector<std::string> monster_info::attributes() const
{
    std::vector<std::string> v;
    if (is(MB_POISONED))
        v.push_back("poisoned");
    if (is(MB_SICK))
        v.push_back("sick");
    if (is(MB_ROTTING))
        v.push_back("rotting away"); //jmf: "covered in sores"?
    if (is(MB_GLOWING))
        v.push_back("softly glowing");
    if (is(MB_SLOWED))
        v.push_back("moving slowly");
    if (is(MB_INSANE))
        v.push_back("frenzied and insane");
    if (is(MB_BERSERK))
        v.push_back("berserk");
    if (is(MB_FRENZIED))
        v.push_back("consumed by blood-lust");
    if (is(MB_HASTED))
        v.push_back("moving very quickly");
    if (is(MB_STRONG))
        v.push_back("unusually strong");
    if (is(MB_CONFUSED))
        v.push_back("bewildered and confused");
    if (is(MB_INVISIBLE))
        v.push_back("slightly transparent");
    if (is(MB_CHARMED))
        v.push_back("in your thrall");
    if (is(MB_BURNING))
        v.push_back("covered in liquid flames");
    if (is(MB_CAUGHT))
        v.push_back("entangled in a net");
    if (is(MB_PETRIFIED))
        v.push_back("petrified");
    if (is(MB_PETRIFYING))
        v.push_back("slowly petrifying");
    if (is(MB_VULN_MAGIC))
        v.push_back("susceptible to magic");
    if (is(MB_SWIFT))
        v.push_back("moving somewhat quickly");
    if (is(MB_SILENCING))
        v.push_back("radiating silence");
    if (is(MB_PARALYSED))
        v.push_back("paralysed");
    if (is(MB_BLEEDING))
        v.push_back("bleeding");
    if (is(MB_DEFLECT_MSL))
        v.push_back("deflecting missiles");
    if (is(MB_PREP_RESURRECT))
        v.push_back("quietly preparing");
    if (is(MB_FADING_AWAY))
        v.push_back("slowly fading away");
    if (is(MB_MOSTLY_FADED))
        v.push_back("mostly faded away");
    if (is(MB_FEAR_INSPIRING))
        v.push_back("inspiring fear");
    if (is(MB_WITHDRAWN))
    {
        v.push_back("regenerating health quickly");
        v.push_back("protected by its shell");
    }
    if (is(MB_ATTACHED))
        v.push_back("attached and sucking blood");
    if (is(MB_DAZED))
        v.push_back("dazed");
    if (is(MB_MUTE))
        v.push_back("permanently mute");
    if (is(MB_BLIND))
        v.push_back("permanently blind");
    if (is(MB_DUMB))
        v.push_back("stupefied");
    if (is(MB_MAD))
        v.push_back("lost in madness");
    return v;
}

std::string monster_info::wounds_description_sentence() const
{
    const std::string wounds = wounds_description();
    if (wounds.empty())
        return "";
    else
        return std::string(pronoun(PRONOUN_CAP)) + " is " + wounds + ".";
}

std::string monster_info::wounds_description(bool use_colour) const
{
    if (dam == MDAM_OKAY)
        return "";

    std::string desc = get_damage_level_string(holi, dam);
    if (use_colour)
    {
        const int col = channel_to_colour(MSGCH_MONSTER_DAMAGE, dam);
        desc = colour_string(desc, col);
    }
    return desc;
}

int monster_info::randarts(artefact_prop_type ra_prop) const
{
    int ret = 0;

    if (itemuse() >= MONUSE_STARTING_EQUIPMENT)
    {
        item_def* weapon = inv[MSLOT_WEAPON].get();
        item_def* second = inv[MSLOT_ALT_WEAPON].get(); // Two-headed ogres, etc.
        item_def* armour = inv[MSLOT_ARMOUR].get();
        item_def* shield = inv[MSLOT_SHIELD].get();

        if (weapon && weapon->base_type == OBJ_WEAPONS && is_artefact(*weapon))
            ret += artefact_wpn_property(*weapon, ra_prop);

        if (second && second->base_type == OBJ_WEAPONS && is_artefact(*second))
            ret += artefact_wpn_property(*second, ra_prop);

        if (armour && armour->base_type == OBJ_ARMOUR && is_artefact(*armour))
            ret += artefact_wpn_property(*armour, ra_prop);

        if (shield && shield->base_type == OBJ_ARMOUR && is_artefact(*shield))
            ret += artefact_wpn_property(*shield, ra_prop);
    }

    return (ret);
}

int monster_info::res_magic() const
{
    int mr = (get_monster_data(type))->resist_magic;
    if (mr == MAG_IMMUNE)
        return (MAG_IMMUNE);

    // Negative values get multiplied with monster hit dice.
    if (mr < 0)
        mr = mons_class_hit_dice(type) * (-mr) * 4 / 3;

    // Randarts have a multiplicative effect.
    mr *= (randarts(ARTP_MAGIC) + 100);
    mr /= 100;

    // ego armour resistance
    if (inv[MSLOT_ARMOUR].get()
        && get_armour_ego_type(*inv[MSLOT_ARMOUR]) == SPARM_MAGIC_RESISTANCE)
    {
        mr += 30;
    }

    if (inv[MSLOT_SHIELD].get()
        && get_armour_ego_type(*inv[MSLOT_SHIELD]) == SPARM_MAGIC_RESISTANCE)
    {
        mr += 30;
    }

    if (is(MB_VULN_MAGIC))
        mr /= 2;

    return (mr);
}

size_type monster_info::body_size() const
{
    const monsterentry *e = get_monster_data(type);
    size_type ret = (e ? e->size : SIZE_MEDIUM);

    // Slime creature size is increased by the number merged.
    if (type == MONS_SLIME_CREATURE)
    {
        if (number == 2)
            ret = SIZE_MEDIUM;
        else if (number == 3)
            ret = SIZE_LARGE;
        else if (number == 4)
            ret = SIZE_BIG;
        else if (number == 5)
            ret = SIZE_GIANT;
    }

    return (ret);
}

void get_monster_info(std::vector<monster_info>& mons)
{
    std::vector<monster* > visible;
    if (crawl_state.game_is_arena())
    {
        for (monster_iterator mi; mi; ++mi)
            visible.push_back(*mi);
    }
    else
        visible = get_nearby_monsters();

    for (unsigned int i = 0; i < visible.size(); i++)
    {
        if (!mons_class_flag(visible[i]->type, M_NO_EXP_GAIN)
            || visible[i]->type == MONS_KRAKEN_TENTACLE
            || visible[i]->type == MONS_BALLISTOMYCETE
                && visible[i]->number > 0)
        {
            mons.push_back(monster_info(visible[i]));
        }
    }
    std::sort(mons.begin(), mons.end(), monster_info::less_than_wrapper);
}
