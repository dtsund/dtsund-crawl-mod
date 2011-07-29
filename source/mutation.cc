/**
 * @file
 * @brief Functions for handling player mutations.
**/

#include "AppHdr.h"
#include "mutation.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sstream>

#if defined(UNIX) && !defined(USE_TILE)
#include "libunix.h"
#endif

#include "externs.h"

#include "abl-show.h"
#include "cio.h"
#include "coordit.h"
#include "delay.h"
#include "defines.h"
#include "dgn-actions.h"
#include "coord.h"
#include "effects.h"
#include "env.h"
#include "format.h"
#include "godabil.h"
#include "godpassive.h"
#include "itemprop.h"
#include "macro.h"
#include "menu.h"
#include "mgen_data.h"
#include "mon-place.h"
#include "mon-iter.h"
#include "mon-stuff.h"
#include "mon-util.h"
#include "notes.h"
#include "ouch.h"
#include "player.h"
#include "player-stats.h"
#include "religion.h"
#include "random.h"
#include "skills2.h"
#include "stuff.h"
#include "transform.h"
#include "hints.h"
#include "xom.h"

static int _body_covered();

mutation_def mut_data[] = {

#include "mutation-data.h"

};

#define MUTDATASIZE (sizeof(mut_data)/sizeof(mutation_def))

static const body_facet_def _body_facets[] =
{
    //{ EQ_NONE, MUT_FANGS, 1 },
    { EQ_HELMET, MUT_HORNS, 1 },
    { EQ_HELMET, MUT_ANTENNAE, 1 },
    //{ EQ_HELMET, MUT_BEAK, 1 },
    { EQ_GLOVES, MUT_CLAWS, 3 },
    { EQ_BOOTS, MUT_HOOVES, 3 },
    { EQ_BOOTS, MUT_TALONS, 3 }
};

static int mut_index[NUM_MUTATIONS];

void init_mut_index()
{
    for (int i = 0; i < NUM_MUTATIONS; ++i)
        mut_index[i] = -1;

    for (unsigned int i = 0; i < MUTDATASIZE; ++i)
    {
        const mutation_type mut = mut_data[i].mutation;
        ASSERT(mut >= 0 && mut < NUM_MUTATIONS);
        ASSERT(mut_index[mut] == -1);
        mut_index[mut] = i;
    }
}

static mutation_def* _seek_mutation(mutation_type mut)
{
    ASSERT(mut >= 0 && mut < NUM_MUTATIONS);
    if (mut_index[mut] == -1)
        return (NULL);
    else
        return (&mut_data[mut_index[mut]]);
}

bool is_valid_mutation(mutation_type mut)
{
    return (mut >= 0 && mut < NUM_MUTATIONS
            && _seek_mutation(mut));
}

bool is_body_facet(mutation_type mut)
{
    for (unsigned i = 0; i < ARRAYSZ(_body_facets); i++)
    {
        if (_body_facets[i].mut == mut)
            return (true);
    }

    return (false);
}

const mutation_def& get_mutation_def(mutation_type mut)
{
    ASSERT(is_valid_mutation(mut));
    return (*_seek_mutation(mut));
}

void fixup_mutations()
{
    if (player_genus(GENPC_DRACONIAN))
    {
        ASSERT(is_valid_mutation(MUT_STINGER));
        _seek_mutation(MUT_STINGER)->rarity = 8;
        ASSERT(is_valid_mutation(MUT_BIG_WINGS));
        _seek_mutation(MUT_BIG_WINGS)->rarity = 4;
    }

    if (you.species == SP_NAGA)
    {
        ASSERT(is_valid_mutation(MUT_STINGER));
        _seek_mutation(MUT_STINGER)->rarity = 8;
    }
}

bool mutation_is_fully_active(mutation_type mut)
{
    // For all except the semi-undead, mutations always apply.
    if (you.is_undead != US_SEMI_UNDEAD)
        return (true);

    // Innate mutations are always active
    if (you.innate_mutations[mut])
        return (true);

    // ... as are all mutations for semi-undead who are fully alive
    if (you.hunger_state == HS_ENGORGED)
        return (true);

    // ... as are physical mutations.
    if (get_mutation_def(mut).physical)
        return (true);

    return (false);
}

static bool _mutation_is_fully_inactive(mutation_type mut)
{
    const mutation_def& mdef = get_mutation_def(mut);
    return (you.is_undead == US_SEMI_UNDEAD && you.hunger_state < HS_SATIATED
            && !you.innate_mutations[mut] && !mdef.physical);
}

