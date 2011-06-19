/**
 * @file
 * @brief God-granted abilities.
**/

#include "AppHdr.h"

#include <queue>

#include "areas.h"
#include "artefact.h"
#include "attitude-change.h"
#include "beam.h"
#include "cloud.h"
#include "colour.h"
#include "coord.h"
#include "coordit.h"
#include "database.h"
#include "delay.h"
#include "dgn-actions.h"
#include "directn.h"
#include "effects.h"
#include "env.h"
#include "files.h"
#include "fprop.h"
#include "godabil.h"
#include "goditem.h"
#include "godpassive.h"
#include "invent.h"
#include "itemprop.h"
#include "items.h"
#include "libutil.h"
#include "message.h"
#include "misc.h"
#include "mon-act.h"
#include "mon-behv.h"
#include "mon-iter.h"
#include "mon-place.h"
#include "mgen_data.h"
#include "mon-stuff.h"
#include "mon-util.h"
#include "mutation.h"
#include "notes.h"
#include "options.h"
#include "player-stats.h"
#include "random.h"
#include "religion.h"
#include "skills.h"
#include "skills2.h"
#include "shopping.h"
#include "shout.h"
#include "spl-book.h"
#include "spl-monench.h"
#include "spl-summoning.h"
#include "spl-transloc.h"
#include "spl-util.h"
#include "stash.h"
#include "state.h"
#include "stuff.h"
#include "terrain.h"
#include "view.h"
#include "viewgeom.h"

#ifdef USE_TILE
#include "tiledef-main.h"
#endif

bool zin_sustenance(bool actual)
{
    return (you.piety >= piety_breakpoint(0)
            && (!actual || you.hunger_state == HS_STARVING));
}

std::string zin_recite_text(int* trits, size_t len, int prayertype, int step)
{

    //'prayertype':
    //This is in enum.h; there are currently five prayers.

    //'step':
    //-1: We're either starting or stopping, so we just want the passage name.
    //2/1/0: That many rounds are left. So, if step = 2, we want to show the passage #1/3.

    //That's too confusing, so:

    if (step > -1)
        step = abs(step-3);

    //We change it to turn 1, turn 2, turn 3.

    //'trits' && 'len':
    //To have deterministic passages we need to store a random seed. Ours consists of an array of trinary bits.

    //Yes, really.

    int chapter = 1 + trits[0] + trits[1] * 3 + trits[2] * 9;
    int verse = 1 + trits[3] + trits[4] * 3 + trits[5] * 9 + trits[6] * 27;

    std::string sinner_text[12] =
    {
        "hordes of the Abyss",
        "bastard children of Xom",
        "amorphous wretches",
        "fetid masses",
        "agents of filth",
        "squalid dregs",
        "unbelievers",
        "heretics",
        "guilty",
        "legions of the damned",
        "servants of Hell",
        "forces of darkness",
    };

    std::string sin_text[12] =
    {
        "chaotic",
        "discordant",
        "anarchic",
        "unclean",
        "impure",
        "contaminated",
        "unfaithful",
        "disloyal",
        "doubting",
        "profane",
        "blasphemous",
        "sacrilegious",
    };

    std::string long_sin_text[12] =
    {
        "chaos",
        "discord",
        "anarchy",
        "uncleanliness",
        "impurity",
        "contamination",
        "unfaithfulness",
        "disloyalty",
        "doubt",
        "profanity",
        "blasphemy",
        "sacrilege",
    };

    std::string virtue_text[12] =
    {
        "ordered",
        "harmonic",
        "lawful",
        "clean",
        "pure",
        "hygienic",
        "faithful",
        "loyal",
        "believing",
        "reverent",
        "pious",
        "obedient",
    };

    std::string long_virtue_text[12] =
    {
        "order",
        "harmony",
        "lawfulness",
        "cleanliness",
        "purity",
        "hygiene",
        "faithfulness",
        "loyalty",
        "belief",
        "reverence",
        "piety",
        "obedience",
    };

    std::string smite_text[9] =
    {
        "purify",
        "censure",
        "condemn",
        "strike down",
        "expel",
        "oust",
        "smite",
        "castigate",
        "rebuke",
    };

    std::string smitten_text[9] =
    {
        "purified",
        "censured",
        "condemned",
        "struck down",
        "expelled",
        "ousted",
        "smitten",
        "castigated",
        "rebuked",
    };

    std::string sinner = sinner_text[trits[3] + prayertype * 3];
    std::string sin[2] = {sin_text[trits[6] + prayertype * 3],
                          long_sin_text[trits[6] + prayertype * 3]};
    std::string virtue[2] = {virtue_text[trits[6] + prayertype * 3],
                             long_virtue_text[trits[6] + prayertype * 3]};
    std::string smite[2] = {smite_text[(trits[4] + trits[5] * 3)],
                            smitten_text[(trits[4] + trits[5] * 3)]};

    std::string turn[4] = {"This is only here because arrays start from 0.",
                           "Zin is a buggy god.",
                           "Please report this.",
                           "This isn't right at all."};

    switch (chapter)
    {
        case 1:
            turn[1] = make_stringf("It was the word of Zin that there would not be %s...", sin[1].c_str());
            turn[2] = make_stringf("...and did the people not suffer until they had %s...", smite[1].c_str());
            turn[3] = make_stringf("...the %s, after which all was well?", sinner.c_str());
            break;
        case 2:
            turn[1] = make_stringf("The voice of Zin, pure and clear, did say that the %s...", sinner.c_str());
            turn[2] = make_stringf("...were not %s! And hearing this, the people rose up...", virtue[0].c_str());
            turn[3] = make_stringf("...and embraced %s, for they feared Zin's wrath.", virtue[1].c_str());
            break;
        case 3:
            turn[1] = make_stringf("Zin spoke of the doctrine of %s, and...", virtue[1].c_str());
            turn[2] = make_stringf("...saw the %s filled with fear, for they were...", sinner.c_str());
            turn[3] = make_stringf("...%s and knew Zin's wrath would come for them.", sin[0].c_str());
            break;
        case 4:
            turn[1] = make_stringf("And so Zin bade the %s to come before...", sinner.c_str());
            turn[2] = make_stringf("...the altar, that judgement might be passed...");
            turn[3] = make_stringf("...upon those who were not %s.", virtue[0].c_str());
            break;
        case 5:
            turn[1] = make_stringf("To the devout, Zin provideth. From the rest...");
            turn[2] = make_stringf("...ye %s, ye guilty...", sinner.c_str());
            turn[3] = make_stringf("...of %s, Zin taketh.", sin[1].c_str());
            break;
        case 6:
            turn[1] = make_stringf("Zin saw the %s of the %s, and...", sin[1].c_str(), sinner.c_str());
            turn[2] = make_stringf("...was displeased, for did the law not say that...");
            turn[3] = make_stringf("...those who did not become %s would be %s?", virtue[0].c_str(), smite[1].c_str());
            break;
        case 7:
            turn[1] = make_stringf("Zin said that %s shall be the law of the land, and...", virtue[1].c_str());
            turn[2] = make_stringf("...those who turn to %s will be %s. This was fair...", sin[1].c_str(), smite[1].c_str());
            turn[3] = make_stringf("...and just, and not a voice dissented.");
            break;
        case 8:
            turn[1] = make_stringf("Damned, damned be the %s and...", sinner.c_str());
            turn[2] = make_stringf("...all else who abandon %s! Let them...", virtue[1].c_str());
            turn[3] = make_stringf("...be %s by the jurisprudence of Zin!", smite[1].c_str());
            break;
        case 9:
            turn[1] = make_stringf("And Zin said to all in attendance, 'Which of ye...");
            turn[2] = make_stringf("...number among the %s? Come before me, that...", sinner.c_str());
            turn[3] = make_stringf("...I may %s you now for your %s!'", smite[0].c_str(), sin[1].c_str());
            break;
        case 10:
            turn[1] = make_stringf("Yea, I say unto thee, bring forth...");
            turn[2] = make_stringf("...the %s that they may know...", sinner.c_str());
            turn[3] = make_stringf("...the wrath of Zin, and thus be %s!", smite[1].c_str());
            break;
        case 11:
            turn[1] = make_stringf("In a great set of silver scales are weighed the...");
            turn[2] = make_stringf("...souls of the %s, and with their %s...", sinner.c_str(), sin[0].c_str());
            turn[3] = make_stringf("...ways, the balance hath tipped against them!");
            break;
        case 12:
            turn[1] = make_stringf("It is just that the %s shall be %s...", sinner.c_str(), smite[1].c_str());
            turn[2] = make_stringf("...in due time, for %s is what Zin has declared...", virtue[1].c_str());
            turn[3] = make_stringf("...the law of the land, and Zin's word is law!");
            break;
        case 13:
            turn[1] = make_stringf("Thus the people made the covenant of %s with...", virtue[1].c_str());
            turn[2] = make_stringf("...Zin, and all was good, for they knew that the...");
            turn[3] = make_stringf("...%s would trouble them no longer.", sinner.c_str());
            break;
        case 14:
            turn[1] = make_stringf("What of the %s? %s for their...", sinner.c_str(), uppercase_first(smite[1]).c_str());
            turn[2] = make_stringf("...%s they shall be! Zin will %s them again...", sin[1].c_str(), smite[0].c_str());
            turn[3] = make_stringf("...and again, and again!");
            break;
        case 15:
            turn[1] = make_stringf("And lo, the wrath of Zin did find...");
            turn[2] = make_stringf("...them wherever they hid, and the %s...", sinner.c_str());
            turn[3] = make_stringf("...were %s for their %s!", smite[1].c_str(), sin[1].c_str());
            break;
        case 16:
            turn[1] = make_stringf("Zin looked out upon the remains of the %s...", sinner.c_str());
            turn[2] = make_stringf("...and declared it good that they had been...");
            turn[3] = make_stringf("...%s. And thus justice was done.", smite[1].c_str());
            break;
        case 17:
            turn[1] = make_stringf("The law of Zin demands thee...");
            turn[2] = make_stringf("...be %s, and that the punishment for %s...", virtue[0].c_str(), sin[1].c_str());
            turn[3] = make_stringf("...shall be swift and harsh!");
            break;
        case 18:
            turn[1] = make_stringf("It was then that Zin bade them...");
            turn[2] = make_stringf("...not to stray from %s, lest...", virtue[1].c_str());
            turn[3] = make_stringf("...they become as damned as the %s.", sinner.c_str());
            break;
        case 19:
            turn[1] = make_stringf("Only the %s shall be judged worthy, and...", virtue[0].c_str());
            turn[2] = make_stringf("...all the %s will be found wanting. Such is...", sinner.c_str());
            turn[3] = make_stringf("...the word of Zin, and such is the law!");
            break;
        case 20:
            turn[1] = make_stringf("To those who would swear an oath of %s on my altar...", virtue[1].c_str());
            turn[2] = make_stringf("...I bring ye salvation. To the rest, ye %s...", sinner.c_str());
            turn[3] = make_stringf("...and the %s, the name of Zin shall be thy damnation.", sin[0].c_str());
            break;
        case 21:
            turn[1] = make_stringf("And Zin decreed that the people would be...");
            turn[2] = make_stringf("...protected from %s in all its forms, and...", sin[1].c_str());
            turn[3] = make_stringf("...preserved in their %s for all the days to come.", virtue[1].c_str());
            break;
        case 22:
            turn[1] = make_stringf("For those who would enter Zin's holy bosom...");
            turn[2] = make_stringf("...and live in %s, Zin provideth. Such is...", virtue[1].c_str());
            turn[3] = make_stringf("...the covenant, and such is the way of things.");
            break;
        case 23:
            turn[1] = make_stringf("Zin hath not damned the %s, but it is they...", sinner.c_str());
            turn[2] = make_stringf("...that have damned themselves for their %s, for...", sin[1].c_str());
            turn[3] = make_stringf("...did Zin not decree that to be %s was wrong?", sin[0].c_str());
            break;
        case 24:
            turn[1] = make_stringf("And Zin, furious at their %s, held...", sin[1].c_str());
            turn[2] = make_stringf("...aloft a silver sceptre! The %s...", sinner.c_str());
            turn[3] = make_stringf("...were %s, and thus the way of things was maintained.", smite[1].c_str());
            break;
        case 25:
            turn[1] = make_stringf("When the law of the land faltered, Zin rose...");
            turn[2] = make_stringf("...from the silver throne, and the %s were...", sinner.c_str());
            turn[3] = make_stringf("...%s. And it was thus that the law was made good.", smite[1].c_str());
            break;
        case 26:
            turn[1] = make_stringf("Zin descended from on high in a silver chariot...");
            turn[2] = make_stringf("...to %s the %s for their...", smite[0].c_str(), sinner.c_str());
            turn[3] = make_stringf("...%s, and thus judgement was rendered.", sin[1].c_str());
            break;
        case 27:
            turn[1] = make_stringf("The %s stood before Zin, and in that instant...", sinner.c_str());
            turn[2] = make_stringf("...they knew they would be found guilty of %s...", sin[1].c_str());
            turn[3] = make_stringf("...for that is the word of Zin, and Zin's word is law.");
            break;
    }

    std::string recite = "Hail Satan.";

    if (step == -1)
    {
        std::string bookname = (prayertype == RECITE_CHAOTIC)  ?  "Abominations"  :
                               (prayertype == RECITE_IMPURE)   ?  "Ablutions"     :
                               (prayertype == RECITE_HERETIC)  ?  "Apostates"     :
                               (prayertype == RECITE_UNHOLY)   ?  "Anathema"      :
                               (prayertype == RECITE_ALLY)     ?  "Alliances"     :
                                                                  "Bugginess";
        std::ostringstream numbers;
        numbers << chapter;
        numbers << ":";
        numbers << verse;
        recite = bookname + " " + numbers.str();
    }
    else
        recite = turn[step];

    return (recite);
}

