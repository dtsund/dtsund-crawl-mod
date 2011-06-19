/**
 * @file
 * @brief Functions with decks of cards.
**/

#include "AppHdr.h"

#include "decks.h"

#include <iostream>
#include <algorithm>

#include "externs.h"

#include "acquire.h"
#include "beam.h"
#include "cio.h"
#include "coordit.h"
#include "database.h"
#include "dgn-actions.h"
#include "dungeon.h"
#include "effects.h"
#include "env.h"
#include "files.h"
#include "food.h"
#include "ghost.h"
#include "godwrath.h"
#include "invent.h"
#include "item_use.h"
#include "itemprop.h"
#include "items.h"
#include "makeitem.h"
#include "maps.h"
#include "message.h"
#include "misc.h"
#include "mon-iter.h"
#include "mon-place.h"
#include "mon-util.h"
#include "mgen_data.h"
#include "coord.h"
#include "mon-stuff.h"
#include "mutation.h"
#include "options.h"
#include "ouch.h"
#include "player.h"
#include "player-equip.h"
#include "player-stats.h"
#include "potion.h"
#include "religion.h"
#include "godconduct.h"
#include "skills2.h"
#include "spl-cast.h"
#include "spl-damage.h"
#include "spl-goditem.h"
#include "spl-miscast.h"
#include "spl-other.h"
#include "spl-selfench.h"
#include "spl-summoning.h"
#include "spl-transloc.h"
#include "spl-util.h"
#include "spl-wpnench.h"
#include "state.h"
#include "stuff.h"
#include "terrain.h"
#include "transform.h"
#include "traps.h"
#include "view.h"
#include "xom.h"

// DECK STRUCTURE: deck.plus is the number of cards the deck *started*
// with, deck.plus2 is the number of cards drawn, deck.special is the
// deck rarity, deck.props["cards"] holds the list of cards (with the
// highest index card being the top card, and index 0 being the bottom
// card), deck.props["drawn_cards"] holds the list of drawn cards
// (with index 0 being the first drawn), deck.props["card_flags"]
// holds the flags for each card, deck.props["num_marked"] is the
// number of marked cards left in the deck, and
// deck.props["non_brownie_draws"] is the number of non-marked draws
// you have to make from that deck before earning brownie points from
// it again.
//
// The card type and per-card flags are each stored as unsigned bytes,
// for a maximum of 256 different kinds of cards and 8 bits of flags.

static void _deck_ident(item_def& deck);

struct card_with_weights
{
    card_type card;
    int weight[3];
};

typedef card_with_weights deck_archetype;

#define END_OF_DECK {NUM_CARDS, {0,0,0}}

const deck_archetype deck_of_transport[] = {
    { CARD_PORTAL,   {5, 5, 5} },
    { CARD_WARP,     {5, 5, 5} },
    { CARD_SWAP,     {5, 5, 5} },
    { CARD_VELOCITY, {5, 5, 5} },
    END_OF_DECK
};

const deck_archetype deck_of_emergency[] = {
    { CARD_TOMB,       {5, 5, 5} },
    { CARD_BANSHEE,    {5, 5, 5} },
    { CARD_DAMNATION,  {0, 1, 2} },
    { CARD_SOLITUDE,   {5, 5, 5} },
    { CARD_WARPWRIGHT, {5, 5, 5} },
    { CARD_FLIGHT,     {5, 5, 5} },
    END_OF_DECK
};

const deck_archetype deck_of_destruction[] = {
    { CARD_VITRIOL, {5, 5, 5} },
    { CARD_FLAME,   {5, 5, 5} },
    { CARD_FROST,   {5, 5, 5} },
    { CARD_VENOM,   {5, 5, 5} },
    { CARD_HAMMER,  {5, 5, 5} },
    { CARD_SPARK,   {5, 5, 5} },
    { CARD_PAIN,    {5, 5, 5} },
    { CARD_TORMENT, {0, 2, 4} },
    END_OF_DECK
};

const deck_archetype deck_of_battle[] = {
    { CARD_ELIXIR,        {5, 5, 5} },
    { CARD_BATTLELUST,    {5, 5, 5} },
    { CARD_METAMORPHOSIS, {5, 5, 5} },
    { CARD_HELM,          {5, 5, 5} },
    { CARD_BLADE,         {5, 5, 5} },
    { CARD_SHADOW,        {5, 5, 5} },
    END_OF_DECK
};

const deck_archetype deck_of_enchantments[] = {
    { CARD_ELIXIR, {5, 5, 5} },
    END_OF_DECK
};

const deck_archetype deck_of_summoning[] = {
    { CARD_CRUSADE,         {5, 5, 5} },
    { CARD_SUMMON_ANIMAL,   {5, 5, 5} },
    { CARD_SUMMON_DEMON,    {4, 4, 4} },
    { CARD_SUMMON_WEAPON,   {5, 5, 5} },
    { CARD_SUMMON_FLYING,   {4, 4, 4} },
    { CARD_SUMMON_SKELETON, {5, 5, 5} },
    { CARD_SUMMON_UGLY,     {5, 5, 5} },
    END_OF_DECK
};

const deck_archetype deck_of_wonders[] = {
    { CARD_POTION,     {5, 5, 5} },
    { CARD_FOCUS,      {1, 1, 2} },
    { CARD_SHUFFLE,    {0, 1, 2} },
    { CARD_EXPERIENCE, {3, 4, 5} },
    { CARD_WILD_MAGIC, {5, 5, 5} },
    { CARD_HELIX,      {5, 5, 5} },
    { CARD_SAGE,       {5, 5, 5} },
    { CARD_ALCHEMIST,  {5, 5, 5} },
    END_OF_DECK
};

const deck_archetype deck_of_dungeons[] = {
    { CARD_WATER,     {5, 5, 5} },
    { CARD_GLASS,     {5, 5, 5} },
    { CARD_MAP,       {5, 5, 5} },
    { CARD_DOWSING,   {5, 5, 5} },
    { CARD_SPADE,     {5, 5, 5} },
    { CARD_TROWEL,    {5, 5, 5} },
    { CARD_MINEFIELD, {5, 5, 5} },
    END_OF_DECK
};

const deck_archetype deck_of_oddities[] = {
    { CARD_GENIE,   {5, 5, 5} },
    { CARD_BARGAIN, {5, 5, 5} },
    { CARD_WRATH,   {5, 5, 5} },
    { CARD_XOM,     {5, 5, 5} },
    { CARD_FEAST,   {5, 5, 5} },
    { CARD_FAMINE,  {5, 5, 5} },
    { CARD_CURSE,   {5, 5, 5} },
    END_OF_DECK
};

const deck_archetype deck_of_punishment[] = {
    { CARD_WRAITH,     {5, 5, 5} },
    { CARD_WILD_MAGIC, {5, 5, 5} },
    { CARD_WRATH,      {5, 5, 5} },
    { CARD_XOM,        {5, 5, 5} },
    { CARD_FAMINE,     {5, 5, 5} },
    { CARD_CURSE,      {5, 5, 5} },
    { CARD_TOMB,       {5, 5, 5} },
    { CARD_DAMNATION,  {5, 5, 5} },
    { CARD_PORTAL,     {5, 5, 5} },
    { CARD_MINEFIELD,  {5, 5, 5} },
    { CARD_SWINE,      {5, 5, 5} },
    END_OF_DECK
};

static void _check_odd_card(uint8_t flags)
{
    if ((flags & CFLAG_ODDITY) && !(flags & CFLAG_SEEN))
        mpr("This card doesn't seem to belong here.");
}

static bool _card_forbidden(card_type card)
{
    if (crawl_state.game_is_zotdef())
        switch(card)
        {
        case CARD_TOMB:
        case CARD_WARPWRIGHT:
        case CARD_WATER:
        case CARD_TROWEL:
        case CARD_MINEFIELD: // with teleport taken away, might be acceptable
        case CARD_STAIRS:
            return true;
        default:
            break;
        }
    return false;
}

int cards_in_deck(const item_def &deck)
{
    ASSERT(is_deck(deck));

    const CrawlHashTable &props = deck.props;
    ASSERT(props.exists("cards"));

    return (props["cards"].get_vector().size());
}

static void _shuffle_deck(item_def &deck)
{
    ASSERT(is_deck(deck));

    CrawlHashTable &props = deck.props;
    ASSERT(props.exists("cards"));

    CrawlVector &cards = props["cards"].get_vector();

    CrawlVector &flags = props["card_flags"].get_vector();
    ASSERT(flags.size() == cards.size());

    // Don't use std::shuffle(), since we want to apply exactly the
    // same shuffling to both the cards vector and the flags vector.
    std::vector<vec_size> pos;
    for (unsigned long i = 0; i < cards.size(); ++i)
        pos.push_back(random2(cards.size()));

    for (vec_size i = 0; i < pos.size(); ++i)
    {
        std::swap(cards[i], cards[pos[i]]);
        std::swap(flags[i], flags[pos[i]]);
    }
}

card_type get_card_and_flags(const item_def& deck, int idx,
                             uint8_t& _flags)
{
    const CrawlHashTable &props = deck.props;
    const CrawlVector    &cards = props["cards"].get_vector();
    const CrawlVector    &flags = props["card_flags"].get_vector();

    // Negative idx means read from the end.
    if (idx < 0)
        idx += static_cast<int>(cards.size());

    _flags = (uint8_t) flags[idx].get_byte();

    return static_cast<card_type>(cards[idx].get_byte());
}

static void _set_card_and_flags(item_def& deck, int idx, card_type card,
                                uint8_t _flags)
{
    CrawlHashTable &props = deck.props;
    CrawlVector    &cards = props["cards"].get_vector();
    CrawlVector    &flags = props["card_flags"].get_vector();

    if (idx == -1)
        idx = static_cast<int>(cards.size()) - 1;

    cards[idx].get_byte() = card;
    flags[idx].get_byte() = _flags;
}

const char* card_name(card_type card)
{
    switch (card)
    {
    case CARD_PORTAL:          return "the Portal";
    case CARD_WARP:            return "the Warp";
    case CARD_SWAP:            return "Swap";
    case CARD_VELOCITY:        return "Velocity";
    case CARD_DAMNATION:       return "Damnation";
    case CARD_SOLITUDE:        return "Solitude";
    case CARD_ELIXIR:          return "the Elixir";
    case CARD_BATTLELUST:      return "Battlelust";
    case CARD_METAMORPHOSIS:   return "Metamorphosis";
    case CARD_HELM:            return "the Helm";
    case CARD_BLADE:           return "the Blade";
    case CARD_SHADOW:          return "the Shadow";
    case CARD_POTION:          return "the Potion";
    case CARD_FOCUS:           return "Focus";
    case CARD_SHUFFLE:         return "Shuffle";
    case CARD_EXPERIENCE:      return "Experience";
    case CARD_HELIX:           return "the Helix";
    case CARD_SAGE:            return "the Sage";
    case CARD_DOWSING:         return "Dowsing";
    case CARD_TROWEL:          return "the Trowel";
    case CARD_MINEFIELD:       return "the Minefield";
    case CARD_STAIRS:          return "the Stairs";
    case CARD_GENIE:           return "the Genie";
    case CARD_TOMB:            return "the Tomb";
    case CARD_WATER:           return "Water";
    case CARD_GLASS:           return "Vitrification";
    case CARD_MAP:             return "the Map";
    case CARD_BANSHEE:         return "the Banshee";
    case CARD_WILD_MAGIC:      return "Wild Magic";
    case CARD_CRUSADE:         return "the Crusade";
    case CARD_SUMMON_ANIMAL:   return "the Herd";
    case CARD_SUMMON_DEMON:    return "the Pentagram";
    case CARD_SUMMON_WEAPON:   return "the Dance";
    case CARD_SUMMON_FLYING:   return "Foxfire";
    case CARD_SUMMON_SKELETON: return "the Bones";
    case CARD_SUMMON_UGLY:     return "Repulsiveness";
    case CARD_SUMMON_ANY:      return "Summoning";
    case CARD_XOM:             return "Xom";
    case CARD_FAMINE:          return "Famine";
    case CARD_FEAST:           return "the Feast";
    case CARD_WARPWRIGHT:      return "Warpwright";
    case CARD_FLIGHT:          return "Flight";
    case CARD_VITRIOL:         return "Vitriol";
    case CARD_FLAME:           return "Flame";
    case CARD_FROST:           return "Frost";
    case CARD_VENOM:           return "Venom";
    case CARD_SPARK:           return "the Spark";
    case CARD_HAMMER:          return "the Hammer";
    case CARD_PAIN:            return "Pain";
    case CARD_TORMENT:         return "Torment";
    case CARD_SPADE:           return "the Spade";
    case CARD_BARGAIN:         return "the Bargain";
    case CARD_WRATH:           return "Wrath";
    case CARD_WRAITH:          return "the Wraith";
    case CARD_CURSE:           return "the Curse";
    case CARD_SWINE:           return "the Swine";
    case CARD_ALCHEMIST:       return "the Alchemist";
    case NUM_CARDS:            return "a buggy card";
    }
    return "a very buggy card";
}