formatted_string describe_mutations()
{
    std::string result;
    bool have_any = false;
    const char *mut_title = "Innate Abilities, Weirdness & Mutations";
    std::string scale_type = "plain brown";

    // center title
    int offset = 39 - strwidth(mut_title) / 2;
    if (offset < 0) offset = 0;

    result += std::string(offset, ' ');

    result += "<white>";
    result += mut_title;
    result += "</white>\n\n";

    // Innate abilities which don't fit as mutations.
    result += "<lightblue>";
    switch (you.species)
    {
    case SP_MERFOLK:
        result += "You revert to your normal form in water.\n";
        have_any = true;
        break;

    case SP_NAGA:
        result += "You cannot wear boots.\n";
        // Breathe poison replaces spit poison.
        if (!you.mutation[MUT_BREATHE_POISON])
            result += "You can spit poison.\n";
        if (you.experience_level > 2)
        {
            std::ostringstream num;
            num << you.experience_level/3;
            result += "Your serpentine skin is tough (AC +" + num.str() + ").\n";
        }
        have_any = true;
        break;

    case SP_GHOUL:
        result += "Your body is rotting away.\n";
        have_any = true;
        break;

    case SP_KENKU:
        if (you.experience_level > 4)
        {
            result += "You can fly";
            if (you.experience_level > 14)
                result += " continuously";
            result += ".\n";
            have_any = true;
        }
        break;

    case SP_MUMMY:
        result += "You do not eat or drink.\n";
        result += "Your flesh is vulnerable to fire.\n";
        if (you.experience_level > 12)
        {
            result += "You are";
            if (you.experience_level > 25)
                result += " strongly";

            result += " in touch with the powers of death.\n";
            result +=
                "You can restore your body by infusing magical energy.\n";
        }
        have_any = true;
        break;

    case SP_GREEN_DRACONIAN:
        result += "You can breathe blasts of noxious fumes.\n";
        have_any = true;
        scale_type = "lurid green";
        break;

    case SP_GREY_DRACONIAN:
        result += "You can walk through water.\n";
        have_any = true;
        scale_type = "dull grey";
        break;

    case SP_RED_DRACONIAN:
        result += "You can breathe blasts of fire.\n";
        have_any = true;
        scale_type = "fiery red";
        break;

    case SP_WHITE_DRACONIAN:
        result += "You can breathe waves of freezing cold.\n";
        result += "You can buffet flying creatures when you breathe cold.\n";
        scale_type = "icy white";
        have_any = true;
        break;

    case SP_BLACK_DRACONIAN:
        result += "You can breathe wild blasts of lightning.\n";
        scale_type = "glossy black";
        have_any = true;
        break;

    case SP_YELLOW_DRACONIAN:
        result += "You can spit globs of acid.\n";
        result += "You can corrode armour when you spit acid.\n";
        result += "You are resistant to acid.\n";
        scale_type = "golden yellow";
        have_any = true;
        break;

    case SP_PURPLE_DRACONIAN:
        result += "You can breathe bolts of energy.\n";
        result += "You can dispel enchantments when you breathe energy.\n";
        scale_type = "rich purple";
        have_any = true;
        break;

    case SP_MOTTLED_DRACONIAN:
        result += "You can spit globs of burning liquid.\n";
        result += "You can ignite nearby creatures when you spit burning liquid.\n";
        scale_type = "weird mottled";
        have_any = true;
        break;

    case SP_PALE_DRACONIAN:
        result += "You can breathe blasts of scalding steam.\n";
        scale_type = "pale cyan-grey";
        have_any = true;
        break;

    case SP_KOBOLD:
        result += "You recuperate from illness quickly.\n";
        have_any = true;
        break;

    case SP_VAMPIRE:
        have_any = true;
        if (you.hunger_state == HS_STARVING)
            result += "<green>You do not heal naturally.</green>\n";
        else if (you.hunger_state == HS_ENGORGED)
            result += "<green>Your natural rate of healing is extremely fast.</green>\n";
        else if (you.hunger_state < HS_SATIATED)
            result += "<green>You heal slowly.</green>\n";
        else if (you.hunger_state >= HS_FULL)
            result += "<green>Your natural rate of healing is unusually fast.</green>\n";
        else
            have_any = false;

        if (you.experience_level >= 6)
        {
            result += "You can bottle blood from corpses.\n";
            have_any = true;
        }
        break;

    case SP_DEEP_DWARF:
        result += "You are resistant to damage.\n";
        result += "You can recharge devices by infusing magical energy.\n";
        have_any = true;
        break;

    case SP_CAT:
        result += "You cannot wear armour.\n";
        result += "You are incapable of any advanced item manipulation.\n";
        result += "Your paws have sharp claws.\n";
        have_any = true;
        break;

    default:
        break;
    }

    switch (you.body_size(PSIZE_TORSO, true))
    {
    case SIZE_LITTLE:
        if (you.species == SP_CAT)
            break;
        result += "You are tiny and cannot use many weapons and most armour.\n";
        have_any = true;
        break;
    case SIZE_SMALL:
        result += "You are small and have problems with some larger weapons.\n";
        have_any = true;
        break;
    case SIZE_LARGE:
        result += "You are too large for most types of armour.\n";
        have_any = true;
        break;
    default:
        break;
    }

    if (player_genus(GENPC_DRACONIAN))
    {
        // Draconians are large for the purposes of armour, but only medium for
        // weapons and carrying capacity.
        std::ostringstream num;
        num << 3 + you.experience_level / 3;
        result += "Your " + scale_type + " scales are hard (AC +" + num.str() + ").\n";

        result += "Your body does not fit into most forms of armour.\n";
        have_any = true;
    }

    result += "</lightblue>";

    if (beogh_water_walk())
    {
        result += "<green>You can walk on water.</green>\n";
        have_any = true;
    }

    if (you.duration[DUR_FIRE_SHIELD])
    {
        result += "<green>You are immune to clouds of flame.</green>\n";
        have_any = true;
    }

    textcolor(LIGHTGREY);

    // First add (non-removable) inborn abilities and demon powers.
    for (int i = 0; i < NUM_MUTATIONS; i++)
    {
        if (you.mutation[i] != 0 && you.innate_mutations[i])
        {
            mutation_type mut_type = static_cast<mutation_type>(i);
            result += mutation_name(mut_type, -1, true);
            result += "\n";
            have_any = true;
        }
    }

    // Now add removable mutations.
    for (int i = 0; i < NUM_MUTATIONS; i++)
    {
        if (you.mutation[i] != 0 && !you.innate_mutations[i])
        {
            mutation_type mut_type = static_cast<mutation_type>(i);
            result += mutation_name(mut_type, -1, true);
            result += "\n";
            have_any = true;
        }
    }

    if (!have_any)
        result +=  "You are rather mundane.\n";

    if (you.species == SP_VAMPIRE)
    {
        result += "\n\n";
        result += "\n\n";
        result +=
#ifndef USE_TILE
            "Press '<w>!</w>'"
#else
            "<w>Right-click</w>"
#endif
            " to toggle between mutations and properties depending on your\n"
            "hunger status.\n";
    }

    return (formatted_string::parse_string(result));
}

static void _display_vampire_attributes()
{
    ASSERT(you.species == SP_VAMPIRE);

    clrscr();
    cgotoxy(1,1);

    std::string result;

    const int lines = 15;
    std::string column[lines][7] =
    {
       {"                     ", "<lightgreen>Alive</lightgreen>      ", "<green>Full</green>    ",
        "Satiated  ", "<yellow>Thirsty</yellow>  ", "<yellow>Near...</yellow>  ",
        "<lightred>Bloodless</lightred>"},
                                //Alive          Full       Satiated      Thirsty   Near...      Bloodless
       {"Metabolism           ", "very fast  ", "fast    ", "fast      ", "normal   ", "slow     ", "none  "},

       {"Regeneration         ", "very fast  ", "fast    ", "normal    ", "slow     ", "slow     ", "none  "},

       {"Stealth boost        ", "none       ", "none    ", "none      ", "minor    ", "major    ", "large "},

       {"Spell hunger         ", "full       ", "full    ", "full      ", "halved   ", "none     ", "none  "},

       {"\n<w>Resistances</w>\n"
        "Poison resistance    ", "           ", "        ", "          ", " +       ", " +       ", " +    "},

       {"Cold resistance      ", "           ", "        ", "          ", " +       ", " ++      ", " ++   "},

       {"Negative resistance  ", "           ", "        ", " +        ", " ++      ", " +++     ", " +++  "},

       {"Rotting resistance   ", "           ", "        ", "          ", " +       ", " +       ", " +    "},

       {"Torment resistance   ", "           ", "        ", "          ", "         ", "         ", " +    "},

       {"\n<w>Other effects</w>\n"
        "Mutation chance      ", "always     ", "often   ", "sometimes ", "never    ", "never    ", "never "},

       {"Non-physical \n"
        "mutation effects     ", "full       ", "capped  ", "capped    ", "none     ", "none     ", "none  "},

       {"Potion effects       ", "full       ", "full    ", "full      ", "halved   ", "halved   ", "halved"},

       {"Bat Form             ", "no         ", "no      ", "yes       ", "yes      ", "yes      ", "yes   "},

       {"Other transformation \n"
        "or going berserk     ", "yes        ", "yes     ", "no        ", "no       ", "no       ", "no    "}
    };

    int current = 0;
    switch (you.hunger_state)
    {
    case HS_ENGORGED:
        current = 1;
        break;
    case HS_VERY_FULL:
    case HS_FULL:
        current = 2;
        break;
    case HS_SATIATED:
        current = 3;
        break;
    case HS_HUNGRY:
    case HS_VERY_HUNGRY:
        current = 4;
        break;
    case HS_NEAR_STARVING:
        current = 5;
        break;
    case HS_STARVING:
        current = 6;
    }

    for (int y = 0; y < lines; y++)  // lines   (properties)
    {
        for (int x = 0; x < 7; x++)  // columns (hunger states)
        {
             if (y > 0 && x == current)
                 result += "<w>";
             result += column[y][x];
             if (y > 0 && x == current)
                 result += "</w>";
        }
        result += "\n";
    }

    result += "\n";
    result +=
#ifndef USE_TILE
        "Press '<w>!</w>'"
#else
        "<w>Right-click</w>"
#endif
        " to toggle between mutations and properties depending on your\n"
        "hunger status.\n";

    const formatted_string vp_props = formatted_string::parse_string(result);
    vp_props.display();

    mouse_control mc(MOUSE_MODE_MORE);
    const int keyin = getchm();
    if (keyin == '!' || keyin == CK_MOUSE_CMD)
        display_mutations();
}

void display_mutations()
{
    clrscr();
    cgotoxy(1,1);

    const formatted_string mutation_fs = describe_mutations();

    if (you.species == SP_VAMPIRE)
    {
        mutation_fs.display();
        mouse_control mc(MOUSE_MODE_MORE);
        const int keyin = getchm();
        if (keyin == '!' || keyin == CK_MOUSE_CMD)
            _display_vampire_attributes();
    }
    else
    {
        Menu mutation_menu(mutation_fs);
        mutation_menu.show();
    }
}