typedef FixedVector<int, NUM_RECITE_TYPES> recite_counts;
// Check whether this monster might be influenced by Recite.
// Returns 0, if no monster found.
// Returns 1, if eligible monster found.
// Returns -1, if monster already affected or too dumb to understand.
int zin_check_recite_to_single_monster(const coord_def& where,
                                       recite_counts &eligibility)
{
    monster* mon = monster_at(where);

    //Can't recite at nothing!
    if (mon == NULL || !you.can_see(mon))
        return 0;

    // Can't recite if they were recently recited to.
    if (mon->has_ench(ENCH_RECITE_TIMER))
        return -1;

    const mon_holy_type holiness = mon->holiness();

    //Can't recite at plants or golems.
    if (holiness == MH_PLANT || holiness == MH_NONLIVING)
        return -1;

    eligibility.init(0);

    //Recitations are based on monster::is_unclean, but are NOT identical to it,
    //because that lumps all forms of uncleanliness together. We want to specify.

    //Anti-chaos prayer:

    //Hits some specific insane or shapeshifted uniques.
    if (mon->type == MONS_CRAZY_YIUF
        || mon->type == MONS_PSYCHE
        || mon->type == MONS_GASTRONOK)
    {
        eligibility[RECITE_CHAOTIC]++;
    }

    //Hits monsters that have chaotic spells memorized.
    if (mon->has_chaotic_spell() && mon->is_actual_spellcaster())
        eligibility[RECITE_CHAOTIC]++;

    //Hits monsters with 'innate' chaos.
    if (mon->is_chaotic())
        eligibility[RECITE_CHAOTIC]++;

    //Hits monsters that are worshipers of a chaotic god.
    if (is_chaotic_god(mon->god))
        eligibility[RECITE_CHAOTIC]++;

    //Hits (again) monsters that are priests of a chaotic god.
    if (is_chaotic_god(mon->god) && mon->is_priest())
        eligibility[RECITE_CHAOTIC]++;

    //Anti-impure prayer:

    //Hits monsters that have unclean spells memorized.
    if (mon->has_unclean_spell())
        eligibility[RECITE_IMPURE]++;

    //Hits monsters that desecrate the dead.
    if (mons_eats_corpses(mon))
        eligibility[RECITE_IMPURE]++;

    //Hits corporeal undead, which are a perversion of natural form.
    if (holiness == MH_UNDEAD && !mon->is_insubstantial())
        eligibility[RECITE_IMPURE]++;

    //Hits monsters that have these brands.
    if (mon->has_attack_flavour(AF_VAMPIRIC))
        eligibility[RECITE_IMPURE]++;
    if (mon->has_attack_flavour(AF_DISEASE))
        eligibility[RECITE_IMPURE]++;
    if (mon->has_attack_flavour(AF_HUNGER))
        eligibility[RECITE_IMPURE]++;
    if (mon->has_attack_flavour(AF_ROT))
        eligibility[RECITE_IMPURE]++;
    if (mon->has_attack_flavour(AF_STEAL))
        eligibility[RECITE_IMPURE]++;
    if (mon->has_attack_flavour(AF_STEAL_FOOD))
        eligibility[RECITE_IMPURE]++;

    //Death drakes and rotting devils get a bump to uncleanliness.
    if (mon->type == MONS_ROTTING_DEVIL || mon->type == MONS_DEATH_DRAKE)
        eligibility[RECITE_IMPURE]++;

    //Sanity check: if a monster is 'really' natural, don't consider it impure.
    if (mons_intel(mon) < I_NORMAL
        && (holiness == MH_NATURAL || holiness == MH_PLANT)
        && mon->type != MONS_UGLY_THING
        && mon->type != MONS_VERY_UGLY_THING
        && mon->type != MONS_DEATH_DRAKE)
    {
        eligibility[RECITE_IMPURE] = 0;
    }

    // Anti-unholy prayer

    // Hits monsters that are undead or demonic.
    if (holiness == MH_UNDEAD || holiness == MH_DEMONIC)
        eligibility[RECITE_UNHOLY]++;

    // Anti-heretic prayer
    // Pro-ally prayer

    // Sleeping or paralyzed monsters will wake up or still perceive their
    // surroundings, respectively.  So, you can still recite to them.

    if (mons_intel(mon) >= I_NORMAL
        && !(mon->has_ench(ENCH_DUMB) || mons_is_confused(mon)))
    {
        // In the eyes of Zin, everyone is a sinner until proven otherwise!
        eligibility[RECITE_HERETIC]++;

        // Any priest is a heretic...
        if (mon->is_priest())
            eligibility[RECITE_HERETIC]++;

        // ...but chaotic gods are worse...
        if (is_evil_god(mon->god))
            eligibility[RECITE_HERETIC]++;

        // ...as are evil gods.
        if (is_evil_god(mon->god) || mon->god == GOD_NAMELESS)
            eligibility[RECITE_HERETIC]++;

        // (The above mean that worshipers will be treated as
        // priests for reciting, even if they aren't actually.)

        // Sanity check: monsters that you can't convert anyway don't get
        // recited against.  Merely behaving evil doesn't get you off.
        if ((mon->is_unclean(false)
             || mon->is_chaotic()
             || mon->is_evil(false)
             || mon->is_unholy(false))
            && eligibility[RECITE_HERETIC] <= 1)
        {
            eligibility[RECITE_HERETIC] = 0;
        }

        // Sanity check: monsters that won't attack you, and aren't
        // priests/evil, don't get recited against.
        if (mon->wont_attack() && eligibility[RECITE_HERETIC] <= 1)
            eligibility[RECITE_HERETIC] = 0;

        // Sanity check: monsters that are holy, know holy spells, or worship
        // holy gods aren't heretics.
        if (mon->is_holy() || is_good_god(mon->god))
            eligibility[RECITE_HERETIC] = 0;

        // Any friendly that meets the above requirements is counted as an ally.
        if (mon->friendly())
            eligibility[RECITE_ALLY]++;

        // Holy friendlies get a boost.
        if ((mon->is_holy() || is_good_god(mon->god))
            && eligibility[RECITE_ALLY] > 0)
        {
            eligibility[RECITE_ALLY]++;
        }

        // Worshipers of Zin get another boost.
        if (mon->god == GOD_ZIN && eligibility[RECITE_ALLY] > 0)
            eligibility[RECITE_ALLY]++;
    }

#ifdef DEBUG_DIAGNOSTICS
    std::string elig;
    for (int i = 0; i < NUM_RECITE_TYPES; i++)
        elig += '0' + eligibility[i];
    dprf("Eligibility: %s", elig.c_str());
#endif

    //Checking to see whether they were eligible for anything at all.
    for (int i = 0; i < NUM_RECITE_TYPES; i++)
        if (eligibility[i] > 0)
            return 1;

    return 0;
}

// Check whether there are monsters who might be influenced by Recite.
// If 'recite' is false, we're just checking whether we can.
// If it's true, we're actually reciting and need to present a menu.

// Returns 0, if no monsters found.
// Returns 1, if eligible audience found.
// Returns -1, if entire audience already affected or too dumb to understand.
bool zin_check_able_to_recite()
{
    if (you.duration[DUR_BREATH_WEAPON])
    {
        mpr("You're too short of breath to recite.");
        return (false);
    }

        return (true);
}

static const char* zin_book_desc[NUM_RECITE_TYPES] =
{
    "Abominations (harms the forces of chaos and mutation)",
    "Ablutions (harms the unclean and walking corpses)",
    "Apostates (harms the faithless and heretics)",
    "Anathema (harms all types of demons and undead)",
    "Alliances (blesses intelligent allies)",
};

int zin_check_recite_to_monsters(recite_type *prayertype)
{
    bool found_ineligible = false;
    bool found_eligible = false;
    recite_counts count(0);

    for (radius_iterator ri(you.pos(), 8); ri; ++ri)
    {
        recite_counts retval;
        switch (zin_check_recite_to_single_monster(*ri, retval))
        {
        case -1:
            found_ineligible = true;
        case 0:
            continue;
        }

        for (int i = 0; i < NUM_RECITE_TYPES; i++)
            if (retval[i] > 0)
                count[i]++, found_eligible = true;
    }

    if (!found_eligible && !found_ineligible)
    {
        dprf("No audience found!");
        return (0);
    }
    else if (!found_eligible && found_ineligible)
    {
        dprf("No sensible audience found!");
        return (-1);
    }

    if (!prayertype)
        return (1);

    int eligible_types = 0;
    for (int i = 0; i < NUM_RECITE_TYPES; i++)
        if (count[i] > 0)
            eligible_types++;

    //If there's only one eligible recitation, and we're actually reciting, then perform it.
    if (eligible_types == 1)
    {
        for (int i = 0; i < NUM_RECITE_TYPES; i++)
            if (count[i] > 0)
                *prayertype = (recite_type)i;

        // If we got this far, we're actually reciting:
        you.increase_duration(DUR_BREATH_WEAPON, 3 + random2(10) + random2(30));
        return (1);
    }

    //But often, you'll have multiple options...
    mesclr();

    mpr("Recite a passage from which book of the Axioms of Law?", MSGCH_PROMPT);

    int menu_cnt = 0;
    recite_type letters[NUM_RECITE_TYPES];

    for (int i = 0; i < NUM_RECITE_TYPES; i++)
    {
        if (count[i] > 0 && i != RECITE_ALLY) // no ally recite yet
        {
            mprf("    [%c] - %s", 'a' + menu_cnt, zin_book_desc[i]);
            letters[menu_cnt++] = (recite_type)i;
        }
    }
    flush_prev_message();

    while (true)
    {
        int keyn = tolower(getch_ck());

        if (keyn >= 'a' && keyn < 'a' + menu_cnt)
        {
            *prayertype = letters[keyn - 'a'];
            break;
        }
        else
            return (0);
    }
    // If we got this far, we're actually reciting and are out of breath from it:
    you.increase_duration(DUR_BREATH_WEAPON, 3 + random2(10) + random2(30));
    return (1);
}

enum zin_eff
{
    ZIN_NOTHING,
    ZIN_SLEEP,
    ZIN_DAZE,
    ZIN_CONFUSE,
    ZIN_FEAR,
    ZIN_PARALYSE,
    ZIN_BLEED,
    ZIN_SMITE,
    ZIN_BLIND,
    ZIN_SILVER_CORONA,
    ZIN_ANTIMAGIC,
    ZIN_MUTE,
    ZIN_MAD,
    ZIN_DUMB,
    ZIN_IGNITE_CHAOS,
    ZIN_SALTIFY,
    ZIN_ROT,
    ZIN_HOLY_WORD,
};

