#include "AppHdr.h"

#include "godconduct.h"

#include "fight.h"
#include "godpassive.h"
#include "godwrath.h"
#include "monster.h"
#include "mon-util.h"
#include "player.h"
#include "random.h"
#include "religion.h"
#include "state.h"
#include "stuff.h"

/////////////////////////////////////////////////////////////////////
// god_conduct_trigger

god_conduct_trigger::god_conduct_trigger(
    conduct_type c, int pg, bool kn, const monster* vict)
  : conduct(c), pgain(pg), known(kn), enabled(true), victim(NULL)
{
    if (vict)
    {
        victim.reset(new monster);
        *(victim.get()) = *vict;
    }
}

void god_conduct_trigger::set(conduct_type c, int pg, bool kn,
                              const monster* vict)
{
    conduct = c;
    pgain = pg;
    known = kn;
    victim.reset(NULL);
    if (vict)
    {
        victim.reset(new monster);
        *victim.get() = *vict;
    }
}

god_conduct_trigger::~god_conduct_trigger()
{
    if (enabled && conduct != NUM_CONDUCTS)
        did_god_conduct(conduct, pgain, known, victim.get());
}

// This function is the merger of done_good() and naughty().
// Returns true if god was interested (good or bad) in conduct.
bool did_god_conduct(conduct_type thing_done, int level, bool known,
                     const monster* victim)
{
    ASSERT(!crawl_state.game_is_arena());

    bool retval = false;

    if (you.religion != GOD_NO_GOD && you.religion != GOD_XOM)
    {
        int piety_change = 0;
        int piety_denom = 1;
        int penance = 0;

        god_acting gdact;

        switch (thing_done)
        {
        case DID_DRINK_BLOOD:
            switch (you.religion)
            {
            case GOD_ZIN:
            case GOD_SHINING_ONE:
            case GOD_ELYVILON:
                if (!known)
                {
                    simple_god_message(" forgives your inadvertent "
                                       "blood-drinking, just this once.");
                    break;
                }
                if (you.religion == GOD_SHINING_ONE)
                    penance = level;
                piety_change = -2*level;
                retval = true;
                break;
            default:
                break;
            }
            break;

        case DID_CANNIBALISM:
            switch (you.religion)
            {
            case GOD_ZIN:
            case GOD_SHINING_ONE:
            case GOD_ELYVILON:
            case GOD_BEOGH:
                piety_change = -level;
                penance = level;
                retval = true;
                break;
            default:
                break;
            }
            break;

        case DID_CORPSE_VIOLATION:
            if (you.religion == GOD_FEDHAS)
            {
                if (known)
                {
                    piety_change = -level;
                    penance = level;
                    retval = true;
                }
                else
                {
                    simple_god_message(" forgives your inadvertent necromancy, "
                                       "just this once.");
                }
            }
            break;

        case DID_NECROMANCY:
        case DID_UNHOLY:
        case DID_ATTACK_HOLY:
        case DID_VIOLATE_HOLY_CORPSE:
            switch (you.religion)
            {
            case GOD_ZIN:
            case GOD_SHINING_ONE:
            case GOD_ELYVILON:
                if (!known && thing_done != DID_ATTACK_HOLY && thing_done != DID_VIOLATE_HOLY_CORPSE)
                {
                    simple_god_message(" forgives your inadvertent unholy act, "
                                       "just this once.");
                    break;
                }

                if (thing_done == DID_ATTACK_HOLY && victim
                    && !testbits(victim->flags, MF_NO_REWARD)
                    && !testbits(victim->flags, MF_WAS_NEUTRAL))
                {
                    break;
                }

                piety_change = -level;
                penance = level * ((you.religion == GOD_SHINING_ONE) ? 2
                                                                     : 1);
                retval = true;
                break;
            default:
                break;
            }
            break;

        case DID_HOLY:
            if (you.religion == GOD_YREDELEMNUL)
            {
                if (!known)
                {
                    simple_god_message(" forgives your inadvertent holy act, "
                                       "just this once.");
                    break;
                }
                retval = true;
                piety_change = -level;
                penance = level * 2;
            }
            break;

        case DID_UNCHIVALRIC_ATTACK:
        case DID_POISON:
            if (you.religion == GOD_SHINING_ONE)
            {
                if (thing_done == DID_UNCHIVALRIC_ATTACK)
                {
                    if (victim && tso_unchivalric_attack_safe_monster(victim))
                        break;

                    if (!known)
                    {
                        simple_god_message(" forgives your inadvertent "
                                           "dishonourable attack, just this "
                                           "once.");
                        break;
                    }
                }
                retval = true;
                piety_change = -level;
                penance = level * 2;
            }
            break;

       case DID_KILL_SLIME:
            if (you.religion == GOD_JIYVA && !victim->is_shapeshifter())
            {
                retval = true;
                piety_change = -level;
                penance = level * 2;
            }
            break;

       case DID_KILL_PLANT:
       case DID_PLANT_KILLED_BY_SERVANT:
           // Piety loss but no penance for killing a plant.
           if (you.religion == GOD_FEDHAS)
           {
               retval = true;
               piety_change = -level;
           }
           break;

        case DID_ATTACK_NEUTRAL:
            switch (you.religion)
            {
            case GOD_SHINING_ONE:
            case GOD_ELYVILON:
                if (!known)
                {
                    simple_god_message(" forgives your inadvertent attack on a "
                                       "neutral, just this once.");
                    break;
                }
                penance = level/2 + 1;
                // deliberate fall through

            case GOD_ZIN:
                if (!known)
                {
                    simple_god_message(" forgives your inadvertent attack on a "
                                       "neutral, just this once.");
                    break;
                }
                piety_change = -(level/2 + 1);
                retval = true;
                break;

            case GOD_JIYVA:
                if (victim && mons_is_slime(victim)
                    && !victim->is_shapeshifter())
                {
                    piety_change = -(level/2 + 3);
                    penance = level/2 + 3;
                    retval = true;
                }
                break;

            default:
                break;
            }
            break;

        case DID_ATTACK_FRIEND:
            if (god_hates_attacking_friend(you.religion, victim))
            {
                if (!known)
                {
                    simple_god_message(" forgives your inadvertent attack on "
                                       "an ally, just this once.");
                    break;
                }

                piety_change = -level;
                if (known)
                    penance = level * 3;
                retval = true;
            }
            break;

        case DID_FRIEND_DIED:
        case DID_SOULED_FRIEND_DIED:
            switch (you.religion)
            {
            case GOD_FEDHAS:
                // Ballistomycetes dying is penalised separately.
                if (victim && fedhas_protects(victim)
                    && victim->mons_species() != MONS_BALLISTOMYCETE)
                {
                    // level is (1 + monsterHD/2) for this conduct,
                    // trying a fixed cost since plant HD aren't that
                    // meaningful. -cao
                    piety_change = -1;
                    retval = true;
                    break;
                }
                break;

            case GOD_ELYVILON: // healer god cares more about this
                // Converted allies (marked as TSOites) can be martyrs.
                if (victim && victim->god == GOD_SHINING_ONE)
                    break;

                if (player_under_penance())
                    penance = 1;  // if already under penance smaller bonus
                else
                    penance = level;
                // fall through

            case GOD_ZIN:
                // Converted allies (marked as TSOites) can be martyrs.
                if (victim && victim->god == GOD_SHINING_ONE)
                    break;

                // Zin only cares about the deaths of those with souls.
                if (thing_done == DID_FRIEND_DIED)
                    break;
                // fall through

            case GOD_OKAWARU:
                piety_change = -level;
                retval = true;
                break;

            default:
                break;
            }
            break;

        case DID_KILL_LIVING:
            switch (you.religion)
            {
            case GOD_ELYVILON:
                // Killing is only disapproved of during prayer.
                if (you.duration[DUR_LIFESAVING])
                {
                    simple_god_message(" does not appreciate your shedding "
                                       "blood when asking for salvation!");
                    retval = true;
                    piety_change = -level;
                    penance = level * 2;
                }
                break;

            case GOD_KIKUBAAQUDGHA:
            case GOD_YREDELEMNUL:
            case GOD_OKAWARU:
            case GOD_VEHUMET:
            case GOD_MAKHLEB:
            case GOD_TROG:
            case GOD_BEOGH:
            case GOD_LUGONU:
                if (god_hates_attacking_friend(you.religion, victim))
                    break;

                simple_god_message(" accepts your kill.");
                retval = true;
                piety_denom = level + 18 - you.experience_level / 2;
                piety_change = piety_denom - 6;
                piety_denom = std::max(piety_denom, 1);
                piety_change = std::max(piety_change, 0);
                break;

            default:
                break;
            }
            break;

        case DID_KILL_UNDEAD:
            switch (you.religion)
            {
            case GOD_SHINING_ONE:
            case GOD_OKAWARU:
            case GOD_VEHUMET:
            case GOD_MAKHLEB:
            case GOD_BEOGH:
            case GOD_LUGONU:
                if (god_hates_attacking_friend(you.religion, victim))
                    break;

                simple_god_message(" accepts your kill.");
                retval = true;
                // Holy gods are easier to please this way.
                piety_denom = level + 18 - (is_good_god(you.religion) ? 0 :
                                          you.experience_level / 2);
                piety_change = piety_denom - 5;
                piety_denom = std::max(piety_denom, 1);
                piety_change = std::max(piety_change, 0);
                break;

            default:
                break;
            }
            break;

        case DID_KILL_DEMON:
            switch (you.religion)
            {
            case GOD_SHINING_ONE:
            case GOD_OKAWARU:
            case GOD_MAKHLEB:
            case GOD_TROG:
            case GOD_KIKUBAAQUDGHA:
            case GOD_BEOGH:
                if (god_hates_attacking_friend(you.religion, victim))
                    break;

                simple_god_message(" accepts your kill.");
                retval = true;
                // Holy gods are easier to please this way.
                piety_denom = level + 18 - (is_good_god(you.religion) ? 0 :
                                          you.experience_level / 2);
                piety_change = piety_denom - 4;
                piety_denom = std::max(piety_denom, 1);
                piety_change = std::max(piety_change, 0);
                break;

            default:
                break;
            }
            break;

        case DID_KILL_NATURAL_UNHOLY:
        case DID_KILL_NATURAL_EVIL:
            if (you.religion == GOD_SHINING_ONE
                && !god_hates_attacking_friend(you.religion, victim))
            {
                simple_god_message(" accepts your kill.");
                retval = true;
                piety_denom = level + 18;
                piety_change = piety_denom - 4;
            }
            break;

        case DID_KILL_UNCLEAN:
        case DID_KILL_CHAOTIC:
            if (you.religion == GOD_ZIN
                && !god_hates_attacking_friend(you.religion, victim))
            {
                simple_god_message(" accepts your kill.");
                retval = true;
                piety_denom = level + 18;
                piety_change = piety_denom - 4;
            }
            break;

        case DID_KILL_PRIEST:
            if (you.religion == GOD_BEOGH
                && !god_hates_attacking_friend(you.religion, victim))
            {
                simple_god_message(" appreciates your killing of a heretic "
                                   "priest.");
                retval = true;
                piety_denom = level + 10;
                piety_change = piety_denom - 6;
            }
            break;

        case DID_KILL_WIZARD:
            if (you.religion == GOD_TROG
                && !god_hates_attacking_friend(you.religion, victim))
            {
                simple_god_message(" appreciates your killing of a magic "
                                   "user.");
                retval = true;
                piety_denom = level + 10;
                piety_change = piety_denom - 6;
            }
            break;

        case DID_KILL_FAST:
            if (you.religion == GOD_CHEIBRIADOS
                && !god_hates_attacking_friend(you.religion, victim))
            {
                retval = true;
                piety_denom = level + 18 - you.experience_level / 2;
                piety_change = piety_denom - 6;
                piety_denom = std::max(piety_denom, 1);
                piety_change = std::max(piety_change, 0);

                const int speed_delta =
                    cheibriados_monster_player_speed_delta(victim);
                dprf("Che DID_KILL_FAST: %s speed delta: %d",
                     victim->name(DESC_PLAIN, true).c_str(),
                     speed_delta);
                if (speed_delta > 0 && x_chance_in_y(speed_delta, 12))
                {
                    piety_change *= 2;
                    simple_god_message(" thoroughly appreciates the change of pace.");
                }
                else
                    simple_god_message(" appreciates the change of pace.");
            }
            break;

        case DID_KILL_ARTIFICIAL:
            if (you.religion == GOD_YREDELEMNUL
                && !god_hates_attacking_friend(you.religion, victim))
            {
                simple_god_message(" accepts your kill.");
                retval = true;
                piety_denom = level + 18;
                piety_change = piety_denom - 3;
            }
            break;

        // Note that holy deaths are special, they are always noticed...
        // If you or any friendly kills one, you'll get the credit or
        // the blame.
        case DID_KILL_HOLY:
            switch (you.religion)
            {
            case GOD_ZIN:
            case GOD_SHINING_ONE:
            case GOD_ELYVILON:
                if (victim
                    && !testbits(victim->flags, MF_NO_REWARD)
                    && !testbits(victim->flags, MF_WAS_NEUTRAL))
                {
                    break;
                }

                penance = level * 3;
                piety_change = -level * 3;
                retval = true;
                break;

            case GOD_YREDELEMNUL:
            case GOD_KIKUBAAQUDGHA:
            case GOD_MAKHLEB:
            case GOD_BEOGH:
            case GOD_LUGONU:
            case GOD_OKAWARU:
                if (god_hates_attacking_friend(you.religion, victim))
                    break;

                simple_god_message(" accepts your kill.");
                retval = true;
                piety_denom = level + 18;
                piety_change = piety_denom - 3;

                if (you.religion == GOD_YREDELEMNUL)
                {
                    simple_god_message(" appreciates your killing of a holy "
                                       "being.");
                    retval = true;
                    piety_change *= 2;
                }
                break;

            default:
                break;
            }
            break;

        case DID_HOLY_KILLED_BY_UNDEAD_SLAVE:
            switch (you.religion)
            {
            case GOD_YREDELEMNUL:
            case GOD_KIKUBAAQUDGHA:
            case GOD_MAKHLEB:
            case GOD_BEOGH:
            case GOD_LUGONU:
                if (god_hates_attacking_friend(you.religion, victim))
                    break;

                simple_god_message(" accepts your slave's kill.");
                retval = true;
                piety_denom = level + 18;
                piety_change = piety_denom - 3;
                break;

            default:
                break;
            }
            break;

        case DID_HOLY_KILLED_BY_SERVANT:
            switch (you.religion)
            {
            case GOD_ZIN:
            case GOD_SHINING_ONE:
            case GOD_ELYVILON:
                if (victim
                    && !testbits(victim->flags, MF_NO_REWARD)
                    && !testbits(victim->flags, MF_WAS_NEUTRAL))
                {
                    break;
                }

                penance = level * 3;
                piety_change = -level * 3;
                retval = true;
                break;

            case GOD_MAKHLEB:
            case GOD_BEOGH:
            case GOD_LUGONU:
                if (god_hates_attacking_friend(you.religion, victim))
                    break;

                simple_god_message(" accepts your collateral kill.");
                retval = true;
                piety_denom = level + 18;
                piety_change = piety_denom - 3;
                break;

            default:
                break;
            }
            break;

        case DID_LIVING_KILLED_BY_UNDEAD_SLAVE:
            switch (you.religion)
            {
            case GOD_YREDELEMNUL:
            case GOD_KIKUBAAQUDGHA:
            case GOD_VEHUMET:
            case GOD_MAKHLEB:
            case GOD_BEOGH:
            case GOD_LUGONU:
                simple_god_message(" accepts your slave's kill.");
                retval = true;
                piety_denom = level + 10 - you.experience_level/3;
                piety_change = piety_denom - 6;
                piety_denom = std::max(piety_denom, 1);
                piety_change = std::max(piety_change, 0);
                break;
            default:
                break;
            }
            break;

        case DID_LIVING_KILLED_BY_SERVANT:
            switch (you.religion)
            {
            case GOD_VEHUMET:
            case GOD_MAKHLEB:
            case GOD_TROG:
            case GOD_BEOGH:
            case GOD_LUGONU:
                simple_god_message(" accepts your collateral kill.");
                retval = true;
                piety_denom = level + 10 - you.experience_level/3;
                piety_change = piety_denom - 6;
                piety_denom = std::max(piety_denom, 1);
                piety_change = std::max(piety_change, 0);
                break;
            default:
                break;
            }
            break;

        case DID_UNDEAD_KILLED_BY_UNDEAD_SLAVE:
            switch (you.religion)
            {
            case GOD_VEHUMET:
            case GOD_MAKHLEB:
            case GOD_BEOGH:
            case GOD_LUGONU:
                simple_god_message(" accepts your slave's kill.");
                retval = true;
                piety_denom = level + 10 - you.experience_level/3;
                piety_change = piety_denom - 6;
                piety_denom = std::max(piety_denom, 1);
                piety_change = std::max(piety_change, 0);
                break;
            default:
                break;
            }
            break;

        case DID_UNDEAD_KILLED_BY_SERVANT:
            switch (you.religion)
            {
            case GOD_SHINING_ONE:
            case GOD_VEHUMET:
            case GOD_MAKHLEB:
            case GOD_BEOGH:
            case GOD_LUGONU:
                simple_god_message(" accepts your collateral kill.");
                retval = true;
                piety_denom = level + 10 - (is_good_god(you.religion) ? 0 :
                                            you.experience_level/3);
                piety_change = piety_denom - 6;
                piety_denom = std::max(piety_denom, 1);
                piety_change = std::max(piety_change, 0);
                break;
            default:
                break;
            }
            break;

        case DID_DEMON_KILLED_BY_UNDEAD_SLAVE:
            switch (you.religion)
            {
            case GOD_KIKUBAAQUDGHA:
            case GOD_MAKHLEB:
            case GOD_BEOGH:
                simple_god_message(" accepts your slave's kill.");
                retval = true;
                piety_denom = level + 10 - you.experience_level/3;
                piety_change = piety_denom - 6;
                piety_denom = std::max(piety_denom, 1);
                piety_change = std::max(piety_change, 0);
                break;
            default:
                break;
            }
            break;

        case DID_DEMON_KILLED_BY_SERVANT:
            switch (you.religion)
            {
            case GOD_SHINING_ONE:
            case GOD_MAKHLEB:
            case GOD_TROG:
            case GOD_BEOGH:
                simple_god_message(" accepts your collateral kill.");
                retval = true;
                piety_denom = level + 10 - (is_good_god(you.religion) ? 0 :
                                            you.experience_level/3);
                piety_change = piety_denom - 6;
                piety_denom = std::max(piety_denom, 1);
                piety_change = std::max(piety_change, 0);
                break;
            default:
                break;
            }
            break;

        case DID_NATURAL_UNHOLY_KILLED_BY_SERVANT:
        case DID_NATURAL_EVIL_KILLED_BY_SERVANT:
            if (you.religion == GOD_SHINING_ONE)
            {
                simple_god_message(" accepts your collateral kill.");
                retval = true;
                piety_denom = level + 10;
                piety_change = piety_denom - 6;
            }
            break;

        case DID_UNCLEAN_KILLED_BY_SERVANT:
        case DID_CHAOTIC_KILLED_BY_SERVANT:
            if (you.religion == GOD_ZIN)
            {
                simple_god_message(" accepts your collateral kill.");
                retval = true;

                piety_denom = level + 10;
                piety_change = piety_denom - 6;
            }
            break;

        case DID_ARTIFICIAL_KILLED_BY_UNDEAD_SLAVE:
            if (you.religion == GOD_YREDELEMNUL)
            {
                simple_god_message(" accepts your slave's kill.");
                retval = true;
                piety_denom = level + 18;
                piety_change = piety_denom - 3;
            }
            break;

        // Currently used only when confused undead kill artificial
        // beings, which Yredelemnul doesn't care about.
        case DID_ARTIFICIAL_KILLED_BY_SERVANT:
            break;

        case DID_SPELL_MEMORISE:
            if (you.religion == GOD_TROG)
            {
                penance = level * 10;
                piety_change = -penance;
                retval = true;
            }
            break;

        case DID_SPELL_CASTING:
            if (you.religion == GOD_TROG)
            {
                piety_change = -level;
                penance = level * 5;
                retval = true;
            }
            break;

        case DID_SPELL_PRACTISE:
            // Like CAST, but for skill advancement.
            // Level is number of skill points gained...
            // typically 10 * exercise, but may be more/less if the
            // skill is at 0 (INT adjustment), or if the PC's pool is
            // low and makes change.
            if (you.religion == GOD_SIF_MUNA)
            {
                // Old curve: random2(12) <= spell-level, this is
                // similar, but faster at low levels (to help ease
                // things for low level spells).  Power averages about
                // (level * 20 / 3) + 10 / 3 now.  Also note that spell
                // skill practise comes just after XP gain, so magical
                // kills tend to do both at the same time (unlike
                // melee).  This means high level spells probably work
                // pretty much like they used to (use spell, get piety).
                piety_change = level + 10;
                piety_denom = 80;
                retval = true;
            }
            break;

        case DID_CARDS:
            if (you.religion == GOD_NEMELEX_XOBEH)
            {
                piety_change = level;
                retval = true;

                // level == 0: stacked, deck not used up
                // level == 1: used up or nonstacked
                // level == 2: used up and nonstacked
                // and there's a 1/3 chance of an additional bonus point
                // for nonstacked cards.
                int chance = 0;
                switch (level)
                {
                case 0: chance = 0;   break;
                case 1: chance = 40;  break;
                case 2: chance = 70;  break;
                default:
                case 3: chance = 100; break;
                }

                if (x_chance_in_y(chance, 100)
                    && you.attribute[ATTR_CARD_COUNTDOWN])
                {
                    you.attribute[ATTR_CARD_COUNTDOWN]--;
#if defined(DEBUG_DIAGNOSTICS) || defined(DEBUG_CARDS) || defined(DEBUG_GIFTS)
                    mprf(MSGCH_DIAGNOSTICS, "Countdown down to %d",
                         you.attribute[ATTR_CARD_COUNTDOWN]);
#endif
                }
            }
            break;

        case DID_CAUSE_GLOWING:
        case DID_DELIBERATE_MUTATING:
            if (you.religion == GOD_ZIN)
            {
                if (!known && thing_done != DID_CAUSE_GLOWING)
                {
                    simple_god_message(" forgives your inadvertent chaotic "
                                       "act, just this once.");
                    break;
                }

                if (thing_done == DID_CAUSE_GLOWING)
                {
                    static long last_glowing_lecture = -1L;
                    if (!level)
                    {
                        simple_god_message(" is not enthusiastic about the "
                                           "mutagenic glow surrounding you.");
                    }
                    else if (last_glowing_lecture != you.num_turns)
                    {
                        last_glowing_lecture = you.num_turns;
                        // Increase contamination within yellow glow.
                        simple_god_message(" does not appreciate the extra "
                                           "mutagenic glow surrounding you!");
                    }
                }

                piety_change = -level;
                retval = true;
            }
            break;

        // level depends on intelligence: normal -> 1, high -> 2
        // cannibalism is still worse
        case DID_EAT_SOULED_BEING:
            if (you.religion == GOD_ZIN)
            {
                piety_change = -level * 5;
                if (level > 1)
                    penance = 5;
                retval = true;
            }
            break;

        case DID_UNCLEAN:
            if (you.religion == GOD_ZIN)
            {
                retval = true;
                if (!known)
                {
                    simple_god_message(" forgives your inadvertent unclean "
                                       "act, just this once.");
                    break;
                }
                piety_change = -level;
                penance      = level;
            }
            break;

        case DID_CHAOS:
            if (you.religion == GOD_ZIN)
            {
                retval = true;
                if (!known)
                {
                    simple_god_message(" forgives your inadvertent chaotic "
                                       "act, just this once.");
                    break;
                }
                piety_change = -level;
                penance      = level;
            }
            break;

        case DID_ATTACK_IN_SANCTUARY:
            if (you.religion == GOD_ZIN)
            {
                piety_change = -level;
                retval = true;
            }
            break;

        case DID_DESECRATE_ORCISH_REMAINS:
            if (you.religion == GOD_BEOGH)
            {
                piety_change = -level;
                retval = true;
            }
            break;

        case DID_DESTROY_ORCISH_IDOL:
            if (you.religion == GOD_BEOGH)
            {
                piety_change = -level;
                penance = level * 3;
                retval = true;
            }
            break;

        case DID_HASTY:
            if (you.religion == GOD_CHEIBRIADOS)
            {
                if (!known)
                {
                    simple_god_message(" forgives your accidental hurry, just this once.");
                    break;
                }
                simple_god_message(" thinks you should slow down.");
                piety_change = -level;
                if (level > 5)
                    penance = level - 5;
                retval = true;
            }
            break;

        case DID_DESTROY_SPELLBOOK:
            if (you.religion == GOD_SIF_MUNA)
            {
                piety_change = -level;
                penance = level * (known ? 2 : 1);
                retval = true;
            }
            break;

        case DID_EXPLORATION:
            if (you.religion == GOD_ASHENZARI)
            {
                // levels: x1, x2, x3, x4
                piety_change = 1 + ash_bondage_level();
                piety_change *= 8; // base gain per dungeon level
                piety_denom = level;
                retval = true;
            }
            break;

        case DID_NOTHING:
        case NUM_CONDUCTS:
            break;
        }

#ifdef DEBUG_DIAGNOSTICS
        int old_piety = you.piety;
#endif

        if (piety_change > 0)
            gain_piety(piety_change, piety_denom);
        else
            dock_piety(div_rand_round(-piety_change, piety_denom), penance);

#ifdef DEBUG_DIAGNOSTICS
        // don't announce exploration piety unless you actually got a boost
        if (retval && (thing_done != DID_EXPLORATION || old_piety != you.piety))
        {
            static const char *conducts[] =
            {
                "",
                "Necromancy", "Holy", "Unholy", "Attack Holy", "Attack Neutral",
                "Attack Friend", "Friend Died", "Unchivalric Attack",
                "Poison", "Kill Living", "Kill Undead",
                "Kill Demon", "Kill Natural Unholy", "Kill Natural Evil",
                "Kill Unclean", "Kill Chaotic", "Kill Wizard", "Kill Priest",
                "Kill Holy", "Kill Fast", "Undead Slave Kill Living",
                "Servant Kill Living", "Undead Slave Kill Undead",
                "Servant Kill Undead", "Undead Slave Kill Demon",
                "Servant Kill Demon", "Servant Kill Natural Unholy",
                "Servant Kill Natural Evil", "Undead Slave Kill Holy",
                "Servant Kill Holy", "Spell Memorise", "Spell Cast",
                "Spell Practise", "Cards",
                "Drink Blood", "Cannibalism","Eat Souled Being",
                "Deliberate Mutation", "Cause Glowing", "Use Unclean",
                "Use Chaos", "Desecrate Orcish Remains", "Destroy Orcish Idol",
                "Kill Slime", "Kill Plant", "Servant Kill Plant",
                "Was Hasty", "Corpse Violation",
                "Souled Friend Died", "Servant Kill Unclean",
                "Servant Kill Chaotic", "Attack In Sanctuary",
                "Kill Artificial", "Undead Slave Kill Artificial",
                "Servant Kill Artificial", "Destroy Spellbook",
                "Exploration", "Desecrated Holy Remains",
            };

            COMPILE_CHECK(ARRAYSZ(conducts) == NUM_CONDUCTS, c1);
            mprf(MSGCH_DIAGNOSTICS,
                 "conduct: %s; piety: %d (%+d/%d); penance: %d (%+d)",
                 conducts[thing_done],
                 you.piety, piety_change, piety_denom,
                 you.penance[you.religion], penance);

        }
#endif
    }

    do_god_revenge(thing_done);

    return (retval);
}