static int _calc_mutation_amusement_value(mutation_type which_mutation)
{
    int amusement = 16 * (11 - get_mutation_def(which_mutation).rarity);

    switch (which_mutation)
    {
    case MUT_STRONG:
    case MUT_CLEVER:
    case MUT_AGILE:
    case MUT_POISON_RESISTANCE:
    case MUT_SHOCK_RESISTANCE:
    case MUT_REGENERATION:
    case MUT_SLOW_METABOLISM:
    case MUT_TELEPORT_CONTROL:
    case MUT_MAGIC_RESISTANCE:
    case MUT_TELEPORT_AT_WILL:
    case MUT_CLARITY:
    case MUT_MUTATION_RESISTANCE:
    case MUT_ROBUST:
    case MUT_HIGH_MAGIC:
        amusement /= 2;  // not funny
        break;

    case MUT_CARNIVOROUS:
    case MUT_HERBIVOROUS:
    case MUT_SLOW_HEALING:
    case MUT_FAST_METABOLISM:
    case MUT_WEAK:
    case MUT_DOPEY:
    case MUT_CLUMSY:
    case MUT_TELEPORT:
    case MUT_FAST:
    case MUT_DEFORMED:
    case MUT_SPIT_POISON:
    case MUT_BREATHE_FLAMES:
    case MUT_BLINK:
    case MUT_HORNS:
    case MUT_BEAK:
    case MUT_SCREAM:
    case MUT_BERSERK:
    case MUT_DETERIORATION:
    case MUT_BLURRY_VISION:
    case MUT_FRAIL:
    case MUT_CLAWS:
    case MUT_FANGS:
    case MUT_HOOVES:
    case MUT_TALONS:
    case MUT_BREATHE_POISON:
    case MUT_STINGER:
    case MUT_BIG_WINGS:
    case MUT_LOW_MAGIC:
    case MUT_SLOW:
        amusement *= 2; // funny!
        break;

    default:
        break;
    }

    return (amusement);
}

static bool _accept_mutation(mutation_type mutat, bool ignore_rarity = false)
{
    if (!is_valid_mutation(mutat))
        return (false);

    const mutation_def& mdef = get_mutation_def(mutat);

    if (you.mutation[mutat] >= mdef.levels)
        return (false);

    if (ignore_rarity)
        return (true);

    const int rarity = mdef.rarity + you.innate_mutations[mutat];

    // Low rarity means unlikely to choose it.
    return (x_chance_in_y(rarity, 10));
}

static mutation_type _get_random_slime_mutation()
{
    const mutation_type slime_muts[] = {
        MUT_GELATINOUS_BODY, MUT_EYEBALLS, MUT_TRANSLUCENT_SKIN,
        MUT_PSEUDOPODS, MUT_FOOD_JELLY, MUT_ACIDIC_BITE
    };

    return RANDOM_ELEMENT(slime_muts);
}

static mutation_type _delete_random_slime_mutation()
{
    mutation_type mutat;

    while (true)
    {
        mutat = _get_random_slime_mutation();

        if (you.mutation[mutat] > 0)
            break;

        if (one_chance_in(500))
        {
            mutat = NUM_MUTATIONS;
            break;
        }
    }

    return mutat;
}

static bool _is_slime_mutation(mutation_type m)
{
    return (m == MUT_GELATINOUS_BODY || m == MUT_EYEBALLS
            || m == MUT_TRANSLUCENT_SKIN || m == MUT_PSEUDOPODS
            || m == MUT_FOOD_JELLY || m == MUT_ACIDIC_BITE);
}

static mutation_type _get_random_xom_mutation()
{
    const mutation_type bad_muts[] = {
        MUT_WEAK,          MUT_DOPEY,
        MUT_CLUMSY,        MUT_DEFORMED,      MUT_SCREAM,
        MUT_DETERIORATION, MUT_BLURRY_VISION, MUT_FRAIL
    };

    mutation_type mutat = NUM_MUTATIONS;

    do
    {
        mutat = static_cast<mutation_type>(random2(NUM_MUTATIONS));

        if (one_chance_in(1000))
            return (NUM_MUTATIONS);
        else if (one_chance_in(5))
            mutat = RANDOM_ELEMENT(bad_muts);
    }
    while (!_accept_mutation(mutat, false));

    return (mutat);
}

static bool _mut_matches_class(mutation_type mutclass, const mutation_def& mdef)
{
    switch (mutclass)
    {
    case RANDOM_MUTATION:
        return (true);
    case RANDOM_BAD_MUTATION:
        return (mdef.bad);
    case RANDOM_GOOD_MUTATION:
        return (!mdef.bad);
    default:
        die("invalid mutation class: %d", mutclass);
    }
}

static mutation_type _get_random_mutation(mutation_type mutclass)
{
    int cweight = 0;
    mutation_type chosen = NUM_MUTATIONS;
    for (unsigned i = 0; i < NUM_MUTATIONS; ++i)
    {
        const mutation_type curr = static_cast<mutation_type>(i);
        if (!is_valid_mutation(curr))
            continue;

        const mutation_def& mdef = get_mutation_def(curr);
        if (!mdef.rarity)
            continue;

        if (!_mut_matches_class(mutclass, mdef))
            continue;

        if (!_accept_mutation(curr, true))
            continue;

        cweight += mdef.rarity;

        if (x_chance_in_y(mdef.rarity, cweight))
            chosen = curr;
    }

    return (chosen);
}

// Tries to give you the mutation by deleting a conflicting
// one, or clears out conflicting mutations if we should give
// you the mutation anyway.
// Return:
//  1 if we should stop processing (success);
//  0 if we should continue processing;
// -1 if we should stop processing (failure).
static int _handle_conflicting_mutations(mutation_type mutation,
                                         bool override)
{
    const int conflict[][3] = {
        { MUT_REGENERATION,    MUT_SLOW_METABOLISM,  0},
        { MUT_REGENERATION,    MUT_SLOW_HEALING,     0},
        { MUT_ACUTE_VISION,    MUT_BLURRY_VISION,    0},
        { MUT_FAST,            MUT_SLOW,             0},
        { MUT_FANGS,           MUT_BEAK,            -1},
        { MUT_HOOVES,          MUT_TALONS,          -1},
        { MUT_STRONG,          MUT_WEAK,             1},
        { MUT_CLEVER,          MUT_DOPEY,            1},
        { MUT_AGILE,           MUT_CLUMSY,           1},
        { MUT_STRONG_STIFF,    MUT_FLEXIBLE_WEAK,    1},
        { MUT_ROBUST,          MUT_FRAIL,            1},
        { MUT_HIGH_MAGIC,      MUT_LOW_MAGIC,        1},
        { MUT_CARNIVOROUS,     MUT_HERBIVOROUS,      1},
        { MUT_SLOW_METABOLISM, MUT_FAST_METABOLISM,  1},
        { MUT_REGENERATION,    MUT_SLOW_HEALING,     1},
        { MUT_ACUTE_VISION,    MUT_BLURRY_VISION,    1},
        { MUT_FAST,            MUT_SLOW,             1},
        };

    // If we have one of the pair, delete all levels of the other,
    // and continue processing.
    for (unsigned i = 0; i < ARRAYSZ(conflict); ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            const mutation_type a = (mutation_type)conflict[i][j];
            const mutation_type b = (mutation_type)conflict[i][1-j];

            if (mutation == a && you.mutation[b] > 0)
            {
                int res = conflict[i][2];
                switch(res)
                {
                case -1:
                    // Fail if not forced, otherwise override.
                    if (!override)
                        return -1;
                case 0:
                    // Ignore if not forced, otherwise override.
                    // All cases but regen:slowmeta will currently trade off.
                    if (override)
                        while (delete_mutation(b, true, true))
                            ;
                    break;
                case 1:
                    // If we have one of the pair, delete a level of the
                    // other, and that's it.
                    delete_mutation(b, true, true);
                    return (1);     // Nothing more to do.
                default:
                    die("bad mutation conflict resulution");
                }
            }
        }
    }

    return (0);
}

