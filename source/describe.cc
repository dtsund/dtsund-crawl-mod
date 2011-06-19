/**
 * @file
 * @brief Functions used to print information about various game objects.
**/

#include "AppHdr.h"

#include "describe.h"
#include "database.h"

#include <stdio.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <numeric>

#include "externs.h"
#include "options.h"
#include "species.h"

#include "abl-show.h"
#include "artefact.h"
#include "cio.h"
#include "clua.h"
#include "debug.h"
#include "decks.h"
#include "delay.h"
#include "fight.h"
#include "food.h"
#include "ghost.h"
#include "godabil.h"
#include "goditem.h"
#include "godpassive.h"
#include "invent.h"
#include "item_use.h"
#include "itemname.h"
#include "itemprop.h"
#include "items.h"
#include "it_use3.h"
#include "jobs.h"
#include "libutil.h"
#include "macro.h"
#include "menu.h"
#include "message.h"
#include "mon-stuff.h"
#include "mon-util.h"
#include "output.h"
#include "player.h"
#include "quiver.h"
#include "random.h"
#include "religion.h"
#include "rng.h"
#include "skills2.h"
#include "spl-book.h"
#include "spl-clouds.h"
#include "stuff.h"
#include "env.h"
#include "spl-cast.h"
#include "spl-util.h"
#include "transform.h"
#include "hints.h"
#include "xom.h"
#include "mon-info.h"

#define LONG_DESC_KEY "long_desc_key"
#define QUOTE_KEY "quote_key"

// ========================================================================
//      Internal Functions
// ========================================================================

//---------------------------------------------------------------
//
// append_value
//
// Appends a value to the string. If plussed == 1, will add a + to
// positive values (itoa always adds - to -ve ones).
//
//---------------------------------------------------------------
static void _append_value(std::string & description, int valu, bool plussed)
{
    if (valu >= 0 && plussed == 1)
        description += "+";

    char value_str[80];

    itoa(valu, value_str, 10);

    description += value_str;
}

int count_desc_lines(const std::string _desc, const int width)
{
    std::string desc = get_linebreak_string(_desc, width);

    int count = 0;
    for (int i = 0, size = desc.size(); i < size; ++i)
    {
        const char ch = desc[i];

        if (ch == '\n')
            count++;
    }

    return count;
}

//---------------------------------------------------------------
//
// print_description
//
// Takes a descrip string filled up with stuff from other functions,
// and displays it with minor formatting to avoid cut-offs in mid
// word and such.
//
//---------------------------------------------------------------

void print_description(const std::string &body)
{
    describe_info inf;
    inf.body << body;
    print_description(inf);
}

void print_quote (const std::string &body)
{
    describe_info inf;
    inf.quote = body;
    print_quote(inf);
}

class default_desc_proc
{
public:
    int width() { return get_number_of_cols() - 1; }
    int height() { return get_number_of_lines(); }
    void print(const std::string &str) { cprintf("%s", str.c_str()); }

    void nextline()
    {
        if (wherey() < height())
            cgotoxy(1, wherey() + 1);
        else
            cgotoxy(1, height());
        // Otherwise cgotoxy asserts; let's just clobber the last line
        // instead, which should be noticable enough.
    }
};

void print_description(const describe_info &inf)
{
    clrscr();
    textcolor(LIGHTGREY);

    default_desc_proc proc;
    process_description<default_desc_proc>(proc, inf);
}

void print_quote (const describe_info &inf)
{
    clrscr();
    textcolor(LIGHTGREY);

    default_desc_proc proc;
    process_quote<default_desc_proc>(proc, inf);
}

const char* jewellery_base_ability_string(int subtype)
{
    switch (subtype)
    {
    case RING_REGENERATION:      return "Regen";
    case RING_SUSTAIN_ABILITIES: return "SustAb";
    case RING_SUSTENANCE:        return "Hunger-";
    case RING_WIZARDRY:          return "Wiz";
    case RING_FIRE:              return "Fire";
    case RING_ICE:               return "Ice";
    case RING_TELEPORT_CONTROL:  return "cTele";
    case AMU_CLARITY:            return "Clar";
    case AMU_WARDING:            return "Ward";
    case AMU_RESIST_CORROSION:   return "rCorr";
    case AMU_THE_GOURMAND:       return "Gourm";
    case AMU_CONSERVATION:       return "Cons";
    case AMU_CONTROLLED_FLIGHT:  return "cFly";
    case AMU_RESIST_MUTATION:    return "rMut";
    case AMU_GUARDIAN_SPIRIT:    return "Spirit";
    case AMU_FAITH:              return "Faith";
    case AMU_STASIS:             return "Stasis";
    case AMU_INACCURACY:         return "Inacc";
    }
    return "";
}


#define known_proprt(prop) (proprt[(prop)] && known[(prop)])

struct property_annotators
{
    const char* name;
    artefact_prop_type prop;
    int spell_out;              // 0: "+3", 1: "+++", 2: value doesn't matter
};

static std::vector<std::string> _randart_propnames(const item_def& item,
                                                   bool no_comma = false)
{
    artefact_properties_t  proprt;
    artefact_known_props_t known;
    artefact_desc_properties(item, proprt, known, true);

    std::vector<std::string> propnames;

    // list the following in rough order of importance
    const property_annotators propanns[] = {

        // (Generally) negative attributes
        // These come first, so they don't get chopped off!
        { "-CAST",  ARTP_PREVENT_SPELLCASTING,  2 },
        { "-TELE",  ARTP_PREVENT_TELEPORTATION, 2 },
        { "MUT",    ARTP_MUTAGENIC,             2 }, // handled specially
        { "*Rage",  ARTP_ANGRY,                 2 },
        { "*TELE",  ARTP_CAUSE_TELEPORTATION,   2 },
        { "Hunger", ARTP_METABOLISM,            2 }, // handled specially
        { "Noisy",  ARTP_NOISES,                2 },
        { "Slow",   ARTP_PONDEROUS,             2 },

        // Evokable abilities come second
        { "+Blink", ARTP_BLINK,                 2 },
        { "+Rage",  ARTP_BERSERK,               2 },
        { "+Inv",   ARTP_INVISIBLE,             2 },
        { "+Lev",   ARTP_LEVITATE,              2 },

        // Resists, also really important
        { "rElec",  ARTP_ELECTRICITY,           2 },
        { "rPois",  ARTP_POISON,                2 },
        { "rF",     ARTP_FIRE,                  1 },
        { "rC",     ARTP_COLD,                  1 },
        { "rN",     ARTP_NEGATIVE_ENERGY,       1 },
        { "MR",     ARTP_MAGIC,                 2 },

        // Quantitative attributes
        { "AC",     ARTP_AC,                    0 },
        { "EV",     ARTP_EVASION,               0 },
        { "Str",    ARTP_STRENGTH,              0 },
        { "Dex",    ARTP_DEXTERITY,             0 },
        { "Int",    ARTP_INTELLIGENCE,          0 },
        { "Acc",    ARTP_ACCURACY,              0 },
        { "Dam",    ARTP_DAMAGE,                0 },

        // Qualitative attributes
        { "MP",     ARTP_MAGICAL_POWER,         0 },
        { "SInv",   ARTP_EYESIGHT,              2 },
        { "Stlth",  ARTP_STEALTH,               2 }, // handled specially
        { "Curse",  ARTP_CURSED,                2 },
    };

    // For randart jewellery, note the base jewellery type if it's not
    // covered by artefact_desc_properties()
    if (item.base_type == OBJ_JEWELLERY
        && item_ident(item, ISFLAG_KNOW_PROPERTIES))
    {
        const std::string type = jewellery_base_ability_string(item.sub_type);
        if (!type.empty())
            propnames.push_back(type);
    }
    else if (item_ident(item, ISFLAG_KNOW_TYPE)
             || is_artefact(item)
                && artefact_known_wpn_property(item, ARTP_BRAND))
    {
        std::string ego;
        if (item.base_type == OBJ_WEAPONS)
            ego = weapon_brand_name(item, true);
        else if (item.base_type == OBJ_ARMOUR)
            ego = armour_ego_name(item, true);
        if (!ego.empty())
        {
            // XXX: Ugly hack to remove the brackets...
            ego = ego.substr(2, ego.length() - 3);

            // ... and another one for adding a comma if needed.
            for (unsigned i = 0; i < ARRAYSZ(propanns); ++i)
                if (known_proprt(propanns[i].prop)
                    && propanns[i].prop != ARTP_BRAND
                    && !no_comma)
                {
                    ego += ",";
                    break;
                }

            propnames.push_back(ego);
        }
    }

    for (unsigned i = 0; i < ARRAYSZ(propanns); ++i)
    {
        if (known_proprt(propanns[i].prop))
        {
            const int val = proprt[propanns[i].prop];

            // Don't show rF+/rC- for =Fire, or vice versa for =Ice.
            if (item.base_type == OBJ_JEWELLERY)
            {
                if (item.sub_type == RING_FIRE
                    && (propanns[i].prop == ARTP_FIRE && val == 1
                        || propanns[i].prop == ARTP_COLD && val == -1))
                {
                    continue;
                }
                if (item.sub_type == RING_ICE
                    && (propanns[i].prop == ARTP_COLD && val == 1
                        || propanns[i].prop == ARTP_FIRE && val == -1))
                {
                    continue;
                }
            }

            std::ostringstream work;
            switch (propanns[i].spell_out)
            {
            case 0: // e.g. AC+4
                work << std::showpos << propanns[i].name << val;
                break;
            case 1: // e.g. F++
            {
                int sval = std::min(std::abs(val), 3);
                work << propanns[i].name
                     << std::string(sval, (val > 0 ? '+' : '-'));
                break;
            }
            case 2: // e.g. rPois or SInv
                if (propanns[i].prop == ARTP_CURSED && val < 1)
                    continue;

                work << propanns[i].name;

                // these need special handling, so we don't give anything away
                if (propanns[i].prop == ARTP_METABOLISM && val > 2
                    || propanns[i].prop == ARTP_MUTAGENIC && val > 3)
                {
                    work << "+";
                }
                else if (propanns[i].prop == ARTP_STEALTH)
                {
                    if (val > 20)
                        work << "++";
                    else if (val > 0)
                        work << "+";
                    else if (val < -20)
                        work << "--";
                    else if (val < 0)
                        work << "-";
                }
                break;
            }
            propnames.push_back(work.str());
        }
    }

    return propnames;
}

// Remove randart auto-inscription.  Do it once for each property
// string, rather than the return value of artefact_auto_inscription(),
// in case more information about the randart has been learned since
// the last auto-inscription.
void trim_randart_inscrip(item_def& item)
{
    std::vector<std::string> propnames = _randart_propnames(item, true);

    for (unsigned int i = 0; i < propnames.size(); ++i)
    {
        item.inscription = replace_all(item.inscription, propnames[i]+",", "");
        item.inscription = replace_all(item.inscription, propnames[i],     "");
    }
    trim_string(item.inscription);
}

std::string artefact_auto_inscription(const item_def& item)
{
    if (item.base_type == OBJ_BOOKS)
        return ("");

    const std::vector<std::string> propnames = _randart_propnames(item);

    return (comma_separated_line(propnames.begin(), propnames.end(),
                                 " ", " "));
}

void add_autoinscription(item_def &item, std::string ainscrip)
{
    // Remove previous randart inscription.
    trim_randart_inscrip(item);

    add_inscription(item, ainscrip);
}

void add_autoinscription(item_def &item)
{
    // Remove previous randart inscription.
    trim_randart_inscrip(item);

    add_inscription(item, artefact_auto_inscription(item));
}

void add_inscription(item_def &item, std::string inscrip)
{
    if (!item.inscription.empty())
    {
        if (ends_with(item.inscription, ","))
            item.inscription += " ";
        else
            item.inscription += ", ";
    }

    item.inscription += inscrip;
}

struct property_descriptor
{
    artefact_prop_type property;
    const char* desc;           // If it contains %d, will be replaced by value.
    bool is_graded_resist;
};

static std::string _randart_descrip(const item_def &item)
{
    std::string description;

    artefact_properties_t  proprt;
    artefact_known_props_t known;
    artefact_desc_properties(item, proprt, known);

    const property_descriptor propdescs[] = {
        { ARTP_AC, "It affects your AC (%d).", false },
        { ARTP_EVASION, "It affects your evasion (%d).", false},
        { ARTP_STRENGTH, "It affects your strength (%d).", false},
        { ARTP_INTELLIGENCE, "It affects your intelligence (%d).", false},
        { ARTP_DEXTERITY, "It affects your dexterity (%d).", false},
        { ARTP_ACCURACY, "It affects your accuracy (%d).", false},
        { ARTP_DAMAGE, "It affects your damage-dealing abilities (%d).", false},
        { ARTP_FIRE, "fire", true},
        { ARTP_COLD, "cold", true},
        { ARTP_ELECTRICITY, "It insulates you from electricity.", false},
        { ARTP_POISON, "It protects you from poison.", false},
        { ARTP_NEGATIVE_ENERGY, "negative energy", true},
        { ARTP_MAGIC, "It increases your resistance to enchantments.", false},
        { ARTP_MAGICAL_POWER, "It affects your mana capacity (%d).", false},
        { ARTP_EYESIGHT, "It enhances your eyesight.", false},
        { ARTP_INVISIBLE, "It lets you turn invisible.", false},
        { ARTP_LEVITATE, "It lets you levitate.", false},
        { ARTP_BLINK, "It lets you blink.", false},
        { ARTP_BERSERK, "It lets you go berserk.", false},
        { ARTP_NOISES, "It makes noises.", false},
        { ARTP_PREVENT_SPELLCASTING, "It prevents spellcasting.", false},
        { ARTP_CAUSE_TELEPORTATION, "It causes teleportation.", false},
        { ARTP_PREVENT_TELEPORTATION, "It prevents most forms of teleportation.",
          false},
        { ARTP_ANGRY,  "It makes you angry.", false},
        { ARTP_CURSED, "It may recurse itself.", false},
        { ARTP_PONDEROUS, "It slows your movement.", false},
    };

    for (unsigned i = 0; i < ARRAYSZ(propdescs); ++i)
    {
        if (known_proprt(propdescs[i].property))
        {
            // Only randarts with ARTP_CURSED > 0 may recurse themselves.
            if (propdescs[i].property == ARTP_CURSED
                && proprt[propdescs[i].property] < 1)
            {
                continue;
            }

            std::string sdesc = propdescs[i].desc;

            // FIXME Not the nicest hack.
            char buf[80];
            snprintf(buf, sizeof buf, "%+d", proprt[propdescs[i].property]);
            sdesc = replace_all(sdesc, "%d", buf);

            if (propdescs[i].is_graded_resist)
            {
                int idx = proprt[propdescs[i].property] + 3;
                idx = std::min(idx, 6);
                idx = std::max(idx, 0);

                const char* prefixes[] = {
                    "It makes you extremely vulnerable to ",
                    "It makes you very vulnerable to ",
                    "It makes you vulnerable to ",
                    "Buggy descriptor!",
                    "It protects you from ",
                    "It greatly protects you from ",
                    "It renders you almost immune to "
                };
                sdesc = prefixes[idx] + sdesc + '.';
            }

            description += '\n';
            description += sdesc;
        }
    }

    // Some special cases which don't fit into the above.
    if (known_proprt(ARTP_METABOLISM))
    {
        if (proprt[ ARTP_METABOLISM ] >= 3)
            description += "\nIt greatly speeds your metabolism.";
        else if (proprt[ ARTP_METABOLISM ])
            description += "\nIt speeds your metabolism. ";
    }

    if (known_proprt(ARTP_STEALTH))
    {
        const int stval = proprt[ARTP_STEALTH];
        char buf[80];
        snprintf(buf, sizeof buf, "\nIt makes you %s%s stealthy.",
                 (stval < -20 || stval > 20) ? "much " : "",
                 (stval < 0) ? "less" : "more");
        description += buf;
    }

    if (known_proprt(ARTP_MUTAGENIC))
    {
        if (proprt[ ARTP_MUTAGENIC ] > 3)
            description += "\nIt glows with mutagenic radiation.";
        else
            description += "\nIt emits mutagenic radiation.";
    }

    return description;
}
#undef known_proprt

static const char *trap_names[] =
{
    "dart", "arrow", "spear", "axe",
    "teleport", "alarm", "blade",
    "bolt", "net", "zot", "needle",
    "shaft", "passage", "pressure plate",
};

const char *trap_name(trap_type trap)
{
    ASSERT(NUM_TRAPS == sizeof(trap_names) / sizeof(*trap_names));

    if (trap >= TRAP_DART && trap < NUM_TRAPS)
        return trap_names[trap];
    return (NULL);
}

int str_to_trap(const std::string &s)
{
    ASSERT(NUM_TRAPS == sizeof(trap_names) / sizeof(*trap_names));

    // allow a couple of synonyms
    if (s == "random" || s == "any")
        return (TRAP_RANDOM);
    else if (s == "suitable")
        return (TRAP_INDEPTH);
    else if (s == "nonteleport" || s == "noteleport"
             || s == "nontele" || s == "notele")
    {
        return (TRAP_NONTELEPORT);
    }

    for (int i = 0; i < NUM_TRAPS; ++i)
        if (trap_names[i] == s)
            return (i);

    return (-1);
}