static const deck_archetype* _random_sub_deck(uint8_t deck_type)
{
    const deck_archetype *pdeck = NULL;
    switch (deck_type)
    {
    case MISC_DECK_OF_ESCAPE:
        pdeck = (coinflip() ? deck_of_transport : deck_of_emergency);
        break;
    case MISC_DECK_OF_DESTRUCTION: pdeck = deck_of_destruction; break;
    case MISC_DECK_OF_DUNGEONS:    pdeck = deck_of_dungeons;    break;
    case MISC_DECK_OF_SUMMONING:   pdeck = deck_of_summoning;   break;
    case MISC_DECK_OF_WONDERS:     pdeck = deck_of_wonders;     break;
    case MISC_DECK_OF_PUNISHMENT:  pdeck = deck_of_punishment;  break;
    case MISC_DECK_OF_WAR:
        switch (random2(6))
        {
        case 0: pdeck = deck_of_destruction;  break;
        case 1: pdeck = deck_of_enchantments; break;
        case 2: pdeck = deck_of_battle;       break;
        case 3: pdeck = deck_of_summoning;    break;
        case 4: pdeck = deck_of_transport;    break;
        case 5: pdeck = deck_of_emergency;    break;
        }
        break;
    case MISC_DECK_OF_CHANGES:
        switch (random2(3))
        {
        case 0: pdeck = deck_of_battle;       break;
        case 1: pdeck = deck_of_dungeons;     break;
        case 2: pdeck = deck_of_wonders;      break;
        }
        break;
    case MISC_DECK_OF_DEFENCE:
        pdeck = (coinflip() ? deck_of_emergency : deck_of_battle);
        break;
    }

    ASSERT(pdeck);

    return (pdeck);
}

static card_type _choose_from_archetype(const deck_archetype* pdeck,
                                        deck_rarity_type rarity)
{
    // We assume here that common == 0, rare == 1, legendary == 2.

    // FIXME: We should use one of the various choose_random_weighted
    // functions here, probably with an iterator, instead of
    // duplicating the implementation.

    int totalweight = 0;
    card_type result = NUM_CARDS;
    for (int i = 0; pdeck[i].card != NUM_CARDS; ++i)
    {
        const card_with_weights& cww = pdeck[i];
        if (_card_forbidden(cww.card))
            continue;
        totalweight += cww.weight[rarity];
        if (x_chance_in_y(cww.weight[rarity], totalweight))
            result = cww.card;
    }
    return result;
}

static card_type _random_card(uint8_t deck_type, deck_rarity_type rarity,
                              bool &was_oddity)
{
    const deck_archetype *pdeck = _random_sub_deck(deck_type);

    if (one_chance_in(100))
    {
        pdeck      = deck_of_oddities;
        was_oddity = true;
    }

    return _choose_from_archetype(pdeck, rarity);
}

static card_type _random_card(const item_def& item, bool &was_oddity)
{
    return _random_card(item.sub_type, deck_rarity(item), was_oddity);
}

static card_type _draw_top_card(item_def& deck, bool message,
                                uint8_t &_flags)
{
    CrawlHashTable &props = deck.props;
    CrawlVector    &cards = props["cards"].get_vector();
    CrawlVector    &flags = props["card_flags"].get_vector();

    int num_cards = cards.size();
    int idx       = num_cards - 1;

    ASSERT(num_cards > 0);

    card_type card = get_card_and_flags(deck, idx, _flags);
    cards.pop_back();
    flags.pop_back();

    if (message)
    {
        if (_flags & CFLAG_MARKED)
            mprf("You draw %s.", card_name(card));
        else
            mprf("You draw a card... It is %s.", card_name(card));

        _check_odd_card(_flags);
    }

    return card;
}

static void _push_top_card(item_def& deck, card_type card,
                           uint8_t _flags)
{
    CrawlHashTable &props = deck.props;
    CrawlVector    &cards = props["cards"].get_vector();
    CrawlVector    &flags = props["card_flags"].get_vector();

    cards.push_back((char) card);
    flags.push_back((char) _flags);
}

static void _remember_drawn_card(item_def& deck, card_type card, bool allow_id)
{
    ASSERT(is_deck(deck));
    CrawlHashTable &props = deck.props;
    CrawlVector &drawn = props["drawn_cards"].get_vector();
    drawn.push_back(static_cast<char>(card));

    // Once you've drawn two cards, you know the deck.
    if (allow_id && (drawn.size() >= 2 || origin_is_god_gift(deck)))
        _deck_ident(deck);
}

const std::vector<card_type> get_drawn_cards(const item_def& deck)
{
    std::vector<card_type> result;
    if (is_deck(deck))
    {
        const CrawlHashTable &props = deck.props;
        const CrawlVector &drawn = props["drawn_cards"].get_vector();
        for (unsigned int i = 0; i < drawn.size(); ++i)
        {
            const char tmp = drawn[i];
            result.push_back(static_cast<card_type>(tmp));
        }
    }
    return result;
}

static bool _check_buggy_deck(item_def& deck)
{
    std::ostream& strm = msg::streams(MSGCH_DIAGNOSTICS);
    if (!is_deck(deck))
    {
        crawl_state.zero_turns_taken();
        strm << "This isn't a deck at all!" << std::endl;
        return (true);
    }

    CrawlHashTable &props = deck.props;

    if (!props.exists("cards")
        || props["cards"].get_type() != SV_VEC
        || props["cards"].get_vector().get_type() != SV_BYTE
        || cards_in_deck(deck) == 0)
    {
        crawl_state.zero_turns_taken();

        if (!props.exists("cards"))
            strm << "Seems this deck never had any cards in the first place!";
        else if (props["cards"].get_type() != SV_VEC)
            strm << "'cards' property isn't a vector.";
        else
        {
            if (props["cards"].get_vector().get_type() != SV_BYTE)
                strm << "'cards' vector doesn't contain bytes.";

            if (cards_in_deck(deck) == 0)
            {
                strm << "Strange, this deck is already empty.";

                int cards_left = 0;
                if (deck.plus2 >= 0)
                    cards_left = deck.plus - deck.plus2;
                else
                    cards_left = -deck.plus;

                if (cards_left != 0)
                {
                    strm << " But there should have been " <<  cards_left
                         << " cards left.";
                }
            }
        }
        strm << std::endl
             << "A swarm of software bugs snatches the deck from you "
            "and whisks it away."
             << std::endl;

        if (deck.link == you.equip[EQ_WEAPON])
            unwield_item();

        dec_inv_item_quantity(deck.link, 1);
        did_god_conduct(DID_CARDS, 1);

        return (true);
    }

    bool problems = false;

    CrawlVector &cards = props["cards"].get_vector();
    CrawlVector &flags = props["card_flags"].get_vector();

    vec_size num_cards = cards.size();
    vec_size num_flags = flags.size();

    unsigned int num_buggy     = 0;
    unsigned int num_marked    = 0;

    for (vec_size i = 0; i < num_cards; ++i)
    {
        uint8_t card   = cards[i].get_byte();
        uint8_t _flags = flags[i].get_byte();
        if (card >= NUM_CARDS)
        {
            cards.erase(i);
            flags.erase(i);
            i--;
            num_cards--;
            num_buggy++;
        }
        else
        {
            if (_flags & CFLAG_MARKED)
                num_marked++;
        }
    }

    if (num_buggy > 0)
    {
        strm << num_buggy << " buggy cards found in the deck, discarding them."
             << std::endl;

        deck.plus2 += num_buggy;

        num_cards = cards.size();
        num_flags = cards.size();

        problems = true;
    }

    if (num_cards == 0)
    {
        crawl_state.zero_turns_taken();

        strm << "Oops, all of the cards seem to be gone." << std::endl
             << "A swarm of software bugs snatches the deck from you "
             "and whisks it away." << std::endl;

        if (deck.link == you.equip[EQ_WEAPON])
            unwield_item();

        dec_inv_item_quantity(deck.link, 1);
        did_god_conduct(DID_CARDS, 1);

        return (true);
    }

    if (num_cards > deck.plus)
    {
        if (deck.plus == 0)
            strm << "Deck was created with zero cards???" << std::endl;
        else if (deck.plus < 0)
            strm << "Deck was created with *negative* cards?!" << std::endl;
        else
        {
            strm << "Deck has more cards than it was created with?"
                 << std::endl;
        }

        deck.plus = num_cards;
        problems  = true;
    }

    if (num_cards > num_flags)
    {
#ifdef WIZARD
        strm << (num_cards - num_flags) << " more cards than flags.";
#else
        strm << "More cards than flags.";
#endif
        strm << std::endl;
        for (unsigned int i = num_flags + 1; i <= num_cards; ++i)
            flags[i] = static_cast<char>(0);

        problems = true;
    }
    else if (num_flags > num_cards)
    {
#ifdef WIZARD
        strm << (num_cards - num_flags) << " more cards than flags.";
#else
        strm << "More cards than flags.";
#endif
        strm << std::endl;

        for (unsigned int i = num_flags; i > num_cards; --i)
            flags.erase(i);

        problems = true;
    }

    if (props["num_marked"].get_byte() > static_cast<char>(num_cards))
    {
        strm << "More cards marked than in the deck?" << std::endl;
        props["num_marked"] = static_cast<char>(num_marked);
        problems = true;
    }
    else if (props["num_marked"].get_byte() != static_cast<char>(num_marked))
    {
#ifdef WIZARD

        strm << "Oops, counted " << static_cast<int>(num_marked)
             << " marked cards, but num_marked is "
             << (static_cast<int>(props["num_marked"].get_byte()));
#else
        strm << "Oops, book-keeping on marked cards is wrong.";
#endif
        strm << std::endl;

        props["num_marked"] = static_cast<char>(num_marked);
        problems = true;
    }

    if (deck.plus2 >= 0)
    {
        if (deck.plus != (deck.plus2 + static_cast<long>(num_cards)))
        {
#ifdef WIZARD
            strm << "Have you used " << deck.plus2 << " cards, or "
                 << (deck.plus - num_cards) << "? Oops.";
#else
            strm << "Oops, book-keeping on used cards is wrong.";
#endif
            strm << std::endl;
            deck.plus2 = deck.plus - num_cards;
            problems = true;
        }
    }
    else
    {
        if (-deck.plus2 != static_cast<long>(num_cards))
        {
#ifdef WIZARD
            strm << "There are " << num_cards << " cards left, not "
                 << (-deck.plus2) << ".  Oops.";
#else
            strm << "Oops, book-keeping on cards left is wrong.";
#endif
            strm << std::endl;
            deck.plus2 = -num_cards;
            problems = true;
        }
    }

    if (!problems)
        return (false);

    you.wield_change = true;

    if (!yesno("Problems might not have been completely fixed; "
               "still use deck?", true, 'n'))
    {
        crawl_state.zero_turns_taken();
        return (true);
    }
    return (false);
}