static const mutation_type _all_scales[] = {
    MUT_DISTORTION_FIELD,           MUT_ICY_BLUE_SCALES,
    MUT_IRIDESCENT_SCALES,          MUT_LARGE_BONE_PLATES,
    MUT_MOLTEN_SCALES,              MUT_ROUGH_BLACK_SCALES,
    MUT_RUGGED_BROWN_SCALES,        MUT_SLIMY_GREEN_SCALES,
    MUT_THIN_METALLIC_SCALES,       MUT_THIN_SKELETAL_STRUCTURE,
    MUT_YELLOW_SCALES,
};

static int _is_covering(mutation_type mut)
{
    for (unsigned i = 0; i < ARRAYSZ(_all_scales); ++i)
        if (_all_scales[i] == mut)
            return (1);

    return (0);
}

static int _body_covered()
{
    // Check how much of your body is covered by scales, etc.
    int covered = 0;

    if (you.species == SP_NAGA)
        covered++;

    if (player_genus(GENPC_DRACONIAN))
        covered += 3;

    for (unsigned i = 0; i < ARRAYSZ(_all_scales); ++i)
        covered += you.mutation[_all_scales[i]];

    return (covered);
}

static bool _physiology_mutation_conflict(mutation_type mutat)
{
    // If demonspawn, and mutat is a scale, see if they were going
    // to get it sometime in the future anyway; otherwise, conflict.
    if (you.species == SP_DEMONSPAWN && _is_covering(mutat) &&
            std::find(_all_scales, _all_scales+ARRAYSZ(_all_scales), mutat) !=
                _all_scales+ARRAYSZ(_all_scales))
    {
        bool found = false;

        for (unsigned j = 0; j < you.demonic_traits.size(); ++j)
        {
            if (you.demonic_traits[j].mutation == mutat)
                found = true;
        }

        return (!found);
    }

    // Strict 3-scale limit.
    if (_is_covering(mutat) && _body_covered() >= 3)
        return (true);

    // Only Nagas and Draconians can get this one.
    if (mutat == MUT_STINGER
        && you.species != SP_NAGA && !player_genus(GENPC_DRACONIAN))
    {
        return (true);
    }

    if ((mutat == MUT_HOOVES || mutat == MUT_TALONS) && !player_has_feet())
        return (true);

    // Already innate.
    if (mutat == MUT_BREATHE_POISON && you.species != SP_NAGA)
        return (true);

    // Red Draconians can already breathe flames.
    if (mutat == MUT_BREATHE_FLAMES && you.species == SP_RED_DRACONIAN)
        return (true);

    // Green Draconians can already breathe poison, so they don't need
    // to spit it.
    if (mutat == MUT_SPIT_POISON && you.species == SP_GREEN_DRACONIAN)
        return (true);

    // Only Draconians can get wings.
    if (mutat == MUT_BIG_WINGS && !player_genus(GENPC_DRACONIAN))
        return (true);

    // Vampires' healing and thirst rates depend on their blood level.
    if (you.species == SP_VAMPIRE
        && (mutat == MUT_CARNIVOROUS || mutat == MUT_HERBIVOROUS
            || mutat == MUT_REGENERATION || mutat == MUT_SLOW_HEALING
            || mutat == MUT_FAST_METABOLISM || mutat == MUT_SLOW_METABOLISM))
    {
        return (true);
    }

    // Already innate, and unlike trolls/ghouls, no increases for you!
    if (mutat == MUT_CLAWS && you.species == SP_CAT)
        return (true);

    equipment_type eq_type = EQ_NONE;

    // Mutations of the same slot conflict
    if (is_body_facet(mutat))
    {
        // Find equipment slot of attempted mutation
        for (unsigned i = 0; i < ARRAYSZ(_body_facets); i++)
            if (mutat == _body_facets[i].mut)
                eq_type = _body_facets[i].eq;

        if (eq_type != EQ_NONE)
        {
            for (unsigned i = 0; i < ARRAYSZ(_body_facets); i++)
            {
                if (eq_type == _body_facets[i].eq
                    && mutat != _body_facets[i].mut
                    && player_mutation_level(_body_facets[i].mut))
                {
                    return (true);
                }
            }
        }
    }

    return (false);
}

static const char* _stat_mut_desc(mutation_type mut, bool gain)
{
    stat_type stat = STAT_STR;
    bool positive = gain;
    switch (mut)
    {
    case MUT_WEAK:
        positive = !positive;
    case MUT_STRONG:
        stat = STAT_STR;
        break;

    case MUT_DOPEY:
        positive = !positive;
    case MUT_CLEVER:
        stat = STAT_INT;
        break;

    case MUT_CLUMSY:
        positive = !positive;
    case MUT_AGILE:
        stat = STAT_DEX;
        break;

    default:
        die("invalid stat mutation: %d", mut);
    }
    return (stat_desc(stat, positive ? SD_INCREASE : SD_DECREASE));
}