bool zin_recite_to_single_monster(const coord_def& where,
                                  recite_type prayertype)
{
    // That's a pretty good sanity check, I guess.
    if (you.religion != GOD_ZIN)
        return (false);

    monster* mon = monster_at(where);

    if (!mon)
        return (false);

    recite_counts eligibility;
    bool affected = false;

    if (zin_check_recite_to_single_monster(where, eligibility) < 1)
        return (false);

    // First check: are they even eligible for this kind of recitation?
    // (Monsters that have been hurt by recitation aren't eligible.)
    if (eligibility[prayertype] < 1)
        return (false);

    // Second check: because this affects the whole screen over several turns,
    // its effects are staggered. There's a 50% chance per monster, per turn,
    // that nothing will happen - so the cumulative odds of nothing happening
    // are one in eight, since you recite three times.
    if (coinflip())
        return (false);

    // Resistance is now based on HD. You can affect up to (30+30)/2 = 30 'power' (HD).
    int power = (skill_bump(SK_INVOCATIONS) + you.piety * 3 / 20) / 2;
    // Old recite was mostly deterministic, which is bad.
    int resist = mon->get_experience_level() + random2(6);
    int check = power - resist;

    // We abort if we didn't *beat* their HD - but first we might get a cute message.
    if (mon->can_speak() && one_chance_in(5))
    {
        if (check < -10)
            simple_monster_message(mon, " guffaws at your puny god.");
        else if (check < -5)
            simple_monster_message(mon, " sneers at your recitation.");
    }

    if (check <= 0)
        return (false);

    // To what degree are they eligible for this prayertype?
    int degree = eligibility[prayertype];
    bool minor = degree <= ((prayertype == RECITE_HERETIC) ? 2 : 1);
    int spellpower = power * 2 + degree * 20;
    zin_eff effect = ZIN_NOTHING;

    switch (prayertype)
    {
    case RECITE_ALLY:
        // Stub. Not implemented yet.
        break;

    case RECITE_HERETIC:
        if (degree == 1)
        {
            if (mon->asleep())
                break;
            // This is the path for 'conversion' effects.
            // Their degree is only 1 if they weren't a priest,
            // a worshiper of an evil or chaotic god, etc.

            // Right now, it only has the 'failed conversion' effects, though.
            // This branch can't hit sleeping monsters - until they wake up.

            if (check < 5)
            {
#if 0
                // Sleep doesn't really work well. This should be more
                // 'forceful'. But how?
                if (one_chance_in(4))
                    effect = ZIN_SLEEP;
                else
#endif
                    effect = ZIN_DAZE;
            }
            else if (check < 10)
            {
                if (coinflip())
                    effect = ZIN_CONFUSE;
                else
                    effect = ZIN_DAZE;
            }
            else if (check < 15)
            {
                if (one_chance_in(3))
                    effect = ZIN_FEAR;
                else
                    effect = ZIN_CONFUSE;
            }
            else
            {
                if (one_chance_in(3))
                    effect = ZIN_PARALYSE;
                else
                    effect = ZIN_FEAR;
            }
        }
        else
        {
            // This is the path for 'smiting' effects.
            // Their degree is only greater than 1 if
            // they're unable to be redeemed.
            if (check < 5)
            {
                if (coinflip())
                    effect = ZIN_BLEED;
                else
                    effect = ZIN_SMITE;
            }
            else if (check < 10)
            {
                if (one_chance_in(3))
                    effect = ZIN_BLIND;
                else if (coinflip())
                    effect = ZIN_SILVER_CORONA;
                else
                    effect = ZIN_ANTIMAGIC;
            }
            else if (check < 15)
            {
                if (one_chance_in(3))
                    effect = ZIN_BLIND;
                else if (coinflip())
                    effect = ZIN_PARALYSE;
                else
                    effect = ZIN_MUTE;
            }
            else
            {
                if (coinflip())
                    effect = ZIN_MAD;
                else
                    effect = ZIN_DUMB;
            }
        }
        break;

    case RECITE_CHAOTIC:
        if (check < 5)
        {
            // nastier -- fallthrough if immune
            if (coinflip() && mon->can_bleed())
                effect = ZIN_BLEED;
            else
                effect = ZIN_SMITE;
        }
        else if (check < 10)
        {
            if (coinflip())
                effect = ZIN_SILVER_CORONA;
            else
                effect = ZIN_SMITE;
        }
        else if (check < 15)
        {
            if (coinflip())
                effect = ZIN_IGNITE_CHAOS;
            else
                effect = ZIN_SILVER_CORONA;
        }
        else
            effect = ZIN_SALTIFY;
        break;

    case RECITE_IMPURE:
        // Many creatures normally resistant to rotting are still affected,
        // because this is divine punishment.  Those with no real flesh are
        // immune, of course.
        if (check < 5)
        {
            if (coinflip() && mon->can_bleed())
                effect = ZIN_BLEED;
            else
                effect = ZIN_SMITE;
        }
        else if (check < 10)
        {
            if (coinflip() && mon->res_rotting() <= 1)
                effect = ZIN_ROT;
            else
                effect = ZIN_SILVER_CORONA;
        }
        else if (check < 15)
        {
            if (mon->undead_or_demonic() && coinflip())
                effect = ZIN_HOLY_WORD;
            else
                effect = ZIN_SILVER_CORONA;
        }
        else
            effect = ZIN_SALTIFY;
        break;

    case RECITE_UNHOLY:
        if (check < 5)
        {
            if (mons_intel(mon) > I_PLANT && coinflip())
                effect = ZIN_DAZE;
            else
                effect = ZIN_CONFUSE;
        }
        else if (check < 10)
        {
            if (coinflip())
                effect = ZIN_FEAR;
            else
                effect = ZIN_SILVER_CORONA;
        }
        // Half of the time, the anti-unholy prayer will be capped at this
        // level of effect.
        else if (check < 15 || coinflip())
        {
            if (coinflip())
                effect = ZIN_HOLY_WORD;
            else
                effect = ZIN_SILVER_CORONA;
        }
        else
            effect = ZIN_SALTIFY;
        break;

    case NUM_RECITE_TYPES:
        die("invalid recite type");
    }

    // And the actual effects...
    switch (effect)
    {
    case ZIN_NOTHING:
        break;

    case ZIN_SLEEP:
        if (mon->can_sleep())
        {
            mon->put_to_sleep(&you, 0);
            simple_monster_message(mon, " nods off for a moment.");
            affected = true;
        }
        break;

    case ZIN_DAZE:
        if (mon->add_ench(mon_enchant(ENCH_DAZED, degree, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            simple_monster_message(mon, " is dazed by your recitation.");
            affected = true;
        }
        break;

    case ZIN_CONFUSE:
        if (mons_class_is_confusable(mon->type)
            && mon->add_ench(mon_enchant(ENCH_CONFUSION, degree, &you,
                             (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            if (prayertype == RECITE_HERETIC)
                simple_monster_message(mon, " is confused by your recitation.");
            else
                simple_monster_message(mon, " stumbles about in disarray.");
            affected = true;
        }
        break;

    case ZIN_FEAR:
        if (mon->add_ench(mon_enchant(ENCH_FEAR, degree, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            if (prayertype == RECITE_HERETIC)
                simple_monster_message(mon, " is terrified by your recitation.");
            else if (minor)
                simple_monster_message(mon, " tries to escape the wrath of Zin.");
            else
                simple_monster_message(mon, " flees in terror at the wrath of Zin!");
            behaviour_event(mon, ME_SCARE, MHITNOT, you.pos());
            affected = true;
        }
        break;

    case ZIN_PARALYSE:
        if (mon->add_ench(mon_enchant(ENCH_PARALYSIS, 0, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            simple_monster_message(mon,
                minor ? " is awed by your recitation."
                      : " is aghast at the heresy of your recitation.");
            affected = true;
        }
        break;

    case ZIN_BLEED:
        if (mon->can_bleed()
            && mon->add_ench(mon_enchant(ENCH_BLEED, degree, &you,
                             (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            mon->add_ench(mon_enchant(ENCH_SICK, degree, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY));
            switch (prayertype)
            {
            case RECITE_HERETIC:
                if (minor)
                    simple_monster_message(mon, "'s eyes and ears begin to bleed.");
                else
                {
                    mprf("%s bleeds profusely from %s eyes and ears.",
                         mon->name(DESC_CAP_THE).c_str(),
                         mons_pronoun(mon->type, PRONOUN_NOCAP_POSSESSIVE));
                }
                break;
            case RECITE_CHAOTIC:
                simple_monster_message(mon,
                    minor ? "'s chaotic flesh is covered in bleeding sores."
                          : "'s chaotic flesh erupts into weeping sores!");
                break;
            case RECITE_IMPURE:
                simple_monster_message(mon,
                    minor ? "'s impure flesh is covered in bleeding sores."
                          : "'s impure flesh erupts into weeping sores!");
                break;
            default:
                die("bad recite bleed");
            }
            affected = true;
        }
        break;

    case ZIN_SMITE:
        if (minor)
            simple_monster_message(mon, " is smitten by the wrath of Zin.");
        else
            simple_monster_message(mon, " is blasted by the fury of Zin!");
        // XXX: This duplicates code in cast_smiting().
        mon->hurt(&you, 7 + (random2(spellpower) * 33 / 191));
        if (mon->alive())
            print_wounds(mon);
        affected = true;
        break;

    case ZIN_BLIND:
        if (mon->add_ench(mon_enchant(ENCH_BLIND, degree, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            simple_monster_message(mon, " is struck blind by the wrath of Zin!");
            affected = true;
        }
        break;

    case ZIN_SILVER_CORONA:
        if (mon->add_ench(mon_enchant(ENCH_SILVER_CORONA, degree, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            simple_monster_message(mon, " is limned with silver light.");
            affected = true;
        }
        break;

    case ZIN_ANTIMAGIC:
        if (mon->add_ench(mon_enchant(ENCH_ANTIMAGIC, degree, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            ASSERT(prayertype == RECITE_HERETIC);
            simple_monster_message(mon,
                minor ? " quails at your recitation."
                      : " looks feeble and powerless before your recitation.");
            affected = true;
        }
        break;

    case ZIN_MUTE:
        if (mon->add_ench(mon_enchant(ENCH_MUTE, degree, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            simple_monster_message(mon, " is struck mute by the wrath of Zin!");
            affected = true;
        }
        break;

    case ZIN_MAD:
        if (mon->add_ench(mon_enchant(ENCH_MAD, degree, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            simple_monster_message(mon, " is driven mad by the wrath of Zin!");
            affected = true;
        }
        break;

    case ZIN_DUMB:
        if (mon->add_ench(mon_enchant(ENCH_DUMB, degree, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            simple_monster_message(mon, " is left stupefied by the wrath of Zin!");
            affected = true;
        }
        break;

    case ZIN_IGNITE_CHAOS:
        ASSERT(prayertype == RECITE_CHAOTIC);
        {
            bolt beam;
            dice_def dam_dice(0, 5 + spellpower/7);  // Dice added below if applicable.
            dam_dice.num = degree;

            int damage = dam_dice.roll();
            if (damage > 0)
            {
                mon->hurt(&you, damage, BEAM_MISSILE, false);

                if (mon->alive())
                {
                    simple_monster_message(mon,
                      (damage < 25) ? "'s chaotic flesh sizzles and spatters!" :
                      (damage < 50) ? "'s chaotic flesh bubbles and boils."
                                    : "'s chaotic flesh runs like molten wax.");

                    print_wounds(mon);
                    behaviour_event(mon, ME_WHACK, MHITYOU);
                    affected = true;
                }
                else
                {
                    simple_monster_message(mon,
                        " melts away into a sizzling puddle of chaotic flesh.");
                    monster_die(mon, KILL_YOU, NON_MONSTER);
                }
            }
        }
        break;

    case ZIN_SALTIFY:
        zin_saltify(mon);
        break;

    case ZIN_ROT:
        ASSERT(prayertype == RECITE_IMPURE);
        if (mon->res_rotting() <= 1
            && mon->add_ench(mon_enchant(ENCH_ROT, degree, &you,
                             (degree + random2(spellpower)) * BASELINE_DELAY)))
        {
            mon->add_ench(mon_enchant(ENCH_SICK, degree, &you,
                          (degree + random2(spellpower)) * BASELINE_DELAY));
            simple_monster_message(mon,
                minor ? "'s impure flesh begins to rot away."
                      : "'s impure flesh sloughs off!");
            affected = true;
        }
        break;

    case ZIN_HOLY_WORD:
        holy_word_monsters(where, spellpower, HOLY_WORD_ZIN, &you);
        affected = true;
        break;
    }

    // Recite time, to prevent monsters from being recited against
    // more than once in a given recite instance.
    if (affected)
        mon->add_ench(mon_enchant(ENCH_RECITE_TIMER, degree, &you, 40));

    // Monsters that have been affected may shout.
    if (affected
        && one_chance_in(3)
        && mon->alive()
        && !mon->asleep()
        && !mon->cannot_move()
        && mons_shouts(mon->type, false) != S_SILENT)
    {
        force_monster_shout(mon);
    }

    return (true);
}

void zin_saltify(monster* mon)
{
    const coord_def where = mon->pos();
    const monster_type pillar_type =
        mons_is_zombified(mon) ? mons_zombie_base(mon)
                               : mons_species(mon->type);
    const int hd = mon->get_experience_level();

    simple_monster_message(mon, " is turned into a pillar of salt by the wrath of Zin!");

    // If the monster leaves a corpse when it dies, destroy the corpse.
    int corpse = monster_die(mon, KILL_YOU, NON_MONSTER);
    if (corpse != -1)
        destroy_item(corpse);

    const int pillar = create_monster(
                        mgen_data(MONS_SALT_PILLAR,
                                  BEH_HOSTILE,
                                  0,
                                  0,
                                  0,
                                  where,
                                  MHITNOT,
                                  MG_FORCE_PLACE,
                                  GOD_NO_GOD,
                                  pillar_type),
                                  false);

    if (pillar != -1)
    {
        // Enemies with more HD leave longer-lasting pillars of salt.
        int time_left = (random2(8) + hd) * BASELINE_DELAY;
        mon_enchant temp_en(ENCH_SLOWLY_DYING, 1, 0, time_left);
        env.mons[pillar].update_ench(temp_en);
    }
}

static bool _kill_duration(duration_type dur)
{
    const bool rc = (you.duration[dur] > 0);
    you.duration[dur] = 0;
    return (rc);
}

bool zin_vitalisation()
{
    bool success = false;
    int type = 0;

    // Remove negative afflictions.
    if (you.disease || you.rotting || you.confused()
        || you.duration[DUR_PARALYSIS] || you.duration[DUR_POISONING]
        || you.petrified())
    {
        do
        {
            switch (random2(6))
            {
            case 0:
                if (you.disease)
                {
                    success = true;
                    you.disease = 0;
                }
                break;
            case 1:
                if (you.rotting)
                {
                    success = true;
                    you.rotting = 0;
                }
                break;
            case 2:
                success = _kill_duration(DUR_CONF);
                break;
            case 3:
                success = _kill_duration(DUR_PARALYSIS);
                break;
            case 4:
                success = _kill_duration(DUR_POISONING);
                break;
            case 5:
                success = _kill_duration(DUR_PETRIFIED);
                break;
            }
        }
        while (!success);
    }
    // Restore stats.
    else if (you.strength() < you.max_strength()
             || you.intel() < you.max_intel()
             || you.dex() < you.max_dex())
    {
        type = 1;
        restore_stat(STAT_RANDOM, 0, true);
        success = true;
    }
    else
    {
        // Add divine stamina.
        if (!you.duration[DUR_DIVINE_STAMINA])
        {
            success = true;
            type = 2;

            mprf("%s grants you divine stamina.",
                 god_name(GOD_ZIN).c_str());

            const int stamina_amt = 3;
            you.attribute[ATTR_DIVINE_STAMINA] = stamina_amt;
            you.set_duration(DUR_DIVINE_STAMINA,
                             40 + (you.skill(SK_INVOCATIONS)*5)/2);

            notify_stat_change(STAT_STR, stamina_amt, true, "");
            notify_stat_change(STAT_INT, stamina_amt, true, "");
            notify_stat_change(STAT_DEX, stamina_amt, true, "");
        }
    }

    // If vitalisation has succeeded, display an appropriate message.
    if (success)
    {
        mprf("You feel %s.", (type == 0) ? "better" :
                             (type == 1) ? "renewed"
                                         : "powerful");
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return (success);
}

void zin_remove_divine_stamina()
{
    mpr("Your divine stamina fades away.", MSGCH_DURATION);
    notify_stat_change(STAT_STR, -you.attribute[ATTR_DIVINE_STAMINA],
                true, "Zin's divine stamina running out");
    notify_stat_change(STAT_INT, -you.attribute[ATTR_DIVINE_STAMINA],
                true, "Zin's divine stamina running out");
    notify_stat_change(STAT_DEX, -you.attribute[ATTR_DIVINE_STAMINA],
                true, "Zin's divine stamina running out");
    you.duration[DUR_DIVINE_STAMINA] = 0;
    you.attribute[ATTR_DIVINE_STAMINA] = 0;
}

bool zin_remove_all_mutations()
{
    if (!how_mutated())
    {
        mpr("You have no mutations to be cured!");
        return (false);
    }

    you.num_current_gifts[GOD_ZIN]++;
    you.num_total_gifts[GOD_ZIN]++;
    take_note(Note(NOTE_GOD_GIFT, you.religion));

    simple_god_message(" draws all chaos from your body!");
    delete_all_mutations();

    return (true);
}

bool zin_sanctuary()
{
    // Casting is disallowed while previous sanctuary in effect.
    // (Checked in abl-show.cc.)
    if (env.sanctuary_time)
        return (false);

    // Yes, shamelessly stolen from NetHack...
    if (!silenced(you.pos())) // How did you manage that?
        mpr("You hear a choir sing!", MSGCH_SOUND);
    else
        mpr("You are suddenly bathed in radiance!");

    flash_view(WHITE);

    holy_word(100, HOLY_WORD_ZIN, you.pos(), true);

#ifndef USE_TILE
    // Allow extra time for the flash to linger.
    delay(1000);
#endif

    // Pets stop attacking and converge on you.
    you.pet_target = MHITYOU;

    create_sanctuary(you.pos(), 7 + you.skill(SK_INVOCATIONS) / 2);

    return (true);
}

// shield bonus = attribute for duration turns, then decreasing by 1
//                every two out of three turns
// overall shield duration = duration + attribute
// recasting simply resets those two values (to better values, presumably)
void tso_divine_shield()
{
    if (!you.duration[DUR_DIVINE_SHIELD])
    {
        if (you.shield()
            || you.duration[DUR_CONDENSATION_SHIELD])
        {
            mprf("Your shield is strengthened by %s's divine power.",
                 god_name(GOD_SHINING_ONE).c_str());
        }
        else
            mpr("A divine shield forms around you!");
    }
    else
        mpr("Your divine shield is renewed.");

    you.redraw_armour_class = true;

    // duration of complete shield bonus from 35 to 80 turns
    you.set_duration(DUR_DIVINE_SHIELD,
                     35 + (you.skill(SK_INVOCATIONS) * 4) / 3);

    // shield bonus up to 8
    you.attribute[ATTR_DIVINE_SHIELD] = 3 + you.skill(SK_SHIELDS) / 5;

    you.redraw_armour_class = true;
}

void tso_remove_divine_shield()
{
    mpr("Your divine shield disappears!", MSGCH_DURATION);
    you.duration[DUR_DIVINE_SHIELD] = 0;
    you.attribute[ATTR_DIVINE_SHIELD] = 0;
    you.redraw_armour_class = true;
}

void elyvilon_purification()
{
    mpr("You feel purified!");

    you.disease = 0;
    you.rotting = 0;
    you.duration[DUR_POISONING] = 0;
    you.duration[DUR_CONF] = 0;
    you.duration[DUR_SLOW] = 0;
    you.duration[DUR_PARALYSIS] = 0;          // can't currently happen -- bwr
    you.duration[DUR_PETRIFIED] = 0;
}

bool elyvilon_divine_vigour()
{
    bool success = false;

    if (!you.duration[DUR_DIVINE_VIGOUR])
    {
        mprf("%s grants you divine vigour.",
             god_name(GOD_ELYVILON).c_str());

        const int vigour_amt = 1 + (you.skill(SK_INVOCATIONS)/3);
        const int old_hp_max = you.hp_max;
        const int old_mp_max = you.max_magic_points;
        you.attribute[ATTR_DIVINE_VIGOUR] = vigour_amt;
        you.set_duration(DUR_DIVINE_VIGOUR,
                         40 + (you.skill(SK_INVOCATIONS)*5)/2);

        calc_hp();
        inc_hp(you.hp_max - old_hp_max, false);
        calc_mp();
        inc_mp(you.max_magic_points - old_mp_max, false);

        success = true;
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return (success);
}

void elyvilon_remove_divine_vigour()
{
    mpr("Your divine vigour fades away.", MSGCH_DURATION);
    you.duration[DUR_DIVINE_VIGOUR] = 0;
    you.attribute[ATTR_DIVINE_VIGOUR] = 0;
    calc_hp();
    calc_mp();
}

bool vehumet_supports_spell(spell_type spell)
{
    if (spell_typematch(spell, SPTYP_CONJURATION | SPTYP_SUMMONING))
        return (true);

    // Conjurations work by conjuring up a chunk of short-lived matter and
    // propelling it towards the victim.  This is the most popular way, but
    // by no means it has a monopoly for being destructive.
    // Vehumet loves all direct physical destruction.
    if (spell == SPELL_SHATTER
        || spell == SPELL_FRAGMENTATION // LRD
        || spell == SPELL_SANDBLAST
        || spell == SPELL_AIRSTRIKE
        || spell == SPELL_TORNADO
        || spell == SPELL_FREEZE
        || spell == SPELL_IGNITE_POISON
        || spell == SPELL_OZOCUBUS_REFRIGERATION)
        // Toxic Radiance does no direct damage
    {
        return (true);
    }

    return (false);
}

// Returns false if the invocation fails (no spellbooks in sight, etc.).
bool trog_burn_spellbooks()
{
    if (you.religion != GOD_TROG)
        return (false);

    god_acting gdact;

    for (stack_iterator si(you.pos()); si; ++si)
    {
        if (item_is_spellbook(*si))
        {
            mpr("Burning your own feet might not be such a smart idea!");
            return (false);
        }
    }

    int totalpiety = 0;
    int totalblocked = 0;

    for (radius_iterator ri(you.pos(), LOS_RADIUS, true, true, true); ri; ++ri)
    {
        const unsigned short cloud = env.cgrid(*ri);
        int count = 0;
        int rarity = 0;
        for (stack_iterator si(*ri); si; ++si)
        {
            if (!item_is_spellbook(*si))
                continue;

            // If a grid is blocked, books lying there will be ignored.
            // Allow bombing of monsters.
            if (feat_is_solid(grd(*ri))
                || cloud != EMPTY_CLOUD && env.cloud[cloud].type != CLOUD_FIRE)
            {
                totalblocked++;
                continue;
            }

            // Ignore {!D} inscribed books.
            if (!check_warning_inscriptions(*si, OPER_DESTROY))
            {
                mpr("Won't ignite {!D} inscribed spellbook.");
                continue;
            }

            // Piety increases by 2 for books never cracked open, else 1.
            // Conversely, rarity influences the duration of the pyre.
            if (!item_type_known(*si))
                totalpiety += 2;
            else
                totalpiety++;

            rarity += book_rarity(si->sub_type);

            dprf("Burned spellbook rarity: %d", rarity);
            destroy_spellbook(*si);
            destroy_item(si.link());
            count++;
        }

        if (count)
        {
            if (cloud != EMPTY_CLOUD)
            {
                // Reinforce the cloud.
                mpr("The fire roars with new energy!");
                const int extra_dur = count + random2(rarity / 2);
                env.cloud[cloud].decay += extra_dur * 5;
                env.cloud[cloud].set_whose(KC_YOU);
                continue;
            }

            const int duration = std::min(4 + count + random2(rarity/2), 23);
            place_cloud(CLOUD_FIRE, *ri, duration, &you);

            mprf(MSGCH_GOD, "The spellbook%s burst%s into flames.",
                 count == 1 ? ""  : "s",
                 count == 1 ? "s" : "");
        }
    }

    if (totalpiety)
    {
        simple_god_message(" is delighted!", GOD_TROG);
        gain_piety(totalpiety);
    }
    else if (totalblocked)
    {
        mprf("The spellbook%s fail%s to ignite!",
             totalblocked == 1 ? ""  : "s",
             totalblocked == 1 ? "s" : "");
        return (false);
    }
    else
    {
        mpr("You cannot see a spellbook to ignite!");
        return (false);
    }

    return (true);
}

bool beogh_water_walk()
{
    return (you.religion == GOD_BEOGH && !player_under_penance()
            && you.piety >= piety_breakpoint(4));
}

bool jiyva_can_paralyse_jellies()
{
    return (you.religion == GOD_JIYVA && !player_under_penance()
            && you.piety >= piety_breakpoint(2));
}

void jiyva_paralyse_jellies()
{
    int jelly_count = 0;
    for (radius_iterator ri(you.pos(), 9); ri; ++ri)
    {
        monster* mon = monster_at(*ri);

        if (mon != NULL && mons_is_slime(mon))
        {
            mon->add_ench(mon_enchant(ENCH_PARALYSIS, 0,
                                      &you, 200));
            jelly_count++;
        }
    }

    if (jelly_count > 0)
    {
        mprf(MSGCH_PRAY, "%s.",
             jelly_count > 1 ? "The nearby slimes join your prayer"
                             : "A nearby slime joins your prayer");
        lose_piety(5);
    }
}

bool jiyva_remove_bad_mutation()
{
    if (!how_mutated())
    {
        mpr("You have no bad mutations to be cured!");
        return (false);
    }

    // Ensure that only bad mutations are removed.
    if (!delete_mutation(RANDOM_BAD_MUTATION, true, false, true, true))
    {
        canned_msg(MSG_NOTHING_HAPPENS);
        return (false);
    }

    mpr("You feel cleansed.");
    return (true);
}

bool yred_injury_mirror()
{
    return (you.religion == GOD_YREDELEMNUL && !player_under_penance()
            && you.piety >= piety_breakpoint(1)
            && you.duration[DUR_MIRROR_DAMAGE]);
}

bool yred_can_animate_dead()
{
    return (you.religion == GOD_YREDELEMNUL && !player_under_penance()
            && you.piety >= piety_breakpoint(2));
}

void yred_animate_remains_or_dead()
{
    if (yred_can_animate_dead())
    {
        mpr("You call on the dead to rise...");

        animate_dead(&you, you.skill(SK_INVOCATIONS) + 1, BEH_FRIENDLY,
                     MHITYOU, &you, "", GOD_YREDELEMNUL);
    }
    else
    {
        mpr("You attempt to give life to the dead...");

        if (animate_remains(you.pos(), CORPSE_BODY, BEH_FRIENDLY,
                            MHITYOU, &you, "", GOD_YREDELEMNUL) < 0)
        {
            mpr("There are no remains here to animate!");
        }
    }
}

void yred_drain_life()
{
    mpr("You draw life from your surroundings.");

    flash_view(DARKGREY);
    more();
    mesclr();

    const int pow = you.skill(SK_INVOCATIONS);
    const int hurted = 3 + random2(7) + random2(pow);
    int hp_gain = 0;

    for (monster_iterator mi(you.get_los()); mi; ++mi)
    {
        if (mi->res_negative_energy())
            continue;

        if (mi->wont_attack())
            continue;

        mprf("You draw life from %s.",
             mi->name(DESC_NOCAP_THE).c_str());

        behaviour_event(*mi, ME_WHACK, MHITYOU, you.pos());

        mi->hurt(&you, hurted);

        if (mi->alive())
            print_wounds(*mi);

        if (!mi->is_summoned())
            hp_gain += hurted;
    }

    hp_gain /= 2;

    hp_gain = std::min(pow * 2, hp_gain);

    if (hp_gain)
    {
        mpr("You feel life flooding into your body.");
        inc_hp(hp_gain, false);
    }
}

void yred_make_enslaved_soul(monster* mon, bool force_hostile)
{
    add_daction(DACT_OLD_ENSLAVED_SOULS_POOF);

    const std::string whose =
        you.can_see(mon) ? apostrophise(mon->name(DESC_CAP_THE))
                         : mon->pronoun(PRONOUN_CAP_POSSESSIVE);

    // If the monster's held in a net, get it out.
    mons_clear_trapping_net(mon);

    // Drop the monster's holy equipment, and keep wielding the rest.
    monster_drop_things(mon, false, is_holy_item);

    const monster orig = *mon;

    // Use the original monster type as the zombified type here, to get
    // the proper stats from it.
    define_zombie(mon, mon->type, MONS_SPECTRAL_THING);

    // If the original monster has been drained or levelled up, its HD
    // might be different from its class HD, in which case its HP should
    // be rerolled to match.
    if (mon->hit_dice != orig.hit_dice)
    {
        mon->hit_dice = std::max(orig.hit_dice, 1);
        roll_zombie_hp(mon);
    }

    mon->colour = ETC_UNHOLY;
    mon->speed  = mons_class_base_speed(mon->base_monster);

    mon->flags |= MF_NO_REWARD;
    mon->flags |= MF_ENSLAVED_SOUL;

    // If the original monster type has melee, spellcasting or priestly
    // abilities, make sure its spectral thing has them as well.
    mon->flags |= orig.flags & (MF_MELEE_MASK | MF_SPELL_MASK);
    mon->spells = orig.spells;

    name_zombie(mon, &orig);

    mons_make_god_gift(mon, GOD_YREDELEMNUL);

    mon->attitude = !force_hostile ? ATT_FRIENDLY : ATT_HOSTILE;
    behaviour_event(mon, ME_ALERT, !force_hostile ? MHITNOT : MHITYOU);

    mprf("%s soul %s.", whose.c_str(),
         !force_hostile ? "is now yours" : "fights you");
}

bool kiku_receive_corpses(int pow, coord_def where)
{
    // pow = necromancy * 4, ranges from 0 to 108
    dprf("kiku_receive_corpses() power: %d", pow);

    // Kiku gives branch-appropriate corpses (like shadow creatures).
    int expected_extra_corpses = 1 + random2(pow / 36); // 1 at 0 Nec, up to 4 at 27 Nec.
    int corpse_delivery_radius = 1;

    // We should get the same number of corpses
    // in a hallway as in an open room.
    int spaces_for_corpses = 0;
    for (radius_iterator ri(where, corpse_delivery_radius, C_ROUND,
                            you.get_los(), true);
         ri; ++ri)
    {
        if (mons_class_can_pass(MONS_HUMAN, grd(*ri)))
            spaces_for_corpses++;
    }
    // floating over lava, heavy tomb abuse, etc
    if (!spaces_for_corpses)
        spaces_for_corpses++;

    int percent_chance_a_square_receives_extra_corpse = // can be > 100
        int(float(expected_extra_corpses) / float(spaces_for_corpses) * 100.0);

    int corpses_created = 0;

    for (radius_iterator ri(where, corpse_delivery_radius, C_ROUND,
                            you.get_los());
         ri; ++ri)
    {
        bool square_is_walkable = mons_class_can_pass(MONS_HUMAN, grd(*ri));
        bool square_is_player_square = (*ri == where);
        bool square_gets_corpse =
            (random2(100) < percent_chance_a_square_receives_extra_corpse)
            || (square_is_player_square && random2(100) < 97);

        if (!square_is_walkable || !square_gets_corpse)
            continue;

        corpses_created++;

        // Find an appropriate monster corpse for level and power.
        monster_type mon_type = MONS_PROGRAM_BUG;
        int adjusted_power = 0;
        for (int i = 0; i < 200 && !mons_class_can_be_zombified(mon_type); ++i)
        {
            adjusted_power = std::min(pow / 4, random2(random2(pow)));
            mon_type = pick_local_zombifiable_monster(adjusted_power);
        }

        // Create corpse object.
        monster dummy;
        dummy.type = mon_type;
        define_monster(&dummy);
        int index_of_corpse_created = get_item_slot();

        if (index_of_corpse_created == NON_ITEM)
            break;

        int valid_corpse = fill_out_corpse(&dummy,
                                           dummy.type,
                                           mitm[index_of_corpse_created],
                                           false);
        if (valid_corpse == -1)
        {
            mitm[index_of_corpse_created].clear();
            continue;
        }

        mitm[index_of_corpse_created].props["DoNotDropHide"] = true;

        ASSERT(valid_corpse >= 0);

        // Higher piety means fresher corpses.  One out of ten corpses
        // will always be rotten.
        int rottedness = 200 -
            (!one_chance_in(10) ? random2(200 - you.piety)
                                : random2(100 + random2(75)));
        mitm[index_of_corpse_created].special = rottedness;

        // Place the corpse.
        move_item_to_grid(&index_of_corpse_created, *ri);
    }

    if (corpses_created)
    {
        if (you.religion == GOD_KIKUBAAQUDGHA)
        {
            simple_god_message(corpses_created > 1 ? " delivers you corpses!"
                                                   : " delivers you a corpse!");
        }
        maybe_update_stashes();
        return (true);
    }
    else
    {
        if (you.religion == GOD_KIKUBAAQUDGHA)
            simple_god_message(" can find no cadavers for you!");
        return (false);
    }
}

bool kiku_take_corpse()
{
    for (int i = you.visible_igrd(you.pos()); i != NON_ITEM; i = mitm[i].link)
    {
        item_def &item(mitm[i]);

        if (item.base_type != OBJ_CORPSES || item.sub_type != CORPSE_BODY)
            continue;
        // should only fresh corpses count?

        // only nets currently, but let's check anyway...
        if (item_is_stationary(item))
            continue;

        item_was_destroyed(item);
        destroy_item(i);
        return true;
    }

    return false;
}

bool fedhas_passthrough_class(const monster_type mc)
{
    return (you.religion == GOD_FEDHAS
            && mons_class_is_plant(mc)
            && mons_class_is_stationary(mc));
}

// Fedhas allows worshipers to walk on top of stationary plants and
// fungi.
bool fedhas_passthrough(const monster* target)
{
    return (target
            && fedhas_passthrough_class(target->type)
            && (target->type != MONS_OKLOB_PLANT
                || target->attitude != ATT_HOSTILE));
}

bool fedhas_passthrough(const monster_info* target)
{
    return (target
            && fedhas_passthrough_class(target->type)
            && (target->type != MONS_OKLOB_PLANT
                || target->attitude != ATT_HOSTILE));
}

// Fedhas worshipers can shoot through non-hostile plants, can a
// particular beam go through a particular monster?
bool fedhas_shoot_through(const bolt & beam, const monster* victim)
{
    actor * originator = beam.agent();
    if (!victim || !originator)
        return (false);

    bool origin_worships_fedhas;
    mon_attitude_type origin_attitude;
    if (originator->atype() == ACT_PLAYER)
    {
        origin_worships_fedhas = you.religion == GOD_FEDHAS;
        origin_attitude = ATT_FRIENDLY;
    }
    else
    {
        monster* temp = originator->as_monster();
        if (!temp)
            return (false);
        origin_worships_fedhas = temp->god == GOD_FEDHAS;
        origin_attitude = temp->attitude;
    }

    return (origin_worships_fedhas
            && fedhas_protects(victim)
            && !beam.is_enchantment()
            && !(beam.is_explosion && beam.in_explosion_phase)
            && (mons_atts_aligned(victim->attitude, origin_attitude)
                || victim->neutral()));
}

// Turns corpses in LOS into skeletons and grows toadstools on them.
// Can also turn zombies into skeletons and destroy ghoul-type monsters.
// Returns the number of corpses consumed.
int fedhas_fungal_bloom()
{
    int seen_mushrooms  = 0;
    int seen_corpses    = 0;

    int processed_count = 0;
    bool kills = false;

    for (radius_iterator i(you.pos(), LOS_RADIUS); i; ++i)
    {
        monster* target = monster_at(*i);
        if (target && target->is_summoned())
            continue;

        if (!is_harmless_cloud(cloud_type_at(*i)))
            continue;

        if (target && target->mons_species() != MONS_TOADSTOOL)
        {
            switch (mons_genus(target->mons_species()))
            {
            case MONS_ZOMBIE_SMALL:
                // Maybe turn a zombie into a skeleton.
                if (mons_skeleton(mons_zombie_base(target)))
                {
                    processed_count++;

                    monster_type skele_type = MONS_SKELETON_LARGE;
                    if (mons_zombie_size(mons_zombie_base(target)) == Z_SMALL)
                        skele_type = MONS_SKELETON_SMALL;

                    // Killing and replacing the zombie since upgrade_type
                    // doesn't get skeleton speed right (and doesn't
                    // reduce the victim's HP). This is awkward. -cao
                    mgen_data mg(skele_type, target->behaviour, NULL, 0, 0,
                                 target->pos(),
                                 target->foe,
                                 MG_FORCE_BEH | MG_FORCE_PLACE,
                                 target->god,
                                 mons_zombie_base(target),
                                 target->number);

                    unsigned monster_flags = target->flags;
                    int current_hp = target->hit_points;
                    mon_enchant_list ench = target->enchantments;

                    simple_monster_message(target, "'s flesh rots away.");

                    monster_die(target, KILL_MISC, NON_MONSTER, true);
                    int mons = create_monster(mg);
                    env.mons[mons].flags = monster_flags;
                    env.mons[mons].enchantments = ench;

                    if (env.mons[mons].hit_points > current_hp)
                        env.mons[mons].hit_points = current_hp;

                    behaviour_event(&env.mons[mons], ME_ALERT, MHITYOU);

                    continue;
                }
                // Else fall through and destroy the zombie.
                // Ghoul-type monsters are always destroyed.
            case MONS_GHOUL:
            {
                simple_monster_message(target, " rots away and dies.");

                coord_def pos = target->pos();
                int colour    = target->colour;
                int corpse    = monster_die(target, KILL_MISC, NON_MONSTER, true);
                kills = true;

                // If a corpse didn't drop, create a toadstool.
                // If one did drop, we will create toadstools from it as usual
                // later on.
                if (corpse < 0)
                {
                    const int mushroom = create_monster(
                                mgen_data(MONS_TOADSTOOL,
                                          BEH_GOOD_NEUTRAL,
                                          &you,
                                          0,
                                          0,
                                          pos,
                                          MHITNOT,
                                          MG_FORCE_PLACE,
                                          GOD_NO_GOD,
                                          MONS_NO_MONSTER,
                                          0,
                                          colour),
                                          false);

                    if (mushroom != -1)
                        seen_mushrooms++;

                    processed_count++;

                    continue;
                }
                break;
            }

            default:
                continue;
            }
        }

        for (stack_iterator j(*i); j; ++j)
        {
            bool corpse_on_pos = false;
            if (j->base_type == OBJ_CORPSES && j->sub_type == CORPSE_BODY)
            {
                corpse_on_pos  = true;
                int trial_prob = mushroom_prob(*j);

                processed_count++;
                int target_count = 1 + binomial_generator(20, trial_prob);

                int seen_per;
                spawn_corpse_mushrooms(*j, target_count, seen_per,
                                       BEH_GOOD_NEUTRAL, true);

                seen_mushrooms += seen_per;

                // Either turn this corpse into a skeleton or destroy it.
                if (mons_skeleton(j->plus))
                    turn_corpse_into_skeleton(*j);
                else
                    destroy_item(j->index());
            }

            if (corpse_on_pos && you.see_cell(*i))
                seen_corpses++;
        }
    }

    if (seen_mushrooms > 0)
        mushroom_spawn_message(seen_mushrooms, seen_corpses);

    if (kills)
        mprf("That felt like a moral victory.");

    if (processed_count)
    {
        simple_god_message(" appreciates your contribution to the "
                           "ecosystem.", GOD_FEDHAS);
        // Doubling the expected value per sacrifice to approximate the
        // extra piety gain blood god worshipers get for the initial kill.
        // -cao

        int piety_gain = 0;
        for (int i = 0; i < processed_count * 2; i++)
            piety_gain += random2(15); // avg 1.4 piety per corpse
        gain_piety(piety_gain, 10);
    }

    return (processed_count);
}

static int _create_plant(coord_def & target, int hp_adjust = 0)
{
    if (actor_at(target) || !mons_class_can_pass(MONS_PLANT, grd(target)))
        return (0);

    const int plant = create_monster(mgen_data
                                     (MONS_PLANT,
                                      BEH_FRIENDLY,
                                      &you,
                                      0,
                                      0,
                                      target,
                                      MHITNOT,
                                      MG_FORCE_PLACE,
                                      GOD_FEDHAS));


    if (plant != -1)
    {
        env.mons[plant].flags |= MF_ATT_CHANGE_ATTEMPT;
        env.mons[plant].max_hit_points += hp_adjust;
        env.mons[plant].hit_points += hp_adjust;

        if (you.see_cell(target))
        {
            if (hp_adjust)
            {
                mprf("A plant, strengthened by %s, grows up from the ground.",
                     god_name(GOD_FEDHAS).c_str());
            }
            else
                mpr("A plant grows up from the ground.");
        }
    }

    return (plant != -1);
}

bool fedhas_sunlight()
{
    const int c_size = 5;
    const int x_offset[] = {-1, 0, 0, 0, 1};
    const int y_offset[] = { 0,-1, 0, 1, 0};

    dist spelld;

    bolt temp_bolt;
    temp_bolt.colour = YELLOW;

    direction_chooser_args args;
    args.restricts = DIR_TARGET;
    args.mode = TARG_HOSTILE_SUBMERGED;
    args.range = LOS_RADIUS;
    args.needs_path = false;
    args.may_target_monster = false;
    args.top_prompt = "Select sunlight destination.";
    direction(spelld, args);

    if (!spelld.isValid)
        return (false);

    const coord_def base = spelld.target;

    int evap_count  = 0;
    int plant_count = 0;
    int processed_count = 0;

    // This is dealt with outside of the main loop.
    int cloud_count = 0;

    // FIXME: Uncomfortable level of code duplication here but the explosion
    // code in bolt subjects the input radius to r*(r+1) for the threshold and
    // since r is an integer we can never get just the 4-connected neighbours.
    // Anyway the bolt code doesn't seem to be well set up to handle the
    // 'occasional plant' gimmick.
    for (int i = 0; i < c_size; ++i)
    {
        coord_def target = base;
        target.x += x_offset[i];
        target.y += y_offset[i];

        if (!in_bounds(target) || feat_is_solid(grd(target)))
            continue;

        temp_bolt.explosion_draw_cell(target);

        actor *victim = actor_at(target);

        // If this is a water square we will evaporate it.
        dungeon_feature_type ftype = grd(target);
        dungeon_feature_type orig_type = ftype;

        switch (ftype)
        {
        case DNGN_SHALLOW_WATER:
            ftype = DNGN_FLOOR;
            break;

        case DNGN_DEEP_WATER:
            ftype = DNGN_SHALLOW_WATER;
            break;

        default:
            break;
        }

        if (orig_type != ftype)
        {
            dungeon_terrain_changed(target, ftype);

            if (you.see_cell(target))
                evap_count++;

            // This is a little awkward but if we evaporated all the way to
            // the dungeon floor that may have given a monster
            // ENCH_AQUATIC_LAND, and if that happened the player should get
            // credit if the monster dies. The enchantment is inflicted via
            // the dungeon_terrain_changed call chain and that doesn't keep
            // track of what caused the terrain change. -cao
            monster* mons = monster_at(target);
            if (mons && ftype == DNGN_FLOOR
                && mons->has_ench(ENCH_AQUATIC_LAND))
            {
                mon_enchant temp = mons->get_ench(ENCH_AQUATIC_LAND);
                temp.who = KC_YOU;
                temp.source = MID_PLAYER;
                mons->add_ench(temp);
            }

            processed_count++;
        }

        monster* mons = monster_at(target);

        if (victim)
        {
            if (!mons)
                you.backlight();
            else
            {
                backlight_monsters(target, 1, 0);
                behaviour_event(mons, ME_ALERT, MHITYOU);
            }

            processed_count++;
        }
        else if (one_chance_in(100)
                 && ftype >= DNGN_FLOOR_MIN
                 && ftype <= DNGN_FLOOR_MAX
                 && orig_type == DNGN_SHALLOW_WATER)
        {
            // Create a plant.
            const int plant = create_monster(mgen_data(MONS_PLANT,
                                                       BEH_HOSTILE,
                                                       &you,
                                                       0,
                                                       0,
                                                       target,
                                                       MHITNOT,
                                                       MG_FORCE_PLACE,
                                                       GOD_FEDHAS));

            if (plant != -1 && you.see_cell(target))
                plant_count++;

            processed_count++;
        }
    }

    // We damage clouds for a large radius, though.
    for (radius_iterator ai(base, 7); ai; ++ai)
    {
        if (env.cgrid(*ai) != EMPTY_CLOUD)
        {
            const int cloudidx = env.cgrid(*ai);
            if (env.cloud[cloudidx].type == CLOUD_GLOOM)
            {
                cloud_count++;
                delete_cloud(cloudidx);
            }
        }
    }

#ifndef USE_TILE
    // Move the cursor out of the way (it looks weird).
    coord_def temp = grid2view(base);
    cgotoxy(temp.x, temp.y, GOTO_DNGN);
#endif
    delay(200);

    if (plant_count)
    {
        mprf("%s grow%s in the sunlight.",
             (plant_count > 1 ? "Some plants": "A plant"),
             (plant_count > 1 ? "": "s"));
    }

    if (evap_count)
        mprf("Some water evaporates in the bright sunlight.");

    if (cloud_count)
        mprf("Sunlight penetrates the thick gloom.");

    return (true);
}

template<typename T>
bool less_second(const T & left, const T & right)
{
    return (left.second < right.second);
}

typedef std::pair<coord_def, int> point_distance;

// Find the distance from origin to each of the targets, those results
// are stored in distances (which is the same size as targets). Exclusion
// is a set of points which are considered disconnected for the search.
static void _path_distance(const coord_def & origin,
                           const std::vector<coord_def> & targets,
                           std::set<int> exclusion,
                           std::vector<int> & distances)
{
    std::queue<point_distance> fringe;
    fringe.push(point_distance(origin,0));
    distances.clear();
    distances.resize(targets.size(), INT_MAX);

    while (!fringe.empty())
    {
        point_distance current = fringe.front();
        fringe.pop();

        // did we hit a target?
        for (unsigned i = 0; i < targets.size(); ++i)
        {
            if (current.first == targets[i])
            {
                distances[i] = current.second;
                break;
            }
        }

        for (adjacent_iterator adj_it(current.first); adj_it; ++adj_it)
        {
            int idx = adj_it->x + adj_it->y * X_WIDTH;
            if (you.see_cell(*adj_it)
                && !feat_is_solid(env.grid(*adj_it))
                && *adj_it != you.pos()
                && exclusion.insert(idx).second)
            {
                monster* temp = monster_at(*adj_it);
                if (!temp || (temp->attitude == ATT_HOSTILE
                              && !mons_is_stationary(temp)))
                {
                    fringe.push(point_distance(*adj_it, current.second+1));
                }
            }
        }
    }
}


// Find the minimum distance from each point of origin to one of the targets
// The distance is stored in 'distances', which is the same size as origins.
static void _point_point_distance(const std::vector<coord_def> & origins,
                                  const std::vector<coord_def> & targets,
                                  std::vector<int> & distances)
{
    distances.clear();
    distances.resize(origins.size(), INT_MAX);

    // Consider all points of origin as blocked (you can search outward
    // from one, but you can't form a path across a different one).
    std::set<int> base_exclusions;
    for (unsigned i = 0; i < origins.size(); ++i)
    {
        int idx = origins[i].x + origins[i].y * X_WIDTH;
        base_exclusions.insert(idx);
    }

    std::vector<int> current_distances;
    for (unsigned i = 0; i < origins.size(); ++i)
    {
        // Find the distance from the point of origin to each of the targets.
        _path_distance(origins[i], targets, base_exclusions,
                       current_distances);

        // Find the smallest of those distances
        int min_dist = current_distances[0];
        for (unsigned j = 1; j < current_distances.size(); ++j)
            if (current_distances[j] < min_dist)
                min_dist = current_distances[j];

        distances[i] = min_dist;
    }
}

// So the idea is we want to decide which adjacent tiles are in the most 'danger'
// We claim danger is proportional to the minimum distances from the point to a
// (hostile) monster. This function carries out at most 7 searches to calculate
// the distances in question.
bool prioritise_adjacent(const coord_def &target, std::vector<coord_def> & candidates)
{
    radius_iterator los_it(target, LOS_RADIUS, true, true, true);

    std::vector<coord_def> mons_positions;
    // collect hostile monster positions in LOS
    for (; los_it; ++los_it)
    {
        monster* hostile = monster_at(*los_it);

        if (hostile && hostile->attitude == ATT_HOSTILE
            && you.can_see(hostile))
        {
            mons_positions.push_back(hostile->pos());
        }
    }

    if (mons_positions.empty())
    {
        std::random_shuffle(candidates.begin(), candidates.end());
        return (true);
    }

    std::vector<int> distances;

    _point_point_distance(candidates, mons_positions, distances);

    std::vector<point_distance> possible_moves(candidates.size());

    for (unsigned i = 0; i < possible_moves.size(); ++i)
    {
        possible_moves[i].first  = candidates[i];
        possible_moves[i].second = distances[i];
    }

    std::sort(possible_moves.begin(), possible_moves.end(),
              less_second<point_distance>);

    for (unsigned i = 0; i < candidates.size(); ++i)
        candidates[i] = possible_moves[i].first;

    return (true);
}

static bool _prompt_amount(int max, int& selected, const std::string& prompt)
{
    selected = max;
    while (true)
    {
        msg::streams(MSGCH_PROMPT) << prompt << " (" << max << " max) "
                                   << std::endl;

        const unsigned char keyin = get_ch();

        // Cancel
        if (key_is_escape(keyin) || keyin == ' ' || keyin == '0')
        {
            canned_msg(MSG_OK);
            return (false);
        }

        // Default is max
        if (keyin == '\n'  || keyin == '\r')
            return (true);

        // Otherwise they should enter a digit
        if (isadigit(keyin))
        {
            selected = keyin - '0';
            if (selected > 0 && selected <= max)
                return (true);
        }
        // else they entered some garbage?
    }

    return (max);
}

static int _collect_fruit(std::vector<std::pair<int,int> >& available_fruit)
{
    int total = 0;

    for (int i = 0; i < ENDOFPACK; i++)
    {
        if (you.inv[i].defined() && is_fruit(you.inv[i]))
        {
            total += you.inv[i].quantity;
            available_fruit.push_back(std::make_pair(you.inv[i].quantity, i));
        }
    }
    std::sort(available_fruit.begin(), available_fruit.end());

    return (total);
}

static void _decrease_amount(std::vector<std::pair<int, int> >& available,
                             int amount)
{
    const int total_decrease = amount;
    for (unsigned int i = 0; i < available.size() && amount > 0; i++)
    {
        const int decrease_amount = std::min(available[i].first, amount);
        amount -= decrease_amount;
        dec_inv_item_quantity(available[i].second, decrease_amount);
    }
    if (total_decrease > 1)
        mprf("%d pieces of fruit are consumed!", total_decrease);
    else
        mpr("A piece of fruit is consumed!");
}

// Create a ring or partial ring around the caster.  The user is
// prompted to select a stack of fruit, and then plants are placed on open
// squares adjacent to the user.  Of course, one piece of fruit is
// consumed per plant, so a complete ring may not be formed.
bool fedhas_plant_ring_from_fruit()
{
    // How much fruit is available?
    std::vector<std::pair<int, int> > collected_fruit;
    int total_fruit = _collect_fruit(collected_fruit);

    // How many adjacent open spaces are there?
    std::vector<coord_def> adjacent;
    for (adjacent_iterator adj_it(you.pos()); adj_it; ++adj_it)
    {
        if (mons_class_can_pass(MONS_PLANT, env.grid(*adj_it))
            && !actor_at(*adj_it))
        {
            adjacent.push_back(*adj_it);
        }
    }

    const int max_use = std::min(total_fruit,
                                 static_cast<int>(adjacent.size()));

    // Don't prompt if we can't do anything (due to having no fruit or
    // no squares to place plants on).
    if (max_use == 0)
    {
        if (adjacent.size() == 0)
            mprf("No empty adjacent squares.");
        else
            mprf("No fruit available.");

        return (false);
    }

    prioritise_adjacent(you.pos(), adjacent);

    // Screwing around with display code I don't really understand. -cao
    crawl_state.darken_range = 1;
    viewwindow(false);

    for (int i = 0; i < max_use; ++i)
    {
#ifndef USE_TILE
        coord_def temp = grid2view(adjacent[i]);
        cgotoxy(temp.x, temp.y, GOTO_DNGN);
        put_colour_ch(GREEN, '1' + i);
#else
        tiles.add_overlay(adjacent[i], TILE_INDICATOR + i);
#endif
    }

    // And how many plants does the user want to create?
    int target_count;
    if (!_prompt_amount(max_use, target_count,
                        "How many plants will you create?"))
    {
        // User canceled at the prompt.
        crawl_state.darken_range = -1;
        viewwindow(false);
        return (false);
    }

    const int hp_adjust = you.skill(SK_INVOCATIONS) * 10;

    // The user entered a number, remove all number overlays which
    // are higher than that number.
#ifndef USE_TILE
    unsigned not_used = adjacent.size() - unsigned(target_count);
    for (unsigned i = adjacent.size() - not_used;
         i < adjacent.size();
         i++)
    {
        view_update_at(adjacent[i]);
    }
#else
    // For tiles we have to clear all overlays and redraw the ones
    // we want.
    tiles.clear_overlays();
    for (int i = 0; i < target_count; ++i)
        tiles.add_overlay(adjacent[i], TILE_INDICATOR + i);
#endif

    int created_count = 0;
    for (int i = 0; i < target_count; ++i)
    {
        if (_create_plant(adjacent[i], hp_adjust))
            created_count++;

        // Clear the overlay and draw the plant we just placed.
        // This is somewhat more complicated in tiles.
        view_update_at(adjacent[i]);
#ifdef USE_TILE
        tiles.clear_overlays();
        for (int j = i + 1; j < target_count; ++j)
            tiles.add_overlay(adjacent[j], TILE_INDICATOR + j);
        viewwindow(false);
#endif
        delay(200);
    }

    _decrease_amount(collected_fruit, created_count);

    crawl_state.darken_range = -1;
    viewwindow(false);

    return (created_count);
}

// Create a circle of water around the target, with a radius of
// approximately 2.  This turns normal floor tiles into shallow water
// and turns (unoccupied) shallow water into deep water.  There is a
// chance of spawning plants or fungus on unoccupied dry floor tiles
// outside of the rainfall area.  Return the number of plants/fungi
// created.
int fedhas_rain(const coord_def &target)
{
    int spawned_count = 0;
    int processed_count = 0;

    for (radius_iterator rad(target, LOS_RADIUS, true, true, true); rad; ++rad)
    {
        // Adjust the shape of the rainfall slightly to make it look
        // nicer.  I want a threshold of 2.5 on the euclidean distance,
        // so a threshold of 6 prior to the sqrt is close enough.
        int rain_thresh = 6;
        coord_def local = *rad - target;

        dungeon_feature_type ftype = grd(*rad);

        if (local.abs() > rain_thresh)
        {
            // Maybe spawn a plant on (dry, open) squares that are in
            // LOS but outside the rainfall area.  In open space, there
            // are 213 squares in LOS, and we are going to drop water on
            // (25-4) of those, so if we want x plants to spawn on
            // average in open space, the trial probability should be
            // x/192.
            if (x_chance_in_y(5, 192)
                && !actor_at(*rad)
                && ftype >= DNGN_FLOOR_MIN
                && ftype <= DNGN_FLOOR_MAX)
            {
                const int plant = create_monster(mgen_data
                                     (coinflip() ? MONS_PLANT : MONS_FUNGUS,
                                      BEH_GOOD_NEUTRAL,
                                      &you,
                                      0,
                                      0,
                                      *rad,
                                      MHITNOT,
                                      MG_FORCE_PLACE,
                                      GOD_FEDHAS));

                if (plant != -1)
                    spawned_count++;

                processed_count++;
            }

            continue;
        }

        // Turn regular floor squares only into shallow water.
        if (ftype >= DNGN_FLOOR_MIN && ftype <= DNGN_FLOOR_MAX)
        {
            dungeon_terrain_changed(*rad, DNGN_SHALLOW_WATER);

            processed_count++;
        }
        // We can also turn shallow water into deep water, but we're
        // just going to skip cases where there is something on the
        // shallow water.  Destroying items will probably be annoying,
        // and insta-killing monsters is clearly out of the question.
        else if (!actor_at(*rad)
                 && igrd(*rad) == NON_ITEM
                 && ftype == DNGN_SHALLOW_WATER)
        {
            dungeon_terrain_changed(*rad, DNGN_DEEP_WATER);

            processed_count++;
        }

        if (ftype >= DNGN_MINMOVE)
        {
            // Maybe place a raincloud.
            //
            // The rainfall area is 20 (5*5 - 4 (corners) - 1 (center));
            // the expected number of clouds generated by a fixed chance
            // per tile is 20 * p = expected.  Say an Invocations skill
            // of 27 gives expected 5 clouds.
            int max_expected = 5;
            int expected = div_rand_round(max_expected
                                          * you.skill(SK_INVOCATIONS), 27);

            if (x_chance_in_y(expected, 20))
            {
                place_cloud(CLOUD_RAIN, *rad, 10, &you);

                processed_count++;
            }
        }
    }

    if (spawned_count > 0)
    {
        mprf("%s grow%s in the rain.",
             (spawned_count > 1 ? "Some plants" : "A plant"),
             (spawned_count > 1 ? "" : "s"));
    }

    return (processed_count);
}

// Destroy corpses in the player's LOS (first corpse on a stack only)
// and make 1 giant spore per corpse.  Spores are given the input as
// their starting behavior; the function returns the number of corpses
// processed.
int fedhas_corpse_spores(beh_type behavior, bool interactive)
{
    int count = 0;
    std::vector<stack_iterator> positions;

    for (radius_iterator rad(you.pos(), LOS_RADIUS, true, true, true); rad;
         ++rad)
    {
        if (actor_at(*rad))
            continue;

        for (stack_iterator stack_it(*rad); stack_it; ++stack_it)
        {
            if (stack_it->base_type == OBJ_CORPSES
                && stack_it->sub_type == CORPSE_BODY)
            {
                positions.push_back(stack_it);
                count++;
                break;
            }
        }
    }

    if (count == 0)
        return (count);

    crawl_state.darken_range = 0;
    viewwindow(false);
    for (unsigned i = 0; i < positions.size(); ++i)
    {
#ifndef USE_TILE

        coord_def temp = grid2view(positions[i]->pos);
        cgotoxy(temp.x, temp.y, GOTO_DNGN);

        unsigned color = GREEN | COLFLAG_FRIENDLY_MONSTER;
        color = real_colour(color);

        unsigned character = mons_char(MONS_GIANT_SPORE);
        put_colour_ch(color, character);
#else
        tiles.add_overlay(positions[i]->pos, TILE_SPORE_OVERLAY);
#endif
    }

    if (interactive && yesnoquit("Will you create these spores?",
                                 true, 'y') <= 0)
    {
        crawl_state.darken_range = -1;
        viewwindow(false);
        return (-1);
    }

    for (unsigned i = 0; i < positions.size(); ++i)
    {
        count++;
        int rc = create_monster(mgen_data(MONS_GIANT_SPORE,
                                          behavior,
                                          &you,
                                          0,
                                          0,
                                          positions[i]->pos,
                                          MHITNOT,
                                          MG_FORCE_PLACE,
                                          GOD_FEDHAS));

        if (rc != -1)
        {
            env.mons[rc].flags |= MF_ATT_CHANGE_ATTEMPT;
            if (behavior == BEH_FRIENDLY)
            {
                env.mons[rc].behaviour = BEH_WANDER;
                env.mons[rc].foe = MHITNOT;
            }
        }

        if (mons_skeleton(positions[i]->plus))
            turn_corpse_into_skeleton(*positions[i]);
        else
            destroy_item(positions[i]->index());
    }

    crawl_state.darken_range = -1;
    viewwindow(false);

    return (count);
}

struct monster_conversion
{
    monster_conversion() :
        base_monster(NULL),
        piety_cost(0),
        fruit_cost(0)
    {
    }

    monster* base_monster;
    int piety_cost;
    int fruit_cost;
    monster_type new_type;
};


// Given a monster (which should be a plant/fungus), see if
// fedhas_evolve_flora() can upgrade it, and set up a monster_conversion
// structure for it.  Return true (and fill in possible_monster) if the
// monster can be upgraded, and return false otherwise.
static bool _possible_evolution(const monster* input,
                                monster_conversion & possible_monster)
{
    switch (input->type)
    {
    case MONS_PLANT:
    case MONS_BUSH:
        possible_monster.new_type = MONS_OKLOB_PLANT;
        possible_monster.fruit_cost = 1;
        break;

    case MONS_FUNGUS:
        possible_monster.new_type = MONS_WANDERING_MUSHROOM;
        possible_monster.piety_cost = 1;
        break;

    case MONS_TOADSTOOL:
        possible_monster.new_type = MONS_WANDERING_MUSHROOM;
        possible_monster.piety_cost = 2;
        break;

    case MONS_BALLISTOMYCETE:
        possible_monster.new_type = MONS_HYPERACTIVE_BALLISTOMYCETE;
        possible_monster.piety_cost = 4;
        break;

    default:
        return (false);
    }

    return (true);
}

bool mons_is_evolvable(const monster* mon)
{
    monster_conversion temp;
    return (_possible_evolution(mon, temp));
}

static bool _place_ballisto(const coord_def & pos)
{
    const int ballisto = create_monster(mgen_data(MONS_BALLISTOMYCETE,
                                                  BEH_FRIENDLY,
                                                  &you,
                                                  0,
                                                  0,
                                                  pos,
                                                  MHITNOT,
                                                  MG_FORCE_PLACE,
                                                  GOD_FEDHAS));

    if (ballisto != -1)
    {
        remove_mold(pos);
        mprf("The mold grows into a ballistomycete.");
        mprf("Your piety has decreased.");
        lose_piety(1);
        return (true);
    }

    // Monster placement failing should be quite unusual, but it could happen.
    // Not entirely sure what to say about it, but a more informative message
    // might be good. -cao
    canned_msg(MSG_NOTHING_HAPPENS);
    return (false);
}

bool fedhas_evolve_flora()
{
    monster_conversion upgrade;

    // This is a little sloppy, but cancel early if nothing useful is in
    // range.
    bool in_range = false;
    for (radius_iterator rad(you.get_los()); rad; ++rad)
    {
        const monster* temp = monster_at(*rad);
        if (is_moldy(*rad) && mons_class_can_pass(MONS_BALLISTOMYCETE,
                                                  env.grid(*rad))
            || temp && mons_is_evolvable(temp))
        {
            in_range = true;
            break;
        }
    }

    if (!in_range)
    {
        mprf("No evolvable flora in sight.");
        return (false);
    }

    dist spelld;

    direction_chooser_args args;
    args.restricts = DIR_TARGET;
    args.mode = TARG_EVOLVABLE_PLANTS;
    args.range = LOS_RADIUS;
    args.needs_path = false;
    args.may_target_monster = false;
    args.show_floor_desc = true;
    args.top_prompt = "Select plant or fungus to evolve.";

    direction(spelld, args);

    if (!spelld.isValid)
    {
        // Check for user cancel.
        canned_msg(MSG_OK);
        return (false);
    }

    monster* const target = monster_at(spelld.target);

    if (!target)
    {
        if (!is_moldy(spelld.target)
            || !mons_class_can_pass(MONS_BALLISTOMYCETE,
                                    env.grid(spelld.target)))
        {
            if (feat_is_tree(env.grid(spelld.target)))
                mprf("The tree has already reached the pinnacle of evolution.");
            else
                mprf("You must target a plant or fungus.");
            return (false);
        }
        return (_place_ballisto(spelld.target));

    }

    if (!_possible_evolution(target, upgrade))
    {
        if (mons_is_plant(target))
            simple_monster_message(target, " has already reached "
                                   "the pinnacle of evolution.");
        else
            mprf("Only plants or fungi may be upgraded.");

        return (false);
    }

    std::vector<std::pair<int, int> > collected_fruit;
    if (upgrade.fruit_cost)
    {
        const int total_fruit = _collect_fruit(collected_fruit);

        if (total_fruit < upgrade.fruit_cost)
        {
            mprf("Not enough fruit available.");
            return (false);
        }
    }

    if (upgrade.piety_cost && upgrade.piety_cost > you.piety)
    {
        mprf("Not enough piety.");
        return (false);
    }

    switch (target->type)
    {
    case MONS_PLANT:
    case MONS_BUSH:
    {
        std::string evolve_desc = " can now spit acid";
        if (you.skill(SK_INVOCATIONS) >= 20)
            evolve_desc += " continuously";
        else if (you.skill(SK_INVOCATIONS) >= 15)
            evolve_desc += " quickly";
        else if (you.skill(SK_INVOCATIONS) >= 10)
            evolve_desc += " rather quickly";
        else if (you.skill(SK_INVOCATIONS) >= 5)
            evolve_desc += " somewhat quickly";
        evolve_desc += ".";

        simple_monster_message(target, evolve_desc.c_str());
        break;
    }

    case MONS_FUNGUS:
    case MONS_TOADSTOOL:
        simple_monster_message(target,
                               " can now pick up its mycelia and move.");
        break;

    case MONS_BALLISTOMYCETE:
        simple_monster_message(target, " appears agitated.");
        break;

    default:
        break;
    }

    target->upgrade_type(upgrade.new_type, true, true);
    target->god = GOD_FEDHAS;
    target->attitude = ATT_FRIENDLY;
    target->flags |= MF_NO_REWARD;
    target->flags |= MF_ATT_CHANGE_ATTEMPT;
    behaviour_event(target, ME_ALERT);
    mons_att_changed(target);

    // Try to remove slowly dying in case we are upgrading a
    // toadstool, and spore production in case we are upgrading a
    // ballistomycete.
    target->del_ench(ENCH_SLOWLY_DYING);
    target->del_ench(ENCH_SPORE_PRODUCTION);

    if (target->type == MONS_HYPERACTIVE_BALLISTOMYCETE)
        target->add_ench(ENCH_EXPLODING);

    target->hit_dice += you.skill(SK_INVOCATIONS);

    if (upgrade.fruit_cost)
        _decrease_amount(collected_fruit, upgrade.fruit_cost);

    if (upgrade.piety_cost)
    {
        lose_piety(upgrade.piety_cost);
        mprf("Your piety has decreased.");
    }

    return (true);
}

static int _lugonu_warp_monster(monster* mon, int pow)
{
    if (mon == NULL)
        return (0);

    if (!mon->friendly())
        behaviour_event(mon, ME_ANNOY, MHITYOU);

    int res_margin = mon->check_res_magic(pow * 2);
    if (res_margin > 0)
    {
        mprf("%s%s",
             mon->name(DESC_CAP_THE).c_str(),
             mons_resist_string(mon, res_margin).c_str());
        return (1);
    }

    const int damage = 1 + random2(pow / 6);
    if (mons_genus(mon->type) == MONS_BLINK_FROG)
        mon->heal(damage, false);
    else if (mon->check_res_magic(pow) <= 0)
    {
        mon->hurt(&you, damage);
        if (!mon->alive())
            return (1);
    }

    mon->blink();

    return (1);
}

static void _lugonu_warp_area(int pow)
{
    apply_monsters_around_square(_lugonu_warp_monster, you.pos(), pow);
}

void lugonu_bend_space()
{
    const int pow = 4 + skill_bump(SK_INVOCATIONS);
    const bool area_warp = random2(pow) > 9;

    mprf("Space bends %saround you!", area_warp ? "sharply " : "");

    if (area_warp)
        _lugonu_warp_area(pow);

    random_blink(false, true);

    const int damage = roll_dice(1, 4);
    ouch(damage, NON_MONSTER, KILLED_BY_WILD_MAGIC, "a spatial distortion");
}

bool is_ponderousifiable(const item_def& item)
{
    return (item.base_type == OBJ_ARMOUR
            && you_tran_can_wear(item)
            && !is_shield(item)
            && !is_artefact(item)
            && get_armour_ego_type(item) != SPARM_RUNNING
            && get_armour_ego_type(item) != SPARM_PONDEROUSNESS);
}

bool ponderousify_armour()
{
    const int item_slot = prompt_invent_item("Make which item ponderous?",
                                             MT_INVLIST, OSEL_PONDER_ARM,
                                             true, true, false);

    if (prompt_failed(item_slot))
        return (false);

    item_def& arm(you.inv[item_slot]);
    if (!is_ponderousifiable(arm)) // player pressed '*' and made a bad choice
    {
        mpr("That item can't be made ponderous.");
        return (false);
    }

    const int old_ponder = player_ponderousness();
    cheibriados_make_item_ponderous(arm);

    you.redraw_armour_class = true;
    you.redraw_evasion = true;

    simple_god_message(" says: Use this wisely!");

    const int new_ponder = player_ponderousness();
    if (new_ponder > old_ponder)
    {
        mprf("You feel %s ponderous.",
             old_ponder? "even more" : "rather");
        che_handle_change(CB_PONDEROUS, new_ponder - old_ponder);
    }

    return (true);
}

void cheibriados_time_bend(int pow)
{
    mpr("The flow of time bends around you.");

    for (adjacent_iterator ai(you.pos()); ai; ++ai)
    {
        monster* mon = monster_at(*ai);
        if (mon && !mons_is_stationary(mon))
        {
            int res_margin = roll_dice(mon->hit_dice, 3) - random2avg(pow, 2);
            if (res_margin > 0)
            {
                mprf("%s%s",
                     mon->name(DESC_CAP_THE).c_str(),
                     mons_resist_string(mon, res_margin).c_str());
                continue;
            }

            simple_god_message(
                make_stringf(" rebukes %s.",
                             mon->name(DESC_NOCAP_THE).c_str()).c_str(),
                             GOD_CHEIBRIADOS);
            do_slow_monster(mon, &you);
        }
    }
}

static int _slouch_monsters(coord_def where, int pow, int, actor* agent)
{
    monster* mon = monster_at(where);
    if (mon == NULL || mons_is_stationary(mon) || mon->cannot_move()
        || mons_is_projectile(mon->type)
        || mon->asleep() && !mons_is_confused(mon))
    {
        return (0);
    }

    int dmg = (mon->speed - 1000/player_movement_speed()/player_speed());
    dmg = (dmg > 0 ? roll_dice(dmg*4, 3)/2 : 0);

    mon->hurt(agent, dmg, BEAM_MMISSILE, true);
    return (1);
}

int cheibriados_slouch(int pow)
{
    return (apply_area_visible(_slouch_monsters, pow, true, &you));
}

void cheibriados_time_step(int pow) // pow is the number of turns to skip
{
    const coord_def old_pos = you.pos();

    mpr("You step out of the flow of time.");
    flash_view(LIGHTBLUE);
    you.moveto(coord_def(0, 0));
    you.duration[DUR_TIME_STEP] = pow;

    you.time_taken = 10;
    do
    {
        run_environment_effects();
        handle_monsters();
        manage_clouds();
    }
    while (--you.duration[DUR_TIME_STEP] > 0);
    // Update corpses, etc.  This does also shift monsters, but only by
    // a tiny bit.
    update_level(pow * 10);

#ifndef USE_TILE
    delay(1000);
#endif

    monster* mon;
    if (mon = monster_at(old_pos))
    {
        mon->blink();
        if (mon = monster_at(old_pos))
            mon->teleport(true);
    }

    you.moveto(old_pos);
    you.duration[DUR_TIME_STEP] = 0;

    flash_view(0);
    mpr("You return to the normal time flow.");
}

bool ashenzari_transfer_knowledge()
{
    if (you.transfer_skill_points > 0)
        if (!ashenzari_end_transfer())
            return false;

    while(true)
    {
        skill_menu(true);
        if (is_invalid_skill(you.transfer_from_skill))
        {
            redraw_screen();
            return false;
        }

        you.transfer_skill_points = skill_transfer_amount(
                                                    you.transfer_from_skill);

        skill_menu(true);
        if (is_invalid_skill(you.transfer_to_skill))
        {
            you.transfer_from_skill = SK_NONE;
            you.transfer_skill_points = 0;
            continue;
        }

        break;
    }

    mprf("As you forget about %s, you feel ready to understand %s.",
         skill_name(you.transfer_from_skill),
         skill_name(you.transfer_to_skill));

    you.transfer_total_skill_points = you.transfer_skill_points;

    redraw_screen();
    return true;
}

bool ashenzari_end_transfer(bool finished, bool force)
{
    if (!force && !finished)
    {
        mprf("You are currently transferring knowledge from %s to %s.",
             skill_name(you.transfer_from_skill),
             skill_name(you.transfer_to_skill));
        if (!yesno("Are you sure you want to cancel the transfer?", false, 'n'))
            return false;
    }

    mprf("You %s forgetting about %s and learning about %s.",
         finished ? "have finished" : "stop",
         skill_name(you.transfer_from_skill),
         skill_name(you.transfer_to_skill));
    you.transfer_from_skill = SK_NONE;
    you.transfer_to_skill = SK_NONE;
    you.transfer_skill_points = 0;
    you.transfer_total_skill_points = 0;
    return true;
}