// Choose a deck from inventory and return its slot (or -1).
static int _choose_inventory_deck(const char* prompt)
{
    const int slot = prompt_invent_item(prompt,
                                         MT_INVLIST, OSEL_DRAW_DECK,
                                         true, true, true, 0, -1, NULL,
                                         OPER_EVOKE);

    if (prompt_failed(slot))
        return -1;

    if (!is_deck(you.inv[slot]))
    {
        mpr("That isn't a deck!");
        return -1;
    }

    return slot;
}

// Select a deck from inventory and draw a card from it.
bool choose_deck_and_draw()
{
    const int slot = _choose_inventory_deck("Draw from which deck?");

    if (slot == -1)
    {
        crawl_state.zero_turns_taken();
        return (false);
    }

    evoke_deck(you.inv[slot]);
    return (true);
}

static void _deck_ident(item_def& deck)
{
    if (in_inventory(deck) && !item_ident(deck, ISFLAG_KNOW_TYPE))
    {
        set_ident_flags(deck, ISFLAG_KNOW_TYPE);
        mprf("This is %s.", deck.name(DESC_NOCAP_A).c_str());
        you.wield_change = true;
    }
}

// This also shuffles the deck.
static void _deck_lose_card(item_def& deck)
{
    uint8_t flags = 0;
    // Seen cards are only half as likely to fall out,
    // marked cards only one-quarter as likely (note that marked
    // cards are also seen.)
    do
    {
        _shuffle_deck(deck);
        get_card_and_flags(deck, -1, flags);
    }
    while ((flags & CFLAG_MARKED) && coinflip()
            || (flags & CFLAG_SEEN) && coinflip());

    _draw_top_card(deck, false, flags);
    deck.plus2++;
}

// Peek at two cards in a deck, then shuffle them back in.
// Return false if the operation was failed/aborted along the way.
bool deck_peek()
{
    const int slot = _choose_inventory_deck("Peek at which deck?");
    if (slot == -1)
    {
        crawl_state.zero_turns_taken();
        return (false);
    }
    item_def& deck(you.inv[slot]);

    if (_check_buggy_deck(deck))
        return (false);

    if (cards_in_deck(deck) > 2)
    {
        _deck_lose_card(deck);
        mpr("A card falls out of the deck.");
    }

    CrawlVector &cards     = deck.props["cards"].get_vector();
    const int    num_cards = cards.size();

    card_type card1, card2;
    uint8_t flags1, flags2;

    card1 = get_card_and_flags(deck, 0, flags1);

    if (num_cards == 1)
    {
        mpr("There's only one card in the deck!");

        _set_card_and_flags(deck, 0, card1, flags1 | CFLAG_SEEN | CFLAG_MARKED);
        deck.props["num_marked"]++;
        deck.plus2 = -1;
        you.wield_change = true;

        return (true);
    }

    card2 = get_card_and_flags(deck, 1, flags2);

    int already_seen = 0;
    if (flags1 & CFLAG_SEEN)
        already_seen++;
    if (flags2 & CFLAG_SEEN)
        already_seen++;

    // Always increase if seen 2, 50% increase if seen 1.
    if (already_seen && x_chance_in_y(already_seen, 2))
        deck.props["non_brownie_draws"]++;

    mprf("You draw two cards from the deck. They are: %s and %s.",
         card_name(card1), card_name(card2));

    _set_card_and_flags(deck, 0, card1, flags1 | CFLAG_SEEN);
    _set_card_and_flags(deck, 1, card2, flags2 | CFLAG_SEEN);

    mpr("You shuffle the cards back into the deck.");
    _shuffle_deck(deck);

    // Peeking identifies the deck.
    _deck_ident(deck);

    you.wield_change = true;
    return (true);
}

// Mark a deck: look at the next four cards, mark them, and shuffle
// them back into the deck. The player won't know what order they're
// in, and if the top card is non-marked then the player won't
// know what the next card is.  Return false if the operation was
// failed/aborted along the way.
bool deck_mark()
{
    const int slot = _choose_inventory_deck("Mark which deck?");
    if (slot == -1)
    {
        crawl_state.zero_turns_taken();
        return (false);
    }
    item_def& deck(you.inv[slot]);
    if (_check_buggy_deck(deck))
        return (false);

    CrawlHashTable &props = deck.props;
    if (props["num_marked"].get_byte() > 0)
    {
        mpr("The deck is already marked.");
        crawl_state.zero_turns_taken();
        return (false);
    }

    // Lose some cards, but keep at least two.
    if (cards_in_deck(deck) > 2)
    {
        const int num_lost = std::min(cards_in_deck(deck)-2, random2(3) + 1);
        for (int i = 0; i < num_lost; ++i)
            _deck_lose_card(deck);

        if (num_lost == 1)
            mpr("A card falls out of the deck.");
        else if (num_lost > 1)
            mpr("Some cards fall out of the deck.");
    }

    const int num_cards   = cards_in_deck(deck);
    const int num_to_mark = (num_cards < 4 ? num_cards : 4);

    if (num_cards == 1)
        mpr("There's only one card left!");
    else if (num_cards < 4)
        mprf("The deck only has %d cards.", num_cards);

    std::vector<std::string> names;
    for (int i = 0; i < num_to_mark; ++i)
    {
        uint8_t flags;
        card_type     card = get_card_and_flags(deck, i, flags);

        flags |= CFLAG_SEEN | CFLAG_MARKED;
        _set_card_and_flags(deck, i, card, flags);

        names.push_back(card_name(card));
    }
    mpr_comma_separated_list("You draw and mark ", names);
    props["num_marked"] = (char) num_to_mark;

    if (num_cards == 1)
        ;
    else if (num_cards < 4)
    {
        mprf("You shuffle the deck.");
        deck.plus2 = -num_cards;
    }
    else
        mprf("You shuffle the cards back into the deck.");

    _shuffle_deck(deck);
    _deck_ident(deck);
    you.wield_change = true;

    return (true);
}

static void _redraw_stacked_cards(const std::vector<card_type>& draws,
                                  unsigned int selected)
{
    for (unsigned int i = 0; i < draws.size(); ++i)
    {
        cgotoxy(1, i+2);
        textcolor(selected == i ? WHITE : LIGHTGREY);
        cprintf("%u - %s", i+1, card_name(draws[i]));
        clear_to_end_of_line();
    }
}

static void _describe_cards(std::vector<card_type> cards)
{
    ASSERT(!cards.empty());

    std::ostringstream data;
    for (unsigned int i = 0; i < cards.size(); ++i)
    {
        std::string name = card_name(cards[i]);
        std::string desc = getLongDescription(name + " card");
        if (desc.empty())
            desc = "No description found.";

        name = uppercase_first(name);
        data << "<w>" << name << "</w>\n"
             << get_linebreak_string(desc, get_number_of_cols())
             << "\n";
    }
    formatted_string fs = formatted_string::parse_string(data.str());
    clrscr();
    fs.display();
    wait_for_keypress();
    redraw_screen();
}

// Stack a deck: look at the next five cards, put them back in any
// order, discard the rest of the deck.
// Return false if the operation was failed/aborted along the way.
bool deck_stack()
{
    const int slot = _choose_inventory_deck("Stack which deck?");
    if (slot == -1)
    {
        crawl_state.zero_turns_taken();
        return (false);
    }

    item_def& deck(you.inv[slot]);
    if (_check_buggy_deck(deck))
        return (false);

    CrawlHashTable &props = deck.props;
    if (props["num_marked"].get_byte() > 0)
    {
        mpr("You can't stack a marked deck.");
        crawl_state.zero_turns_taken();
        return (false);
    }

    _deck_ident(deck);
    const int num_cards    = cards_in_deck(deck);
    const int num_to_stack = (num_cards < 5 ? num_cards : 5);

    if (num_cards == 1)
        mpr("There's only one card left!");
    else if (num_cards < 5)
        mprf("The deck only has %d cards.", num_to_stack);
    else if (num_cards == 5)
        mpr("The deck has exactly five cards.");
    else
    {
        mprf("You draw the first five cards out of %d and discard the rest.",
             num_cards);
    }
    more();

    std::vector<card_type> draws;
    std::vector<uint8_t>   flags;
    for (int i = 0; i < num_cards; ++i)
    {
        uint8_t   _flags;
        card_type card = _draw_top_card(deck, false, _flags);

        if (i < num_to_stack)
        {
            draws.push_back(card);
            flags.push_back(_flags | CFLAG_SEEN | CFLAG_MARKED);
        }
        // Rest of deck is discarded.
    }

    // Re-add the cards, with changed flags, in case the game is closed
    // while the swapping takes place, so we don't leak information about
    // the deck.
    // If it does get closed, the order of the top five cards will be
    // unchanged, but the deck will be marked as stacked. (jpeg)
    for (unsigned int i = 0; i < draws.size(); ++i)
    {
        _push_top_card(deck, draws[draws.size() - 1 - i],
                       flags[flags.size() - 1 - i]);
    }
    deck.plus2 = -num_to_stack;
    props["num_marked"] = static_cast<char>(num_to_stack);
    you.wield_change = true;

    if (draws.size() > 1)
    {
        bool need_prompt_redraw = true;
        unsigned int selected = draws.size();
        while (true)
        {
            if (need_prompt_redraw)
            {
                clrscr();
                cgotoxy(1,1);
                textcolor(WHITE);
                cprintf("Press a digit to select a card, then another digit "
                        "to swap it.");
                cgotoxy(1,10);
                cprintf("Press ? for the card descriptions, or Enter to "
                        "accept.");

                _redraw_stacked_cards(draws, selected);
                need_prompt_redraw = false;
            }

            // Hand-hacked implementation, instead of using Menu. Oh well.
            const int c = getchk();
            if (c == CK_ENTER)
            {
                cgotoxy(1,11);
                textcolor(LIGHTGREY);
                cprintf("Are you sure? (press y or Y to confirm)");
                if (toupper(getchk()) == 'Y')
                    break;

                cgotoxy(1,11);
                clear_to_end_of_line();
                continue;
            }

            if (c == '?')
            {
                _describe_cards(draws);
                need_prompt_redraw = true;
            }
            else if (c >= '1' && c <= '0' + static_cast<int>(draws.size()))
            {
                const unsigned int new_selected = c - '1';
                if (selected < draws.size())
                {
                    std::swap(draws[selected], draws[new_selected]);
                    std::swap(flags[selected], flags[new_selected]);
                    selected = draws.size();
                }
                else
                    selected = new_selected;

                _redraw_stacked_cards(draws, selected);
            }
        }
        redraw_screen();
    }
    // Remove the cards again, and add them
    for (unsigned int i = 0; i < draws.size(); ++i)
    {
        uint8_t   _flags;
        _draw_top_card(deck, false, _flags);
    }
    for (unsigned int i = 0; i < draws.size(); ++i)
    {
        _push_top_card(deck, draws[draws.size() - 1 - i],
                       flags[flags.size() - 1 - i]);
    }

    _check_buggy_deck(deck);

    return (true);
}