bool mutate(mutation_type which_mutation, bool failMsg,
            bool force_mutation, bool god_gift, bool stat_gain_potion,
            bool demonspawn)
{
    if (!god_gift)
    {
        const god_type god =
            (crawl_state.is_god_acting()) ? crawl_state.which_god_acting()
                                          : GOD_NO_GOD;

        if (god != GOD_NO_GOD)
            god_gift = true;
    }

    if (demonspawn)
        force_mutation = true;

    mutation_type mutat = which_mutation;

    if (!force_mutation)
    {
        // God gifts override all sources of mutation resistance other
        // than divine protection, and stat gain potions override all
        // sources of mutation resistance other than the mutation
        // resistance mutation.
        if (!god_gift)
        {
            if ((wearing_amulet(AMU_RESIST_MUTATION)
                    && !one_chance_in(10) && !stat_gain_potion)
                || player_mutation_level(MUT_MUTATION_RESISTANCE) == 3
                || (player_mutation_level(MUT_MUTATION_RESISTANCE)
                    && !one_chance_in(3)))
            {
                if (failMsg)
                    mpr("You feel odd for a moment.", MSGCH_MUTATION);
                return (false);
            }
        }

        // Zin's protection.
        if (you.religion == GOD_ZIN && x_chance_in_y(you.piety, MAX_PIETY)
            && !stat_gain_potion)
        {
            simple_god_message(" protects your body from mutation!");
            return (false);
        }
    }

    bool rotting = you.is_undead;

    if (you.is_undead == US_SEMI_UNDEAD)
    {
        // The stat gain mutations always come through at Satiated or
        // higher (mostly for convenience), and, for consistency, also
        // their negative counterparts.
        if (which_mutation == MUT_STRONG || which_mutation == MUT_CLEVER
            || which_mutation == MUT_AGILE || which_mutation == MUT_WEAK
            || which_mutation == MUT_DOPEY || which_mutation == MUT_CLUMSY)
        {
            if (you.hunger_state >= HS_SATIATED)
                rotting = false;
        }
        else
        {
            // Else, chances depend on hunger state.
            switch (you.hunger_state)
            {
            case HS_SATIATED:  rotting = !one_chance_in(3); break;
            case HS_FULL:      rotting = coinflip();        break;
            case HS_VERY_FULL: rotting = one_chance_in(3);  break;
            case HS_ENGORGED:  rotting = false;             break;
            }
        }
    }

    // Undead bodies don't mutate, they fall apart. -- bwr
    // except for demonspawn (or other permamutations) in lichform -- haranp
    if (rotting && !demonspawn)
    {
        mpr("Your body decomposes!", MSGCH_MUTATION);

        if (coinflip())
            lose_stat(STAT_RANDOM, 1, false, "mutating");
        else
        {
            ouch(3, NON_MONSTER, KILLED_BY_ROTTING);
            rot_hp(roll_dice(1, 3));
        }

        xom_is_stimulated(64);
        return (true);
    }

    if (which_mutation == RANDOM_MUTATION
        || which_mutation == RANDOM_XOM_MUTATION)
    {
        // If already heavily mutated, remove a mutation instead.
        if (x_chance_in_y(how_mutated(false, true), 15))
        {
            // God gifts override mutation loss due to being heavily
            // mutated.
            if (!one_chance_in(3) && !god_gift && !force_mutation)
                return (false);
            else
                return (delete_mutation(RANDOM_MUTATION, failMsg,
                                        force_mutation, false));
        }
    }

    switch (which_mutation)
    {
    case RANDOM_MUTATION:
    case RANDOM_GOOD_MUTATION:
    case RANDOM_BAD_MUTATION:
        mutat = _get_random_mutation(which_mutation);
        break;
    case RANDOM_XOM_MUTATION:
        mutat = _get_random_xom_mutation();
        break;
    case RANDOM_SLIME_MUTATION:
        mutat = _get_random_slime_mutation();
        break;
    default:
        break;
    }

    if (!is_valid_mutation(mutat))
        return (false);

    // [Cha] don't allow teleportation or teleport at will mutations in sprint
    if ((mutat == MUT_TELEPORT || mutat == MUT_TELEPORT_AT_WILL)
        && crawl_state.game_is_sprint())
    {
        return (false);
    }

    if (you.species == SP_NAGA)
    {
        // gdl: Spit poison 'upgrades' to breathe poison.  Why not...
        if (mutat == MUT_SPIT_POISON)
        {
            if (coinflip())
                return (false);

            mutat = MUT_BREATHE_POISON;

            // Breathe poison replaces spit poison (so it takes the slot).
            for (int i = 0; i < 52; ++i)
                if (you.ability_letter_table[i] == ABIL_SPIT_POISON)
                    you.ability_letter_table[i] = ABIL_BREATHE_POISON;
        }
    }

    if (_physiology_mutation_conflict(mutat))
        return (false);

    const mutation_def& mdef = get_mutation_def(mutat);

    if (you.mutation[mutat] >= mdef.levels)
    {
        bool found = false;
        if (you.species == SP_DEMONSPAWN)
            for (unsigned i = 0; i < you.demonic_traits.size(); ++i)
                if (you.demonic_traits[i].mutation == mutat)
                {
                    // This mutation is about to be re-gained, so there is
                    // no need to redraw any stats or print any messages.
                    found = true;
                    you.mutation[mutat]--;
                    break;
                }
        if (!found)
            return (false);
    }

    // God gifts and forced mutations clear away conflicting mutations.
    int rc =_handle_conflicting_mutations(mutat, god_gift || force_mutation);
    if (rc == 1)
        return (true);
    if (rc == -1)
        return (false);

    ASSERT(rc == 0);

    const unsigned int old_talents = your_talents(false).size();

    bool gain_msg = true;

    you.mutation[mutat]++;

    // More than three messages, need to give them by hand.
    switch (mutat)
    {
    case MUT_STRONG: case MUT_AGILE:  case MUT_CLEVER:
    case MUT_WEAK:   case MUT_CLUMSY: case MUT_DOPEY:
        mprf(MSGCH_MUTATION, "You feel %s.", _stat_mut_desc(mutat, true));
        gain_msg = false;
        break;

    default:
        break;
    }

    // For all those scale mutations.
    you.redraw_armour_class = true;

    notify_stat_change("gaining a mutation");

    if (gain_msg)
        mpr(mdef.gain[you.mutation[mutat]-1], MSGCH_MUTATION);

    // Do post-mutation effects.
    switch (mutat)
    {
    case MUT_FRAIL:
    case MUT_ROBUST:
    case MUT_RUGGED_BROWN_SCALES:
        calc_hp();
        break;

    case MUT_LOW_MAGIC:
    case MUT_HIGH_MAGIC:
        calc_mp();
        break;

    case MUT_PASSIVE_MAPPING:
        add_daction(DACT_REAUTOMAP);
        break;

    case MUT_HOOVES:
    case MUT_TALONS:
        // Hooves and talons force boots off at 3.
        if (you.mutation[mutat] >= 3 && !you.melded[EQ_BOOTS])
            remove_one_equip(EQ_BOOTS, false, true);
        break;

    case MUT_CLAWS:
        // Gloves aren't prevented until level 3.
        if (you.mutation[mutat] >= 3 && !you.melded[EQ_GLOVES])
            remove_one_equip(EQ_GLOVES, false, true);
        break;

    case MUT_HORNS:
    case MUT_ANTENNAE:
        // Horns & Antennae 3 removes all headgear.  Same algorithm as with
        // glove removal.
        if (you.mutation[mutat] >= 3 && !you.melded[EQ_HELMET])
            remove_one_equip(EQ_HELMET, false, true);
        // Intentional fall-through
    case MUT_BEAK:
        // Horns, beaks, and antennae force hard helmets off.
        if (you.equip[EQ_HELMET] != -1
            && is_hard_helmet(you.inv[you.equip[EQ_HELMET]])
            && !you.melded[EQ_HELMET])
        {
            remove_one_equip(EQ_HELMET, false, true);
        }
        break;

    case MUT_ACUTE_VISION:
        // We might have to turn autopickup back on again.
        autotoggle_autopickup(false);
        break;

    case MUT_NIGHTSTALKER:
        update_vision_range();
        break;

    default:
        break;
    }

    // Amusement value will be 16 * (11-rarity) * Xom's-sense-of-humor.
    xom_is_stimulated(_calc_mutation_amusement_value(mutat));

    take_note(Note(NOTE_GET_MUTATION, mutat, you.mutation[mutat]));

    if (crawl_state.game_is_hints()
        && your_talents(false).size() > old_talents)
    {
        learned_something_new(HINT_NEW_ABILITY_MUT);
    }
    return (true);
}

static bool _delete_single_mutation_level(mutation_type mutat)
{
    if (you.mutation[mutat] == 0)
        return (false);

    if (you.innate_mutations[mutat] >= you.mutation[mutat])
        return (false);

    const mutation_def& mdef = get_mutation_def(mutat);

    bool lose_msg = true;

    you.mutation[mutat]--;

    switch (mutat)
    {
    case MUT_STRONG: case MUT_AGILE:  case MUT_CLEVER:
    case MUT_WEAK:   case MUT_CLUMSY: case MUT_DOPEY:
        mprf(MSGCH_MUTATION, "You feel %s.", _stat_mut_desc(mutat, false));
        lose_msg = false;
        break;

    case MUT_BREATHE_POISON:
        // can't be removed yet, but still covered:
        if (you.species == SP_NAGA)
        {
            // natural ability to spit poison retakes the slot
            for (int i = 0; i < 52; ++i)
            {
                if (you.ability_letter_table[i] == ABIL_BREATHE_POISON)
                    you.ability_letter_table[i] = ABIL_SPIT_POISON;
            }
        }
        break;

    case MUT_NIGHTSTALKER:
        update_vision_range();
        break;

    default:
        break;
    }

    // For all those scale mutations.
    you.redraw_armour_class = true;

    notify_stat_change("losing a mutation");

    if (lose_msg)
        mpr(mdef.lose[you.mutation[mutat]], MSGCH_MUTATION);

    // Do post-mutation effects.
    if (mutat == MUT_FRAIL || mutat == MUT_ROBUST
        || mutat == MUT_RUGGED_BROWN_SCALES)
    {
        calc_hp();
    }
    if (mutat == MUT_LOW_MAGIC || mutat == MUT_HIGH_MAGIC)
        calc_mp();

    take_note(Note(NOTE_LOSE_MUTATION, mutat, you.mutation[mutat]));

    return (true);
}