//---------------------------------------------------------------
//
// describe_demon
//
// Describes the random demons you find in Pandemonium.
//
//---------------------------------------------------------------
static std::string _describe_demon(const std::string& name, flight_type fly)
{
    const long seed =
        std::accumulate(name.begin(), name.end(), 0L) *
        name.length();

    rng_save_excursion exc;
    seed_rng(seed);

    const char* body_descs[] = {
        " huge, barrel-shaped ",
        " wispy, insubstantial ",
        " spindly ",
        " skeletal ",
        " horribly deformed ",
        " spiny ",
        " waif-like ",
        " scaly ",
        " sickeningly deformed ",
        " bruised and bleeding ",
        " sickly ",
        " mass of writhing tentacles for a ",
        " mass of ropey tendrils for a ",
        " tree trunk-like ",
        " hairy ",
        " furry ",
        " fuzzy ",
        "n obese ",
        " fat ",
        " slimy ",
        " wrinkled ",
        " metallic ",
        " glassy ",
        " crystalline ",
        " muscular ",
        "n icky ",
        " swollen ",
        " lumpy ",
        "n armoured ",
        " carapaced ",
        " slender "
    };

    const char* wing_names[] = {
        " with small insectoid wings",
        " with large insectoid wings",
        " with moth-like wings",
        " with butterfly wings",
        " with huge, bat-like wings",
        " with fleshy wings",
        " with small, bat-like wings",
        " with hairy wings",
        " with great feathered wings",
        " with shiny metal wings"
    };

    const char* lev_names[] = {
        " which hovers in mid-air",
        " with sacs of gas hanging from its back"
    };

    const char* nonfly_names[] = {
        " covered in tiny crawling spiders",
        " covered in tiny crawling insects",
        " and the head of a crocodile",
        " and the head of a hippopotamus",
        " and a cruel curved beak for a mouth",
        " and a straight sharp beak for a mouth",
        " and no head at all",
        " and a hideous tangle of tentacles for a mouth",
        " and an elephantine trunk",
        " and an evil-looking proboscis",
        " and dozens of eyes",
        " and two ugly heads",
        " and a long serpentine tail",
        " and a pair of huge tusks growing from its jaw",
        " and a single huge eye, in the centre of its forehead",
        " and spikes of black metal for teeth",
        " and a disc-shaped sucker for a head",
        " and huge, flapping ears",
        " and a huge, toothy maw in the centre of its chest",
        " and a giant snail shell on its back",
        " and a dozen heads",
        " and the head of a jackal",
        " and the head of a baboon",
        " and a huge, slobbery tongue",
        " which is covered in oozing lacerations",
        " and the head of a frog",
        " and the head of a yak",
        " and eyes out on stalks",
    };

    const char* misc_descs[] = {
        " It seethes with hatred of the living.",
        " Tiny orange flames dance around it.",
        " Tiny purple flames dance around it.",
        " It is surrounded by a weird haze.",
        " It glows with a malevolent light.",
        " It looks incredibly angry.",
        " It oozes with slime.",
        " It dribbles constantly.",
        " Mould grows all over it.",
        " It looks diseased.",
        " It looks as frightened of you as you are of it.",
        " It moves in a series of hideous convulsions.",
        " It moves with an unearthly grace.",
        " It hungers for your soul!",
        " It leaves a glistening oily trail.",
        " It shimmers before your eyes.",
        " It is surrounded by a brilliant glow.",
        " It radiates an aura of extreme power.",
    };

    std::ostringstream description;
    description << "A powerful demon, " << name << " has a"
                << RANDOM_ELEMENT(body_descs) << "body";

    switch (fly)
    {
    case FL_FLY:
        description << RANDOM_ELEMENT(wing_names);
        break;

    case FL_LEVITATE:
        description << RANDOM_ELEMENT(lev_names);
        break;

    case FL_NONE:  // does not fly
        if (!one_chance_in(4))
            description << RANDOM_ELEMENT(nonfly_names);
        break;
    }

    description << ".";

    if (x_chance_in_y(3, 40))
    {
        if (you.can_smell())
        {
            switch (random2(3))
            {
            case 0:
                description << " It stinks of brimstone.";
                break;
            case 1:
                description << " It is surrounded by a sickening stench.";
                break;
            case 2:
                description << " It smells like rotting flesh"
                            << (player_mutation_level(MUT_SAPROVOROUS) == 3 ?
                                " - yum!" : ".");
                break;
            }
        }
    }
    else if (coinflip())
        description << RANDOM_ELEMENT(misc_descs);

    return description.str();
}

void append_weapon_stats(std::string &description, const item_def &item)
{
    description += "\nAccuracy rating: ";
    _append_value(description, property(item, PWPN_HIT), true);
    description += "    ";

    description += "Damage rating: ";
    _append_value(description, property(item, PWPN_DAMAGE), false);
    description += "   ";

    description += "Base attack delay: ";
    _append_value(description, property(item, PWPN_SPEED) * 10, false);
   description += "%";
}

static std::string _corrosion_resistance_string(const item_def &item)
{
    const int ench = item.base_type == OBJ_WEAPONS ? item.plus2 : item.plus;
    const char* format = "\nIts enchantment level renders it %s to acidic "
                         "corrosion.";

    if (is_artefact(item))
        return "";
    if (ench >= 5 && item_ident(item, ISFLAG_KNOW_PLUSES))
        return make_stringf(format, "immune");
    else if (ench >= 4 && item_ident(item, ISFLAG_KNOW_PLUSES))
        return make_stringf(format, "extremely resistant");
    else if (item.base_type == OBJ_ARMOUR
             && item.sub_type == ARM_CRYSTAL_PLATE_MAIL)
    {
        return "\nBeing made of crystal renders it very resistant to acidic "
               "corrosion.";
    }
    else if (get_equip_race(item) == ISFLAG_DWARVEN)
    {
        return "\nBeing of dwarven fabrication renders it very resistant to "
               "acidic corrosion.";
    }
    else if (ench >= 3 && item_ident(item, ISFLAG_KNOW_PLUSES))
        return make_stringf(format, "resistant");
    else if (ench >= 2 && item_ident(item, ISFLAG_KNOW_PLUSES))
        return make_stringf(format, "somewhat resistant");
    else
        return "";
}

//---------------------------------------------------------------
//
// describe_weapon
//
//---------------------------------------------------------------
static std::string _describe_weapon(const item_def &item, bool verbose)
{
    std::string description;

    description.reserve(200);

    description = "";

    if (verbose)
        append_weapon_stats(description, item);

    int spec_ench = get_weapon_brand(item);

    if (!is_artefact(item) && !verbose)
        spec_ench = SPWPN_NORMAL;

    // special weapon descrip
    if (spec_ench != SPWPN_NORMAL && item_type_known(item))
    {
        description += "\n\n";

        switch (spec_ench)
        {
        case SPWPN_FLAMING:
            description += "It emits flame when wielded, causing extra "
                "injury to most foes and up to double damage against "
                "particularly susceptible opponents.";
            if (get_vorpal_type(item) & (DVORP_SLICING | DVORP_CHOPPING))
            {
                description += " Big, fiery blades are also staple armaments "
                    "of hydra-hunters.";
            }
            break;
        case SPWPN_FREEZING:
            description += "It has been specially enchanted to freeze "
                "those struck by it, causing extra injury to most foes "
                "and up to double damage against particularly "
                "susceptible opponents. It can also slow down "
                "cold-blooded creatures.";
            break;
        case SPWPN_HOLY_WRATH:
            description += "It has been blessed by the Shining One to "
                "cause great damage to the undead and demons.";
            break;
        case SPWPN_ELECTROCUTION:
            if (is_range_weapon(item))
            {
                description += "It charges the ammunition it shoots with "
                    "electricity; occasionally upon a hit, such missiles "
                    "may discharge and cause terrible harm.";
            }
            else
            {
                description += "Occasionally, upon striking a foe, it will "
                    "discharge some electrical energy and cause terrible "
                    "harm.";
            }
            break;
        case SPWPN_ORC_SLAYING:
            description += "It is especially effective against all of "
                "orcish descent.";
            break;
        case SPWPN_DRAGON_SLAYING:
            description += "This legendary weapon is deadly to all "
                "dragonkind. It also provides some protection from the "
                "breath attacks of dragons and other creatures.";
            break;
        case SPWPN_VENOM:
            if (is_range_weapon(item))
                description += "It poisons the ammo it fires.";
            else
                description += "It poisons the flesh of those it strikes.";
            break;
        case SPWPN_PROTECTION:
            description += "It protects the one who wields it against "
                "injury (+5 to AC).";
            break;
        case SPWPN_EVASION:
            description += "It affects your evasion (+5 to EV).";
            break;
        case SPWPN_DRAINING:
            description += "A truly terrible weapon, it drains the "
                "life of those it strikes.";
            break;
        case SPWPN_SPEED:
            description += "Attacks with this weapon take half as long, "
                "but cause less damage.";
            break;
        case SPWPN_VORPAL:
            if (is_range_weapon(item))
            {
                description += "Any ";
                description += ammo_name(item);
                description += " fired from it inflicts extra damage.";
            }
            else
            {
                description += "It inflicts extra damage upon your "
                    "enemies.";
            }
            break;
        case SPWPN_FLAME:
            description += "It turns projectiles fired from it into "
                "bolts of flame.";
            break;
        case SPWPN_FROST:
            description += "It turns projectiles fired from it into "
                "bolts of frost.";
            break;
        case SPWPN_CHAOS:
            if (is_range_weapon(item))
            {
                description += "Each time it fires, it turns the "
                    "launched projectile into a different, random type "
                    "of bolt.";
            }
            else
            {
                description += "Each time it hits an enemy it has a "
                    "different, random effect.";
            }
            break;
        case SPWPN_VAMPIRICISM:
            description += "It inflicts no extra harm, but heals its "
                "wielder somewhat when it strikes a living foe.";
            break;
        case SPWPN_PAIN:
            description += "In the hands of one skilled in necromantic "
                "magic, it inflicts extra damage on living creatures.";
            break;
        case SPWPN_DISTORTION:
            description += "It warps and distorts space around it. "
                "Unwielding it can cause banishment or high damage.";
            break;
        case SPWPN_REACHING:
            description += "It can be evoked to extend its reach.";
            break;
        case SPWPN_RETURNING:
            description += "A skilled user can throw it in such a way "
                "that it will return to its owner.";
            break;
        case SPWPN_PENETRATION:
            description += "Ammo fired by it will pass through the "
                "targets it hits, potentially hitting all targets in "
                "its path until it reaches maximum range.";
            break;
        case SPWPN_REAPING:
            if (is_range_weapon(item))
            {
                description += "If ammo fired by it kills a monster, "
                    "causing it to leave a corpse, the corpse will be "
                    "animated as a zombie friendly to the killer.";
            }
            else
            {
                description += "If a monster killed with it leaves a "
                    "corpse in good enough shape, the corpse will be "
                    "animated as a zombie friendly to the killer.";
            }
            break;
        case SPWPN_ANTIMAGIC:
            description += "It disrupts the flow of magical energy around "
                    "spellcasters and certain magical creatures (including "
                    "the wielder).";
            break;
        }
    }

    if (is_artefact(item))
    {
        std::string rand_desc = _randart_descrip(item);
        if (!rand_desc.empty())
        {
            description += "\n\n";
            description += rand_desc;
        }

        // XXX: Can't happen, right?
        if (!item_ident(item, ISFLAG_KNOW_PROPERTIES)
            && item_type_known(item))
        {
            description += "\nThis weapon may have some hidden properties.";
        }
    }

    const bool launcher = is_range_weapon(item);
    if (verbose)
    {
        description += "\n\nThis weapon falls into the";

        const skill_type skill =
            is_range_weapon(item)? range_skill(item) : weapon_skill(item);

        description +=
            make_stringf(" '%s' category. ",
                         skill == SK_FIGHTING ? "buggy" : skill_name(skill));

        if (!launcher)
        {
            switch (hands_reqd(item, you.body_size()))
            {
            case HANDS_ONE:
                description += "It is a one handed weapon";
                 break;
            case HANDS_HALF:
                description += "It can be used with one hand, or more "
                       "effectively with two (i.e. when not using a "
                       "shield)";
                break;
            case HANDS_TWO:
                description += "It is a two handed weapon";
                break;
            case HANDS_DOUBLE:
                description += "It is a buggy weapon";
                break;
            }

            const int str_weight = weapon_str_weight(item.base_type,
                                                     item.sub_type);

            if (str_weight >= 8)
                description += ", and it is best used by the strong";
            else if (str_weight > 5)
                description += ", and it is better for the strong";
            else if (str_weight <= 2)
                description += ", and it is best used by the dexterous";
            else if (str_weight < 5)
                description += ", and it is better for the dexterous";
            description += ".";
        }
        if (!you.could_wield(item, true))
            description += "\nIt is too large for you to wield.";
    }

    if (verbose)
    {
        if (is_demonic(item) && !launcher)
            description += "\nDemonspawn deal slightly more damage with it.";
        else if (get_equip_race(item) != ISFLAG_NO_RACE)
        {
            iflags_t race = get_equip_race(item);

            if (race == ISFLAG_DWARVEN)
                description += "\nIt is well-crafted and durable. Dwarves "
                               "deal slightly more damage with it.";

            if (race == ISFLAG_ORCISH)
                description += "\nOrcs deal slightly more damage with it.";

            if (race == ISFLAG_ELVEN)
                description += "\nElves are slightly more accurate with it.";

            if (launcher)
            {
                description += " It is most effective when used with ";
                description += racial_description_string(item);
                description += "ammunition.";
            }

        }
    }

    if (!is_artefact(item))
    {
        if (item_ident(item, ISFLAG_KNOW_PLUSES)
            && item.plus >= MAX_WPN_ENCHANT && item.plus2 >= MAX_WPN_ENCHANT)
        {
            description += "\nIt is maximally enchanted.";
        }
        else
        {
            description += "\nIt can be maximally enchanted to +";
            _append_value(description, MAX_WPN_ENCHANT, false);
            if (item.sub_type != WPN_BLOWGUN)
            {
                description += ", +";
                _append_value(description, MAX_WPN_ENCHANT, false);
            }
            description += ".";
        }
    }

    description += _corrosion_resistance_string(item);

    return (description);
}


//---------------------------------------------------------------
//
// describe_ammo
//
//---------------------------------------------------------------
static std::string _describe_ammo(const item_def &item)
{
    std::string description;

    description.reserve(64);

    if (item.sub_type == MI_THROWING_NET)
    {
        if (item.plus > 1 || item.plus < 0)
        {
            std::string how;

            if (item.plus > 1)
                how = "brand-new";
            else if (item.plus < 0)
            {
                if (item.plus > -3)
                    how = "a little worn";
                else if (item.plus > -5)
                    how = "slightly damaged";
                else if (item.plus > -7)
                    how = "damaged";
                else
                    how = "heavily frayed";
            }

            description += "It looks ";
            description += how;
            description += ".";
        }
    }

    bool can_launch       = has_launcher(item);
    bool can_throw        = is_throwable(&you, item, true);
    bool need_new_line    = true;
    bool always_destroyed = false;

    if (item.special && item_type_known(item))
    {
        description += "\n\n";
        std::string bolt_name;

        std::string threw_or_fired;
        if (can_throw)
        {
            threw_or_fired += "threw";
            if (can_launch)
                threw_or_fired += " or ";
        }
        if (can_launch)
            threw_or_fired += "fired";

        switch (item.special)
        {
        case SPMSL_FLAME:
            description += "It turns into a bolt of flame.";
            break;
        case SPMSL_FROST:
            description += "It turns into a bolt of frost.";
            break;
            break;
        case SPMSL_CHAOS:
            if (bolt_name.empty())
                bolt_name = "a random type";

            description += "When ";

            if (can_throw)
            {
                description += "thrown, ";
                if (can_launch)
                    description += "or ";
            }

            if (can_launch)
                description += "fired from an appropriate launcher, ";

            description += "it turns into a bolt of ";
            description += bolt_name;
            description += ".";
            always_destroyed = true;
            break;
        case SPMSL_POISONED:
            description += "It is coated with poison.";
            break;
        case SPMSL_CURARE:
            description += "It is tipped with asphyxiating poison.";
            break;
        case SPMSL_PARALYSIS:
            description += "It is tipped with a paralysing substance.";
            break;
        case SPMSL_SLOW:
            description += "It is coated with a substance that causes slowness of the body.";
            break;
        case SPMSL_SLEEP:
            description += "It is coated with a fast-acting tranquilizer.";
            break;
        case SPMSL_CONFUSION:
            description += "It is tipped with a substance that causes confusion.";
            break;
        case SPMSL_SICKNESS:
            description += "It has been contaminated by something likely to cause disease.";
            break;
        case SPMSL_RAGE:
            description += "It is tipped with a substance that causes a mindless, berserk rage.";
            break;
       case SPMSL_RETURNING:
            description += "A skilled user can throw it in such a way "
                "that it will return to its owner.";
            break;
        case SPMSL_REAPING:
            description += "If it kills a monster, causing it to leave "
                "a corpse, the corpse will be animated as a zombie "
                "friendly to the one who " + threw_or_fired + " it.";
            break;
        case SPMSL_PENETRATION:
            description += "It will pass through any targets it hits, "
                "potentially hitting all targets in its path until it "
                "reaches maximum range.";
            break;
        case SPMSL_DISPERSAL:
            description += "Any target it hits will blink, with a "
                "tendency towards blinking further away from the one "
                "who " + threw_or_fired + " it.";
            always_destroyed = true;
            break;
        case SPMSL_EXPLODING:
            description += "It will explode into fragments upon "
                "hitting a target, hitting an obstruction, or reaching "
                "the end of its range.";
            always_destroyed = true;
            break;
        case SPMSL_STEEL:
            description += "Compared to normal ammo, it does 50% more "
                "damage, is destroyed upon impact only 1/10th of the "
                "time, and weighs three times as much.";
            break;
        case SPMSL_SILVER:
            description += "The touch of silver hurts beings which are out "
                "of their natural form. Compared to normal ammo, it does twice "
                "as much damage to the corporeal undead, shapechangers and "
                "chaotic beings. It also does extra damage against mutated "
                "beings according to how mutated they are. With due "
                "care, silver ammo can still be handled by those folks. "
                "Silver ammo weighs twice as much as normal ammo.";
            break;
        }

        need_new_line = false;
    }

    if (get_equip_race(item) != ISFLAG_NO_RACE)
    {
        description += "\n";

        if (need_new_line)
            description += "\n";

        if (can_throw)
        {
            iflags_t race = get_equip_race(item);

            description += "It is more deadly when thrown by ";
            description += (race == ISFLAG_DWARVEN) ? "dwarves" :
                           (race == ISFLAG_ELVEN)   ? "elves"
                                                    : "orcs";
            description += (can_launch) ? ", and it" : ".";
            description += " ";
        }

        if (can_launch)
        {
            if (!can_throw)
                description += "It ";

            description += "is more effective in conjunction with ";
            description += racial_description_string(item);
            description += "launchers.";
        }
    }

    if (always_destroyed)
        description += "\nIt will always be destroyed upon impact.";
    else if (item.sub_type != MI_THROWING_NET)
        append_missile_info(description);

    if (item_ident(item, ISFLAG_KNOW_PLUSES) && item.plus >= MAX_WPN_ENCHANT)
        description += "\nIt is maximally enchanted.";
    else
    {
        description += "\nIt can be maximally enchanted to +";
        _append_value(description, MAX_WPN_ENCHANT, false);
        description += ".";
    }

    return (description);
}