// Draw the next three cards, discard two and pick one.
bool deck_triple_draw()
{
    const int slot = _choose_inventory_deck("Triple draw from which deck?");
    if (slot == -1)
    {
        crawl_state.zero_turns_taken();
        return (false);
    }

    item_def& deck(you.inv[slot]);

    if (_check_buggy_deck(deck))
        return (false);

    const int num_cards = cards_in_deck(deck);

    // We have to identify the deck before removing cards from it.
    // Otherwise, _remember_drawn_card() will implicitly call
    // _deck_ident() when the deck might have no cards left.
    _deck_ident(deck);

    if (num_cards == 1)
    {
        // Only one card to draw, so just draw it.
        evoke_deck(deck);
        return (true);
    }

    const int num_to_draw = (num_cards < 3 ? num_cards : 3);
    std::vector<card_type> draws;
    std::vector<uint8_t>   flags;

    for (int i = 0; i < num_to_draw; ++i)
    {
        uint8_t _flags;
        card_type card = _draw_top_card(deck, false, _flags);

        draws.push_back(card);
        flags.push_back(_flags);
    }

    int selected = -1;
    bool need_prompt_redraw = true;
    while (true)
    {
        if (need_prompt_redraw)
        {
            mpr("You draw... (choose one card, ? for their descriptions)");
            for (int i = 0; i < num_to_draw; ++i)
            {
                msg::streams(MSGCH_PROMPT) << (static_cast<char>(i + 'a')) << " - "
                                           << card_name(draws[i]) << std::endl;
            }
            need_prompt_redraw = false;
        }
        const int keyin = tolower(get_ch());
        if (keyin == '?')
        {
            _describe_cards(draws);
            need_prompt_redraw = true;
        }
        else if (keyin >= 'a' && keyin < 'a' + num_to_draw)
        {
            selected = keyin - 'a';
            break;
        }
        else
            canned_msg(MSG_HUH);
    }

    // Note how many cards were removed from the deck.
    deck.plus2 += num_to_draw;

    // Don't forget to update the number of marked ones, too.
    // But don't reduce the number of non-brownie draws.
    uint8_t num_marked_left = deck.props["num_marked"].get_byte();
    for (int i = 0; i < num_to_draw; ++i)
    {
        _remember_drawn_card(deck, draws[i], false);
        if (flags[i] & CFLAG_MARKED)
        {
            ASSERT(num_marked_left > 0);
            --num_marked_left;
        }
    }
    deck.props["num_marked"] = num_marked_left;

    you.wield_change = true;

    // Make deck disappear *before* the card effect, since we
    // don't want to unwield an empty deck.
    deck_rarity_type rarity = deck_rarity(deck);
    if (cards_in_deck(deck) == 0)
    {
        mpr("The deck of cards disappears in a puff of smoke.");
        if (slot == you.equip[EQ_WEAPON])
            unwield_item();

        dec_inv_item_quantity(slot, 1);
    }

    // Note that card_effect() might cause you to unwield the deck.
    card_effect(draws[selected], rarity,
                flags[selected] | CFLAG_SEEN | CFLAG_MARKED, false);

    return (true);
}

// This is Nemelex retribution.
void draw_from_deck_of_punishment()
{
    bool oddity;
    card_type card = _random_card(MISC_DECK_OF_PUNISHMENT, DECK_RARITY_COMMON,
                                  oddity);

    mpr("You draw a card...");
    card_effect(card, DECK_RARITY_COMMON);
}

static int _xom_check_card(item_def &deck, card_type card,
                           uint8_t flags)
{
    int amusement = 64;

    if (!item_type_known(deck))
        amusement *= 2;
    // Expecting one type of card but got another, real funny.
    else if (flags & CFLAG_ODDITY)
        amusement = 255;

    if (player_in_a_dangerous_place())
        amusement *= 2;

    switch (card)
    {
    case CARD_XOM:
        // Handled elsewhere
        amusement = 0;
        break;

    case CARD_DAMNATION:
        // Nothing happened, boring.
        if (you.level_type != LEVEL_DUNGEON)
            amusement = 0;
        break;

    case CARD_MINEFIELD:
    case CARD_FAMINE:
    case CARD_CURSE:
    case CARD_SWINE:
        // Always hilarious.
        amusement = 255;

    default:
        break;
    }

    return amusement;
}

void evoke_deck(item_def& deck)
{
    if (_check_buggy_deck(deck))
        return;

    int brownie_points = 0;
    bool allow_id = in_inventory(deck) && !item_ident(deck, ISFLAG_KNOW_TYPE);

    const deck_rarity_type rarity = deck_rarity(deck);
    CrawlHashTable &props = deck.props;

    uint8_t flags = 0;
    card_type card = _draw_top_card(deck, true, flags);

    // Oddity cards don't give any information about the deck.
    if (flags & CFLAG_ODDITY)
        allow_id = false;

    // Passive Nemelex retribution: sometimes a card gets swapped out.
    // More likely to happen with marked decks.
    if (you.penance[GOD_NEMELEX_XOBEH])
    {
        int c = 1;
        if ((flags & (CFLAG_MARKED | CFLAG_SEEN))
            || props["num_marked"].get_byte() > 0)
        {
            c = 3;
        }

        if (x_chance_in_y(c * you.penance[GOD_NEMELEX_XOBEH], 3000))
        {
            card_type old_card = card;
            card = _choose_from_archetype(deck_of_punishment, rarity);
            if (card != old_card)
            {
                simple_god_message(" seems to have exchanged this card "
                                   "behind your back!", GOD_NEMELEX_XOBEH);
                mprf("It's actually %s.", card_name(card));
                // You never completely appease Nemelex, but the effects
                // get less frequent.
                you.penance[GOD_NEMELEX_XOBEH] -=
                    random2((you.penance[GOD_NEMELEX_XOBEH]+18) / 10);
            }
        }
    }

    const int amusement   = _xom_check_card(deck, card, flags);
    const bool no_brownie = (props["non_brownie_draws"].get_byte() > 0);

    // Do these before the deck item_def object is gone.
    if (flags & CFLAG_MARKED)
        props["num_marked"]--;
    if (no_brownie)
        props["non_brownie_draws"]--;

    deck.plus2++;
    _remember_drawn_card(deck, card, allow_id);

    // Get rid of the deck *before* the card effect because a card
    // might cause a wielded deck to be swapped out for something else,
    // in which case we don't want an empty deck to go through the
    // swapping process.
    const bool deck_gone = (cards_in_deck(deck) == 0);
    if (deck_gone)
    {
        mpr("The deck of cards disappears in a puff of smoke.");
        dec_inv_item_quantity(deck.link, 1);
        // Finishing the deck will earn a point, even if it
        // was marked or stacked.
        brownie_points++;
    }

    const bool fake_draw = !card_effect(card, rarity, flags, false);
    if (fake_draw && !deck_gone)
        props["non_brownie_draws"]++;

    if (!(flags & CFLAG_MARKED))
    {
        // Could a Xom worshipper ever get a stacked deck in the first
        // place?
        xom_is_stimulated(amusement);

        // Nemelex likes gamblers.
        if (!no_brownie)
        {
            brownie_points++;
            if (one_chance_in(3))
                brownie_points++;
        }

        // You can't ID off a marked card
        allow_id = false;
    }

    if (!deck_gone && allow_id
        && you.skill(SK_EVOCATIONS) > 5 + random2(35))
    {
        mpr("Your skill with magical items lets you identify the deck.");
        set_ident_flags(deck, ISFLAG_KNOW_TYPE);
        msg::streams(MSGCH_EQUIPMENT) << deck.name(DESC_INVENTORY)
                                      << std::endl;
    }

    if (!fake_draw)
        did_god_conduct(DID_CARDS, brownie_points);

    // Always wield change, since the number of cards used/left has
    // changed.
    you.wield_change = true;
}

int get_power_level(int power, deck_rarity_type rarity)
{
    int power_level = 0;
    switch (rarity)
    {
    case DECK_RARITY_COMMON:
        break;
    case DECK_RARITY_LEGENDARY:
        if (x_chance_in_y(power, 500))
            ++power_level;
        // deliberate fall-through
    case DECK_RARITY_RARE:
        if (x_chance_in_y(power, 700))
            ++power_level;
        break;
    }
    return power_level;
}

// Actual card implementations follow.
static void _portal_card(int power, deck_rarity_type rarity)
{
    const int control_level = get_power_level(power, rarity);
    bool instant = false;
    bool controlled = false;
    if (control_level >= 2)
    {
        instant = true;
        controlled = true;
    }
    else if (control_level == 1)
    {
        if (coinflip())
            instant = true;
        else
            controlled = true;
    }

    int threshold = 6;
    const bool was_controlled = player_control_teleport();
    const bool short_control = (you.duration[DUR_CONTROL_TELEPORT] > 0
                                && you.duration[DUR_CONTROL_TELEPORT]
                                                < threshold * BASELINE_DELAY);

    if (controlled && (!was_controlled || short_control))
        you.set_duration(DUR_CONTROL_TELEPORT, threshold); // Long enough to kick in.

    if (instant)
        you_teleport_now(true);
    else
        you_teleport();
}

static void _warp_card(int power, deck_rarity_type rarity)
{
    if (item_blocks_teleport(true, true))
    {
        canned_msg(MSG_STRANGE_STASIS);
        return;
    }

    const int control_level = get_power_level(power, rarity);
    if (control_level >= 2)
        blink(1000, false);
    else if (control_level == 1)
        cast_semi_controlled_blink(power / 4);
    else
        random_blink(false);
}

static void _swap_monster_card(int power, deck_rarity_type rarity)
{
    // Swap between you and another monster.
    // Don't choose yourself unless there are no monsters nearby.
    monster* mon_to_swap = choose_random_nearby_monster(0);
    if (!mon_to_swap)
        mpr("You spin around.");
    else
        swap_with_monster(mon_to_swap);
}

static void _velocity_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    if (power_level >= 2)
        potion_effect(POT_SPEED, random2(power / 4));
    else if (power_level == 1)
    {
        cast_fly(random2(power / 4));
        cast_swiftness(random2(power / 4));
    }
    else
        cast_swiftness(random2(power / 4));
}

static void _damnation_card(int power, deck_rarity_type rarity)
{
    if (you.level_type != LEVEL_DUNGEON)
    {
        canned_msg(MSG_NOTHING_HAPPENS);
        return;
    }

    // Calculate how many extra banishments you get.
    const int power_level = get_power_level(power, rarity);
    int nemelex_bonus = 0;
    if (you.religion == GOD_NEMELEX_XOBEH && !player_under_penance())
        nemelex_bonus = you.piety / 20;

    int extra_targets = power_level + random2(you.skill(SK_EVOCATIONS)
                                              + nemelex_bonus) / 12;

    for (int i = 0; i < 1 + extra_targets; ++i)
    {
        // Pick a random monster nearby to banish (or yourself).
        monster* mon_to_banish = choose_random_nearby_monster(1);

        // Bonus banishments only banish monsters.
        if (i != 0 && !mon_to_banish)
            continue;

        if (!mon_to_banish) // Banish yourself!
        {
            banished(DNGN_ENTER_ABYSS, "drawing a card");
            break;              // Don't banish anything else.
        }
        else
            mon_to_banish->banish();
    }

}

static void _warpwright_card(int power, deck_rarity_type rarity)
{
    if (you.level_type == LEVEL_ABYSS)
    {
        mpr("The power of the Abyss blocks your magic.");
        return;
    }

    int count = 0;
    coord_def f;
    for (adjacent_iterator ai(you.pos()); ai; ++ai)
        if (grd(*ai) == DNGN_FLOOR && !find_trap(*ai) && one_chance_in(++count))
            f = *ai;

    if (count > 0)              // found a spot
    {
        if (place_specific_trap(f, TRAP_TELEPORT))
        {
            // Mark it discovered if enough power.
            if (get_power_level(power, rarity) >= 1)
                find_trap(f)->reveal();
        }
    }
}

static void _flight_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);

    // Assume something _will_ happen.
    bool success = true;

    if (power_level == 0)
    {
        if (!transform(random2(power/4), coinflip() ? TRAN_SPIDER : TRAN_BAT,
                       true))
        {
            // Oops, something went wrong here (either because of cursed
            // equipment or the possibility of stat loss).
            success = false;
        }
    }
    else if (power_level >= 1)
    {
        cast_fly(random2(power/4));
        cast_swiftness(random2(power/4));
    }

    if (power_level == 2) // Stacks with the above.
    {
        if (is_valid_shaft_level() && grd(you.pos()) == DNGN_FLOOR)
        {
            if (place_specific_trap(you.pos(), TRAP_SHAFT))
            {
                find_trap(you.pos())->reveal();
                mpr("A shaft materialises beneath you!");
            }
        }
    }
    if (one_chance_in(4 - power_level))
        potion_effect(POT_INVISIBILITY, random2(power)/4);
    else if (!success)
        canned_msg(MSG_NOTHING_HAPPENS);
}