bool delete_mutation(mutation_type which_mutation, bool failMsg,
                     bool force_mutation, bool god_gift,
                     bool disallow_mismatch)
{
    if (!god_gift)
    {
        const god_type god =
            (crawl_state.is_god_acting()) ? crawl_state.which_god_acting()
                                          : GOD_NO_GOD;

        if (god != GOD_NO_GOD)
            god_gift = true;
    }

    mutation_type mutat = which_mutation;

    if (!force_mutation)
    {
        if (!god_gift)
        {
            if (player_mutation_level(MUT_MUTATION_RESISTANCE) > 1
                && (player_mutation_level(MUT_MUTATION_RESISTANCE) == 3
                    || coinflip()))
            {
                if (failMsg)
                    mpr("You feel rather odd for a moment.", MSGCH_MUTATION);
                return (false);
            }
        }
    }

    if (which_mutation == RANDOM_MUTATION
        || which_mutation == RANDOM_XOM_MUTATION
        || which_mutation == RANDOM_GOOD_MUTATION
        || which_mutation == RANDOM_BAD_MUTATION
        || which_mutation == RANDOM_NON_SLIME_MUTATION)
    {
        while (true)
        {
            if (one_chance_in(1000))
                return (false);

            mutat = static_cast<mutation_type>(random2(NUM_MUTATIONS));

            if (you.mutation[mutat] == 0
                && mutat != MUT_STRONG
                && mutat != MUT_CLEVER
                && mutat != MUT_AGILE
                && mutat != MUT_WEAK
                && mutat != MUT_DOPEY
                && mutat != MUT_CLUMSY)
            {
                continue;
            }

            if (which_mutation == RANDOM_NON_SLIME_MUTATION
                && _is_slime_mutation(mutat))
            {
                continue;
            }

            if (you.innate_mutations[mutat] >= you.mutation[mutat])
                continue;

            const mutation_def& mdef = get_mutation_def(mutat);

            if (random2(10) >= mdef.rarity && !_is_slime_mutation(mutat))
                continue;

            const bool mismatch =
                (which_mutation == RANDOM_GOOD_MUTATION && mdef.bad)
                    || (which_mutation == RANDOM_BAD_MUTATION && !mdef.bad);

            if (mismatch && (disallow_mismatch || !one_chance_in(10)))
                continue;

            break;
        }
    }
    else if (which_mutation == RANDOM_SLIME_MUTATION)
    {
        mutat = _delete_random_slime_mutation();

        if (mutat == NUM_MUTATIONS)
            return false;
    }

    return (_delete_single_mutation_level(mutat));
}

bool delete_all_mutations()
{
    for (int i = 0; i < NUM_MUTATIONS; ++i)
    {
        while (_delete_single_mutation_level(static_cast<mutation_type>(i)))
            ;
    }

    return (!how_mutated());
}

// Return a string describing the mutation.
// If colour is true, also add the colour annotation.
std::string mutation_name(mutation_type mut, int level, bool colour)
{
    const bool fully_active = mutation_is_fully_active(mut);
    const bool fully_inactive =
        (!fully_active && _mutation_is_fully_inactive(mut));

    // level == -1 means default action of current level
    if (level == -1)
    {
        if (!fully_inactive)
            level = player_mutation_level(mut);
        else // give description of fully active mutation
            level = you.mutation[mut];
    }

    std::string result;
    bool innate = false;

    if (mut == MUT_BREATHE_POISON && you.species == SP_NAGA)
        innate = true;

    const mutation_def& mdef = get_mutation_def(mut);

    if (mut == MUT_STRONG || mut == MUT_CLEVER
        || mut == MUT_AGILE || mut == MUT_WEAK
        || mut == MUT_DOPEY || mut == MUT_CLUMSY)
    {
        std::ostringstream ostr;
        ostr << mdef.have[0] << level << ").";
        result = ostr.str();
    }
    else if (mut == MUT_ICEMAIL)
    {
        std::ostringstream ostr;
        ostr << mdef.have[0] << player_icemail_armour_class() << ").";
        result = ostr.str();
    }
    else if (result.empty() && level > 0)
        result = mdef.have[level - 1];

    if (fully_inactive || player_mutation_level(mut) < you.mutation[mut])
    {
        result = "(" + result;
        result += ")";
    }

    if (colour)
    {
        const char* colourname = (mdef.bad ? "red" : "lightgrey");
        const bool permanent   = (you.innate_mutations[mut] > 0);
        if (innate)
            colourname = (level > 0 ? "cyan" : "lightblue");
        else if (permanent)
        {
            const bool demonspawn = (you.species == SP_DEMONSPAWN);
            const bool extra = (you.mutation[mut] > you.innate_mutations[mut]);

            if (fully_inactive)
                colourname = "darkgrey";
            else if (!fully_active)
                colourname = demonspawn ? "yellow"   : "blue";
            else if (extra)
                colourname = demonspawn ? "lightmagenta" : "cyan";
            else
                colourname = demonspawn ? "magenta"      : "lightblue";
        }
        else if (fully_inactive)
            colourname = "darkgrey";
        else if (_is_slime_mutation(mut))
            colourname = "green";

        // Build the result
        std::ostringstream ostr;
        ostr << '<' << colourname << '>' << result
             << "</" << colourname << '>';
        result = ostr.str();
    }

    return (result);
}

// The tiers of each mutation within a facet should never exceed the overall
// tier of the entire facet, lest ordering / scheduling issues arise.
static const facet_def _demon_facets[] =
{
    // Body Slot facets
    { 2,  { MUT_CLAWS, MUT_CLAWS, MUT_CLAWS },
      { 2, 2, 2 } },
    { 2, { MUT_HORNS, MUT_HORNS, MUT_HORNS },
      { 2, 2, 2 } },
    { 2, { MUT_ANTENNAE, MUT_ANTENNAE, MUT_ANTENNAE },
      { 2, 2, 2 } },
    { 2, { MUT_HOOVES, MUT_HOOVES, MUT_HOOVES },
      { 2, 2, 2 } },
    { 2, { MUT_TALONS, MUT_TALONS, MUT_TALONS },
      { 2, 2, 2 } },
    // Regular facets
    { 3, { MUT_CONSERVE_SCROLLS, MUT_HEAT_RESISTANCE, MUT_HURL_HELLFIRE },
      { 2, 2, 3 } },
    { 3, { MUT_ROBUST, MUT_ROBUST, MUT_ROBUST },
      { 3, 3, 3 } },
    { 3, { MUT_NEGATIVE_ENERGY_RESISTANCE, MUT_NEGATIVE_ENERGY_RESISTANCE,
          MUT_NEGATIVE_ENERGY_RESISTANCE },
      { 2, 2, 3 } },
    { 3, { MUT_STOCHASTIC_TORMENT_RESISTANCE, MUT_STOCHASTIC_TORMENT_RESISTANCE,
          MUT_STOCHASTIC_TORMENT_RESISTANCE },
      { 3, 3, 3 } },
    { 2, { MUT_POWERED_BY_DEATH, MUT_POWERED_BY_DEATH, MUT_POWERED_BY_DEATH },
      { 2, 2, 2 } },
    { 2, { MUT_MAGIC_RESISTANCE, MUT_MAGIC_RESISTANCE, MUT_MAGIC_RESISTANCE },
      { 1, 2, 2 } },
    { 2, { MUT_PASSIVE_MAPPING, MUT_PASSIVE_MAPPING, MUT_PASSIVE_MAPPING },
      { 1, 2, 2 } },
    { 2, { MUT_COLD_RESISTANCE, MUT_CONSERVE_POTIONS, MUT_ICEMAIL },
      { 2, 2, 2 } },
    { 2, { MUT_COLD_RESISTANCE, MUT_CONSERVE_POTIONS, MUT_PASSIVE_FREEZE },
      { 2, 2, 2 } },
    { 2, { MUT_DEMONIC_GUARDIAN, MUT_DEMONIC_GUARDIAN, MUT_DEMONIC_GUARDIAN },
      { 2, 2, 2 } },
    { 2, { MUT_NIGHTSTALKER, MUT_NIGHTSTALKER, MUT_NIGHTSTALKER },
      { 2, 2, 2 } },
    { 2, { MUT_SPINY, MUT_SPINY, MUT_SPINY },
      { 2, 2, 2 } },
    { 2, { MUT_POWERED_BY_PAIN, MUT_POWERED_BY_PAIN, MUT_POWERED_BY_PAIN },
      { 2, 2, 2 } },
    // Scale mutations
    { 1, { MUT_DISTORTION_FIELD, MUT_DISTORTION_FIELD, MUT_DISTORTION_FIELD },
      { 1, 1, 1 } },
    { 1, { MUT_ICY_BLUE_SCALES, MUT_ICY_BLUE_SCALES, MUT_ICY_BLUE_SCALES },
      { 1, 1, 1 } },
    { 1, { MUT_IRIDESCENT_SCALES, MUT_IRIDESCENT_SCALES, MUT_IRIDESCENT_SCALES },
      { 1, 1, 1 } },
    { 1, { MUT_LARGE_BONE_PLATES, MUT_LARGE_BONE_PLATES, MUT_LARGE_BONE_PLATES },
      { 1, 1, 1 } },
    { 1, { MUT_MOLTEN_SCALES, MUT_MOLTEN_SCALES, MUT_MOLTEN_SCALES },
      { 1, 1, 1 } },
    { 1, { MUT_ROUGH_BLACK_SCALES, MUT_ROUGH_BLACK_SCALES, MUT_ROUGH_BLACK_SCALES },
      { 1, 1, 1 } },
    { 1, { MUT_RUGGED_BROWN_SCALES, MUT_RUGGED_BROWN_SCALES,
        MUT_RUGGED_BROWN_SCALES },
      { 1, 1, 1 } },
    { 1, { MUT_SLIMY_GREEN_SCALES, MUT_SLIMY_GREEN_SCALES, MUT_SLIMY_GREEN_SCALES },
      { 1, 1, 1 } },
    { 1, { MUT_THIN_METALLIC_SCALES, MUT_THIN_METALLIC_SCALES,
        MUT_THIN_METALLIC_SCALES },
      { 1, 1, 1 } },
    { 1, { MUT_THIN_SKELETAL_STRUCTURE, MUT_THIN_SKELETAL_STRUCTURE,
        MUT_THIN_SKELETAL_STRUCTURE },
      { 1, 1, 1 } },
    { 1, { MUT_YELLOW_SCALES, MUT_YELLOW_SCALES, MUT_YELLOW_SCALES },
      { 1, 1, 1 } }
};