// These two arrays deal with the situation where a beam hits a non-fleeing
// monster, the monster starts to flee because of the damage, and then the
// beam bounces and hits the monster again.  If the monster wasn't fleeing
// when the beam started then hits from bounces shouldn't count as
// unchivalric attacks, but if the first hit from the beam *was* unchivalrous
// then all the bounces should count as unchivalrous as well.
//
// Also do the same sort of check for harming a friendly monster,
// since a Beogh worshipper zapping an orc with lightning might cause it to
// become a follower on the first hit, and the second hit would be
// against a friendly orc.
static FixedVector<bool, MAX_MONSTERS> _first_attack_conduct;
static FixedVector<bool, MAX_MONSTERS> _first_attack_was_unchivalric;
static FixedVector<bool, MAX_MONSTERS> _first_attack_was_friendly;

void god_conduct_turn_start()
{
    _first_attack_conduct.init(true);
    _first_attack_was_unchivalric.init(false);
    _first_attack_was_friendly.init(false);
}

void set_attack_conducts(god_conduct_trigger conduct[3], const monster* mon,
                         bool known)
{
    const unsigned int midx = mon->mindex();

    if (mon->friendly())
    {
        if (_first_attack_conduct[midx]
            || _first_attack_was_friendly[midx])
        {
            conduct[0].set(DID_ATTACK_FRIEND, 5, known, mon);
            _first_attack_was_friendly[midx] = true;
        }
    }
    else if (mon->neutral())
    {
        conduct[0].set(DID_ATTACK_NEUTRAL, 5, known, mon);
    }

    if (is_unchivalric_attack(&you, mon)
        && (_first_attack_conduct[midx]
            || _first_attack_was_unchivalric[midx]))
    {
        conduct[1].set(DID_UNCHIVALRIC_ATTACK, 4, known, mon);
        _first_attack_was_unchivalric[midx] = true;
    }

    if (mon->is_holy())
        conduct[2].set(DID_ATTACK_HOLY, mon->hit_dice, known, mon);

    _first_attack_conduct[midx] = false;
}

void enable_attack_conducts(god_conduct_trigger conduct[3])
{
    for (int i = 0; i < 3; ++i)
        conduct[i].enabled = true;
}

void disable_attack_conducts(god_conduct_trigger conduct[3])
{
    for (int i = 0; i < 3; ++i)
        conduct[i].enabled = false;
}