static void _minefield_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    const int radius = power_level * 2 + 2;
    for (radius_iterator ri(you.pos(), radius, false, false, false); ri; ++ri)
    {
        if (*ri == you.pos())
            continue;

        if (grd(*ri) == DNGN_FLOOR && !find_trap(*ri)
            && one_chance_in(4 - power_level))
        {
            if (you.level_type == LEVEL_ABYSS)
                grd(*ri) = coinflip() ? DNGN_DEEP_WATER : DNGN_LAVA;
            else
                place_specific_trap(*ri, TRAP_RANDOM);
        }
    }
}

static int stair_draw_count = 0;

// This does not describe an actual card. Instead, it only exists to test
// the stair movement effect in wizard mode ("&c stairs").
static void _stairs_card(int power, deck_rarity_type rarity)
{
    UNUSED(power);
    UNUSED(rarity);

    you.duration[DUR_REPEL_STAIRS_MOVE]  = 0;
    you.duration[DUR_REPEL_STAIRS_CLIMB] = 0;

    if (feat_stair_direction(grd(you.pos())) == CMD_NO_CMD)
        you.duration[DUR_REPEL_STAIRS_MOVE]  = 1000;
    else
        you.duration[DUR_REPEL_STAIRS_CLIMB] =  500; // more annoying

    std::vector<coord_def> stairs_avail;

    for (radius_iterator ri(you.pos(), LOS_RADIUS, false, true, true); ri; ++ri)
    {
        dungeon_feature_type feat = grd(*ri);
        if (feat_stair_direction(feat) != CMD_NO_CMD
            && feat != DNGN_ENTER_SHOP)
        {
            stairs_avail.push_back(*ri);
        }
    }

    if (stairs_avail.size() == 0)
    {
        mpr("No stairs available to move.");
        return;
    }

    std::random_shuffle(stairs_avail.begin(), stairs_avail.end());

    for (unsigned int i = 0; i < stairs_avail.size(); ++i)
        move_stair(stairs_avail[i], stair_draw_count % 2, false);

    stair_draw_count++;
}

static int _drain_monsters(coord_def where, int pow, int, actor *)
{
    if (where == you.pos())
        drain_exp();
    else
    {
        monster* mon = monster_at(where);
        if (mon == NULL)
            return (0);

        if (!mon->drain_exp(&you, false, pow / 50))
            simple_monster_message(mon, " is unaffected.");
    }

    return (1);
}

static void _mass_drain(int pow)
{
    apply_area_visible(_drain_monsters, pow, true);
}

// Return true if it was a "genuine" draw, i.e., there was a monster
// to target. This is still exploitable by finding popcorn monsters.
static bool _damaging_card(card_type card, int power, deck_rarity_type rarity)
{
    bool rc = there_are_monsters_nearby(true, false);
    const int power_level = get_power_level(power, rarity);

    dist target;
    zap_type ztype = ZAP_DEBUGGING_RAY;
    const zap_type firezaps[3]   = { ZAP_FLAME, ZAP_STICKY_FLAME, ZAP_FIRE };
    const zap_type frostzaps[3]  = { ZAP_FROST, ZAP_THROW_ICICLE, ZAP_COLD };
    const zap_type hammerzaps[3] = { ZAP_STONE_ARROW, ZAP_IRON_SHOT,
                                     ZAP_CRYSTAL_SPEAR };
    const zap_type venomzaps[3]  = { ZAP_STING, ZAP_VENOM_BOLT,
                                     ZAP_POISON_ARROW };
    const zap_type sparkzaps[3]  = { ZAP_ELECTRICITY, ZAP_LIGHTNING,
                                     ZAP_ORB_OF_ELECTRICITY };
    const zap_type painzaps[2]   = { ZAP_AGONY, ZAP_NEGATIVE_ENERGY };

    switch (card)
    {
    case CARD_VITRIOL:
        ztype = (one_chance_in(3) ? ZAP_DEGENERATION : ZAP_BREATHE_ACID);
        break;

    case CARD_FLAME:
        ztype = (coinflip() ? ZAP_FIREBALL : firezaps[power_level]);
        break;

    case CARD_FROST:  ztype = frostzaps[power_level];  break;
    case CARD_HAMMER: ztype = hammerzaps[power_level]; break;
    case CARD_VENOM:  ztype = venomzaps[power_level];  break;
    case CARD_SPARK:  ztype = sparkzaps[power_level];  break;

    case CARD_PAIN:
        if (power_level == 2)
        {
            mprf("You have drawn %s.", card_name(card));
            _mass_drain(power);
            return (true);
        }
        else
            ztype = painzaps[power_level];
        break;

    default:
        break;
    }

    std::string prompt = "You have drawn ";
    prompt += card_name(card);
    prompt += ".";

    bolt beam;
    beam.range = LOS_RADIUS;
    if (spell_direction(target, beam, DIR_NONE, TARG_HOSTILE,
                        LOS_RADIUS, true, true, false, NULL, prompt.c_str())
        && player_tracer(ZAP_DEBUGGING_RAY, power/4, beam))
    {
        zapping(ztype, random2(power/4), beam);
    }
    else
        rc = false;

    return (rc);
}

static void _elixir_card(int power, deck_rarity_type rarity)
{
    int power_level = get_power_level(power, rarity);

    if (power_level == 1 && you.hp * 2 > you.hp_max)
        power_level = 0;

    if (power_level == 0)
    {
        if (coinflip())
            potion_effect(POT_HEAL_WOUNDS, 40); // power doesn't matter
        else
            cast_regen(random2(power / 4));
    }
    else if (power_level == 1)
    {
        set_hp(you.hp_max, false);
        you.magic_points = 0;
    }
    else if (power_level >= 2)
    {
        set_hp(you.hp_max, false);
        you.magic_points = you.max_magic_points;
    }
    you.redraw_hit_points = true;
    you.redraw_magic_points = true;
}

static void _battle_lust_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    if (power_level >= 2)
    {
        you.set_duration(DUR_SLAYING, random2(power/6) + 1,
                         0, "You feel deadly.");
    }
    else if (power_level == 1)
    {
        you.set_duration(DUR_BUILDING_RAGE, 2,
                         0, "You feel your rage building.");
    }
    else if (power_level == 0)
        potion_effect(POT_MIGHT, random2(power/4));
}

static void _metamorphosis_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    transformation_type trans;

    if (power_level >= 2)
        trans = coinflip() ? TRAN_DRAGON : TRAN_LICH;
    else if (power_level == 1)
        trans = coinflip() ? TRAN_STATUE : TRAN_BLADE_HANDS;
    else
    {
        trans = one_chance_in(3) ? TRAN_SPIDER :
                coinflip()       ? TRAN_ICE_BEAST
                                 : TRAN_BAT;
    }

    // Might fail, e.g. because of cursed equipment or potential death by
    // stat loss. Aren't we being nice? (jpeg)
    if (!transform(random2(power/4), trans, true))
        canned_msg(MSG_NOTHING_HAPPENS);
}

static void _helm_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    bool do_phaseshift = false;
    bool do_stoneskin  = false;
    bool do_shield     = false;
    int num_resists = 0;

    // Chances are cumulative.
    if (power_level >= 2)
    {
        if (coinflip()) do_phaseshift = true;
        if (coinflip()) do_stoneskin  = true;
        if (coinflip()) do_shield     = true;
        num_resists = random2(4);
    }
    if (power_level >= 1)
    {
        if (coinflip()) do_phaseshift = true;
        if (coinflip()) do_stoneskin  = true;
        if (coinflip()) do_shield     = true;
    }
    if (power_level >= 0)
    {
        if (coinflip())
            do_phaseshift = true;
        else
            do_stoneskin  = true;
    }

    if (do_phaseshift)
        cast_phase_shift(random2(power/4));
    if (do_stoneskin)
        cast_stoneskin(random2(power/4));
    if (num_resists)
    {
        const duration_type possible_resists[4] = {
            DUR_RESIST_POISON, DUR_INSULATION,
            DUR_RESIST_FIRE, DUR_RESIST_COLD
        };
        const char* resist_names[4] = {
            "poison", "electricity", "fire", "cold"
        };

        for (int i = 0; i < 4 && num_resists; ++i)
        {
            // If there are n left, of which we need to choose
            // k, we have chance k/n of selecting the next item.
            if (x_chance_in_y(num_resists, 4-i))
            {
                // Add a temporary resistance.
                you.increase_duration(possible_resists[i], random2(power/7) +1);
                msg::stream << "You feel resistant to " << resist_names[i]
                            << '.' << std::endl;
                --num_resists;
            }
        }
    }

    if (do_shield)
    {
        if (you.duration[DUR_MAGIC_SHIELD] == 0)
            mpr("A magical shield forms in front of you.");
        you.increase_duration(DUR_MAGIC_SHIELD, random2(power/6) + 1);
    }
}

static void _blade_card(int power, deck_rarity_type rarity)
{
    if (you.species == SP_CAT)
    {
        mpr("You feel like a smilodon for a moment.");
        return;
    }

    // Pause before jumping to the list.
    if (Options.auto_list)
        more();

    wield_weapon(false);

    const int power_level = get_power_level(power, rarity);
    if (power_level >= 2)
    {
        cast_tukimas_dance(random2(power/4));
    }
    else if (power_level == 1)
    {
        cast_sure_blade(random2(power/4));
    }
    else
    {
        const brand_type brands[] = {
            SPWPN_FLAMING, SPWPN_FREEZING, SPWPN_VENOM, SPWPN_DRAINING,
            SPWPN_VORPAL, SPWPN_DISTORTION, SPWPN_PAIN, SPWPN_DUMMY_CRUSHING
        };

        if (!brand_weapon(RANDOM_ELEMENT(brands), random2(power/4)))
        {
            item_def* wpn = you.weapon();

            if (wpn)
            {
                mprf("%s vibrate%s crazily for a second.",
                     wpn->name(DESC_CAP_YOUR).c_str(),
                     wpn->quantity == 1 ? "s" : "");
            }
            else
                mprf("Your %s twitch.", your_hand(true).c_str());
        }
    }
}

static void _shadow_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);

    if (power_level >= 1)
    {
        mpr(you.duration[DUR_STEALTH] ? "You feel more catlike."
                                      : "You feel stealthy.");
        you.increase_duration(DUR_STEALTH, random2(power/4) + 1);
    }

    potion_effect(POT_INVISIBILITY, random2(power/4));
}

static void _potion_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    potion_type pot_effects[] = {
        POT_AGILITY, POT_AGILITY, POT_BRILLIANCE,
        POT_BRILLIANCE, POT_MIGHT, POT_MIGHT,
        POT_HEALING, POT_HEALING, POT_CONFUSION,
        POT_SLOWING, POT_PARALYSIS
    };

    potion_type pot = RANDOM_ELEMENT(pot_effects);

    if (power_level >= 1 && coinflip())
        pot = (coinflip() ? POT_MAGIC : POT_INVISIBILITY);

    if (power_level >= 2 && coinflip())
        pot = (coinflip() ? POT_SPEED : POT_RESISTANCE);

    potion_effect(pot, random2(power/4));
}

static void _focus_card(int power, deck_rarity_type rarity)
{
    stat_type best_stat = STAT_STR;
    stat_type worst_stat = STAT_STR;

    for (int i = 1; i < 3; ++i)
    {
        stat_type s = static_cast<stat_type>(i);
        const int best_diff = you.max_stat(s) - you.max_stat(best_stat);
        if (best_diff > 0 || best_diff == 0 && coinflip())
            best_stat = s;

        const int worst_diff = you.max_stat(s) - you.max_stat(worst_stat);
        if (worst_diff < 0 || worst_diff == 0 && coinflip())
            worst_stat = s;
    }

    while (best_stat == worst_stat)
    {
        best_stat  = static_cast<stat_type>(random2(3));
        worst_stat = static_cast<stat_type>(random2(3));
    }

    std::string cause = "the Focus card";

    if (crawl_state.is_god_acting())
    {
        god_type which_god = crawl_state.which_god_acting();
        if (crawl_state.is_god_retribution())
            cause = "the wrath of " + god_name(which_god);
        else
        {
            if (which_god == GOD_XOM)
                cause = "the capriciousness of Xom";
            else
                cause = "the 'helpfulness' of " + god_name(which_god);
        }
    }

    modify_stat(best_stat, 1, true, cause.c_str(), true);
    modify_stat(worst_stat, -1, true, cause.c_str(), true);
}