static bool _works_at_tier(const facet_def& facet, int tier)
{
    return facet.tier == tier;
}

static int _rank_for_tier(const facet_def& facet, int tier)
{
    int k;

    for (k = 0; k < 3 && facet.tiers[k] <= tier; ++k);

    return (k);
}

static bool _slot_is_unique(const mutation_type mut[],
                            std::set<const facet_def *> facets_used)
{
    std::set<const facet_def *>::const_iterator iter;
    equipment_type eq[ARRAYSZ(mut)];

    int k = 0;
    // find the equipment slot(s) used by mut
    for (unsigned i = 0; i < ARRAYSZ(_body_facets); i++)
    {
        for (unsigned j = 0; j < ARRAYSZ(mut); j++)
        {
            if (_body_facets[i].mut == mut[j])
                eq[k++] = _body_facets[i].eq;
        }
    }

    if (k == 0)
        return true;

    for (iter = facets_used.begin() ; iter != facets_used.end() ; iter++)
    {
        for (unsigned i = 0; i < ARRAYSZ(_body_facets); i++)
        {
            if (_body_facets[i].mut == (*iter)->muts[0])
            {
                for (unsigned j = 0; j < ARRAYSZ(eq); j++)
                {
                    if (_body_facets[i].eq == eq[j])
                        return false;
                }
            }
        }
    }

    return true;
}

static std::vector<demon_mutation_info> _select_ds_mutations()
{
    int NUM_BODY_SLOTS = 1;
    int ct_of_tier[] = { 0, 1, 3, 1 };
    // 1 in 10 chance to create a monstrous set
    /* Actually, let's just disable monstrous demonspawn.  Bit startscummy. -dtsund
    if (one_chance_in(10))
    {
        NUM_BODY_SLOTS = 3;
        ct_of_tier[1] = 0;
        ct_of_tier[2] = 5;
    }
    */

try_again:
    std::vector<demon_mutation_info> ret;

    ret.clear();
    int absfacet = 0;
    int scales = 0;
    int slots_lost = 0;
    int ice_elemental = 0;
    int fire_elemental = 0;

    std::set<const facet_def *> facets_used;

    for (int tier = ARRAYSZ(ct_of_tier) - 1; tier >= 0; --tier)
    {
        for (int nfacet = 0; nfacet < ct_of_tier[tier]; ++nfacet)
        {
            const facet_def* next_facet;

            do
            {
                next_facet = &RANDOM_ELEMENT(_demon_facets);
            }
            while (!_works_at_tier(*next_facet, tier)
                   || facets_used.find(next_facet) != facets_used.end()
                   || !_slot_is_unique(next_facet->muts, facets_used));

            facets_used.insert(next_facet);

            for (int i = 0; i < _rank_for_tier(*next_facet, tier); ++i)
            {
                mutation_type m = next_facet->muts[i];

                ret.push_back(demon_mutation_info(m, next_facet->tiers[i],
                                                  absfacet));

                if (_is_covering(m))
                    ++scales;

                if (m == MUT_COLD_RESISTANCE)
                    ice_elemental++;

                if (m == MUT_CONSERVE_SCROLLS)
                    fire_elemental++;

                if (m == MUT_CLAWS && i == 2
                    || m == MUT_HORNS && i == 0
                    || m == MUT_BEAK && i == 0
                    || m == MUT_ANTENNAE && i == 0
                    || m == MUT_TALONS && i == 2
                    || m == MUT_HOOVES && i == 2)
                {
                    ++slots_lost;
                }
            }

            ++absfacet;
        }
    }

    if (scales > 3)
        goto try_again;

    if (slots_lost != NUM_BODY_SLOTS)
        goto try_again;

    if (ice_elemental + fire_elemental > 1)
        goto try_again;

    return ret;
}

static std::vector<mutation_type>
_order_ds_mutations(std::vector<demon_mutation_info> muts)
{
    std::vector<mutation_type> out;

    while (!muts.empty())
    {
        int first_tier = 99;

        for (unsigned i = 0; i < muts.size(); ++i)
            first_tier = std::min(first_tier, muts[i].tier);

        int ix;

        do
        {
            ix = random2(muts.size());
        }
        // Don't consider mutations from more than two tiers at a time
        while (muts[ix].tier >= first_tier + 2);

        // Don't reorder mutations within a facet
        for (int j = 0; j < ix; ++j)
        {
            if (muts[j].facet == muts[ix].facet)
            {
                ix = j;
                break;
            }
        }

        out.push_back(muts[ix].mut);
        muts.erase(muts.begin() + ix);
    }

    return out;
}

static std::vector<player::demon_trait>
_schedule_ds_mutations(std::vector<mutation_type> muts)
{
    std::list<mutation_type> muts_left(muts.begin(), muts.end());

    std::list<int> slots_left;

    std::vector<player::demon_trait> out;

    for (int level = 2; level <= 27; ++level)
    {
        int ct = coinflip() ? 2 : 1;

        for (int i = 0; i < ct; ++i)
            slots_left.push_back(level);
    }

    while (!muts_left.empty())
    {
        if (x_chance_in_y(muts_left.size(), slots_left.size()))
        {
            player::demon_trait dt;

            dt.level_gained = slots_left.front();
            dt.mutation     = muts_left.front();

#ifdef DEBUG_DIAGNOSTICS
            mprf(MSGCH_DIAGNOSTICS, "Demonspawn will gain %s at level %d",
                    get_mutation_def(dt.mutation).wizname, dt.level_gained);
#endif

            out.push_back(dt);

            muts_left.pop_front();
        }
        slots_left.pop_front();
    }

    return out;
}