void append_armour_stats(std::string &description, const item_def &item)
{
    description += "\nArmour rating: ";
    _append_value(description, property(item, PARM_AC), false);
    description += "       ";

    description += "Evasion modifier: ";
    _append_value(description, property(item, PARM_EVASION), true);
}

void append_missile_info(std::string &description)
{
    description += "\nAll pieces of ammunition may get destroyed upon impact. "
                   "Enchantment reduces the chances of such loss.";
}

//---------------------------------------------------------------
//
// describe_armour
//
//---------------------------------------------------------------
static std::string _describe_armour(const item_def &item, bool verbose)
{
    std::string description;

    description.reserve(200);

    if (verbose
        && item.sub_type != ARM_SHIELD
        && item.sub_type != ARM_BUCKLER
        && item.sub_type != ARM_LARGE_SHIELD)
    {
        append_armour_stats(description, item);
    }

    const int ego = get_armour_ego_type(item);
    if (ego != SPARM_NORMAL && item_type_known(item) && verbose)
    {
        description += "\n\n";

        switch (ego)
        {
        case SPARM_RUNNING:
            description += "It allows its wearer to run at a great speed.";
            break;
        case SPARM_FIRE_RESISTANCE:
            description += "It protects its wearer from heat and fire.";
            break;
        case SPARM_COLD_RESISTANCE:
            description += "It protects its wearer from cold.";
            break;
        case SPARM_POISON_RESISTANCE:
            description += "It protects its wearer from poison.";
            break;
        case SPARM_SEE_INVISIBLE:
            description += "It allows its wearer to see invisible things.";
            break;
        case SPARM_DARKNESS:
            description += "When activated it hides its wearer from "
                "the sight of others, but also increases "
                "their metabolic rate by a large amount.";
            break;
        case SPARM_STRENGTH:
            description += "It increases the physical power of its wearer (+3 to strength).";
            break;
        case SPARM_DEXTERITY:
            description += "It increases the dexterity of its wearer (+3 to dexterity).";
            break;
        case SPARM_INTELLIGENCE:
            description += "It makes you more clever (+3 to intelligence).";
            break;
        case SPARM_PONDEROUSNESS:
            description += "It is very cumbersome, thus slowing your movement.";
            break;
        case SPARM_LEVITATION:
            description += "It can be activated to allow its wearer to "
                "float above the ground and remain so indefinitely.";
            break;
        case SPARM_MAGIC_RESISTANCE:
            description += "It increases its wearer's resistance "
                "to enchantments.";
            break;
        case SPARM_PROTECTION:
            description += "It protects its wearer from harm (+3 to AC).";
            break;
        case SPARM_STEALTH:
            description += "It enhances the stealth of its wearer.";
            break;
        case SPARM_RESISTANCE:
            description += "It protects its wearer from the effects "
                "of both cold and heat.";
            break;

        // These two are only for robes.
        case SPARM_POSITIVE_ENERGY:
            description += "It protects its wearer from "
                "the effects of negative energy.";
            break;
        case SPARM_ARCHMAGI:
            description += "It increases the power of its wearer's "
                "magical spells.";
            break;

        case SPARM_PRESERVATION:
            description += "It protects its wearer's possessions "
                "from damage and destruction.";
            break;

        case SPARM_REFLECTION:
            description += "It reflects blocked things back in the "
                "direction they came from.";
            break;

        case SPARM_SPIRIT_SHIELD:
            description += "It shields its wearer from harm at the cost "
                "of magical power.";
            break;

        // This is only for bracers (gloves).
        case SPARM_ARCHERY:
            description += "These improve your skills with ranged weaponry "
                "but interfere slightly with melee combat.";
            break;
        }
    }

    if (is_artefact(item))
    {
        std::string rand_desc = _randart_descrip(item);
        if (!rand_desc.empty())
        {
            description += "\n\n";
            description += rand_desc;
        }

        // Can't happen, right? (XXX)
        if (!item_ident(item, ISFLAG_KNOW_PROPERTIES) && item_type_known(item))
            description += "\nThis armour may have some hidden properties.";
    }
    else if (get_equip_race(item) != ISFLAG_NO_RACE)
    {
        // Randart armour can't be racial.
        description += "\n";

        iflags_t race = get_equip_race(item);

        if (race == ISFLAG_DWARVEN)
            description += "\nIt is well-crafted and durable.";
        else if (race == ISFLAG_ELVEN)
        {
            description += "\nIt is well-crafted and unobstructive";
            if (item.sub_type == ARM_CLOAK || item.sub_type == ARM_BOOTS)
                description += ", and helps its wearer avoid being noticed";
            description += ".";
        }

        description += "\nIt fits ";
        description += (race == ISFLAG_DWARVEN) ? "dwarves" :
                       (race == ISFLAG_ELVEN)   ? "elves"
                                                : "orcs";
        description += " well.";
    }

    if (!is_artefact(item))
    {
        const int max_ench = armour_max_enchant(item);
        if (armour_is_hide(item))
        {
            description += "\nEnchanting it will turn it into a suit of "
                           "magical armour.";
        }
        else if (item.plus < max_ench || !item_ident(item, ISFLAG_KNOW_PLUSES))
        {
            description += "\nIt can be maximally enchanted to +";
            _append_value(description, max_ench, false);
            description += ".";
        }
        else
            description += "\nIt is maximally enchanted.";
    }

    description += _corrosion_resistance_string(item);

    return description;
}

//---------------------------------------------------------------
//
// describe_jewellery
//
//---------------------------------------------------------------
static std::string _describe_jewellery(const item_def &item, bool verbose)
{
    std::string description;

    description.reserve(200);

    if ((verbose || is_artefact(item))
        && item_ident(item, ISFLAG_KNOW_PLUSES))
    {
        // Explicit description of ring power (useful for randarts)
        // Note that for randarts we'll print out the pluses even
        // in the case that its zero, just to avoid confusion. -- bwr
        if (item.plus != 0
            || item.sub_type == RING_SLAYING && item.plus2 != 0
            || is_artefact(item))
        {
            switch (item.sub_type)
            {
            case RING_PROTECTION:
                description += "\nIt affects your AC (";
                _append_value(description, item.plus, true);
                description += ").";
                break;

            case RING_EVASION:
                description += "\nIt affects your evasion (";
                _append_value(description, item.plus, true);
                description += ").";
                break;

            case RING_STRENGTH:
                description += "\nIt affects your strength (";
                _append_value(description, item.plus, true);
                description += ").";
                break;

            case RING_INTELLIGENCE:
                description += "\nIt affects your intelligence (";
                _append_value(description, item.plus, true);
                description += ").";
                break;

            case RING_DEXTERITY:
                description += "\nIt affects your dexterity (";
                _append_value(description, item.plus, true);
                description += ").";
                break;

            case RING_SLAYING:
                if (item.plus != 0)
                {
                    description += "\nIt affects your accuracy (";
                    _append_value(description, item.plus, true);
                    description += ").";
                }

                if (item.plus2 != 0)
                {
                    description += "\nIt affects your damage-dealing abilities (";
                    _append_value(description, item.plus2, true);
                    description += ").";
                }

                if (item.plus == 0 && item.plus2 == 0)
                {
                    description += "This buggy ring affects neither your "
                                   "accuracy nor your damage-dealing "
                                   "abilities.";
                }
                break;

            default:
                break;
            }
        }
    }

    // Artefact properties.
    if (is_artefact(item))
    {
        std::string rand_desc = _randart_descrip(item);
        if (!rand_desc.empty())
        {
            description += "\n";
            description += rand_desc;
        }
        if (!item_ident(item, ISFLAG_KNOW_PROPERTIES) ||
            !item_ident(item, ISFLAG_KNOW_TYPE))
        {
            description += "\nThis ";
            description += (jewellery_is_amulet(item) ? "amulet" : "ring");
            description += " may have hidden properties.";
        }
    }

    return (description);
}                               // end describe_jewellery()

static bool _compare_card_names(card_type a, card_type b)
{
    return std::string(card_name(a)) < std::string(card_name(b));
}

//---------------------------------------------------------------
//
// describe_misc_item
//
//---------------------------------------------------------------
static bool _check_buggy_deck(const item_def &deck, std::string &desc)
{
    if (!is_deck(deck))
    {
        desc += "This isn't a deck at all!\n";
        return (true);
    }

    const CrawlHashTable &props = deck.props;

    if (!props.exists("cards")
        || props["cards"].get_type() != SV_VEC
        || props["cards"].get_vector().get_type() != SV_BYTE
        || cards_in_deck(deck) == 0)
    {
        return (true);
    }

    return (false);
}

static std::string _describe_deck(const item_def &item)
{
    std::string description;

    description.reserve(100);

    description += "\n";

    if (_check_buggy_deck(item, description))
        return "";

    const std::vector<card_type> drawn_cards = get_drawn_cards(item);
    if (!drawn_cards.empty())
    {
        description += "Drawn card(s): ";
        for (unsigned int i = 0; i < drawn_cards.size(); ++i)
        {
            if (i != 0)
                description += ", ";
            description += card_name(drawn_cards[i]);
        }
        description += "\n";
    }

    const int num_cards = cards_in_deck(item);
    int last_known_card = -1;
    if (top_card_is_known(item))
    {
        description += "Next card(s): ";
        for (int i = 0; i < num_cards; ++i)
        {
            uint8_t flags;
            const card_type card = get_card_and_flags(item, -i-1, flags);
            if (flags & CFLAG_MARKED)
            {
                if (i != 0)
                    description += ", ";
                description += card_name(card);
                last_known_card = i;
            }
            else
                break;
        }
        description += "\n";
    }

    // Marked cards which we don't know straight off.
    std::vector<card_type> marked_cards;
    for (int i = last_known_card + 1; i < num_cards; ++i)
    {
        uint8_t flags;
        const card_type card = get_card_and_flags(item, -i-1, flags);
        if (flags & CFLAG_MARKED)
            marked_cards.push_back(card);
    }
    if (!marked_cards.empty())
    {
        std::sort(marked_cards.begin(), marked_cards.end(),
                  _compare_card_names);

        description += "Marked card(s): ";
        for (unsigned int i = 0; i < marked_cards.size(); ++i)
        {
            if (i != 0)
                description += ", ";
            description += card_name(marked_cards[i]);
        }
        description += "\n";
    }

    // Seen cards in the deck.
    std::vector<card_type> seen_cards;
    for (int i = 0; i < num_cards; ++i)
    {
        uint8_t flags;
        const card_type card = get_card_and_flags(item, -i-1, flags);

        // This *might* leak a bit of information...oh well.
        if ((flags & CFLAG_SEEN) && !(flags & CFLAG_MARKED))
            seen_cards.push_back(card);
    }
    if (!seen_cards.empty())
    {
        std::sort(seen_cards.begin(), seen_cards.end(),
                  _compare_card_names);

        description += "Seen card(s): ";
        for (unsigned int i = 0; i < seen_cards.size(); ++i)
        {
            if (i != 0)
                description += ", ";
            description += card_name(seen_cards[i]);
        }
        description += "\n";
    }

    return (description);
}

// Adds a list of all spells contained in a book or rod to its
// description string.
void append_spells(std::string &desc, const item_def &item)
{
    if (!item.has_spells())
        return;

    desc += "\n\nSpells                             Type                      Level\n";

    for (int j = 0; j < SPELLBOOK_SIZE; ++j)
    {
        spell_type stype = which_spell_in_book(item, j);
        if (stype == SPELL_NO_SPELL)
            continue;

        std::string name = (is_memorised(stype) ? "*" : "");
                    name += spell_title(stype);
        desc += chop_string(name, 35);

        std::string schools;
        if (item.base_type == OBJ_STAVES)
            schools = "Evocations";
        else
            schools = spell_schools_string(stype);

        desc += chop_string(schools, 65 - 36);

        char sval[3];
        itoa(spell_difficulty(stype), sval, 10);
        desc += sval;
        desc += "\n";
    }
}

// ========================================================================
//      Public Functions
// ========================================================================

bool is_dumpable_artefact(const item_def &item, bool verbose)
{
    bool ret = false;

    if (is_known_artefact(item))
    {
        ret = item_ident(item, ISFLAG_KNOW_PROPERTIES);
    }
    else if (verbose && item.base_type == OBJ_ARMOUR
             && item_type_known(item))
    {
        const int spec_ench = get_armour_ego_type(item);
        ret = (spec_ench >= SPARM_RUNNING && spec_ench <= SPARM_PRESERVATION);
    }
    else if (verbose && item.base_type == OBJ_JEWELLERY
             && item_type_known(item))
    {
        ret = true;
    }

    return (ret);
}