static void _shuffle_card(int power, deck_rarity_type rarity)
{
    int perm[NUM_STATS] = { 0, 1, 2 };
    std::random_shuffle(perm, perm + 3);

    FixedVector<int8_t, NUM_STATS> new_base;
    for (int i = 0; i < NUM_STATS; ++i)
        new_base[perm[i]]  = you.base_stats[i];

    std::string cause = "the Shuffle card";

    if (crawl_state.is_god_acting())
    {
        god_type which_god = crawl_state.which_god_acting();
        if (crawl_state.is_god_retribution())
            cause = "the wrath of " + god_name(which_god);
        else
        {
            if (which_god == GOD_XOM)
                cause = "the capriciousness of Xom";
            else
                cause = "the 'helpfulness' of " + god_name(which_god);
        }
    }

    for (int i = 0; i < NUM_STATS; ++i)
    {
        modify_stat(static_cast<stat_type>(i),
                    new_base[i] - you.base_stats[i],
                    true, cause.c_str(), true);
    }
}

static void _experience_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);

    if (you.experience_level < 27)
        mpr("You feel more experienced.");
    else
        mpr("You feel knowledgeable.");

    // Put some free XP into pool; power_level 2 means +20k
    int exp_gain = HIGH_EXP_POOL;
    if (power_level <= 1)
        exp_gain = std::min(exp_gain, power * 50);
    exp_gain -= ash_reduce_xp(exp_gain);
    you.exp_available += exp_gain;

    // After level 27, boosts you get don't get increased (matters for
    // charging V:8 with no rN+++ and for felids).
    const int xp_cap = exp_needed(1 + you.experience_level)
                     - exp_needed(you.experience_level);

    // power_level 2 means automatic level gain.
    if (power_level == 2 && you.experience_level < 27)
        adjust_level(1);
    else
    {
        // Likely to give a level gain (power of ~500 is reasonable
        // at high levels even for non-Nemelexites, so 50,000 XP.)
        // But not guaranteed.
        // Overrides archmagi effect, like potions of experience.
        you.experience += std::min(xp_cap, power * 100);
        level_change();
    }
}

static void _remove_bad_mutation()
{
    // Ensure that only bad mutations are removed.
    if (!delete_mutation(RANDOM_BAD_MUTATION, false, false, false, true))
        mpr("You feel transcendent for a moment.");
}

static void _helix_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);

    if (power_level == 0)
    {
        switch (how_mutated() ? random2(3) : 0)
        {
        case 0:
            mutate(RANDOM_MUTATION);
            break;
        case 1:
            delete_mutation(RANDOM_MUTATION);
            mutate(RANDOM_MUTATION);
            break;
        case 2:
            delete_mutation(RANDOM_MUTATION);
            break;
        }
    }
    else if (power_level == 1)
    {
        switch (how_mutated() ? random2(3) : 0)
        {
        case 0:
            mutate(coinflip() ? RANDOM_GOOD_MUTATION : RANDOM_MUTATION);
            break;
        case 1:
            if (coinflip())
                _remove_bad_mutation();
            else
                delete_mutation(RANDOM_MUTATION);
            break;
        case 2:
            if (coinflip())
            {
                if (coinflip())
                {
                    _remove_bad_mutation();
                    mutate(RANDOM_MUTATION);
                }
                else
                {
                    delete_mutation(RANDOM_MUTATION);
                    mutate(RANDOM_GOOD_MUTATION);
                }
            }
            else
            {
                delete_mutation(RANDOM_MUTATION);
                mutate(RANDOM_MUTATION);
            }
            break;
        }
    }
    else
    {
        switch (random2(3))
        {
        case 0:
            _remove_bad_mutation();
            break;
        case 1:
            mutate(RANDOM_GOOD_MUTATION);
            break;
        case 2:
            if (coinflip())
            {
                // If you get unlucky, you could get here with no bad
                // mutations and simply get a mutation effect. Oh well.
                _remove_bad_mutation();
                mutate(RANDOM_MUTATION);
            }
            else
            {
                delete_mutation(RANDOM_MUTATION);
                mutate(RANDOM_GOOD_MUTATION);
            }
            break;
        }
    }
}

void sage_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    int c;                      // how much to weight your skills
    if (power_level == 0)
        c = 0;
    else if (power_level == 1)
        c = random2(10) + 1;
    else
        c = 10;

    // FIXME: yet another reproduction of random_choose_weighted
    // Ah for Python:
    // skill = random_choice([x*(40-x)*c/10 for x in skill_levels])
    int totalweight = 0;
    skill_type result = SK_NONE;
    for (int i = SK_FIRST_SKILL; i < NUM_SKILLS; ++i)
    {
        skill_type s = static_cast<skill_type>(i);
        if (skill_name(s) == NULL || is_useless_skill(s))
            continue;

        if (you.skills[s] < MAX_SKILL_LEVEL)
        {
            // Choosing a skill is likelier if you are somewhat skilled in it.
            const int curweight = 1 + you.skills[s] * (40 - you.skills[s]) * c;
            totalweight += curweight;
            if (x_chance_in_y(curweight, totalweight))
                result = s;
        }
    }

    if (result == SK_NONE)
        mpr("You feel omnipotent.");  // All skills maxed.
    else
    {
        you.set_duration(DUR_SAGE, random2(1800) + 200);
        you.sage_bonus_skill   = result;
        you.sage_bonus_degree  = power / 25;
        mprf(MSGCH_PLAIN, "You feel studious about %s.", skill_name(result));
    }
}

void create_pond(const coord_def& center, int radius, bool allow_deep)
{
    for (radius_iterator ri(center, radius, false); ri; ++ri)
    {
        const coord_def p = *ri;
        if (p != you.pos() && coinflip())
        {
            if (grd(p) == DNGN_FLOOR)
            {
                dungeon_feature_type feat;

                if (allow_deep && coinflip())
                    feat = DNGN_DEEP_WATER;
                else
                    feat = DNGN_SHALLOW_WATER;

                dungeon_terrain_changed(p, feat);
            }
        }
    }
}

static void _deepen_water(const coord_def& center, int radius)
{
    for (radius_iterator ri(center, radius, false); ri; ++ri)
    {
        // FIXME The iteration shouldn't affect the later squares in the
        // same iteration, i.e., a newly-flooded square shouldn't count
        // in the decision as to whether to make the next square flooded.
        const coord_def p = *ri;
        if (grd(p) == DNGN_SHALLOW_WATER
            && p != you.pos()
            && x_chance_in_y(1+count_neighbours(p, DNGN_DEEP_WATER), 8))
        {
            dungeon_terrain_changed(p, DNGN_DEEP_WATER);
        }
        if (grd(p) == DNGN_FLOOR
            && random2(3) < random2(count_neighbours(p, DNGN_DEEP_WATER)
                                    + count_neighbours(p, DNGN_SHALLOW_WATER)))
        {
            dungeon_terrain_changed(p, DNGN_SHALLOW_WATER);
        }
    }
}

static void _water_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    if (power_level == 0)
    {
        mpr("You create a pond!");
        create_pond(you.pos(), 4, false);
    }
    else if (power_level == 1)
    {
        mpr("You feel the tide rushing in!");
        create_pond(you.pos(), 6, true);
        for (int i = 0; i < 2; ++i)
            _deepen_water(you.pos(), 6);
    }
    else
    {
        mpr("Water floods your area!");

        // Flood all visible squares.
        for (radius_iterator ri(you.pos(), LOS_RADIUS, false); ri; ++ri)
        {
            coord_def p = *ri;
            destroy_trap(p);
            if (grd(p) == DNGN_FLOOR)
            {
                dungeon_feature_type new_feature = DNGN_SHALLOW_WATER;
                if (p != you.pos() && coinflip())
                    new_feature = DNGN_DEEP_WATER;
                dungeon_terrain_changed(p, new_feature);
            }
        }
    }
}

static void _glass_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    const int radius = (power_level == 2) ? 1000
                                          : random2(power/40) + 2;
    vitrify_area(radius);
}

static void _dowsing_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    bool things_to_do[3] = { false, false, false };
    things_to_do[random2(3)] = true;

    if (power_level == 1)
        things_to_do[random2(3)] = true;

    if (power_level >= 2)
    {
        for (int i = 0; i < 3; ++i)
            things_to_do[i] = true;
    }

    if (things_to_do[0])
        cast_detect_secret_doors(random2(power/4));
    if (things_to_do[1])
        detect_traps(random2(power/4));
    if (things_to_do[2])
    {
        you.set_duration(DUR_TELEPATHY, random2(power/4), 0,
                         "You feel telepathic!");
        detect_creatures(1 + you.duration[DUR_TELEPATHY] / 2 / BASELINE_DELAY,
                         true);
    }
}

static void _trowel_card(int power, deck_rarity_type rarity)
{
    // Early exit: don't clobber important features.
    if (is_critical_feature(grd(you.pos())))
    {
        mpr("The dungeon trembles momentarily.");
        return;
    }

    const int power_level = get_power_level(power, rarity);
    bool done_stuff = false;

    // [ds] FIXME: Remove the LEVEL_DUNGEON restriction once Crawl
    // handles stacked level_area_types correctly. We should also
    // review whether Trowel being able to create infinite portal
    // vaults is a Good Thing, because it looks pretty broken to me.
    if (power_level >= 2 && you.level_type == LEVEL_DUNGEON
        && crawl_state.game_standard_levelgen())
    {
        // Generate a portal to something.
        const map_def *map = random_map_for_tag("trowel_portal");
        if (!map)
        {
            mpr("A buggy portal flickers into view, then vanishes.");
        }
        else
        {
            {
                no_messages n;
                dgn_safe_place_map(map, true, true, you.pos());
            }
            mpr("A mystic portal forms.");
        }
        done_stuff = true;
    }
    else if (power_level == 1)
    {
        if (coinflip())
        {
            // Create a random bad statue and a friendly, timed golem.
            // This could be really bad, because they're placed adjacent
            // to you...
            int num_made = 0;

            const monster_type statues[] = {
                MONS_ORANGE_STATUE, MONS_SILVER_STATUE, MONS_ICE_STATUE
            };

            if (create_monster(
                    mgen_data::hostile_at(
                        RANDOM_ELEMENT(statues), "the Trowel card",
                        true, 0, 0, you.pos())) != -1)
            {
                mpr("A menacing statue appears!");
                num_made++;
            }

            const monster_type golems[] = {
                MONS_CLAY_GOLEM, MONS_WOOD_GOLEM, MONS_STONE_GOLEM,
                MONS_IRON_GOLEM, MONS_CRYSTAL_GOLEM, MONS_TOENAIL_GOLEM
            };

            if (create_monster(
                    mgen_data(RANDOM_ELEMENT(golems),
                              BEH_FRIENDLY, &you, 5, 0,
                              you.pos(), MHITYOU)) != -1)
            {
                mpr("You construct a golem!");
                num_made++;
            }

            if (num_made == 2)
                mpr("The constructs glare at each other.");

            done_stuff = (num_made > 0);
        }
        else
        {
            // Do-nothing (effectively): create a cosmetic feature
            const coord_def pos = pick_adjacent_free_square(you.pos());
            if (in_bounds(pos))
            {
                const dungeon_feature_type statfeat[] = {
                    DNGN_GRANITE_STATUE, DNGN_ORCISH_IDOL
                };
                // We leave the items on the square
                grd(pos) = RANDOM_ELEMENT(statfeat);
                mpr("A statue takes form beside you.");
                done_stuff = true;
            }
        }
    }
    else
    {
        // Generate an altar.
        if (grd(you.pos()) == DNGN_FLOOR)
        {
            // Might get GOD_NO_GOD and no altar.
            god_type rgod = static_cast<god_type>(random2(NUM_GODS));

            if (rgod == GOD_JIYVA && jiyva_is_dead())
                rgod = GOD_NO_GOD;

            grd(you.pos()) = altar_for_god(rgod);

            if (grd(you.pos()) != DNGN_FLOOR)
            {
                done_stuff = true;
                mprf("An altar to %s grows from the floor before you!",
                     god_name(rgod).c_str());
            }
        }
    }

    if (!done_stuff)
        canned_msg(MSG_NOTHING_HAPPENS);
}