void roll_demonspawn_mutations()
{
    you.demonic_traits = _schedule_ds_mutations(
                         _order_ds_mutations(
                         _select_ds_mutations()));
}

bool perma_mutate(mutation_type which_mut, int how_much)
{
    ASSERT(is_valid_mutation(which_mut));

    how_much = std::min(static_cast<short>(how_much),
                        get_mutation_def(which_mut).levels);

    int rc = 1;
    // clear out conflicting mutations
    int count = 0;
    while (rc == 1 && ++count < 100)
        rc = _handle_conflicting_mutations(which_mut, true);
    ASSERT(rc == 0);

    int levels = 0;
    while (how_much-- > 0)
        if (you.mutation[which_mut] > you.innate_mutations[which_mut]
            || mutate(which_mut, false, true, false, false, true))
        {
            levels++;
        }

    you.innate_mutations[which_mut] += levels;

    return (levels > 0);
}

int how_mutated(bool all, bool levels)
{
    int j = 0;

    for (int i = 0; i < NUM_MUTATIONS; ++i)
    {
        if (you.mutation[i])
        {
            if (!all && you.innate_mutations[i] >= you.mutation[i])
                continue;

            if (levels)
            {
                // These allow for 14 levels.
                if (i == MUT_STRONG || i == MUT_CLEVER || i == MUT_AGILE
                    || i == MUT_WEAK || i == MUT_DOPEY || i == MUT_CLUMSY)
                {
                    j += (you.mutation[i] / 5 + 1);
                }
                else
                    j += you.mutation[i];
            }
            else
                j++;
        }
    }

    dprf("how_mutated(): all = %u, levels = %u, j = %d", all, levels, j);

    return (j);
}

// Return whether current tension is balanced
bool balance_demonic_guardian()
{
    const int mutlevel = player_mutation_level(MUT_DEMONIC_GUARDIAN);

    int tension = get_tension(GOD_NO_GOD), mons_val = 0, total = 0;
    monster_iterator mons;

    // tension is unfavorably high, perhaps another guardian should spawn
    if (tension*3/4 > mutlevel*6 + random2(mutlevel*mutlevel*2))
        return (false);

    for (int i = 0; mons && i <= 20/mutlevel; mons++)
    {
        mons_val = get_monster_tension(*mons, GOD_NO_GOD);
        const mon_attitude_type att = mons_attitude(*mons);

        if (testbits(mons->flags, MF_DEMONIC_GUARDIAN)
            && total < random2(mutlevel * 5)
            && att == ATT_FRIENDLY
            && !one_chance_in(3)
            && !mons->has_ench(ENCH_LIFE_TIMER))
        {
            mpr(mons->name(DESC_CAP_THE) + " "
                + summoned_poof_msg(*mons) + "!", MSGCH_PLAIN);
            monster_die(*mons, KILL_NONE, NON_MONSTER);
        }
        else
            total += mons_val;
    }

    return (true);
}

#define random_mons(...) static_cast<monster_type>(random_choose(__VA_ARGS__))

// Primary function to handle and balance demonic guardians, if the tension
// is unfavorably high and a guardian was not recently spawned, a new guardian
// will be made, if tension is below a threshold (determined by the mutations
// level and a bit of randomness), guardians may be dismissed in
// balance_demonic_guardian()
void check_demonic_guardian()
{
    const int mutlevel = player_mutation_level(MUT_DEMONIC_GUARDIAN);

    if (!balance_demonic_guardian() &&
        you.duration[DUR_DEMONIC_GUARDIAN] == 0)
    {
        monster_type mt;

        switch(mutlevel)
        {
        case 1:
            mt = random_mons(MONS_WHITE_IMP, MONS_LEMURE, MONS_UFETUBUS,
                             MONS_IRON_IMP, MONS_MIDGE, -1);
            break;
        case 2:
            mt = random_mons(MONS_ORANGE_DEMON, MONS_SMOKE_DEMON, MONS_IRON_DEVIL,
                             MONS_BLUE_DEVIL, MONS_HAIRY_DEVIL, -1);
            break;
        case 3:
            mt = random_mons(MONS_EXECUTIONER, MONS_BALRUG, MONS_ICE_DEVIL,
                             MONS_SOUL_EATER, MONS_SUN_DEMON, -1);
            break;
        default:
            die("Invalid demonic guardian level: %d", mutlevel);
        }

        const int guardian = create_monster(mgen_data(mt, BEH_FRIENDLY, &you,
                                                      2, 0, you.pos(),
                                                      MHITYOU, MG_FORCE_BEH));

        if (guardian == -1)
            return;

        menv[guardian].flags |= MF_NO_REWARD;
        menv[guardian].flags |= MF_DEMONIC_GUARDIAN;

        menv[guardian].add_ench(ENCH_LIFE_TIMER);

        // no more guardians for mutlevel+1 to mutlevel+20 turns
        you.duration[DUR_DEMONIC_GUARDIAN] = 10*(mutlevel + random2(20));
    }
}

void check_antennae_detect()
{
    int radius = player_mutation_level(MUT_ANTENNAE) * 2 - 1;
    if (you.religion == GOD_ASHENZARI && !player_under_penance())
        radius = std::max(radius, you.piety / 20);
    if (radius <= 0)
        return;
    radius = std::min(radius, LOS_RADIUS);

    for (radius_iterator ri(you.pos(), radius, C_ROUND); ri; ++ri)
    {
        monster* mon = monster_at(*ri);
        map_cell& cell = env.map_knowledge(*ri);
        if (!mon)
        {
            if (cell.detected_monster())
                cell.clear_monster();
        }
        else if (!mons_is_firewood(mon))
        {
            // [ds] If the PC remembers the correct monster at this
            // square, don't trample it with MONS_SENSED. Forgetting
            // legitimate monster memory affects travel, which can
            // path around mimics correctly only if it can actually
            // *see* them in monster memory -- overwriting the mimic
            // with MONS_SENSED causes travel to bounce back and
            // forth, since every time it leaves LOS of the mimic, the
            // mimic is forgotten (replaced by MONS_SENSED).
            const monster_type remembered_monster = cell.monster();
            if (remembered_monster != mon->type)
            {
                if (mons_is_unknown_mimic(mon))
                    discover_mimic(mon);
                monster_type mc = MONS_SENSED;
                if (you.religion == GOD_ASHENZARI && !player_under_penance())
                    mc = ash_monster_tier(mon);
                env.map_knowledge(*ri).set_detected_monster(mc);

                if (mc == MONS_SENSED_TRIVIAL || mc == MONS_SENSED_EASY
                    || mc == MONS_SENSED_FRIENDLY
                    || testbits(mon->flags, MF_SENSED))
                {
                    continue;
                }

                for (radius_iterator ri2(mon->pos(), 2, C_SQUARE); ri2; ++ri2)
                    if (you.see_cell(*ri2))
                    {
                        mon->flags |= MF_SENSED;
                        interrupt_activity(AI_SENSE_MONSTER);
                    }
            }
        }
    }
}

int handle_pbd_corpses(bool do_rot)
{
    int corpse_count = 0;

    for (radius_iterator ri(you.pos(),
         player_mutation_level(MUT_POWERED_BY_DEATH) * 3); ri; ++ri)
    {
        for (stack_iterator j(*ri); j; ++j)
        {
            if (j->base_type == OBJ_CORPSES && j->sub_type == CORPSE_BODY
                && j->special > 50)
            {
                ++corpse_count;

                int chance = player_mutation_level(MUT_POWERED_BY_DEATH)*16;
                if (do_rot && x_chance_in_y(you.duration[DUR_POWERED_BY_DEATH],
                                chance))
                {
                    j->special -= random2(3);
                }

                if (corpse_count == 7)
                    break;
            }
        }
    }

    return (corpse_count);
}