//---------------------------------------------------------------
//
// get_item_description
//
// Note that the string will include dollar signs which should
// be interpreted as carriage returns.
//
//---------------------------------------------------------------
std::string get_item_description(const item_def &item, bool verbose,
                                 bool dump, bool noquote)
{
    if (dump)
        noquote = true;

    std::ostringstream description;

    if (!dump)
        description << item.name(DESC_INVENTORY_EQUIP);

#ifdef DEBUG_DIAGNOSTICS
    if (!dump)
    {
        description << std::setfill('0');
        description << "\n\n"
                    << "base: " << static_cast<int>(item.base_type)
                    << " sub: " << static_cast<int>(item.sub_type)
                    << " plus: " << item.plus << " plus2: " << item.plus2
                    << " special: " << item.special
                    << "\n"
                    << "quant: " << item.quantity
                    << " colour: " << static_cast<int>(item.colour)
                    << " flags: " << std::hex << std::setw(8) << item.flags
                    << std::dec << "\n"
                    << "x: " << item.pos.x << " y: " << item.pos.y
                    << " link: " << item.link
                    << " slot: " << item.slot
                    << " ident_type: "
                    << static_cast<int>(get_ident_type(item))
                    << " book_number: " << item.book_number();
    }
#endif

    if (verbose || (item.base_type != OBJ_WEAPONS
                    && item.base_type != OBJ_ARMOUR
                    && item.base_type != OBJ_BOOKS))
    {
        description << "\n\n";

        if (dump)
        {
            description << "["
                        << item.name(DESC_DBNAME, true, false, false)
                        << "]";
        }
        else if (is_unrandom_artefact(item)
                 && (unrandart_descrip(0, item)[0] != '\0'
                     || unrandart_descrip(1, item)[1] != '\0'))
        {
            const char *desc    = unrandart_descrip(0, item);
            const char *desc_id = unrandart_descrip(1, item);

            if (item_type_known(item) && desc_id[0] != '\0')
                description << desc_id << "\n";
            else if (desc[0] != '\0')
                description << desc << "\n";
        }
        else
        {
            std::string db_name = item.name(DESC_DBNAME, true, false, false);
            std::string db_desc = getLongDescription(db_name);
            if (!noquote && !is_known_artefact(item))
            {
                const unsigned int lineWidth = get_number_of_cols();
                const          int height    = get_number_of_lines();

                std::string quote = getQuoteString(db_name);

                if (count_desc_lines(db_desc, lineWidth)
                    + count_desc_lines(quote, lineWidth) <= height)
                {
                    if (!db_desc.empty())
                        db_desc += "\n";
                    db_desc += quote;
                }
            }

            if (db_desc.empty())
            {
                if (item_type_known(item))
                {
                    description << "[ERROR: no desc for item name '" << db_name
                                << "']\n";
                }
                else
                {
                    description << article_a(item.name(DESC_CAP_A, true,
                                                       false, false), false);
                    description << ".\n";
                }
            }
            else
                description << db_desc;

            if (item.base_type == OBJ_WANDS
                || item.base_type == OBJ_MISSILES
                || item.base_type == OBJ_FOOD && item.sub_type == FOOD_CHUNK)
            {
                // Get rid of newline at end of description, so that
                // either the wand "no charges left" or the meat chunk
                // "unpleasant" description can follow on the same line.
                // Same for missiles' descriptions.
                description.seekp(description.tellp() - (std::streamoff)2);
                description << " ";
            }
        }
    }

    bool need_extra_line = true;
    std::string desc;
    switch (item.base_type)
    {
    // Weapons, armour, jewellery, books might be artefacts.
    case OBJ_WEAPONS:
        desc = _describe_weapon(item, verbose);
        if (desc.empty())
            need_extra_line = false;
        else
            description << desc;
        break;

    case OBJ_ARMOUR:
        desc = _describe_armour(item, verbose);
        if (desc.empty())
            need_extra_line = false;
        else
            description << desc;
        break;

    case OBJ_JEWELLERY:
        desc = _describe_jewellery(item, verbose);
        if (desc.empty())
            need_extra_line = false;
        else
            description << desc;
        break;

    case OBJ_BOOKS:
        if (!player_can_memorise_from_spellbook(item))
        {
            description << "\nThis book is beyond your current level of "
                           "understanding.";

            if (!item_type_known(item))
                break;
        }
        else if (is_dangerous_spellbook(item))
        {
            description << "\nWARNING: If you fail in an attempt to memorise a "
                           "spell from this book, the book will lash out at "
                           "you.";
        }

        if (!verbose
            && (Options.dump_book_spells || is_random_artefact(item)))
        {
            append_spells(desc, item);
            if (desc.empty())
                need_extra_line = false;
            else
                description << desc;
        }
        break;

    case OBJ_MISSILES:
        description << _describe_ammo(item);
        break;

    case OBJ_WANDS:
        if (item_type_known(item))
        {
            const int max_charges = wand_max_charges(item.sub_type);
            if (item.plus < max_charges
                || !item_ident(item, ISFLAG_KNOW_PLUSES))
            {
                description << "\nIt can have at most " << max_charges
                            << " charges.";
            }
            else
                description << "\nIt is fully charged.";
        }

        if (item_ident(item, ISFLAG_KNOW_PLUSES) && item.plus == 0
            || item.plus2 == ZAPCOUNT_EMPTY)
        {
            description << "\nUnfortunately, it has no charges left.";
        }
        description << "\n";
        break;

    case OBJ_CORPSES:
        if (item.sub_type == CORPSE_SKELETON)
            break;
        // intentional fall-through
    case OBJ_FOOD:
        if (item.base_type == OBJ_CORPSES || item.sub_type == FOOD_CHUNK)
        {
            if (food_is_rotten(item))
            {
                if (player_mutation_level(MUT_SAPROVOROUS) == 3)
                    description << "It looks nice and ripe.";
                else
                {
                    description << "In fact, it is rotting away before your "
                                   "eyes.";

                    if (!you.is_undead
                        && !player_mutation_level(MUT_SAPROVOROUS))
                    {
                        description << " Eating it is completely out of the "
                                       "question!";
                    }
                }
            }
            else if (player_mutation_level(MUT_SAPROVOROUS) < 3)
                description << "It looks rather unpleasant.";

            switch (mons_corpse_effect(item.plus))
            {
            case CE_POISONOUS:
                description << "\n\nThis meat is poisonous.";
                break;
            case CE_MUTAGEN_RANDOM:
                if (you.species != SP_GHOUL)
                {
                    description << "\n\nEating this meat will cause random "
                                   "mutations.";
                }
                break;
            case CE_HCL:
                if (you.species != SP_GHOUL)
                    description << "\n\nEating this meat will cause rotting.";
                break;
            case CE_CONTAMINATED:
                if (player_mutation_level(MUT_SAPROVOROUS) < 3)
                {
                    description << "\n\nMeat like this may occasionally cause "
                                   "sickness.";
                }
                break;
            case CE_POISON_CONTAM:
                description << "\n\nThis meat is poisonous";
                if (player_mutation_level(MUT_SAPROVOROUS) < 3)
                {
                    description << " and may cause sickness even if poison "
                                   "resistant";
                }
                description << ".";
                break;
            default:
                break;
            }

            if (god_hates_cannibalism(you.religion)
                   && is_player_same_species(item.plus)
                || you.religion == GOD_ZIN
                   && mons_class_intel(item.plus) >= I_NORMAL)
            {
                description << "\n\n" << god_name(you.religion) << " disapproves "
                               "of eating such meat.";
            }
            description << "\n";
        }
        break;

    case OBJ_STAVES:
        if (item_is_rod(item))
        {
            if (verbose)
            {
                description <<
                    "\nIt uses its own mana reservoir for casting spells, and "
                    "recharges automatically according to the recharging "
                    "rate.";

                const int max_charges = MAX_ROD_CHARGE;
                const int max_recharge_rate = MAX_WPN_ENCHANT;
                if (item_ident(item, ISFLAG_KNOW_PLUSES))
                {
                    const int num_charges = item.plus2 / ROD_CHARGE_MULT;
                    if (max_charges > num_charges)
                    {
                        description << "\nIt can currently hold " << num_charges
                                    << " charges. It can be magically "
                                    << "recharged to contain up to "
                                    << max_charges << " charges.";
                    }
                    else
                        description << "\nIt is fully charged.";

                    const int recharge_rate = short(item.props["rod_enchantment"]);
                    if (recharge_rate < max_recharge_rate)
                    {
                        description << "\nIts current recharge rate is "
                                    << (recharge_rate >= 0 ? "+" : "")
                                    << recharge_rate << ". It can be magically "
                                    << "recharged up to +" << max_recharge_rate
                                    << ".";
                    }
                    else
                        description << "\nIts recharge rate is at maximum.";
                }
                else
                {
                    description << "\nIt can have at most " << max_charges
                                << " charges and +" << max_recharge_rate
                                << " recharge rate.";
                }
            }
            else if (Options.dump_book_spells)
            {
                append_spells(desc, item);
                if (desc.empty())
                    need_extra_line = false;
                else
                    description << desc;
            }
            std::string stats = "";
            append_weapon_stats(stats, item);
            description << stats;
            description << "\n\nIt falls into the 'Maces & Flails' category.";
        }
        else
        {
            std::string stats = "";
            append_weapon_stats(stats, item);
            description << stats;
            description << "\n\nIt falls into the 'Staves' category.";
        }
        break;

    case OBJ_MISCELLANY:
        if (is_deck(item))
            description << _describe_deck(item);
        break;

    case OBJ_POTIONS:
#ifdef DEBUG_BLOOD_POTIONS
        // List content of timer vector for blood potions.
        if (!dump && is_blood_potion(item))
        {
            item_def stack = static_cast<item_def>(item);
            CrawlHashTable &props = stack.props;
            ASSERT(props.exists("timer"));
            CrawlVector &timer = props["timer"].get_vector();
            ASSERT(!timer.empty());

            description << "\nQuantity: " << stack.quantity
                        << "        Timer size: " << (int) timer.size();
            description << "\nTimers:\n";
            for (int i = 0; i < timer.size(); ++i)
                 description << (timer[i].get_int()) << "  ";
        }
#endif
        if (item_type_known(item) && you.has_spell(SPELL_EVAPORATE))
        {
            description << "\nEvaporating this potion will create clouds of "
                        << get_evaporate_result_list(item.sub_type)
                        << ".";
        }
        break;

    case OBJ_SCROLLS:
    case OBJ_ORBS:
    case OBJ_GOLD:
        // No extra processing needed for these item types.
        break;

    default:
        die("Bad item class");
    }

    if (is_unrandom_artefact(item)
        && strlen(unrandart_descrip(2, item)) != 0)
    {
        description << "\n\n";
        description << unrandart_descrip(2, item);
    }

    if (!verbose && item_known_cursed(item))
        description << "\nIt has a curse placed upon it.";
    else
    {
        if (verbose)
        {
            if (need_extra_line)
                description << "\n";
            description << "\nIt";
            if (item_known_cursed(item))
                description << " has a curse placed upon it, and it";

            const int mass = item_mass(item);
            description << " weighs around " << (mass / 10)
                        << "." << (mass % 10)
                        << " aum. "; // arbitrary unit of mass

            if (is_artefact(item))
            {
                if (item.base_type == OBJ_ARMOUR
                    || item.base_type == OBJ_WEAPONS)
                {
                    description << "\n\nThis ancient artefact cannot be changed "
                        "by magic or mundane means.";
                }
                else
                    description << "\n\nIt is an ancient artefact.";
            }
        }
    }

    if (conduct_type ct = good_god_hates_item_handling(item))
    {
        description << "\n\n" << god_name(you.religion) << " opposes the use of "
                    << "such an ";

        if (ct == DID_NECROMANCY)
            description << "evil";
        else
            description << "unholy";

        description << " item.";
    }
    else if (god_hates_item_handling(item))
    {
        description << "\n\n" << god_name(you.religion) << " disapproves of the "
                    << "use of such an item.";
    }

    return description.str();
}

static std::string _get_feature_description_wide(int feat)
{
    return std::string();
}

void get_feature_desc(const coord_def &pos, describe_info &inf)
{
    dungeon_feature_type feat = grd(pos);
    bool mimic = false;
    monster* mimic_mons = NULL;

    if (monster_at(pos))
    {
        mimic_mons = monster_at(pos);
        if (mons_is_feat_mimic(mimic_mons->type) && mons_is_unknown_mimic(mimic_mons))
        {
            mimic = true;
            feat = get_mimic_feat(mimic_mons);
        }
    }
    std::string desc      = feature_description(pos, false, DESC_CAP_A, false);
    std::string db_name   = feat == DNGN_ENTER_SHOP ? "A shop" : desc;
    std::string long_desc = getLongDescription(db_name);

    inf.title = desc;
    if (!ends_with(desc, ".") && !ends_with(desc, "!")
        && !ends_with(desc, "?"))
    {
        inf.title += ".";
    }

    // If we couldn't find a description in the database then see if
    // the feature's base name is different.
    if (long_desc.empty())
    {
        db_name   = feature_description(pos, false, DESC_CAP_A, false, true);
        long_desc = getLongDescription(db_name);
    }

    bool custom_desc = false;

    const std::string marker_desc =
        env.markers.property_at(pos, MAT_ANY, "feature_description_long");

    if (!marker_desc.empty())
    {
        long_desc   = marker_desc;
        custom_desc = true;
    }

    if (feat == DNGN_ENTER_PORTAL_VAULT && !custom_desc && !mimic)
    {
        long_desc = "UNDESCRIBED PORTAL VAULT ENTRANCE.";
        custom_desc = true;
    }

    const CrawlHashTable &props = env.properties;
    if (!custom_desc && props.exists(LONG_DESC_KEY))
    {
        const CrawlHashTable &desc_table = props[LONG_DESC_KEY].get_table();

        // First try the modified name, then the base name.
        std::string key = raw_feature_description(feat);
        if (!desc_table.exists(key))
            key = raw_feature_description(feat, NUM_TRAPS, true);

        if (desc_table.exists(key))
        {
            long_desc   = desc_table[key].get_string();
            custom_desc = true;
        }

        std::string quote = getQuoteString(key);
        if (!quote.empty())
            db_name = key;
    }

    inf.body << long_desc;

    // For things which require logic
    if (!custom_desc)
        inf.body << _get_feature_description_wide(grd(pos));

    inf.quote = getQuoteString(db_name);

    // Quotes don't care about custom descriptions.
    if (props.exists(QUOTE_KEY))
    {
        const CrawlHashTable &quote_table = props[QUOTE_KEY].get_table();

        if (quote_table.exists(db_name))
            inf.quote = quote_table[db_name].get_string();
    }
}

static bool _print_toggle_message(const describe_info &inf)
{
    if (inf.quote.empty())
    {
        mouse_control mc(MOUSE_MODE_MORE);
        getchm();
        return (false);
    }
    else
    {
        const int bottom_line = std::min(30, get_number_of_lines());
        cgotoxy(1, bottom_line);
        formatted_string::parse_string(
#ifndef USE_TILE
            "Press '<w>!</w>'"
#else
            "<w>Right-click</w>"
#endif
            " to toggle between the overview and the extended description.").display();

        mouse_control mc(MOUSE_MODE_MORE);
        const int keyin = getchm();

        if (keyin == '!' || keyin == CK_MOUSE_CMD)
            return (true);

        return (false);
    }
}

void describe_feature_wide(const coord_def& pos, bool show_quote)
{
    describe_info inf;
    get_feature_desc(pos, inf);

    if (show_quote)
        print_quote(inf);
    else
        print_description(inf);

    if (crawl_state.game_is_hints())
        hints_describe_pos(pos.x, pos.y);

    if (_print_toggle_message(inf))
        describe_feature_wide(pos, not show_quote);
}

void set_feature_desc_long(const std::string &raw_name,
                           const std::string &desc)
{
    ASSERT(!raw_name.empty());

    CrawlHashTable &props = env.properties;

    if (!props.exists(LONG_DESC_KEY))
        props[LONG_DESC_KEY].new_table();

    CrawlHashTable &desc_table = props[LONG_DESC_KEY].get_table();

    if (desc.empty())
        desc_table.erase(raw_name);
    else
        desc_table[raw_name] = desc;
}

void set_feature_quote(const std::string &raw_name,
                       const std::string &quote)
{
    ASSERT(!raw_name.empty());

    CrawlHashTable &props = env.properties;

    if (!props.exists(QUOTE_KEY))
        props[QUOTE_KEY].new_table();

    CrawlHashTable &quote_table = props[QUOTE_KEY].get_table();

    if (quote.empty())
        quote_table.erase(raw_name);
    else
        quote_table[raw_name] = quote;
}

void get_item_desc(const item_def &item, describe_info &inf, bool terse)
{
    // Don't use verbose descriptions if terse and the item contains spells,
    // so we can actually output these spells if space is scarce.
    const bool verbose = !terse || !item.has_spells();
    inf.body << get_item_description(item, verbose, false,
                                     crawl_state.game_is_hints_tutorial());
}

// Returns true if spells can be shown to player.
static bool _can_show_spells(const item_def &item)
{
    return (item.has_spells()
            && (item.base_type != OBJ_BOOKS || item_type_known(item)
                || player_can_memorise_from_spellbook(item)));
}

static void _show_spells(const item_def &item)
{
    formatted_string fs;
    item_def dup = item;
    spellbook_contents(dup, item.base_type == OBJ_BOOKS ? RBOOK_READ_SPELL
                                                        : RBOOK_USE_STAFF,
                       &fs);
    fs.display(2, -2);
}

static void _show_item_description(const item_def &item)
{
    const unsigned int lineWidth = get_number_of_cols() - 1;
    const          int height    = get_number_of_lines();

    std::string desc =
        get_item_description(item, true, false,
                             crawl_state.game_is_hints_tutorial());

    int num_lines = count_desc_lines(desc, lineWidth) + 1;

    // XXX: hack: Leave room for "Inscribe item?" and the blank line above
    // it by removing item quote.  This should really be taken care of
    // by putting the quotes into a separate DB and treating them as
    // a suffix that can be ignored by print_description().
    if (height - num_lines <= 2)
        desc = get_item_description(item, true, false, true);

    print_description(desc);
    if (crawl_state.game_is_hints())
        hints_describe_item(item);

    if (_can_show_spells(item))
      _show_spells(item);
}

static bool _describe_spells(const item_def &item)
{
    int c = getchm();
    if (c < 'a' || c > 'h')     //jmf: was 'g', but 8=h
    {
        mesclr();
        return (false);
    }

    const int spell_index = letter_to_index(c);

    spell_type nthing =
        which_spell_in_book(item, spell_index);
    if (nthing == SPELL_NO_SPELL)
        return (false);

    describe_spell(nthing, &item);
    return item.is_valid();
}

static bool _can_memorise(item_def &item)
{
    return (item.base_type == OBJ_BOOKS && in_inventory(item)
            && player_can_memorise_from_spellbook(item));
}

static void _update_inscription(item_def &item)
{
    if (item.base_type == OBJ_BOOKS && in_inventory(item)
        && !_can_memorise(item))
    {
        inscribe_book_highlevel(item);
    }
}