static void _genie_card(int power, deck_rarity_type rarity)
{
    if (coinflip())
    {
        mpr("A genie takes form and thunders: "
            "\"Choose your reward, mortal!\"");
        more();
        acquirement(OBJ_RANDOM, AQ_CARD_GENIE);
    }
    else
    {
        mpr("A genie takes form and thunders: "
            "\"You disturbed me, fool!\"");
        potion_effect(coinflip() ? POT_DEGENERATION : POT_DECAY, 40);
    }
}

// Special case for *your* god, maybe?
static void _godly_wrath()
{
    int tries = 100;
    while (tries-- > 0)
    {
        god_type god = static_cast<god_type>(random2(NUM_GODS - 1) + 1);

        // Don't recursively make player draw from the Deck of Punishment.
        if (god == GOD_NEMELEX_XOBEH)
            continue;

        // Stop once we find a god willing to punish the player.
        if (divine_retribution(god))
            break;
    }

    if (tries <= 0)
        mpr("You somehow manage to escape divine attention...");
}

static void _curse_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);

    mpr("You feel a malignant aura surround you.");
    if (power_level >= 2)
    {
        // Curse (almost) everything + chance of decay.
        while (curse_an_item(one_chance_in(6), true) && !one_chance_in(1000))
            ;
    }
    else if (power_level == 1)
    {
        // Curse an average of four items.
        do
            curse_an_item(false);
        while (!one_chance_in(4));
    }
    else
    {
        // Curse 1.5 items on average.
        curse_an_item(false);
        if (coinflip())
            curse_an_item(false);
    }
}

static void _crusade_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    if (power_level >= 1)
    {
        // A chance to convert opponents.
        for (monster_iterator mi(you.get_los()); mi; ++mi)
        {
             if (mi->friendly()
                || mi->holiness() != MH_NATURAL
                || mons_is_unique(mi->type)
                || mons_immune_magic(*mi)
                || player_will_anger_monster(*mi))
            {
                continue;
            }

            // Note that this bypasses the magic resistance
            // (though not immunity) check.  Specifically,
            // you can convert Killer Klowns this way.
            // Might be too good.
            if (mi->hit_dice * 35 < random2(power))
            {
                simple_monster_message(*mi, " is converted.");

                if (one_chance_in(5 - power_level))
                {
                    mi->attitude = ATT_FRIENDLY;

                    // If you worship a god that lets you recruit
                    // permanent followers, or a god allied with one,
                    // count this as a recruitment.
                    if (is_good_god(you.religion)
                        || you.religion == GOD_BEOGH
                            && mons_genus(mi->type) == MONS_ORC
                            && !mi->is_summoned()
                            && !mi->is_shapeshifter())
                    {
                        mons_make_god_gift(*mi, is_good_god(you.religion) ?
                                           GOD_SHINING_ONE : GOD_BEOGH);
                    }
                }
                else
                    mi->add_ench(ENCH_CHARM);
                mons_att_changed(*mi);
            }
        }
    }
    abjuration(power/4);
}

static void _summon_demon_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    demon_class_type dct;
    if (power_level >= 2)
        dct = DEMON_GREATER;
    else if (power_level == 1)
        dct = DEMON_COMMON;
    else
        dct = DEMON_LESSER;

    // FIXME: The manual testing for message printing is there because
    // we can't rely on create_monster() to do it for us. This is
    // because if you are completely surrounded by walls, create_monster()
    // will never manage to give a position which isn't (-1,-1)
    // and thus not print the message.
    // This hack appears later in this file as well.
    if (create_monster(
            mgen_data(summon_any_demon(dct), BEH_FRIENDLY, &you,
                      std::min(power/50 + 1, 5), 0,
                      you.pos(), MHITYOU),
            false) == -1)
    {
        mpr("You see a puff of smoke.");
    }
}

static void _summon_any_monster(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    monster_type mon_chosen = NUM_MONSTERS;
    coord_def chosen_spot;
    int num_tries;

    if (power_level == 0)
        num_tries = 1;
    else if (power_level == 1)
        num_tries = 4;
    else
        num_tries = 18;

    for (int i = 0; i < num_tries; ++i)
    {
        int dx, dy;
        do
        {
            dx = random2(3) - 1;
            dy = random2(3) - 1;
        }
        while (dx == 0 && dy == 0);

        coord_def delta(dx,dy);

        monster_type cur_try;
        do
        {
            cur_try = random_monster_at_grid(you.pos() + delta);
        }
        while (mons_is_unique(cur_try));

        if (mon_chosen == NUM_MONSTERS
            || mons_power(mon_chosen) < mons_power(cur_try))
        {
            mon_chosen = cur_try;
            chosen_spot = you.pos();
        }
    }

    if (mon_chosen == NUM_MONSTERS) // Should never happen.
        return;

    const bool friendly = (power_level > 0 || !one_chance_in(4));

    if (create_monster(mgen_data(mon_chosen,
                                 friendly ? BEH_FRIENDLY : BEH_HOSTILE, &you,
                                 3, 0, chosen_spot, MHITYOU),
                       false) == -1)
    {
        mpr("You see a puff of smoke.");
    }
}

static void _summon_dancing_weapon(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    const bool friendly   = (power_level > 0 || !one_chance_in(4));

    const int mon =
        create_monster(
            mgen_data(MONS_DANCING_WEAPON,
                      friendly ? BEH_FRIENDLY : BEH_HOSTILE, &you,
                      power_level + 2, 0, you.pos(), MHITYOU),
            false);

    // Given the abundance of Nemelex decks, not setting hard reset
    // leaves a trail of weapons behind, most of which just get
    // offered to Nemelex again, adding an unnecessary source of
    // piety.
    if (mon != -1)
    {
        // Override the weapon.
        ASSERT(menv[mon].weapon() != NULL);
        item_def& wpn(*menv[mon].weapon());

        // FIXME: Mega-hack (breaks encapsulation too).
        wpn.flags &= ~ISFLAG_RACIAL_MASK;

        if (power_level == 0)
        {
            // Wimpy, negative-enchantment weapon.
            wpn.plus  = -random2(4);
            wpn.plus2 = -random2(4);
            wpn.sub_type = (coinflip() ? WPN_SHORT_SWORD : WPN_HAMMER);

            set_item_ego_type(wpn, OBJ_WEAPONS, SPWPN_NORMAL);
        }
        else if (power_level == 1)
        {
            // This is getting good.
            wpn.plus  = random2(4) - 1;
            wpn.plus2 = random2(4) - 1;
            wpn.sub_type = (coinflip() ? WPN_LONG_SWORD : WPN_TRIDENT);

            if (coinflip())
            {
                set_item_ego_type(wpn, OBJ_WEAPONS,
                                  coinflip() ? SPWPN_FLAMING : SPWPN_FREEZING);
            }
            else
                set_item_ego_type(wpn, OBJ_WEAPONS, SPWPN_NORMAL);
        }
        else if (power_level == 2)
        {
            // Rare and powerful.
            wpn.plus  = random2(4) + 2;
            wpn.plus2 = random2(4) + 2;
            wpn.sub_type = (coinflip() ? WPN_KATANA : WPN_EXECUTIONERS_AXE);

            set_item_ego_type(wpn, OBJ_WEAPONS,
                              coinflip() ? SPWPN_SPEED : SPWPN_ELECTROCUTION);
        }

        item_colour(wpn);

        menv[mon].flags |= MF_HARD_RESET;

        ghost_demon newstats;
        newstats.init_dancing_weapon(wpn, power / 4);

        menv[mon].set_ghost(newstats);
        menv[mon].dancing_weapon_init();
    }
    else
        mpr("You see a puff of smoke.");
}

static void _summon_flying(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);

    const monster_type flytypes[] = {
        MONS_BUTTERFLY, MONS_BUMBLEBEE, MONS_INSUBSTANTIAL_WISP,
        MONS_VAMPIRE_MOSQUITO, MONS_VAPOUR, MONS_YELLOW_WASP,
        MONS_RED_WASP
    };

    // Choose what kind of monster.
    // Be nice and don't summon invisibles with no SInv.
    monster_type result = MONS_PROGRAM_BUG;
    do
        result = flytypes[random2(5) + power_level];
    while (mons_class_flag(result, M_INVIS) && !you.can_see_invisible());

    for (int i = 0; i < power_level * 5 + 2; ++i)
    {
        const bool friendly = (!one_chance_in(power_level + 4));

        create_monster(
            mgen_data(result,
                      friendly ? BEH_FRIENDLY : BEH_HOSTILE, &you,
                      std::min(power/50 + 1, 5), 0,
                      you.pos(), MHITYOU));
    }
}

static void _summon_skeleton(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    const bool friendly = (power_level > 0 || !one_chance_in(4));
    const monster_type skeltypes[] = {
        MONS_SKELETON_LARGE, MONS_SKELETAL_WARRIOR, MONS_BONE_DRAGON
    };

    if (create_monster(mgen_data(skeltypes[power_level],
                                 friendly ? BEH_FRIENDLY : BEH_HOSTILE, &you,
                                 std::min(power/50 + 1, 5), 0,
                                 you.pos(), MHITYOU),
                       false) == -1)
    {
        mpr("You see a puff of smoke.");
    }
}

static void _summon_ugly(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    const bool friendly = (power_level > 0 || !one_chance_in(4));
    monster_type ugly;
    if (power_level >= 2)
        ugly = MONS_VERY_UGLY_THING;
    else if (power_level == 1)
        ugly = coinflip() ? MONS_VERY_UGLY_THING : MONS_UGLY_THING;
    else
        ugly = MONS_UGLY_THING;

    if (create_monster(mgen_data(ugly,
                                 friendly ? BEH_FRIENDLY : BEH_HOSTILE,
                                 &you,
                                 std::min(power/50 + 1, 6), 0,
                                 you.pos(), MHITYOU),
                       false) == -1)
    {
        mpr("You see a puff of smoke.");
    }
}

static void _alchemist_card(int power, deck_rarity_type rarity)
{
    const int power_level = get_power_level(power, rarity);
    int gold_used = std::min(you.gold, random2avg(100, 2) * (1 + power_level));
    bool done_stuff = false;

    you.del_gold(gold_used);
    dprf("%d gold available to spend.", gold_used);

    // Spend some gold to regain health
    int hp = std::min(gold_used / 3, you.hp_max - you.hp);
    if (hp > 0)
    {
        inc_hp(hp, false);
        gold_used -= hp * 2;
        done_stuff = true;
        mpr("You feel better.");
        dprf("Gained %d health, %d gold remaining.", hp, gold_used);
    }

    // Maybe spend some more gold to regain magic
    if (x_chance_in_y(power_level + 1, 5))
    {
        int mp = std::min(gold_used / 6, you.max_magic_points - you.magic_points);
        if (mp > 0)
        {
            inc_mp(mp, false);
            gold_used -= mp * 4;
            done_stuff = true;
            mpr("You feel your power returning.");
            dprf("Gained %d magic, %d gold remaining.", mp, gold_used);
        }
    }

    if (done_stuff)
        mpr("Some of your gold vanishes!");
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    // Add back any remaining gold
    you.add_gold(gold_used);
}

static int _card_power(deck_rarity_type rarity)
{
    int result = 0;

    if (you.penance[GOD_NEMELEX_XOBEH])
    {
        result -= you.penance[GOD_NEMELEX_XOBEH];
    }
    else if (you.religion == GOD_NEMELEX_XOBEH)
    {
        result = you.piety;
        result *= (you.skill(SK_EVOCATIONS) + 25);
        result /= 27;
    }

    result += you.skill(SK_EVOCATIONS) * 9;
    if (rarity == DECK_RARITY_RARE)
        result += 150;
    else if (rarity == DECK_RARITY_LEGENDARY)
        result += 300;

    if (result < 0)
        result = 0;

    return (result);
}