static bool _describe_spellbook(item_def &item)
{
    while (true)
    {
        // Memorised spell while reading a spellbook.
        if (already_learning_spell(-1))
            return (false);

        _show_item_description(item);
        _update_inscription(item);

        cgotoxy(1, wherey());
        textcolor(LIGHTGREY);

        if (_can_memorise(item) && !crawl_state.player_is_dead())
        {
            cprintf("Select a spell to read its description, to "
                    "memorise it or to forget it.");
        }
        else
            cprintf("Select a spell to read its description.");

        if (_describe_spells(item))
            continue;

        return true;
    }
}


// it takes a key and a list of commands and it returns
// the command from the list which corresponds to the key
static command_type _get_action(int key, std::vector<command_type> actions)
{
    static bool act_key_init = true; // Does act_key needs to be initialise?
    static std::map<command_type, int> act_key;
    if (act_key_init)
    {
        act_key[CMD_WIELD_WEAPON]       = 'w';
        act_key[CMD_UNWIELD_WEAPON]     = 'u';
        act_key[CMD_QUIVER_ITEM]        = 'q';
        act_key[CMD_WEAR_ARMOUR]        = 'w';
        act_key[CMD_REMOVE_ARMOUR]      = 't';
        act_key[CMD_EVOKE]              = 'v';
        act_key[CMD_EAT]                = 'e';
        act_key[CMD_READ]               = 'r';
        act_key[CMD_WEAR_JEWELLERY]     = 'p';
        act_key[CMD_REMOVE_JEWELLERY]   = 'r';
        act_key[CMD_QUAFF]              = 'q';
        act_key[CMD_DROP]               = 'd';
        act_key[CMD_INSCRIBE_ITEM]      = 'i';
        act_key[CMD_MAKE_NOTE]          = 'a'; //autoinscribe
        act_key_init = false;
    }

    for (std::vector<command_type>::const_iterator at = actions.begin();
         at < actions.end(); ++at)
    {
        if (key == act_key[*at])
            return *at;
    }
    return CMD_NO_CMD;
}

static bool _need_autoinscribe (item_def &item)
{
    // Only allow autoinscription if we don't have all the text already.
    if (is_artefact(item))
    {
        std::string ainscrip = artefact_auto_inscription(item);
        if (!ainscrip.empty()
            && (item.inscription.empty()
                || item.inscription.find(ainscrip) == std::string::npos))
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------
//
// _actions_prompt
//
// print a list of actions to be performed on the item
static bool _actions_prompt(item_def &item, bool allow_inscribe)
{
    std::string prompt = "You can ";
    int keyin;
    std::vector<command_type> actions;
    switch(item.base_type)
    {
    case OBJ_WEAPONS:
    case OBJ_STAVES:
    case OBJ_MISCELLANY:
        if (item.sub_type == MISC_EMPTY_EBONY_CASKET
            || item.sub_type == MISC_RUNE_OF_ZOT)
                break;

        if (item_is_equipped(item))
            actions.push_back(CMD_UNWIELD_WEAPON);
        else
        {
            actions.push_back(CMD_WIELD_WEAPON);
            if (is_throwable(&you, item))
                actions.push_back(CMD_QUIVER_ITEM);
        }
        break;
    case OBJ_MISSILES:
        if (you.species != SP_CAT)
            actions.push_back(CMD_QUIVER_ITEM);
        break;
    case OBJ_ARMOUR:
        if (item_is_equipped(item))
            actions.push_back(CMD_REMOVE_ARMOUR);
        else
            actions.push_back(CMD_WEAR_ARMOUR);
        break;
    case OBJ_FOOD:
        if (can_ingest(item, true, true, false))
            actions.push_back(CMD_EAT);
        break;
    case OBJ_SCROLLS:
    case OBJ_BOOKS: // only unknown ones
        actions.push_back(CMD_READ);
        break;
    case OBJ_JEWELLERY:
        if (item_is_equipped(item))
            actions.push_back(CMD_REMOVE_JEWELLERY);
        else
            actions.push_back(CMD_WEAR_JEWELLERY);
        break;
    case OBJ_POTIONS:
        if (you.is_undead != US_UNDEAD) // mummies and lich form only
            actions.push_back(CMD_QUAFF);
        break;
    default:
        ;
    }
#if defined(CLUA_BINDINGS)
    if (clua.callbooleanfn(false, "ch_item_wieldable", "i", &item))
        actions.push_back(CMD_WIELD_WEAPON);
#endif

    if (item_is_evokable(item))
        actions.push_back(CMD_EVOKE);

    actions.push_back(CMD_DROP);

    if (allow_inscribe)
        actions.push_back(CMD_INSCRIBE_ITEM);

    if (_need_autoinscribe(item))
        actions.push_back(CMD_MAKE_NOTE); //autoinscribe

    static bool act_str_init = true; // Does act_str needs to be initialised?
    static std::map<command_type, std::string> act_str;
    if (act_str_init)
    {
        act_str[CMD_WIELD_WEAPON]       = "(w)ield";
        act_str[CMD_UNWIELD_WEAPON]     = "(u)nwield";
        act_str[CMD_QUIVER_ITEM]        = "(q)uiver";
        act_str[CMD_WEAR_ARMOUR]        = "(w)ear";
        act_str[CMD_REMOVE_ARMOUR]      = "(t)ake off";
        act_str[CMD_EVOKE]              = "e(v)oke";
        act_str[CMD_EAT]                = "(e)at";
        act_str[CMD_READ]               = "(r)ead";
        act_str[CMD_WEAR_JEWELLERY]     = "(p)ut on";
        act_str[CMD_REMOVE_JEWELLERY]   = "(r)emove";
        act_str[CMD_QUAFF]              = "(q)uaff";
        act_str[CMD_DROP]               = "(d)rop";
        act_str[CMD_INSCRIBE_ITEM]      = "(i)nscribe";
        act_str[CMD_MAKE_NOTE]          = "(a)utoinscribe";
        act_str_init = false;
    }

    for (std::vector<command_type>::const_iterator at = actions.begin();
         at < actions.end(); ++at)
    {
        prompt += act_str[*at];
        if (at < actions.end() - 2)
            prompt += ", ";
        else if (at == actions.end() - 2)
            prompt += " or ";
    }
    prompt += " the " + item.name(DESC_BASENAME) + ".";
    prompt = "<cyan>" + prompt + "</cyan>";
    formatted_string::parse_string(prompt).display();

    keyin = tolower(getch_ck());
    command_type action = _get_action(keyin, actions);
    int slot = letter_to_index(item.link);

    switch (action)
    {
    case CMD_WIELD_WEAPON:
        redraw_screen();
        wield_weapon(true, slot);
        return false;
    case CMD_UNWIELD_WEAPON:
        redraw_screen();
        wield_weapon(true, SLOT_BARE_HANDS);
        return false;
    case CMD_QUIVER_ITEM:
        redraw_screen();
        quiver_item(slot);
        return false;
    case CMD_WEAR_ARMOUR:
        redraw_screen();
        wear_armour(slot);
        return false;
    case CMD_REMOVE_ARMOUR:
        redraw_screen();
        takeoff_armour(slot);
        return false;
    case CMD_EVOKE:
        redraw_screen();
        evoke_item(slot);
        return false;
    case CMD_EAT:
        redraw_screen();
        eat_food(slot);
        return false;
    case CMD_READ:
        read_scroll(slot);
        if (item.base_type == OBJ_BOOKS)
            return true; // We stay in the inventory to see the book content.
        else
        {
            redraw_screen();
            return false;
        }
    case CMD_WEAR_JEWELLERY:
        redraw_screen();
        puton_ring(slot);
        return false;
    case CMD_REMOVE_JEWELLERY:
        redraw_screen();
        remove_ring(slot, true);
        return false;
    case CMD_QUAFF:
        redraw_screen();
        drink(slot);
        return false;
    case CMD_DROP:
        redraw_screen();
        drop_item(slot, you.inv[slot].quantity);
        return false;
    case CMD_INSCRIBE_ITEM:
        inscribe_item(item, false);
        break;
    case CMD_MAKE_NOTE:
        add_autoinscription(item);
        break;
    case CMD_NO_CMD:
    default:
        return true;
    }
    return true;
}

//---------------------------------------------------------------
//
// describe_item
//
// Describes all items in the game.
// Returns false if we should break out of the inventory loop.
//---------------------------------------------------------------
bool describe_item(item_def &item, bool allow_inscribe, bool shopping)
{
    if (!item.defined())
        return (true);

    if (_can_show_spells(item))
      return _describe_spellbook(item);

    _show_item_description(item);
    _update_inscription(item);

    if (allow_inscribe && crawl_state.game_is_tutorial())
        allow_inscribe = false;

    // Don't ask if there aren't enough rows left
    if (wherey() <= get_number_of_lines() - 2 && in_inventory(item))
    {
        cgotoxy(1, wherey() + 2);
        return _actions_prompt(item, allow_inscribe);
    }
    else
        wait_for_keypress();

    return (true);
}

static void _safe_newline()
{
    if (wherey() == get_number_of_lines())
    {
        cgotoxy(1, wherey());
        formatted_string::parse_string(std::string(80, ' ')).display();
        cgotoxy(1, wherey());
    }
    else
        formatted_string::parse_string("\n").display();
}

// There are currently two ways to inscribe an item:
// * using the inscribe command ('{') -> msgwin = true
// * from the inventory when viewing an item -> msgwin = false
//
// msgwin also controls whether a hints mode explanation can be
// shown, or whether the pre- and post-inscription item names need to be
// printed.
void inscribe_item(item_def &item, bool msgwin)
{
    if (msgwin)
        mpr(item.name(DESC_INVENTORY).c_str(), MSGCH_EQUIPMENT);

    const bool is_inscribed = !item.inscription.empty();

    bool need_autoinscribe = _need_autoinscribe(item) && msgwin;
    std::string prompt;
    int keyin;

    // Don't prompt for whether to inscribe in the first place unless
    // autoinscribing or clearing an existing inscription become an option.
    if (need_autoinscribe && !is_inscribed)
        prompt = "You can (i)nscribe or (a)utoinscribe.";
    else if (!need_autoinscribe && is_inscribed)
        prompt = "You can (a)dd to, (r)eplace or (c)lear the inscription.";
    else if (need_autoinscribe && is_inscribed)
        prompt = "You can (a)dd to, (r)eplace, (c)lear the inscription "
                 "or (A)utoinscribe.";

    if (prompt != "")
        if (msgwin)
            mpr(prompt.c_str(), MSGCH_PROMPT);
        else
        {
            _safe_newline();
            prompt = "<cyan>" + prompt + "</cyan>";
            formatted_string::parse_string(prompt).display();

            if (crawl_state.game_is_hints()
                && wherey() <= get_number_of_lines() - 5)
            {
                hints_inscription_info(need_autoinscribe, prompt);
            }
        }

    keyin = (prompt != "" ? getch_ck() : 'i');
    if (keyin != 'A')
        keyin = tolower(keyin);
    switch (keyin)
    {
    case 'c':
        item.inscription.clear();
        break;
    case 'A':
        if (need_autoinscribe)
        {
            add_autoinscription(item);
            break;
        }
        // If autoinscription is impossible, prompt for an inscription instead.
    case 'a':
        if (!is_inscribed)
        {
            add_autoinscription(item);
            break;
        }
        // If it is inscribed, prompt for an inscription instead.
    case 'i':
    case 'r':
    {
        if (!is_inscribed)
            prompt = "Inscribe with what? ";
        else if (keyin == 'i' || keyin == 'a' || keyin == 'A')
            prompt = "Add what to inscription? ";
        else
            prompt = "Replace inscription with what? ";

        char buf[79];
        int ret;
        if (msgwin)
            ret = msgwin_get_line(prompt, buf, sizeof buf);
        else
        {
            _safe_newline();
            prompt = "<cyan>" + prompt + "</cyan>";
            formatted_string::parse_string(prompt).display();
            ret = cancelable_get_line(buf, sizeof buf);
        }

        if (!ret)
        {
            // Strip spaces from the end.
            for (int i = strlen(buf) - 1; i >= 0; --i)
            {
                if (isspace(buf[i]))
                    buf[i] = 0;
                else
                    break;
            }

            if (strlen(buf) > 0)
            {
                if (is_inscribed && keyin == 'r')
                    item.inscription = std::string(buf);
                else
                {
                    if (is_inscribed)
                        item.inscription += ", ";

                    item.inscription += std::string(buf);
                }
            }
        }
        else if (msgwin)
        {
            canned_msg(MSG_OK);
            return;
        }
        break;
    }
    default:
        if (msgwin)
            canned_msg(MSG_OK);
        return;
    }

    if (msgwin)
    {
        mpr(item.name(DESC_INVENTORY).c_str(), MSGCH_EQUIPMENT);
        you.wield_change  = true;
    }
}

static void _append_spell_stats(const spell_type spell,
                                std::string &description,
                                bool rod)
{
    if (rod)
    {
        snprintf(info, INFO_SIZE,
                 "\nLevel: %d",
                 spell_difficulty(spell));
    }
    else
    {
        const std::string schools = spell_schools_string(spell);
        snprintf(info, INFO_SIZE,
                 "\nLevel: %d        School%s:  %s    (%s)",
                 spell_difficulty(spell),
                 schools.find("/") != std::string::npos ? "s" : "",
                 schools.c_str(),
                 failure_rate_to_string(spell_fail(spell)));
    }
    description += info;
    description += "\n\nPower : ";
    description += spell_power_string(spell, rod);
    description += "\nRange : ";
    description += spell_range_string(spell, rod);
    description += "\nHunger: ";
    description += spell_hunger_string(spell, rod);
    description += "\nNoise : ";
    description += spell_noise_string(spell);
}

// Returns BOOK_MEM if you can memorise the spell BOOK_FORGET if you can
// forget it and BOOK_NEITHER if you can do neither
static int _get_spell_description(const spell_type spell,
                                   std::string &description,
                                   const item_def* item = NULL)
{
    description.reserve(500);

    description  = spell_title(spell);
    description += "\n\n";
    const std::string long_descrip = getLongDescription(spell_title(spell));

    if (!long_descrip.empty())
        description += long_descrip;
    else
    {
        description += "This spell has no description. "
                       "Casting it may therefore be unwise. "
#ifdef DEBUG
                       "Instead, go fix it. ";
#else
                       "Please file a bug report.";
#endif
    }

    if (god_hates_spell(spell, you.religion))
    {
        description += god_name(you.religion)
                       + " frowns upon the use of this spell.\n";
    }
    else if (god_likes_spell(spell, you.religion))
    {
        description += god_name(you.religion)
                       + " supports the use of this spell.\n";
    }
    if (item && !player_can_memorise_from_spellbook(*item))
    {
        description += "The spell is scrawled in ancient runes that are beyond "
                       "your current level of understanding.\n";
    }
    if (spell_is_useless(spell))
        description += "This spell will have no effect right now.\n";

    if (crawl_state.player_is_dead())
        return (BOOK_NEITHER);

    bool rod = item && item->base_type == OBJ_STAVES;
    _append_spell_stats(spell, description, rod);

    bool undead = false;
    if (you_cannot_memorise(spell, undead))
    {
        description += "\n\n";
        description += desc_cannot_memorise_reason(undead);
    }
    if (item && item->base_type == OBJ_BOOKS && in_inventory(*item))
        if (you.has_spell(spell))
        {
            description += "\n\n";
            description += "(F)orget this spell by destroying the book.";
            if (you.religion == GOD_SIF_MUNA)
                description +="\nSif Muna frowns upon the destroying of books.";
            return (BOOK_FORGET);
        }
        else if (player_can_memorise_from_spellbook(*item)
                 && !you_cannot_memorise(spell, undead))
        {
            description += "\n\n";
            description += "(M)emorise this spell.";
            return (BOOK_MEM);
        }

    return BOOK_NEITHER;
}

void get_spell_desc(const spell_type spell, describe_info &inf)
{
    std::string desc;
    _get_spell_description(spell, desc);
    inf.body << desc;
}

//---------------------------------------------------------------
//
// describe_spell
//
// Describes (most) every spell in the game.
//
//---------------------------------------------------------------
void describe_spell(spell_type spelled, const item_def* item)
{
    std::string desc;
    int mem_or_forget = _get_spell_description(spelled, desc, item);
    print_description(desc);

    mouse_control mc(MOUSE_MODE_MORE);

    char ch;
    if ((ch = getchm()) == 0)
        ch = getchm();

    if (mem_or_forget == BOOK_MEM && toupper(ch) == 'M')
    {
        redraw_screen();
        if (!learn_spell(spelled, item->sub_type, false) || !you.turn_is_over)
            more();
        redraw_screen();
    }
    else if (mem_or_forget == BOOK_FORGET && toupper(ch) == 'F')
    {
        redraw_screen();
        if (!forget_spell_from_book(spelled, item) || !you.turn_is_over)
            more();
        redraw_screen();
    }
}

static std::string _describe_draconian_role(monster_type type)
{
    switch (type)
    {
    case MONS_DRACONIAN_SHIFTER:
        return "It darts around disconcertingly without taking a step.";
    case MONS_DRACONIAN_SCORCHER:
        return "Its scales are sooty from years of magical pyrotechnics.";
    case MONS_DRACONIAN_ZEALOT:
        return "In its gaze you see all the malefic power of its "
               "terrible god.";
    case MONS_DRACONIAN_ANNIHILATOR:
        return "Crackling balls of pure energy hum and spark up and down its "
               "scaled fists and arms.";
    case MONS_DRACONIAN_CALLER:
        return "It looks especially reptilian, and eager for company.";
    case MONS_DRACONIAN_MONK:
        return "It looks unnaturally strong and dangerous with its fists.";
    case MONS_DRACONIAN_KNIGHT:
        return "It wields a deadly weapon with menacing efficiency.";
    default:
        return ("");
    }
}

static std::string _describe_draconian_colour(int species)
{
    switch (species)
    {
    case MONS_BLACK_DRACONIAN:
        return "Sparks crackle and flare out of its mouth and nostrils.";
    case MONS_MOTTLED_DRACONIAN:
        return "Liquid flames drip from its mouth.";
    case MONS_YELLOW_DRACONIAN:
        return "Acid fumes swirl around it.";
    case MONS_GREEN_DRACONIAN:
        return "Venom steams and drips from its jaws.";
    case MONS_PURPLE_DRACONIAN:
        return "Its outline shimmers with wild energies.";
    case MONS_RED_DRACONIAN:
        return "Smoke pours from its nostrils.";
    case MONS_WHITE_DRACONIAN:
        return "Frost pours from its nostrils.";
    case MONS_GREY_DRACONIAN:
        return "Its scales and tail are adapted to the water.";
    case MONS_PALE_DRACONIAN:
        return "It is cloaked in a pall of superheated steam.";
    }
    return ("");
}

static std::string _describe_draconian(const monster_info& mi)
{
    std::string description;
    const int subsp = mi.draco_subspecies();

    if (subsp == MONS_DRACONIAN)
        description += "A ";
    else
        description += "A muscular ";

    switch (subsp)
    {
    case MONS_DRACONIAN:            description += "brown-";   break;
    case MONS_BLACK_DRACONIAN:      description += "black-";   break;
    case MONS_MOTTLED_DRACONIAN:    description += "mottled-"; break;
    case MONS_YELLOW_DRACONIAN:     description += "yellow-";  break;
    case MONS_GREEN_DRACONIAN:      description += "green-";   break;
    case MONS_PURPLE_DRACONIAN:     description += "purple-";  break;
    case MONS_RED_DRACONIAN:        description += "red-";     break;
    case MONS_WHITE_DRACONIAN:      description += "white-";   break;
    case MONS_GREY_DRACONIAN:       description += "grey-";    break;
    case MONS_PALE_DRACONIAN:       description += "pale-";    break;
    default:
        break;
    }

    description += "scaled humanoid with wings.";

    if (subsp != MONS_DRACONIAN)
    {
        const std::string drac_col = _describe_draconian_colour(subsp);
        if (!drac_col.empty())
            description += " " + drac_col;
    }

    if (subsp != mi.type)
    {
        const std::string drac_role = _describe_draconian_role(mi.type);
        if (!drac_role.empty())
            description += " " + drac_role;
    }

    return (description);
}

static const char* _get_resist_name(mon_resist_flags res_type)
{
    switch (res_type)
    {
    case MR_RES_ELEC:
        return "electricity";
    case MR_RES_POISON:
        return "poison";
    case MR_RES_FIRE:
        return "fire";
    case MR_RES_STEAM:
        return "steam";
    case MR_RES_COLD:
        return "cold";
    case MR_RES_ACID:
        return "acid";
    case MR_RES_ROTTING:
        return "rotting";
    default:
        return "buggy resistance";
    }
}

// Describe a monster's (intrinsic) resistances, speed and a few other
// attributes.
static std::string _monster_stat_description(const monster_info& mi)
{
    std::ostringstream result;

    // Don't leak or duplicate resistance information for ghost demon
    // monsters, except for (very) ugly things.
    const mon_resist_def resist = mi.resists();

    const mon_resist_flags resists[] = {
        MR_RES_ELEC,   MR_RES_POISON, MR_RES_FIRE,
        MR_RES_STEAM,  MR_RES_COLD,   MR_RES_ACID,
        MR_RES_ROTTING
    };

    std::vector<std::string> extreme_resists;
    std::vector<std::string> high_resists;
    std::vector<std::string> base_resists;
    std::vector<std::string> suscept;

    for (unsigned int i = 0; i < ARRAYSZ(resists); ++i)
    {
        int level = resist.get_resist_level(resists[i]);
        if (resists[i] == MR_RES_FIRE && resist.hellfire)
            level = 3;

        if (level != 0)
        {
            const char* attackname = _get_resist_name(resists[i]);
            level = std::max(level, -1);
            level = std::min(level,  3);
            switch (level)
            {
                case -1:
                    suscept.push_back(attackname);
                    break;
                case 1:
                    base_resists.push_back(attackname);
                    break;
                case 2:
                    high_resists.push_back(attackname);
                    break;
                case 3:
                    extreme_resists.push_back(attackname);
                    break;
            }
        }
    }

    std::vector<std::string> resist_descriptions;
    if (!extreme_resists.empty())
    {
        const std::string tmp = "extremely resistant to "
            + comma_separated_line(extreme_resists.begin(),
                                   extreme_resists.end());
        resist_descriptions.push_back(tmp);
    }
    if (!high_resists.empty())
    {
        const std::string tmp = "very resistant to "
            + comma_separated_line(high_resists.begin(), high_resists.end());
        resist_descriptions.push_back(tmp);
    }
    if (!base_resists.empty())
    {
        const std::string tmp = "resistant to "
            + comma_separated_line(base_resists.begin(), base_resists.end());
        resist_descriptions.push_back(tmp);
    }

    const char* pronoun = mi.pronoun(PRONOUN_CAP);

    if (!resist_descriptions.empty())
    {
        result << pronoun << " is "
               << comma_separated_line(resist_descriptions.begin(),
                                       resist_descriptions.end(),
                                       "; and ", "; ")
               << ".\n";
    }

    // Is monster susceptible to anything? (On a new line.)
    if (!suscept.empty())
    {
        result << pronoun << " is susceptible to "
               << comma_separated_line(suscept.begin(), suscept.end())
               << ".\n";
    }

    int mr = mi.res_magic();
    if (mr == MAG_IMMUNE)
        result << pronoun << " is immune to hostile enchantments.\n";
    else // How resistant is it? Same scale as the player.
    {
        if (mr >= 10)
        {
            result << pronoun << make_stringf(" is %s resistant to hostile enchantments.\n",
                    magic_res_adjective(mr).c_str());
        }
    }

    if (mons_class_flag(mi.type, M_STATIONARY)
        && !mons_is_tentacle(mi.type))
    {
        result << pronoun << " cannot move.\n";
    }

    // Monsters can glow from both light and radiation.
    if (mons_class_flag(mi.type, M_GLOWS_LIGHT))
        result << pronoun << " is outlined in light.\n";
    if (mons_class_flag(mi.type, M_GLOWS_RADIATION))
        result << pronoun << " is glowing with mutagenic radiation.\n";

    // These differ between ghost demon monsters, so would be spoily.
    if (!mons_is_ghost_demon(mi.type))
    {
        // Seeing/sensing invisible.
        if (mons_class_flag(mi.type, M_SEE_INVIS))
            result << pronoun << " can see invisible.\n";
        else if (mons_class_flag(mi.type, M_SENSE_INVIS))
            result << pronoun << " can sense the presence of invisible creatures.\n";

        // Unusual monster speed.
        const int speed = mi.base_speed();
        if (speed != 10 && speed != 0)
        {
            result << pronoun << " is ";
            if (speed < 7)
                result << "very slow";
            else if (speed < 10)
                result << "slow";
            else if (speed > 20)
                result << "extremely fast";
            else if (speed > 15)
                result << "very fast";
            else if (speed > 10)
                result << "fast";
            result << ".\n";
        }
    }

    // Can the monster levitate/fly?
    // This doesn't give anything away since no (very) ugly things can
    // fly, all ghosts can fly, and for demons it's already mentioned in
    // their flavour description.
    if (mi.fly != FL_NONE)
    {
        result << pronoun << " can "
               << (mi.fly == FL_FLY ? "fly" : "levitate") << ".\n";
    }

    // Unusual regeneration rates.
    if (!mi.can_regenerate())
        result << pronoun << " cannot regenerate.\n";
    else if (monster_descriptor(mi.type, MDSC_REGENERATES))
        result << pronoun << " regenerates quickly.\n";

    // Size
    const char *sizes[NUM_SIZE_LEVELS] = {
        "tiny",
        "little",
        "small",
        NULL,     // don't display anything for 'medium'
        "large",
        "big",
        "giant",
        "huge",
    };

    const char *mimic_sizes[6]= {
        "as big as a fountain",
        "as big as a shop",
        "as big as a staircase",
        "as big as a trap",
        "as big as a portal",
        "as big as a door",
    };

    if (mons_is_feat_mimic(mi.type))
        result << pronoun << " is " << mimic_sizes[MONS_FOUNTAIN_MIMIC-mi.type] << ".\n";
    else if (sizes[mi.body_size()])
        result << pronoun << " is " << sizes[mi.body_size()] << ".\n";

    return (result.str());
}

// Fetches the monster's database description and reads it into inf.
void get_monster_db_desc(const monster_info& mi, describe_info &inf,
                         bool &has_stat_desc, bool force_seen)
{
    if (inf.title.empty())
        inf.title = mi.full_name(DESC_CAP_A, true);

    std::string db_name;

    if (mi.mname.empty())
        db_name = mi.db_name();
    else
        db_name = mi.full_name(DESC_PLAIN, true);

    // This is somewhat hackish, but it's a good way of over-riding monsters'
    // descriptions in Lua vaults by using MonPropsMarker. This is also the
    // method used by set_feature_desc_long, etc. {due}
    if (!mi.description.empty())
        inf.body << mi.description;
    // Don't get description for player ghosts.
    else if (mi.type != MONS_PLAYER_GHOST
             && mi.type != MONS_PLAYER_ILLUSION)
    {
        inf.body << getLongDescription(db_name);
    }

    // And quotes {due}
    if (!mi.quote.empty())
        inf.quote = mi.quote;
    else
        inf.quote = getQuoteString(db_name);

    std::string symbol;
    symbol += get_monster_data(mi.type)->showchar;
    if (isaupper(symbol[0]))
        symbol = "cap-" + symbol;

    std::string quote2;
    if (!mons_is_unique(mi.type))
    {
        std::string symbol_prefix = "__";
        symbol_prefix += symbol;
        symbol_prefix += "_prefix";
        inf.prefix = getLongDescription(symbol_prefix);
        quote2 = getQuoteString(symbol_prefix);
    }

    if (!inf.quote.empty() && !quote2.empty())
        inf.quote += "\n";
    inf.quote += quote2;

    // Except for draconians and player ghosts, I have to admit I find the
    // following special descriptions rather pointless. I certainly can't
    // say I like them, though "It has come for your soul!" and
    // "It wants to drink your blood!" have something going for them. (jpeg)
    switch (mi.type)
    {
    case MONS_VAMPIRE:
    case MONS_VAMPIRE_KNIGHT:
    case MONS_VAMPIRE_MAGE:
        if (you.is_undead == US_ALIVE && mi.attitude == ATT_HOSTILE)
            inf.body << "\nIt wants to drink your blood!\n";
        break;

    case MONS_REAPER:
        if (you.is_undead == US_ALIVE && mi.attitude == ATT_HOSTILE)
            inf.body <<  "\nIt has come for your soul!\n";
        break;

    case MONS_DRACONIAN:
    case MONS_RED_DRACONIAN:
    case MONS_WHITE_DRACONIAN:
    case MONS_GREEN_DRACONIAN:
    case MONS_PALE_DRACONIAN:
    case MONS_MOTTLED_DRACONIAN:
    case MONS_BLACK_DRACONIAN:
    case MONS_YELLOW_DRACONIAN:
    case MONS_PURPLE_DRACONIAN:
    case MONS_GREY_DRACONIAN:
    case MONS_DRACONIAN_SHIFTER:
    case MONS_DRACONIAN_SCORCHER:
    case MONS_DRACONIAN_ZEALOT:
    case MONS_DRACONIAN_ANNIHILATOR:
    case MONS_DRACONIAN_CALLER:
    case MONS_DRACONIAN_MONK:
    case MONS_DRACONIAN_KNIGHT:
    {
        inf.body << "\n" << _describe_draconian(mi) << "\n";
        break;
    }

    case MONS_PLAYER_GHOST:
        inf.body << "The apparition of " << get_ghost_description(mi) << ".\n";
        break;

    case MONS_PLAYER_ILLUSION:
        inf.body << "An illusion of " << get_ghost_description(mi) << ".\n";
        break;

    case MONS_PANDEMONIUM_DEMON:
        inf.body << _describe_demon(mi.mname, mi.fly) << "\n";
        break;

    case MONS_SERPENT_OF_HELL:
        // XXX: ick
        switch (mi.colour)
        {
        case RED:
            inf.body << "A huge red glowing dragon, burning with hellfire.\n";
            break;

        case WHITE:
            inf.body << "A huge gleaming white dragon, covered in shards of ice.\n";
            break;

        case CYAN:
            inf.body << "A huge metallic dragon, glowing with power.\n";
            break;

        case MAGENTA:
            inf.body << "A huge and dark dragon, wreathed in terrifying shadows.\n";
            break;

        default:
            inf.body << "Well now, isn't this buggy?\n";
        }
        break;

    case MONS_PROGRAM_BUG:
        inf.body << "If this monster is a \"program bug\", then it's "
                "recommended that you save your game and reload.  Please report "
                "monsters who masquerade as program bugs or run around the "
                "dungeon without a proper description to the authorities.\n";
        break;

    default:
        break;
    }

    if (!mons_is_unique(mi.type))
    {
        std::string symbol_suffix = "__";
        symbol_suffix += symbol;
        symbol_suffix += "_suffix";

        std::string suffix = getLongDescription(symbol_suffix);
                    suffix += getLongDescription(symbol_suffix + "_examine");

        if (!suffix.empty())
            inf.body << "\n" << suffix;
    }

    // Get information on resistances, speed, etc.
    std::string result = _monster_stat_description(mi);
    if (!result.empty())
    {
        inf.body << "\n" << result;
        has_stat_desc = true;
    }

    if (!mons_class_can_use_stairs(mi.type))
    {
        inf.body << "\n" << mi.pronoun(PRONOUN_CAP)
                 << " is incapable of using stairs.\n";
    }

    if (mi.is(MB_SUMMONED))
    {
        inf.body << "\n" << "This monster has been summoned, and is thus only "
                       "temporary. Killing it yields no experience, nutrition "
                       "or items.\n";
    }

    if (mi.is(MB_PERM_SUMMON))
    {
        inf.body << "\n" << "This monster has been summoned in a durable "
                       "way, and only partially exists. Killing it yields no "
                       "experience, nutrition or items. You cannot easily "
                       "abjure it, though.\n";
    }

    if (!inf.quote.empty())
        inf.quote += "\n";

#ifdef DEBUG_DIAGNOSTICS
    if (mi.pos.origin())
        return; // not a real monster
    monster& mons = *mi.mon();

    inf.body << "\nMonster health: "
             << mons.hit_points << "/" << mons.max_hit_points << "\n";

    const actor *mfoe = mons.get_foe();
    inf.body << "Monster foe: "
             << (mfoe? mfoe->name(DESC_PLAIN, true)
                 : "(none)");

    std::vector<std::string> attitude;
    if (mons.friendly())
        attitude.push_back("friendly");
    if (mons.neutral())
        attitude.push_back("neutral");
    if (mons.good_neutral())
        attitude.push_back("good_neutral");
    if (mons.strict_neutral())
        attitude.push_back("strict_neutral");
    if (mons.pacified())
        attitude.push_back("pacified");
    if (mons.wont_attack())
        attitude.push_back("wont_attack");
    if (!attitude.empty())
        inf.body << "; " << comma_separated_line(attitude.begin(),
                                                 attitude.end(),
                                                 "; ", "; ");
    inf.body << "\n";

    if (mons.can_use_spells())
    {
        const monster_spells &hspell_pass = mons.spells;
        bool found_spell = false;

        for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; ++i)
        {
            if (hspell_pass[i] != SPELL_NO_SPELL)
            {
                if (!found_spell)
                {
                    inf.body << "\n\nMonster Spells:\n";
                    found_spell = true;
                }

                inf.body << "    " << i << ": "
                         << spell_title(hspell_pass[i])
                         << " (" << static_cast<int>(hspell_pass[i])
                         << ")\n";
            }
        }
    }

    bool has_item = false;
    for (int i = 0; i < NUM_MONSTER_SLOTS; ++i)
    {
        if (mons.inv[i] != NON_ITEM)
        {
            if (!has_item)
            {
                inf.body << "\nMonster Inventory:\n";
                has_item = true;
            }
            inf.body << "    " << i << ") "
                     << mitm[mons.inv[i]].name(DESC_NOCAP_A, false, true);
        }
    }

    if (mons.props.exists("blame"))
    {
        inf.body << "\n\nMonster blame chain:\n";

        const CrawlVector& blame = mons.props["blame"].get_vector();

        for (CrawlVector::const_iterator it = blame.begin();
             it != blame.end(); ++it)
        {
            inf.body << "    " << it->get_string() << "\n";
        }
    }
#endif
}

void describe_monsters(const monster_info &mi, bool force_seen,
                       const std::string &footer,
                       bool wait_until_key_pressed,
                       bool show_quote)
{
    describe_info inf;
    bool has_stat_desc = false;
    get_monster_db_desc(mi, inf, has_stat_desc, force_seen);

    if (!footer.empty())
    {
        if (inf.footer.empty())
            inf.footer = footer;
        else
            inf.footer += "\n" + footer;
    }

    if (show_quote)
    {
        print_quote(inf);
    }
    else
    {
        print_description(inf);

        // TODO enne - this should really move into get_monster_db_desc
        // and an additional tutorial string added to describe_info.
        if (crawl_state.game_is_hints())
            hints_describe_monster(mi, has_stat_desc);
    }


    mouse_control mc(MOUSE_MODE_MORE);

    if (wait_until_key_pressed && _print_toggle_message(inf))
        describe_monsters(mi, force_seen, footer, wait_until_key_pressed, !show_quote);
}

static const char* xl_rank_names[] = {
    "weakling",
    "average",
    "experienced",
    "powerful",
    "mighty",
    "great",
    "awesomely powerful",
    "legendary"
};

static std::string _xl_rank_name(const int xl_rank)
{
    const char* rank = xl_rank_names[xl_rank];

    std::string name = make_stringf("a%s %s",
                                    is_vowel(rank[0]) ? "n" : "",
                                    rank);
    return name;
}

std::string short_ghost_description(const monster *mon, bool abbrev)
{
    ASSERT(mons_is_pghost(mon->type));

    const ghost_demon &ghost = *(mon->ghost);
    const char* rank = xl_rank_names[ghost_level_to_rank(ghost.xl)];

    std::string desc = make_stringf("%s %s %s",
                        rank,
                        species_name(ghost.species).c_str(),
                        get_job_name(ghost.job));

    if (abbrev || strwidth(desc) > 40)
    {
        desc = make_stringf("%s %s%s",
                            rank,
                            get_species_abbrev(ghost.species),
                            get_job_abbrev(ghost.job));
    }

    return desc;
}

// Describes the current ghost's previous owner. The caller must
// prepend "The apparition of" or whatever and append any trailing
// punctuation that's wanted.
std::string get_ghost_description(const monster_info &mi, bool concise)
{
    std::ostringstream gstr;

    const species_type gspecies = mi.u.ghost.species;

    // We're fudging stats so that unarmed combat gets based off
    // of the ghost's species, not the player's stats... exact
    // stats aren't required anyway, all that matters is whether
    // dex >= str. -- bwr
    const int dex = 10;
    int str = 5;

    switch (gspecies)
    {
    case SP_MOUNTAIN_DWARF:
    case SP_DEEP_DWARF:
    case SP_TROLL:
    case SP_OGRE:
    case SP_MINOTAUR:
    case SP_HILL_ORC:
    case SP_CENTAUR:
    case SP_NAGA:
    case SP_MUMMY:
    case SP_GHOUL:
        str += 10;
        break;

    case SP_HUMAN:
    case SP_DEMIGOD:
    case SP_DEMONSPAWN:
        str += 5;
        break;

    default:
        break;
    }

    gstr << mi.mname << " the "
         << skill_title_by_rank(mi.u.ghost.best_skill,
                        mi.u.ghost.best_skill_rank,
                        gspecies,
                        str, dex, mi.u.ghost.religion)
         << ", " << _xl_rank_name(mi.u.ghost.xl_rank) << " ";

    if (concise)
    {
        gstr << get_species_abbrev(gspecies)
             << get_job_abbrev(mi.u.ghost.job);
    }
    else
    {
        gstr << species_name(gspecies)
             << " "
             << get_job_name(mi.u.ghost.job);
    }

    if (mi.u.ghost.religion != GOD_NO_GOD)
    {
        gstr << " of "
             << god_name(mi.u.ghost.religion);
    }

    return (gstr.str());
}

extern ability_type god_abilities[MAX_NUM_GODS][MAX_GOD_ABILITIES];

static bool _print_final_god_abil_desc(int god, const std::string &final_msg,
                                       const ability_type abil)
{
    // If no message then no power.
    if (final_msg.empty())
        return (false);

    std::string buf = final_msg;

    // For ability slots that give more than one ability, display
    // "Various" instead of the cost of the first ability.
    const std::string cost =
        "(" +
              ((abil == ABIL_ELYVILON_LESSER_HEALING_OTHERS
                || abil == ABIL_ELYVILON_GREATER_HEALING_OTHERS
                || abil == ABIL_YRED_RECALL_UNDEAD_SLAVES) ?
                    "Various" : make_cost_description(abil))
            + ")";

    if (cost != "(None)")
    {
        int spacesleft = 79 - strwidth(buf) - strwidth(cost);
        while (spacesleft--)
            buf += ' ';
        buf += cost;
    }

    cprintf("%s\n", buf.c_str());

    return (true);
}

static bool _print_god_abil_desc(int god, int numpower)
{
    const char* pmsg = god_gain_power_messages[god][numpower];

    // If no message then no power.
    if (!pmsg[0])
        return (false);

    // Don't display ability upgrades here.
    std::string buf = adjust_abil_message(pmsg, false);
    if (buf.empty())
        return (false);

    if (!isupper(pmsg[0])) // Complete sentence given?
        buf = "You can " + buf + ".";

    // This might be ABIL_NON_ABILITY for passive abilities.
    const ability_type abil = god_abilities[god][numpower];
    _print_final_god_abil_desc(god, buf, abil);

    return (true);
}

static const std::string _describe_favour_generic(god_type which_god)
{
    const std::string godname = god_name(which_god);
    return (you.piety > 130) ? "A prized avatar of " + godname + ".":
           (you.piety > 100) ? "A shining star in the eyes of " + godname + "." :
           (you.piety >  70) ? "A rising star in the eyes of " + godname + "." :
           (you.piety >  40) ? godname + " is most pleased with you." :
           (you.piety >  20) ? godname + " has noted your presence." :
           (you.piety >   5) ? godname + " is noncommittal."
                             : "You are beneath notice.";
}

//---------------------------------------------------------------
//
// describe_god
//
// Describes the player's standing with his deity.
//
//---------------------------------------------------------------

std::string describe_favour(god_type which_god)
{
    if (player_under_penance())
    {
        const int penance = you.penance[which_god];
        return (penance >= 50) ? "Godly wrath is upon you!" :
               (penance >= 20) ? "You've transgressed heavily! Be penitent!" :
               (penance >= 5) ? "You are under penance."
                               : "You should show more discipline.";
    }

    return (which_god == GOD_XOM) ? describe_xom_favour(true)
                                  : _describe_favour_generic(which_god);
}

static std::string _religion_help(god_type god)
{
    std::string result = "";

    switch (god)
    {
    case GOD_ZIN:
        result += "You can pray at an altar to donate your money.";
        if (!player_under_penance() && you.piety > 160
            && !you.num_total_gifts[god])
        {
            if (!result.empty())
                result += " ";

            result += "You can have all your mutations cured.";
        }
        break;

    case GOD_SHINING_ONE:
    {
        int halo_size = you.halo_radius2();
        if (halo_size >= 0)
        {
            if (!result.empty())
                result += " ";

            result += "You radiate a ";

            if (halo_size > 37)
                result += "large ";
            else if (halo_size > 10)
                result += "";
            else
                result += "small ";

            result += "righteous aura, and all beings within it are "
                      "easier to hit.";
        }
        if (!player_under_penance() && you.piety > 160
            && !you.num_total_gifts[god])
        {
            if (!result.empty())
                result += " ";

            result += "You can pray at an altar to have your weapon "
                      "blessed, especially a long blade or demon "
                      "weapon.";
        }
        break;
    }

    case GOD_ELYVILON:
        result += "You can pray to destroy weapons on the ground in "
                + apostrophise(god_name(god)) + " name. Inscribe them "
                + "with !p, !* or =p to avoid sacrificing them accidentally.";
        break;

    case GOD_LUGONU:
        if (!player_under_penance() && you.piety > 160
            && !you.num_total_gifts[god])
        {
            result += "You can pray at an altar to have your weapon "
                      "corrupted.";
        }
        break;

    case GOD_KIKUBAAQUDGHA:
        if (!player_under_penance() && you.piety > 160
            && !you.num_total_gifts[god])
        {
            result += "You can pray at an altar to have your necromancy "
                      "enhanced.";
        }
        break;

    case GOD_BEOGH:
        result += "You can pray to sacrifice all orcish remains on your "
                  "square. Inscribe orcish remains with !p, !* or =p to avoid "
                  "sacrificing them accidentally.";
        break;

    case GOD_NEMELEX_XOBEH:
        result += "You can pray to sacrifice all items on your square. "
                  "Inscribe items with !p, !* or =p to avoid sacrificing "
                  "them accidentally.";
        break;

    case GOD_VEHUMET:
        if (you.piety >= piety_breakpoint(1))
        {
            result += god_name(god) + " assists you in casting "
                      "Conjurations and Summonings.";
        }
        break;

    case GOD_FEDHAS:
        if (you.piety >= piety_breakpoint(0))
        {
            result += "Evolving plants requires fruit, "
                      "evolving fungi requires piety.";
        }

    default:
        break;
    }

    if (god_likes_fresh_corpses(god))
    {
        if (!result.empty())
            result += " ";

        result += "You can pray to sacrifice all fresh corpses on your "
                  "square. Inscribe fresh corpses with !p, !* or =p to avoid "
                  "sacrificing them accidentally.";
    }

    return result;
}

// The various titles granted by the god of your choice.  Note that Xom
// doesn't use piety the same way as the other gods, so these are just
// placeholders.
const char *divine_title[NUM_GODS][8] =
{
    // No god.
    {"Buglet",             "Firebug",               "Bogeybug",                 "Bugger",
     "Bugbear",            "Bugged One",            "Giant Bug",                "Lord of the Bugs"},

    // Zin.
    {"Sinner",             "Anchorite",             "Apologist",                "Pious",
     "Devout",             "Orthodox",              "Immaculate",               "Bringer of Law"},

    // The Shining One.
    {"Sinner",             "Acolyte",               "Righteous",                "Unflinching",
     "Holy Warrior",       "Exorcist",              "Demon Slayer",             "Bringer of Light"},

    // Kikubaaqudgha -- scholarly death.
    {"Sinner",             "Purveyor of Pain",      "Death's Scholar",          "Merchant of Misery",
     "Death's Artisan",    "Dealer of Despair",     "Black Sun",                "Lord of Darkness"},

    // Yredelemnul -- zombie death.
    {"Sinner",             "Zealot",                "Exhumer",                  "Fey %s",
     "Soul Tainter",       "Sculptor of Flesh",     "Harbinger of Death",       "Grim Reaper"},

    // Xom.
    {"Toy",                "Toy",                   "Toy",                      "Toy",
     "Toy",                "Toy",                   "Toy",                      "Toy"},

    // Vehumet -- battle mage theme.
    {"Meek",               "Sorcerer's Apprentice", "Scholar of Destruction",   "Caster of Ruination",
     "Battle Magician",    "Warlock",               "Annihilator",              "Luminary of Lethal Lore"},

    // Okawaru -- battle theme.
    {"Coward",             "Struggler",             "Combatant",                "Warrior",
     "Knight",             "Warmonger",             "Commander",                "Victor of a Thousand Battles"},

    // Makhleb -- chaos theme.
    {"Orderly",            "Spawn of Chaos",        "Disciple of Annihilation", "Fanfare of Bloodshed",
     "Fiendish",           "Demolition %s",         "Pandemonic",               "Champion of Chaos"},

    // Sif Muna -- scholarly theme.
    {"Ignorant",           "Disciple",              "Student",                  "Adept",
     "Scribe",             "Scholar",               "Sage",                     "Genius of the Arcane"},

    // Trog -- anger theme.
    {"Faithless",          "Troglodyte",            "Angry Troglodyte",         "Frenzied",
     "%s of Prey",         "Rampant",               "Wild %s",                  "Bane of Scribes"},

    // Nemelex Xobeh -- alluding to Tarot and cards.
    {"Unlucky %s",         "Pannier",               "Jester",                   "Fortune-Teller",
     "Soothsayer",         "Magus",                 "Cardsharp",                "Hand of Fortune"},

    // Elyvilon.
    {"Sinner",             "Comforter",             "Caregiver",                "Practitioner",
     "Pacifier",           "Purifying %s",          "Faith Healer",             "Bringer of Life"},

    // Lugonu -- distortion theme.
    {"Faithless",          "Abyss-Baptised",        "Unweaver",                 "Distorting %s",
     "Agent of Entropy",   "Schismatic",            "Envoy of Void",            "Corrupter of Planes"},

    // Beogh -- messiah theme.
    {"Apostate",           "Messenger",             "Proselytiser",             "Priest",
     "Missionary",         "Evangelist",            "Apostle",                  "Messiah"},

    // Jiyva -- slime and jelly theme.
    {"Scum",               "Jelly",                 "Squelcher",                "Dissolver",
     "Putrid Slime",       "Consuming %s",          "Archjelly",                "Royal Jelly"},

    // Fedhas Madash -- nature theme.  Titles could use some work
    {"Walking Fertiliser", "Green %s",              "Inciter",                  "Photosynthesist",
     "Cultivator",         "Green Death",           "Nimbus",                   "Force of Nature"},

    // Cheibriados -- slow theme
    {"Unwound %s",         "Timekeeper",            "Righteous Timekeeper",     "Chronographer",
     "Splendid Chronographer", "Chronicler",        "Eternal Chronicler",       "Ticktocktomancer"},

    // Ashenzari -- divination theme
    {"Star-crossed",       "Cursed",                "Initiated",                "Seer",
     "Soothsayer",         "Oracle",                "Illuminatus",              "Omniscient"},
};

static int _piety_level()
{
    return ((you.piety >  160) ? 7 :
            (you.piety >= 120) ? 6 :
            (you.piety >= 100) ? 5 :
            (you.piety >=  75) ? 4 :
            (you.piety >=  50) ? 3 :
            (you.piety >=  30) ? 2 :
            (you.piety >    5) ? 1
                               : 0);
}

std::string god_title(god_type which_god, species_type which_species)
{
    std::string title;
    if (you.penance[which_god])
        title = divine_title[which_god][0];
    else
        title = divine_title[which_god][_piety_level()];

    title = replace_all(title, "%s",
                        species_name(which_species, true, false));

    return (title);
}

static void _detailed_god_description(god_type which_god)
{
    clrscr();

    const int width = std::min(80, get_number_of_cols());

    std::string godname = god_name(which_god, true);
    int len = get_number_of_cols() - strwidth(godname);
    textcolor(god_colour(which_god));
    cprintf("%s%s\n", std::string(len / 2, ' ').c_str(), godname.c_str());
    textcolor(LIGHTGREY);
    cprintf("\n");

    std::string broken;
    if (which_god != GOD_NEMELEX_XOBEH)
    {
        broken = get_god_powers(which_god);
        if (!broken.empty())
        {
            linebreak_string2(broken, width);
            display_tagged_block(broken);
            cprintf("\n");
            cprintf("\n");
        }
    }

    if (which_god != GOD_XOM)
    {
        broken = get_god_likes(which_god, true);
        linebreak_string2(broken, width);
        display_tagged_block(broken);

        broken = get_god_dislikes(which_god, true);
        if (!broken.empty())
        {
            cprintf("\n");
            cprintf("\n");
            linebreak_string2(broken, width);
            display_tagged_block(broken);
        }
        // Some special handling.
        broken = "";
        switch (which_god)
        {
        case GOD_TROG:
            broken = "Note that Trog does not demand training of the "
                     "Invocations skill. All abilities are purely based on "
                     "piety.";
            break;

        case GOD_ZIN:
            broken = "Zin will feed starving followers upon <w>p</w>rayer.";
            break;

        case GOD_ELYVILON:
            broken = "Using your healing abilities on hostile monsters may "
                     "pacify them, turning them neutral. Pacification works "
                     "best on natural beasts, worse on humanoids of your "
                     "species, worse on other humanoids, worst of all on "
                     "demons and undead, and not at all on sleeping or "
                     "mindless monsters. If it succeeds, the monster is "
                     "healed and you gain half of its experience value and "
                     "possibly some piety. Pacified monsters try to leave "
                     "the level.";
            break;

        case GOD_NEMELEX_XOBEH:
            if (which_god == you.religion)
            {
                broken = "The piety increase when sacrificing mostly depends "
                         "on the value of the item. To prevent items from "
                         "being accidentally sacrificed, you can "
                         "<w>i</w>nscribe them with <w>!p</w> (protects the "
                         "whole stack), with <w>=p</w> (protects only the "
                         "item), or with <w>!D</w> (causes item to be ignored "
                         "in sacrifices)."
                         "\n\n"
                         "Nemelex Xobeh gifts various types of decks of cards. "
                         "Each deck type comes in three power levels: plain, "
                         "ornate, legendary. The latter contain very powerful "
                         "card effects, potentially hazardous. High piety and "
                         "Evocations skill help here, as the power of Nemelex's "
                         "abilities is governed by Evocations instead of "
                         "Invocations. The type of the deck gifts strongly "
                         "depends on the dominating item class sacrificed:\n";

                for (int i = 0; i < NUM_NEMELEX_GIFT_TYPES; ++i)
                {
                    const bool active = you.nemelex_sacrificing[i];
                    std::string desc = "";
                    switch (i)
                    {
                    case NEM_GIFT_ESCAPE:
                        desc = "decks of Escape      -- armour";
                        break;
                    case NEM_GIFT_DESTRUCTION:
                        desc = "decks of Destruction -- weapons and ammunition";
                        break;
                    case NEM_GIFT_DUNGEONS:
                        desc = "decks of Dungeons    -- jewellery, books, "
                                                    "miscellaneous items";
                        break;
                    case NEM_GIFT_SUMMONING:
                        desc = "decks of Summoning   -- corpses, chunks, blood";
                        break;
                    case NEM_GIFT_WONDERS:
                        desc = "decks of Wonders     -- consumables: food, potions, "
                                                    "scrolls, wands";
                        break;
                    }
                    broken += make_stringf(" %c %s%s%s\n",
                                           'a' + (char) i,
                                           active ? "+ " : "- <darkgrey>",
                                           desc.c_str(),
                                           active ? "" : "</darkgrey>");
                }
            }
        default:
            break;
        }

        if (!broken.empty())
        {
            cprintf("\n");
            cprintf("\n");
            linebreak_string2(broken, width);
            display_tagged_block(broken);
        }
    }

    const int bottom_line = std::min(30, get_number_of_lines());

    cgotoxy(1, bottom_line);
    formatted_string::parse_string(
#ifndef USE_TILE
        "Press '<w>!</w>'"
#else
        "<w>Right-click</w>"
#endif
        " to toggle between the overview and the more detailed "
        "description.").display();

    mouse_control mc(MOUSE_MODE_MORE);

    const int keyin = getchm();
    if (you.religion == GOD_NEMELEX_XOBEH
        && keyin >= 'a' && keyin < 'a' + (char) NUM_NEMELEX_GIFT_TYPES)
    {
        const int num = keyin - 'a';
        you.nemelex_sacrificing[num] = !you.nemelex_sacrificing[num];
        _detailed_god_description(which_god);
    }
    else if (keyin == '!' || keyin == CK_MOUSE_CMD)
        describe_god(which_god, true);
}

#define NUM_BONDAGE 3
static const char* bondage_parts[2][NUM_BONDAGE] =
{
  { "weapon", "armour", "magic", },         // normal races
  { "left ring", "right ring", "amulet", }, // felids
};

static std::string _describe_bondage()
{
    int bondage_types[NUM_BONDAGE];
    int sum = 0;
    for (int i = 0; i < NUM_BONDAGE; i++)
    {
        sum += bondage_types[i] = ash_bondage_level(i+1);
        ASSERT(bondage_types[i] == 0 || bondage_types[i] == 1);
    }

    if (!sum)
        return "You are not bound.";
    if (sum == NUM_BONDAGE)
        return "You are completely bound.";

    const char** names = bondage_parts[you.species == SP_CAT];
    std::vector<const char*> parts[2];
    for (int i = 0; i < NUM_BONDAGE; i++)
        parts[bondage_types[i]].push_back(names[i]);
    if (sum == 1)
        return make_stringf("You are bound in %s but not %s or %s.",
                            parts[1][0], parts[0][0], parts[0][1]);
    else
        return make_stringf("You are bound in %s and %s but not %s.",
                            parts[1][0], parts[1][1], parts[0][0]);
}

void describe_god(god_type which_god, bool give_title)
{
    int colour;              // Colour used for some messages.

    clrscr();

    if (give_title)
    {
        textcolor(WHITE);
        cprintf("                                  Religion\n");
        textcolor(LIGHTGREY);
    }

    if (which_god == GOD_NO_GOD) //mv: No god -> say it and go away.
    {
        cprintf("\nYou are not religious.");
        get_ch();
        return;
    }

    colour = god_colour(which_god);

    // Print long god's name.
    textcolor(colour);
    cprintf("%s", god_name(which_god, true).c_str());
    cprintf("\n\n");

    // Print god's description.
    textcolor(LIGHTGREY);

    std::string god_desc = getLongDescription(god_name(which_god));
    const int numcols = get_number_of_cols();
    cprintf("%s", get_linebreak_string(god_desc.c_str(), numcols).c_str());

    // Title only shown for our own god.
    if (you.religion == which_god)
    {
        // Print title based on piety.
        cprintf("\nTitle - ");
        textcolor(colour);

        std::string title = god_title(which_god, you.species);
        cprintf("%s", title.c_str());
    }

    // mv: Now let's print favour as Brent suggested.
    // I know these messages aren't perfect so if you can think up
    // something better, do it.

    textcolor(LIGHTGREY);
    cprintf("\n\nFavour - ");
    textcolor(colour);

    //mv: Player is praying at altar without appropriate religion.
    // It means player isn't checking his own religion and so we only
    // display favour and go out.
    if (you.religion != which_god)
    {
        textcolor(colour);
        int which_god_penance = you.penance[which_god];

        // Give more appropriate message for the good gods.
        if (which_god_penance > 0 && is_good_god(which_god))
        {
            if (is_good_god(you.religion))
                which_god_penance = 0;
            else if (!god_hates_your_god(which_god) && which_god_penance >= 5)
                which_god_penance = 2; // == "Come back to the one true church!"
        }

        cprintf((which_god == GOD_NEMELEX_XOBEH
                     && which_god_penance > 0 && which_god_penance <= 100)
                                             ? "%s doesn't play fair with you." :
                 (which_god_penance >= 50)   ? "%s's wrath is upon you!" :
                 (which_god_penance >= 20)   ? "%s is annoyed with you." :
                 (which_god_penance >=  5)   ? "%s well remembers your sins." :
                 (which_god_penance >   0)   ? "%s is ready to forgive your sins." :
                 (you.worshipped[which_god]) ? "%s is ambivalent towards you."
                                             : "%s is neutral towards you.",
                 god_name(which_god).c_str());
    }
    else
    {
        cprintf(describe_favour(which_god).c_str());
        if (which_god == GOD_ASHENZARI)
            cprintf("\n%s", _describe_bondage().c_str());

        //mv: The following code shows abilities given by your god (if any).

        textcolor(LIGHTGREY);
        cprintf("\n\nGranted powers:                                                          (Cost)\n");
        textcolor(colour);

        // mv: Some gods can protect you from harm.
        // The god isn't really protecting the player - only sometimes saving
        // his life.
        bool have_any = false;

        if (god_can_protect_from_harm(which_god))
        {
            have_any = true;

            int prot_chance = 10 + you.piety/10; // chance * 100
            const char *when = "";

            switch (elyvilon_lifesaving())
            {
            case 1:
                when = ", especially when called upon";
                prot_chance += 100 - 3000/you.piety;
                break;
            case 2:
                when = ", and always does so when called upon";
                prot_chance = 100;
            }

            const char *how = (prot_chance >= 85) ? "carefully" :
                              (prot_chance >= 55) ? "often" :
                              (prot_chance >= 25) ? "sometimes"
                                                  : "occasionally";

            std::string buf = god_name(which_god);
            buf += " ";
            buf += how;
            buf += " watches over you";
            buf += when;
            buf += ".";

            _print_final_god_abil_desc(which_god, buf, ABIL_NON_ABILITY);
        }

        if (which_god == GOD_ZIN)
        {
            if (zin_sustenance(false))
            {
                have_any = true;
                std::string buf = "Praying to "
                                  + god_name(which_god)
                                  + " will provide sustenance if starving.";
                _print_final_god_abil_desc(which_god, buf,
                                           ABIL_ZIN_SUSTENANCE);
            }
            const char *how = (you.piety >= 150) ? "carefully" :
                              (you.piety >= 100) ? "often" :
                              (you.piety >=  50) ? "sometimes" :
                                                   "occasionally";

            cprintf("%s %s shields you from unclean and chaotic effects.\n",
                    god_name(which_god).c_str(), how);
        }
        else if (which_god == GOD_SHINING_ONE)
        {
            have_any = true;
            const char *how = (you.piety >= 150) ? "carefully" :
                              (you.piety >= 100) ? "often" :
                              (you.piety >=  50) ? "sometimes" :
                                                   "occasionally";

            cprintf("%s %s shields you from negative energy.\n",
                    god_name(which_god).c_str(), how);
        }
        else if (which_god == GOD_TROG)
        {
            have_any = true;
            std::string buf = "You can call upon "
                              + god_name(which_god)
                              + " to burn spellbooks in your surroundings.";
            _print_final_god_abil_desc(which_god, buf,
                                       ABIL_TROG_BURN_SPELLBOOKS);
        }
        else if (which_god == GOD_JIYVA)
        {
            if (jiyva_can_paralyse_jellies())
            {
                have_any = true;
                _print_final_god_abil_desc(which_god,
                                           "Your slimes' item consumption is "
                                           "temporarily halted under prayer.",
                                           ABIL_JIYVA_JELLY_PARALYSE);
            }
            if (you.piety >= piety_breakpoint(2))
            {
                have_any = true;
                cprintf("%s shields you from corrosive effects.\n",
                        god_name(which_god).c_str());
            }
            if (you.piety >= piety_breakpoint(1))
            {
                have_any = true;
                std::string buf = "You gain nutrition";
                if (you.piety >= piety_breakpoint(4))
                    buf += ", power and health";
                else if (you.piety >= piety_breakpoint(3))
                    buf += " and power";
                buf += " when your fellow slimes consume items.\n";
                _print_final_god_abil_desc(which_god, buf,
                                           ABIL_NON_ABILITY);
            }
        }
        else if (which_god == GOD_FEDHAS)
        {
            have_any = true;
            _print_final_god_abil_desc(which_god,
                                       "You can pray to speed up decomposition.",
                                       ABIL_NON_ABILITY);
            _print_final_god_abil_desc(which_god,
                                       "You can walk through plants and "
                                       "fire through allied plants.",
                                       ABIL_NON_ABILITY);
        }
        else if (which_god == GOD_ASHENZARI)
        {
            have_any = true;
            _print_final_god_abil_desc(which_god,
                "You are provided with a bounty of information.",
                ABIL_NON_ABILITY);
            std::string buf = "You can pray to bestow "
                              + apostrophise(god_name(which_god))
                              + " curse upon scrolls that usually remove "
                                "them.";
            _print_final_god_abil_desc(which_god, buf,
                                       ABIL_NON_ABILITY);
        }

        // mv: No abilities (except divine protection) under penance
        if (!player_under_penance())
        {
            for (int i = 0; i < MAX_GOD_ABILITIES; ++i)
                if (you.piety >= piety_breakpoint(i)
                    && _print_god_abil_desc(which_god, i))
                {
                    have_any = true;
                }
        }
        if (!have_any)
            cprintf("None.\n");
    }

    int bottom_line = get_number_of_lines();
    if (bottom_line > 30)
        bottom_line = 30;

    // Only give this additional information for worshippers.
    if (which_god == you.religion)
    {
        std::string extra = get_linebreak_string(_religion_help(which_god),
                                                 numcols).c_str();
        cgotoxy(1, bottom_line - std::count(extra.begin(), extra.end(), '\n')-1,
                GOTO_CRT);
        textcolor(LIGHTGREY);
        cprintf("%s", extra.c_str());
    }

    cgotoxy(1, bottom_line);
    textcolor(LIGHTGREY);
    formatted_string::parse_string(
#ifndef USE_TILE
        "Press '<w>!</w>'"
#else
        "<w>Right-click</w>"
#endif
        " to toggle between the overview and the more detailed "
        "description.").display();

    mouse_control mc(MOUSE_MODE_MORE);
    const int keyin = getchm();
    if (keyin == '!' || keyin == CK_MOUSE_CMD)
        _detailed_god_description(which_god);
}

std::string get_skill_description(skill_type skill, bool need_title)
{
    std::string lookup = skill_name(skill);
    std::string result = "";

    if (need_title)
    {
        result = lookup;
        result += "\n\n";
    }

    result += getLongDescription(lookup);

    switch (skill)
    {
    case SK_UNARMED_COMBAT:
    {
        // Give a detailed listing of what attacks the character may perform.
        std::vector<std::string> unarmed_attacks;

        if (you.has_usable_tail())
            unarmed_attacks.push_back("slap with your tail");

        if (you.has_usable_fangs())
            unarmed_attacks.push_back("bite with your sharp teeth");
        else if (player_mutation_level(MUT_BEAK))
            unarmed_attacks.push_back("peck with your beak");

        if (player_mutation_level(MUT_HORNS))
            unarmed_attacks.push_back("headbutt with your horns");
        else if (you.species == SP_NAGA)
            unarmed_attacks.push_back("do a headbutt attack");

        if (player_mutation_level(MUT_HOOVES))
            unarmed_attacks.push_back("kick with your hooves");
        else if (player_mutation_level(MUT_TALONS))
            unarmed_attacks.push_back("claw with your talons");
        else if (you.species != SP_NAGA && you.species != SP_CAT
                 && !you.fishtail)
        {
            unarmed_attacks.push_back("deliver a kick");
        }

        if (you.has_usable_pseudopods())
            unarmed_attacks.push_back("slap with your pseudopods");

        if (you.species == SP_CAT)
            unarmed_attacks.push_back("use your claws");
        else if (!you.weapon())
            unarmed_attacks.push_back("throw a punch");
        else if (you.has_usable_offhand())
            unarmed_attacks.push_back("punch with your free hand");

        if (!unarmed_attacks.empty())
        {
            std::string broken = "For example, you could ";
                        broken += comma_separated_line(unarmed_attacks.begin(),
                                                       unarmed_attacks.end(),
                                                       " or ", ", ");
                        broken += ".";
            linebreak_string2(broken, 72);

            result += "\n";
            result += broken;
        }
        break;
    }

    case SK_INVOCATIONS:
        if (you.species == SP_DEMIGOD)
        {
            result += "\n";
            result += "How on earth did you manage to pick this up?";
        }
        else if (you.religion == GOD_TROG)
        {
            result += "\n";
            result += "Note that Trog doesn't use Invocations, its being too "
                      "closely connected to magic.";
        }
        else if (you.religion == GOD_NEMELEX_XOBEH)
        {
            result += "\n";
            result += "Note that Nemelex uses Evocations rather than "
                      "Invocations.";
        }
        break;

    case SK_EVOCATIONS:
        if (you.religion == GOD_NEMELEX_XOBEH)
        {
            result += "\n";
            result += "This is the skill all of Nemelex's abilities rely on.";
        }
        break;

    case SK_SPELLCASTING:
        if (you.religion == GOD_TROG)
        {
            result += "\n";
            result += "Keep in mind, though, that Trog will greatly disapprove "
                      "of this.";
        }
        break;
    default:
        // No further information.
        break;
    }

    return result;
}

void describe_skill(skill_type skill)
{
    std::ostringstream data;

    data << get_skill_description(skill, true);

    print_description(data.str());
    wait_for_keypress();
}

#ifdef USE_TILE
std::string get_command_description(const command_type cmd, bool terse)
{
    std::string lookup = command_to_name(cmd);

    if (!terse)
        lookup += " verbose";

    std::string result = getLongDescription(lookup);
    if (result.empty())
    {
        if (!terse)
        {
            // Try for the terse description.
            result = get_command_description(cmd, true);
            if (!result.empty())
                return result + ".";
        }
        return command_to_name(cmd);
    }

    return result.substr(0, result.length() - 1);
}
#endif

void alt_desc_proc::nextline()
{
    ostr << "\n";
}

void alt_desc_proc::print(const std::string &str)
{
    ostr << str;
}

int alt_desc_proc::count_newlines(const std::string &str)
{
    int count = 0;
    for (size_t i = 0; i < str.size(); i++)
    {
        if (str[i] == '\n')
            count++;
    }
    return count;
}

void alt_desc_proc::trim(std::string &str)
{
    int idx = str.size();
    while (--idx >= 0)
    {
        if (str[idx] != '\n')
            break;
    }
    str.resize(idx + 1);
}

bool alt_desc_proc::chop(std::string &str)
{
    int loc = -1;
    for (size_t i = 1; i < str.size(); i++)
        if (str[i] == '\n' && str[i-1] == '\n')
            loc = i;

    if (loc == -1)
        return (false);

    str.resize(loc);
    return (true);
}

void alt_desc_proc::get_string(std::string &str)
{
    str = replace_all(ostr.str(), "\n\n\n\n", "\n\n");
    str = replace_all(str, "\n\n\n", "\n\n");

    trim(str);
    while (count_newlines(str) > h)
    {
        if (!chop(str))
            break;
    }
}