bool card_effect(card_type which_card, deck_rarity_type rarity,
                 uint8_t flags, bool tell_card)
{
    ASSERT(!_card_forbidden(which_card));

    bool rc = true;
    const int power = _card_power(rarity);

    const god_type god =
        (crawl_state.is_god_acting()) ? crawl_state.which_god_acting()
                                      : GOD_NO_GOD;

#ifdef DEBUG_DIAGNOSTICS
    msg::streams(MSGCH_DIAGNOSTICS) << "Card power: " << power
                                    << ", rarity: " << static_cast<int>(rarity)
                                    << std::endl;
#endif

    if (tell_card)
    {
        // These card types will usually give this message in the targeting
        // prompt, and the cases where they don't are handled specially.
        if (which_card != CARD_VITRIOL && which_card != CARD_FLAME
            && which_card != CARD_FROST && which_card != CARD_HAMMER
            && which_card != CARD_SPARK && which_card != CARD_PAIN
            && which_card != CARD_VENOM)
        {
           mprf("You have drawn %s.", card_name(which_card));
        }
    }

    if (which_card == CARD_XOM && !crawl_state.is_god_acting())
    {
        if (you.religion == GOD_XOM)
        {
            // Being a self-centered deity, Xom *always* finds this
            // maximally hilarious.
            god_speaks(GOD_XOM, "Xom roars with laughter!");
            you.gift_timeout = 255;
        }
        else if (you.penance[GOD_XOM])
            god_speaks(GOD_XOM, "Xom laughs nastily.");
    }

    switch (which_card)
    {
    case CARD_PORTAL:           _portal_card(power, rarity); break;
    case CARD_WARP:             _warp_card(power, rarity); break;
    case CARD_SWAP:             _swap_monster_card(power, rarity); break;
    case CARD_VELOCITY:         _velocity_card(power, rarity); break;
    case CARD_DAMNATION:        _damnation_card(power, rarity); break;
    case CARD_SOLITUDE:         cast_dispersal(power/4); break;
    case CARD_ELIXIR:           _elixir_card(power, rarity); break;
    case CARD_BATTLELUST:       _battle_lust_card(power, rarity); break;
    case CARD_METAMORPHOSIS:    _metamorphosis_card(power, rarity); break;
    case CARD_HELM:             _helm_card(power, rarity); break;
    case CARD_BLADE:            _blade_card(power, rarity); break;
    case CARD_SHADOW:           _shadow_card(power, rarity); break;
    case CARD_POTION:           _potion_card(power, rarity); break;
    case CARD_FOCUS:            _focus_card(power, rarity); break;
    case CARD_SHUFFLE:          _shuffle_card(power, rarity); break;
    case CARD_EXPERIENCE:       _experience_card(power, rarity); break;
    case CARD_HELIX:            _helix_card(power, rarity); break;
    case CARD_SAGE:             sage_card(power, rarity); break;
    case CARD_WATER:            _water_card(power, rarity); break;
    case CARD_GLASS:            _glass_card(power, rarity); break;
    case CARD_DOWSING:          _dowsing_card(power, rarity); break;
    case CARD_MINEFIELD:        _minefield_card(power, rarity); break;
    case CARD_STAIRS:           _stairs_card(power, rarity); break;
    case CARD_GENIE:            _genie_card(power, rarity); break;
    case CARD_CURSE:            _curse_card(power, rarity); break;
    case CARD_WARPWRIGHT:       _warpwright_card(power, rarity); break;
    case CARD_FLIGHT:           _flight_card(power, rarity); break;
    case CARD_TOMB:             entomb(power); break;
    case CARD_WRAITH:           adjust_level(-1); break;
    case CARD_WRATH:            _godly_wrath(); break;
    case CARD_CRUSADE:          _crusade_card(power, rarity); break;
    case CARD_SUMMON_DEMON:     _summon_demon_card(power, rarity); break;
    case CARD_SUMMON_ANIMAL:    summon_animals(random2(power/3)); break;
    case CARD_SUMMON_ANY:       _summon_any_monster(power, rarity); break;
    case CARD_SUMMON_WEAPON:    _summon_dancing_weapon(power, rarity); break;
    case CARD_SUMMON_FLYING:    _summon_flying(power, rarity); break;
    case CARD_SUMMON_SKELETON:  _summon_skeleton(power, rarity); break;
    case CARD_SUMMON_UGLY:      _summon_ugly(power, rarity); break;
    case CARD_XOM:              xom_acts(5 + random2(power/10)); break;
    case CARD_TROWEL:           _trowel_card(power, rarity); break;
    case CARD_SPADE:            your_spells(SPELL_DIG, random2(power/4), false); break;
    case CARD_BANSHEE:          mass_enchantment(ENCH_FEAR, power); break;
    case CARD_TORMENT:          torment(TORMENT_CARDS, you.pos()); break;
    case CARD_ALCHEMIST:        _alchemist_card(power, rarity); break;

    case CARD_VENOM:
        if (coinflip())
        {
            mprf("You have drawn %s.", card_name(which_card));
            your_spells(SPELL_OLGREBS_TOXIC_RADIANCE, random2(power/4), false);
        }
        else
            rc = _damaging_card(which_card, power, rarity);
        break;

    case CARD_VITRIOL:
    case CARD_FLAME:
    case CARD_FROST:
    case CARD_HAMMER:
    case CARD_SPARK:
    case CARD_PAIN:
        rc = _damaging_card(which_card, power, rarity);
        break;

    case CARD_BARGAIN:
        you.increase_duration(DUR_BARGAIN,
                              random2(power) + random2(power) + 2);
        break;

    case CARD_MAP:
        if (!magic_mapping(random2(power/10) + 15, random2(power), true))
            mpr("The map is blank.");
        break;

    case CARD_WILD_MAGIC:
        // Yes, high power is bad here.
        MiscastEffect(&you, god == GOD_NO_GOD ? NON_MONSTER : -god,
                       SPTYP_RANDOM, random2(power/15) + 5, random2(power),
                       "a card of wild magic");
        break;

    case CARD_FAMINE:
        if (you.is_undead == US_UNDEAD)
            mpr("You feel rather smug.");
        else
            set_hunger(500, true);
        break;

    case CARD_FEAST:
        if (you.is_undead == US_UNDEAD)
            mpr("You feel a horrible emptiness.");
        else
            set_hunger(12000, true);
        break;

    case CARD_SWINE:
        if (!transform(1 + power/2 + random2(power/2), TRAN_PIG, true))
        {
            mpr("You feel like a pig.");
            break;
        }
        break;

    case NUM_CARDS:
        // The compiler will complain if any card remains unhandled.
        mpr("You have drawn a buggy card!");
        break;
    }

    if (you.religion == GOD_NEMELEX_XOBEH && !rc)
        simple_god_message(" does not approve of your wasteful card use.");

    return rc;
}

bool top_card_is_known(const item_def &deck)
{
    if (!is_deck(deck))
        return (false);

    uint8_t flags;
    get_card_and_flags(deck, -1, flags);

    return (flags & CFLAG_MARKED);
}

card_type top_card(const item_def &deck)
{
    if (!is_deck(deck))
        return (NUM_CARDS);

    uint8_t flags;
    card_type card = get_card_and_flags(deck, -1, flags);

    UNUSED(flags);

    return (card);
}

bool is_deck(const item_def &item)
{
    return (item.base_type == OBJ_MISCELLANY
            && item.sub_type >= MISC_DECK_OF_ESCAPE
            && item.sub_type <= MISC_DECK_OF_DEFENCE);
}

bool bad_deck(const item_def &item)
{
    if (!is_deck(item))
        return (false);

    return (!item.props.exists("cards")
            || item.props["cards"].get_type() != SV_VEC
            || item.props["cards"].get_vector().get_type() != SV_BYTE
            || cards_in_deck(item) == 0);
}

deck_rarity_type deck_rarity(const item_def &item)
{
    ASSERT(is_deck(item));

    return static_cast<deck_rarity_type>(item.special);
}

uint8_t deck_rarity_to_color(deck_rarity_type rarity)
{
    switch (rarity)
    {
    case DECK_RARITY_COMMON:
    {
        const uint8_t colours[] = {LIGHTBLUE, GREEN, CYAN, RED};
        return RANDOM_ELEMENT(colours);
    }

    case DECK_RARITY_RARE:
        return (coinflip() ? MAGENTA : BROWN);

    case DECK_RARITY_LEGENDARY:
        return LIGHTMAGENTA;
    }

    return (WHITE);
}

void init_deck(item_def &item)
{
    CrawlHashTable &props = item.props;

    ASSERT(is_deck(item));
    ASSERT(!props.exists("cards"));
    ASSERT(item.plus > 0);
    ASSERT(item.plus <= 127);
    ASSERT(item.special >= DECK_RARITY_COMMON
           && item.special <= DECK_RARITY_LEGENDARY);

    const store_flags fl = SFLAG_CONST_TYPE;

    props["cards"].new_vector(SV_BYTE, fl).resize((vec_size)item.plus);
    props["card_flags"].new_vector(SV_BYTE, fl).resize((vec_size)item.plus);
    props["drawn_cards"].new_vector(SV_BYTE, fl);

    for (int i = 0; i < item.plus; ++i)
    {
        bool      was_odd = false;
        card_type card    = _random_card(item, was_odd);

        uint8_t flags = 0;
        if (was_odd)
            flags = CFLAG_ODDITY;

        _set_card_and_flags(item, i, card, flags);
    }

    ASSERT(cards_in_deck(item) == item.plus);

    props["num_marked"]        = (char) 0;
    props["non_brownie_draws"] = (char) 0;

    props.assert_validity();

    item.plus2  = 0;
    item.colour = deck_rarity_to_color((deck_rarity_type) item.special);
}

static void _unmark_deck(item_def& deck)
{
    if (!is_deck(deck))
        return;

    CrawlHashTable &props = deck.props;
    if (!props.exists("card_flags"))
        return;

    CrawlVector &flags = props["card_flags"].get_vector();

    for (unsigned int i = 0; i < flags.size(); ++i)
    {
        flags[i] =
            static_cast<char>((static_cast<char>(flags[i]) & ~CFLAG_MARKED));
    }

    // We'll be mean and leave non_brownie_draws as-is.
    props["num_marked"] = static_cast<char>(0);
}

static void _unmark_and_shuffle_deck(item_def& deck)
{
    if (is_deck(deck))
    {
        _unmark_deck(deck);
        _shuffle_deck(deck);
    }
}

void shuffle_all_decks_on_level()
{
    for (int i = 0; i < MAX_ITEMS; ++i)
    {
        item_def& item(mitm[i]);
        if (item.defined() && is_deck(item))
        {
#ifdef DEBUG_DIAGNOSTICS
            mprf(MSGCH_DIAGNOSTICS, "Shuffling: %s on level %d, branch %d",
                 item.name(DESC_PLAIN).c_str(),
                 static_cast<int>(you.absdepth0),
                 static_cast<int>(you.where_are_you));
#endif
            _unmark_and_shuffle_deck(item);
        }
    }
}

static bool _shuffle_inventory_decks()
{
    bool success = false;

    for (int i = 0; i < ENDOFPACK; ++i)
    {
        item_def& item(you.inv[i]);
        if (item.defined() && is_deck(item))
        {
#ifdef DEBUG_DIAGNOSTICS
            mprf(MSGCH_DIAGNOSTICS, "Shuffling in inventory: %s",
                 item.name(DESC_PLAIN).c_str());
#endif
            _unmark_and_shuffle_deck(item);

            success = true;
        }
    }

    return success;
}

void nemelex_shuffle_decks()
{
    add_daction(DACT_SHUFFLE_DECKS);
    _shuffle_inventory_decks();

    // Wildly inaccurate, but of similar quality as the old code which
    // was triggered by the presence of any deck anywhere.
    if (you.num_total_gifts[GOD_NEMELEX_XOBEH])
        god_speaks(GOD_NEMELEX_XOBEH, "You hear Nemelex Xobeh chuckle.");
}
