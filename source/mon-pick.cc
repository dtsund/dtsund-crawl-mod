/**
 * @file
 * @brief Functions used to help determine which monsters should appear.
**/

#include "AppHdr.h"

#include "mon-pick.h"

#include "externs.h"
#include "branch.h"
#include "mon-util.h"
#include "place.h"

// NB - When adding new branches or levels above 50, you must
// change pre-game deletion routine in new_game in newgame.cc

//    New branch must be added in:
//    - new_game stair location
//    - down/up stairs (to and back) misc.cc
//    - new_level (2 places) (misc.cc)
//    - item_check items.cc
//    - ouch ouch.cc (death message)
//    - and here...

// NOTE: The lower the level the earlier a monster may appear.
int mons_level(int mcls, const level_id &place)
{
    int monster_level = 0;

    if (place.level_type == LEVEL_ABYSS)
        monster_level = ((mons_abyss(mcls)) ? place.absdepth() : 0);
    else if (place.level_type == LEVEL_PANDEMONIUM)
        monster_level = ((mons_pan(mcls)) ? place.absdepth() : 0);
    else if (place.level_type == LEVEL_DUNGEON)
        monster_level = branches[place.branch].mons_level_function(mcls);

    return monster_level;
}

// NOTE: Higher values returned means the monster is "more common".
// A return value of zero means the monster will never appear. {dlb}
int mons_rarity(int mcls, const level_id &place)
{
    // now, what about pandemonium ??? {dlb}
    if (place.level_type == LEVEL_ABYSS)
        return mons_rare_abyss(mcls);
    else
        return branches[place.branch].mons_rarity_function(mcls);
}

// level_area_type != LEVEL_DUNGEON
// NOTE: Labyrinths and portal vaults have no random monster generation.

// The Abyss
bool mons_abyss(int mcls)
{
    switch (mcls)
    {
    case MONS_ABOMINATION_LARGE:
    case MONS_ABOMINATION_SMALL:
    case MONS_AIR_ELEMENTAL:
    case MONS_ANCIENT_LICH:
    case MONS_ANGEL:
    case MONS_BALRUG:
    case MONS_BLUE_DEATH:
    case MONS_BLUE_DEVIL:
    case MONS_BONE_DRAGON:
    case MONS_BRAIN_WORM:
    case MONS_CACODEMON:
    case MONS_CHAOS_SPAWN:
    case MONS_CLAY_GOLEM:
    case MONS_CRYSTAL_GOLEM:
    case MONS_DAEVA:
    case MONS_DANCING_WEAPON:
    case MONS_DEMONIC_CRAWLER:
    case MONS_EARTH_ELEMENTAL:
    case MONS_EFREET:
    case MONS_EIDOLON:
    case MONS_EXECUTIONER:
    case MONS_EYE_OF_DEVASTATION:
    case MONS_EYE_OF_DRAINING:
    case MONS_FIRE_BAT:
    case MONS_FIRE_ELEMENTAL:
    case MONS_FLAMING_CORPSE:
    case MONS_FLAYED_GHOST:
    case MONS_FLYING_SKULL:
    case MONS_FREEZING_WRAITH:
    case MONS_DEATH_DRAKE:
    case MONS_FUNGUS:
    case MONS_GIANT_EYEBALL:
    case MONS_GIANT_ORANGE_BRAIN:
    case MONS_GIANT_SPORE:
    case MONS_GREAT_ORB_OF_EYES:
    case MONS_GOLDEN_EYE:
    case MONS_GREEN_DEATH:
    case MONS_GUARDIAN_SERPENT:
    case MONS_HAIRY_DEVIL:
    case MONS_HELLEPHANT:
    case MONS_HELLION:
    case MONS_HELLWING:
    case MONS_HELL_HOG:
    case MONS_HELL_HOUND:
    case MONS_HELL_KNIGHT:
    case MONS_HUNGRY_GHOST:
    case MONS_ICE_BEAST:
    case MONS_ICE_DEVIL:
    case MONS_IMP:
    case MONS_INSUBSTANTIAL_WISP:
    case MONS_IRON_DEVIL:
    case MONS_IRON_GOLEM:
    case MONS_IRON_IMP:
    case MONS_JELLY:
    case MONS_LEMURE:
    case MONS_LICH:
    case MONS_LOROCYPROCA:
    case MONS_MIDGE:
    case MONS_MUMMY:
    case MONS_NAGA_MAGE:
    case MONS_NAGA_WARRIOR:
    case MONS_NECROMANCER:
    case MONS_NECROPHAGE:
    case MONS_NEQOXEC:
    case MONS_ORANGE_DEMON:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_PHANTOM:
    case MONS_PIT_FIEND:
    case MONS_RAKSHASA:
    case MONS_REAPER:
    case MONS_RED_DEVIL:
    case MONS_ROTTING_DEVIL:
    case MONS_SHADOW:
    case MONS_SHADOW_DEMON:
    case MONS_SHADOW_IMP:
    case MONS_SHINING_EYE:
    case MONS_SILENT_SPECTRE:
    case MONS_SIXFIRHY:
    case MONS_SKELETAL_WARRIOR:
    case MONS_SKELETON_LARGE:
    case MONS_SKELETON_SMALL:
    case MONS_SKY_BEAST:
    case MONS_SMOKE_DEMON:
    case MONS_SOUL_EATER:
    case MONS_SPINY_WORM:
    case MONS_STONE_GOLEM:
    case MONS_SUN_DEMON:
    case MONS_TENTACLED_MONSTROSITY:
    case MONS_TOENAIL_GOLEM:
    case MONS_TORMENTOR:
    case MONS_UFETUBUS:
    case MONS_UGLY_THING:
    case MONS_UNSEEN_HORROR:
    case MONS_VAMPIRE:
    case MONS_VAPOUR:
    case MONS_VERY_UGLY_THING:
    case MONS_WHITE_IMP:
    case MONS_WIGHT:
    case MONS_WIZARD:
    case MONS_WOOD_GOLEM:
    case MONS_WRAITH:
    case MONS_YNOXINUL:
    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
    case MONS_SIMULACRUM_LARGE:
    case MONS_SIMULACRUM_SMALL:
    case MONS_MOTTLED_DRACONIAN:
    case MONS_YELLOW_DRACONIAN:
    case MONS_BLACK_DRACONIAN:
    case MONS_WHITE_DRACONIAN:
    case MONS_RED_DRACONIAN:
    case MONS_PURPLE_DRACONIAN:
    case MONS_PALE_DRACONIAN:
    case MONS_GREEN_DRACONIAN:
    case MONS_GREY_DRACONIAN:
    case MONS_DRACONIAN_CALLER:
    case MONS_DRACONIAN_MONK:
    case MONS_DRACONIAN_SCORCHER:
    case MONS_DRACONIAN_KNIGHT:
    case MONS_DRACONIAN_ANNIHILATOR:
    case MONS_DRACONIAN_ZEALOT:
    case MONS_DRACONIAN_SHIFTER:
    case MONS_DEEP_DWARF:
    case MONS_DEEP_DWARF_SCION:
    case MONS_DEEP_DWARF_ARTIFICER:
    case MONS_DEEP_DWARF_NECROMANCER:
    case MONS_DEEP_DWARF_BERSERKER:
    case MONS_DEEP_DWARF_DEATH_KNIGHT:
    case MONS_UNBORN_DEEP_DWARF:
        return (true);
    default:
        return (false);
    }
}

int mons_rare_abyss(int mcls)
{
    switch (mcls)
    {
    case MONS_ABOMINATION_LARGE:
    case MONS_ABOMINATION_SMALL:
        return 99;

    case MONS_IRON_IMP:
    case MONS_LEMURE:
    case MONS_MIDGE:
    case MONS_UFETUBUS:
    case MONS_WHITE_IMP:
        return 80;

    case MONS_HELLWING:
    case MONS_NEQOXEC:
    case MONS_ORANGE_DEMON:
    case MONS_SMOKE_DEMON:
    case MONS_YNOXINUL:
        return 50;

    case MONS_SKELETON_LARGE:
    case MONS_SKELETON_SMALL:
    case MONS_SKELETAL_WARRIOR:
        return 40;

    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
        return 35;

    case MONS_BONE_DRAGON:
    case MONS_SIXFIRHY:
        return 20;

    case MONS_EFREET:
        return 18;

    case MONS_RAKSHASA:
        return 17;

    case MONS_BRAIN_WORM:
        return 16;

    case MONS_FLYING_SKULL:
    case MONS_FREEZING_WRAITH:
    case MONS_GIANT_ORANGE_BRAIN:
    case MONS_VERY_UGLY_THING:
    case MONS_WRAITH:
        return 15;

    case MONS_EYE_OF_DRAINING:
    case MONS_FIRE_BAT:
    case MONS_FLAMING_CORPSE:
    case MONS_LICH:
        return 14;

    case MONS_INSUBSTANTIAL_WISP:
    case MONS_UNSEEN_HORROR:
    case MONS_GOLDEN_EYE:
        return 12;

    case MONS_HELL_HOG:
    case MONS_HELL_HOUND:
    case MONS_HUNGRY_GHOST:
    case MONS_SHADOW:
        return 11;

    case MONS_BALRUG:
    case MONS_BLUE_DEATH:
    case MONS_BLUE_DEVIL:
    case MONS_CACODEMON:
    case MONS_CHAOS_SPAWN:
    case MONS_DEMONIC_CRAWLER:
    case MONS_EXECUTIONER:
    case MONS_GREEN_DEATH:
    case MONS_GUARDIAN_SERPENT:
    case MONS_HAIRY_DEVIL:
    case MONS_HELLION:
    case MONS_ICE_DEVIL:
    case MONS_IMP:
    case MONS_LOROCYPROCA:
    case MONS_MUMMY:
    case MONS_NECROPHAGE:
    case MONS_ROTTING_DEVIL:
    case MONS_SHADOW_DEMON:
    case MONS_SHADOW_IMP:
    case MONS_SUN_DEMON:
    case MONS_WIGHT:
        return 10;

    case MONS_ICE_BEAST:
    case MONS_JELLY:
    case MONS_TORMENTOR:
    case MONS_VAMPIRE:
    case MONS_VAPOUR:
    case MONS_SIMULACRUM_LARGE:
    case MONS_SIMULACRUM_SMALL:
    case MONS_SKY_BEAST:
        return 9;

    case MONS_EIDOLON:
    case MONS_FUNGUS:
    case MONS_GIANT_EYEBALL:
    case MONS_PHANTOM:
    case MONS_REAPER:
        return 8;

    case MONS_DAEVA:
    case MONS_SOUL_EATER:
        return 7;

    case MONS_ANGEL:
    case MONS_IRON_DEVIL:
        return 6;

    case MONS_ANCIENT_LICH:
    case MONS_CLAY_GOLEM:
    case MONS_GREAT_ORB_OF_EYES:
    case MONS_IRON_GOLEM:
    case MONS_NAGA_MAGE:
    case MONS_NAGA_WARRIOR:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_PIT_FIEND:
    case MONS_RED_DEVIL:
    case MONS_SHINING_EYE:
    case MONS_SPINY_WORM:
    case MONS_STONE_GOLEM:
    case MONS_TENTACLED_MONSTROSITY:
    case MONS_WIZARD:
    case MONS_WOOD_GOLEM:
    case MONS_DEATH_DRAKE:
    case MONS_SILENT_SPECTRE:
    case MONS_DEEP_DWARF:
    case MONS_DEEP_DWARF_SCION:
    case MONS_DEEP_DWARF_ARTIFICER:
    case MONS_DEEP_DWARF_NECROMANCER:
    case MONS_DEEP_DWARF_BERSERKER:
    case MONS_DEEP_DWARF_DEATH_KNIGHT:
    case MONS_UNBORN_DEEP_DWARF:
        return 5;

    case MONS_AIR_ELEMENTAL:
    case MONS_EARTH_ELEMENTAL:
    case MONS_FIRE_ELEMENTAL:
    case MONS_FLAYED_GHOST:
        return 4;

    case MONS_CRYSTAL_GOLEM:
    case MONS_EYE_OF_DEVASTATION:
    case MONS_HELL_KNIGHT:
    case MONS_NECROMANCER:
    case MONS_UGLY_THING:
    case MONS_MOTTLED_DRACONIAN:
    case MONS_YELLOW_DRACONIAN:
    case MONS_BLACK_DRACONIAN:
    case MONS_WHITE_DRACONIAN:
    case MONS_RED_DRACONIAN:
    case MONS_PURPLE_DRACONIAN:
    case MONS_PALE_DRACONIAN:
    case MONS_GREEN_DRACONIAN:
    case MONS_GREY_DRACONIAN:
    case MONS_DRACONIAN_CALLER:
    case MONS_DRACONIAN_MONK:
    case MONS_DRACONIAN_SCORCHER:
    case MONS_DRACONIAN_KNIGHT:
    case MONS_DRACONIAN_ANNIHILATOR:
    case MONS_DRACONIAN_ZEALOT:
    case MONS_DRACONIAN_SHIFTER:
    case MONS_HELLEPHANT:
        return 3;

    case MONS_DANCING_WEAPON:
    case MONS_GIANT_SPORE:
        return 2;

    case MONS_TOENAIL_GOLEM:
        return 1;

    default:
        return 0;
    }
}

// Pandemonium
bool mons_pan(int mcls)
{
    switch (mcls)
    {
    // icky monsters
    case MONS_BRAIN_WORM:
    case MONS_FUNGUS:
    case MONS_GIANT_ORANGE_BRAIN:
    case MONS_JELLY:
    case MONS_SLIME_CREATURE:
    // undead
    case MONS_FLAMING_CORPSE:
    case MONS_FLAYED_GHOST:
    case MONS_FLYING_SKULL:
    case MONS_FREEZING_WRAITH:
    case MONS_HUNGRY_GHOST:
    case MONS_LICH:
    case MONS_MUMMY:
    case MONS_NECROPHAGE:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_PHANTOM:
    case MONS_SHADOW:
    case MONS_SKELETON_LARGE:
    case MONS_SKELETON_SMALL:
    case MONS_SIMULACRUM_LARGE:
    case MONS_SIMULACRUM_SMALL:
    case MONS_VAMPIRE:
    case MONS_WIGHT:
    case MONS_WRAITH:
    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
    // "things"
    case MONS_ABOMINATION_LARGE:
    case MONS_ABOMINATION_SMALL:
    case MONS_INSUBSTANTIAL_WISP:
    case MONS_PULSATING_LUMP:
    case MONS_UNSEEN_HORROR:
    // eyes
    case MONS_EYE_OF_DRAINING:
    case MONS_GIANT_EYEBALL:
    case MONS_GREAT_ORB_OF_EYES:
    case MONS_GOLDEN_EYE:
    // malign beings
    case MONS_EFREET:
    case MONS_RAKSHASA:
    // golems
    case MONS_CLAY_GOLEM:
    case MONS_CRYSTAL_GOLEM:
    case MONS_IRON_GOLEM:
    case MONS_STONE_GOLEM:
    case MONS_TOENAIL_GOLEM:
    case MONS_WOOD_GOLEM:
    // dragons
    case MONS_MOTTLED_DRAGON:
    // elementals
    case MONS_AIR_ELEMENTAL:
    case MONS_EARTH_ELEMENTAL:
    case MONS_FIRE_ELEMENTAL:
    // humanoids
    case MONS_NECROMANCER:
    case MONS_STONE_GIANT:
    case MONS_WIZARD:
    // demons
    case MONS_BALRUG:
    case MONS_BLUE_DEATH:
    case MONS_CACODEMON:
    case MONS_CHAOS_SPAWN:
    case MONS_DEMONIC_CRAWLER:
    case MONS_EXECUTIONER:
    case MONS_GREEN_DEATH:
    case MONS_HELLWING:
    case MONS_IRON_IMP:
    case MONS_LEMURE:
    case MONS_MIDGE:
    case MONS_NEQOXEC:
    case MONS_ORANGE_DEMON:
    case MONS_SIXFIRHY:
    case MONS_SMOKE_DEMON:
    case MONS_UFETUBUS:
    case MONS_WHITE_IMP:
    case MONS_YNOXINUL:
        return (true);
    default:
        return (false);
    }
}

/* ******************** END EXTERNAL FUNCTIONS ******************** */

// LEVEL_DUNGEON

// The Main Dungeon
int mons_standard_level(int mcls)
{
    switch (mcls)
    {
    case MONS_GOBLIN:
    case MONS_GIANT_NEWT:
        return 1;

    case MONS_GIANT_COCKROACH:
    case MONS_OOZE:
    case MONS_SMALL_SNAKE:
        return 2;

    case MONS_MEGABAT:
    case MONS_KOBOLD:
    case MONS_RAT:
        return 4;

    case MONS_GIANT_GECKO:
    case MONS_GIANT_MITE:
    case MONS_GNOLL:
    case MONS_HOBGOBLIN:
    case MONS_JACKAL:
    case MONS_KILLER_BEE_LARVA:
        return 5;

    case MONS_WORM:
    case MONS_SNAKE:
    case MONS_QUOKKA:
        return 6;

    case MONS_ORC:
    case MONS_ORC_PRIEST:
        return 7;

    case MONS_FUNGUS:
    case MONS_WORKER_ANT:
    case MONS_GIANT_EYEBALL:
    case MONS_HOUND:
    case MONS_IGUANA:
    case MONS_OGRE:
    case MONS_ORC_WIZARD:
    case MONS_PHANTOM:
    case MONS_SCORPION:
    case MONS_SKELETON_SMALL:
        return 8;

    case MONS_WATER_MOCCASIN:
    case MONS_CENTAUR:
    case MONS_ICE_BEAST:
    case MONS_IMP:
    case MONS_JELLY:
    case MONS_NECROPHAGE:
    case MONS_QUASIT:
    case MONS_ZOMBIE_SMALL:
    case MONS_SKY_BEAST:
        return 9;

    case MONS_DEEP_ELF_SOLDIER:
    case MONS_GOLIATH_BEETLE:
    case MONS_GIANT_FROG:
    case MONS_GIANT_SPORE:
    case MONS_MUMMY:
    case MONS_ORC_WARRIOR:
    case MONS_STEAM_DRAGON:
    case MONS_WIGHT:
        return 10;

    case MONS_CROCODILE:
    case MONS_HIPPOGRIFF:
    case MONS_HUNGRY_GHOST:
    case MONS_KILLER_BEE:
    case MONS_SHADOW:
    case MONS_YELLOW_WASP:
        return 11;

    case MONS_EYE_OF_DRAINING:
    case MONS_GILA_MONSTER:
    case MONS_MANTICORE:
    case MONS_PLANT:
    case MONS_WYVERN:
    // item mimics
    case MONS_ARMOUR_MIMIC:
    case MONS_GOLD_MIMIC:
    case MONS_POTION_MIMIC:
    case MONS_SCROLL_MIMIC:
    case MONS_WEAPON_MIMIC:
        return 12;

    case MONS_BIG_KOBOLD:
    case MONS_GIANT_TOAD:
    case MONS_GIANT_CENTIPEDE:
    case MONS_OKLOB_PLANT:
    case MONS_TROLL:
    case MONS_TWO_HEADED_OGRE:
    case MONS_WOOD_GOLEM:
    case MONS_YAK:
        return 13;

    case MONS_HILL_GIANT:
    case MONS_KOMODO_DRAGON:
    case MONS_SOLDIER_ANT:
    case MONS_WOLF_SPIDER:
    case MONS_WRAITH:
    case MONS_UNSEEN_HORROR:
    case MONS_TRAPDOOR_SPIDER:
        return 14;

    case MONS_BRAIN_WORM:
    case MONS_CYCLOPS:
    case MONS_EFREET:
    case MONS_EYE_OF_DEVASTATION:
    case MONS_HYDRA:
    case MONS_MOTTLED_DRAGON:
    case MONS_SKELETAL_WARRIOR:
    // feature mimics
    // case MONS_PORTAL_MIMIC:
    case MONS_STAIR_MIMIC:
    case MONS_SHOP_MIMIC:
    case MONS_FOUNTAIN_MIMIC:
        return 15;

    case MONS_BLINK_FROG:
    case MONS_BUTTERFLY:
    case MONS_VAMPIRE_MOSQUITO:
    case MONS_GUARDIAN_SERPENT:
    case MONS_RAKSHASA:
    case MONS_SLIME_CREATURE:
    case MONS_STONE_GOLEM:
    case MONS_VAMPIRE:
    case MONS_WANDERING_MUSHROOM:
    case MONS_ZOMBIE_LARGE:
        return 16;

    case MONS_BOGGART:
    case MONS_CENTAUR_WARRIOR:
    case MONS_CLAY_GOLEM:
    case MONS_GRIFFON:
    case MONS_SHAPESHIFTER:
    case MONS_UGLY_THING:
    case MONS_WIZARD:
    case MONS_SIMULACRUM_SMALL:
    case MONS_SIMULACRUM_LARGE:
    case MONS_ROCK_WORM:
        return 17;

    case MONS_DRAGON:
    case MONS_GARGOYLE:
    case MONS_GIANT_AMOEBA:
    case MONS_KOBOLD_DEMONOLOGIST:
    case MONS_SKELETON_LARGE: // Higher than actual threat level so that they will still show up in The Vaults
        return 18;

    case MONS_GIANT_SLUG:
    case MONS_IRON_GOLEM:
    case MONS_OGRE_MAGE:
    case MONS_ROCK_TROLL:
    case MONS_TOENAIL_GOLEM:
    case MONS_YAKTAUR:
        return 19;

    case MONS_AIR_ELEMENTAL:
    case MONS_DEEP_ELF_FIGHTER:
    case MONS_DEEP_ELF_KNIGHT:
    case MONS_DEEP_ELF_MAGE:
    case MONS_DEEP_ELF_SUMMONER:
    case MONS_EARTH_ELEMENTAL:
    case MONS_FIRE_ELEMENTAL:
    case MONS_GIANT_ORANGE_BRAIN:
    case MONS_AGATE_SNAIL:
    case MONS_GREAT_ORB_OF_EYES:
    case MONS_ICE_DRAGON:
    case MONS_NAGA_MAGE:
    case MONS_NAGA_WARRIOR:
    case MONS_NECROMANCER:
    case MONS_ORC_KNIGHT:
    case MONS_QUEEN_BEE:
    case MONS_RED_WASP:
    case MONS_SHADOW_WRAITH:
    case MONS_SPINY_WORM:
    case MONS_VERY_UGLY_THING:
    case MONS_HARPY:
    case MONS_FIRE_CRAB:
        return 20;

    case MONS_BOULDER_BEETLE:
    case MONS_ORC_HIGH_PRIEST:
    case MONS_PULSATING_LUMP:
        return 21;

    case MONS_BORING_BEETLE:
    case MONS_CRYSTAL_GOLEM:
    case MONS_FLAYED_GHOST:
    case MONS_FREEZING_WRAITH:
    case MONS_REDBACK:
    case MONS_SPHINX:
    case MONS_VAPOUR:
        return 22;

    case MONS_ORC_SORCERER:
    case MONS_SHINING_EYE:
        return 23;

    case MONS_BUMBLEBEE:
    case MONS_ORC_WARLORD:
    case MONS_IRON_TROLL:
    case MONS_YAKTAUR_CAPTAIN:
        return 24;

    case MONS_DANCING_WEAPON:
    case MONS_DEEP_TROLL:
    case MONS_FIRE_GIANT:
    case MONS_FROST_GIANT:
    case MONS_HELL_KNIGHT:
    case MONS_INSUBSTANTIAL_WISP:
    case MONS_LICH:
    case MONS_STONE_GIANT:
    case MONS_ETTIN:
        return 25;

    case MONS_DEEP_ELF_CONJURER:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_STORM_DRAGON:
        return 26;

    case MONS_DEEP_ELF_PRIEST:
    case MONS_GLOWING_SHAPESHIFTER:
    case MONS_TENTACLED_MONSTROSITY:
        return 27;

    case MONS_ANCIENT_LICH:
    case MONS_BONE_DRAGON:
    case MONS_DEEP_ELF_ANNIHILATOR:
    case MONS_DEEP_ELF_DEATH_MAGE:
    case MONS_DEEP_ELF_DEMONOLOGIST:
    case MONS_DEEP_ELF_HIGH_PRIEST:
    case MONS_DEEP_ELF_SORCERER:
    case MONS_GOLDEN_DRAGON:
    case MONS_IRON_DRAGON:
    case MONS_QUICKSILVER_DRAGON:
    case MONS_SHADOW_DRAGON:
    case MONS_TITAN:
        return 30;

    case MONS_DEEP_ELF_BLADEMASTER:
    case MONS_DEEP_ELF_MASTER_ARCHER:
        return 33;

    case MONS_BIG_FISH:
    case MONS_ELECTRIC_EEL:
    case MONS_GIANT_GOLDFISH:
    case MONS_JELLYFISH:
    case MONS_LAVA_FISH:
    case MONS_LAVA_SNAKE:
    case MONS_LAVA_WORM:
    case MONS_SWAMP_WORM:
    case MONS_WATER_ELEMENTAL:
        return 500;

    default:
        return 99;
    }
}

int mons_standard_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_BIG_FISH:
    case MONS_ELECTRIC_EEL:
    case MONS_GIANT_GOLDFISH:
    case MONS_JELLYFISH:
    case MONS_LAVA_FISH:
    case MONS_LAVA_SNAKE:
    case MONS_LAVA_WORM:
    case MONS_SWAMP_WORM:
    case MONS_WATER_ELEMENTAL:
    case MONS_SALAMANDER:
        return 500;

    case MONS_MEGABAT:
    case MONS_GIANT_FROG:
    case MONS_GOBLIN:
    case MONS_HILL_GIANT:
    case MONS_HOBGOBLIN:
    case MONS_IMP:
    case MONS_KOBOLD:
    case MONS_SKELETON_LARGE:
    case MONS_ORC:
    case MONS_RAT:
    case MONS_RED_DEVIL:
    case MONS_SKELETON_SMALL:
    case MONS_UGLY_THING:
    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
        return 99;

    case MONS_CENTAUR_WARRIOR:
    case MONS_WORKER_ANT:
    case MONS_SNAKE:
        return 80;

    case MONS_MERFOLK:
    case MONS_MERMAID:
    case MONS_FLYING_SKULL:
    case MONS_SLIME_CREATURE:
        return 75;

    case MONS_HELL_HOUND:
        return 71;

    case MONS_CENTAUR:
    case MONS_CYCLOPS:
    case MONS_GIANT_TOAD:
    case MONS_HELLION:
    case MONS_HOUND:
    case MONS_OGRE:
    case MONS_ORC_WARRIOR:
    case MONS_TROLL:
    case MONS_YAK:
    case MONS_YAKTAUR_CAPTAIN:
        return 70;

    case MONS_JELLY:
    case MONS_ORC_KNIGHT:
    case MONS_ROTTING_DEVIL:
        return 60;

    case MONS_SHAPESHIFTER:
        return 59;

    case MONS_STONE_GIANT:
        return 53;

    case MONS_BIG_KOBOLD:
    case MONS_GOLIATH_BEETLE:
    case MONS_VAMPIRE_MOSQUITO:
    case MONS_GIANT_COCKROACH:
    case MONS_GIANT_GECKO:
    case MONS_IGUANA:
    case MONS_GIANT_NEWT:
    case MONS_HIPPOGRIFF:
    case MONS_HYDRA:
    case MONS_ICE_BEAST:
    case MONS_SKY_BEAST:
    case MONS_KILLER_BEE:
    case MONS_ORC_WIZARD:
    case MONS_QUOKKA:
    case MONS_SCORPION:
    case MONS_TORMENTOR:
    case MONS_UNSEEN_HORROR:
    case MONS_WORM:
        return 50;

    case MONS_ROCK_TROLL:
        return 48;

    case MONS_MANTICORE:
    case MONS_OGRE_MAGE:
        return 45;

    case MONS_HUNGRY_GHOST:
    case MONS_SHADOW:
        return 41;

    case MONS_GIANT_CENTIPEDE:
    case MONS_GIANT_EYEBALL:
    case MONS_GIANT_SPORE:
    case MONS_GRIFFON:
    case MONS_HAIRY_DEVIL:
    case MONS_JACKAL:
    case MONS_MOTTLED_DRAGON:
    case MONS_PHANTOM:
    case MONS_REAPER:
    case MONS_TWO_HEADED_OGRE:
    case MONS_WIGHT:
    case MONS_WRAITH:
    case MONS_WYVERN:
    case MONS_YAKTAUR:
        return 40;

    case MONS_WOLF_SPIDER:
        return 36;

    case MONS_FREEZING_WRAITH:
    case MONS_GIANT_AMOEBA:
    case MONS_GILA_MONSTER:
    case MONS_GLOWING_SHAPESHIFTER:
    case MONS_SOLDIER_ANT:
        return 35;

    case MONS_BOULDER_BEETLE:
        return 34;

    case MONS_EYE_OF_DRAINING:
    case MONS_TRAPDOOR_SPIDER:
    case MONS_ROCK_WORM:
        return 33;

    case MONS_GIANT_SLUG:
        return 32;

    case MONS_WATER_MOCCASIN:
    case MONS_DRAGON:
    case MONS_ETTIN:
    case MONS_FIRE_VORTEX:
    case MONS_CROCODILE:
    case MONS_GIANT_MITE:
    case MONS_GNOLL:
    case MONS_KOMODO_DRAGON:
    case MONS_MUMMY:
    case MONS_NECROPHAGE:
    case MONS_QUASIT:
    case MONS_SKELETAL_WARRIOR:
    case MONS_SMALL_SNAKE:
    case MONS_SOUL_EATER:
    case MONS_SPINY_WORM:
    case MONS_VAMPIRE:
    case MONS_YELLOW_WASP:
        return 30;

    case MONS_FLAYED_GHOST:
        return 29;

    case MONS_BRAIN_WORM:
        return 26;

    case MONS_BOGGART:
    case MONS_DEEP_ELF_FIGHTER:
    case MONS_DEEP_ELF_KNIGHT:
    case MONS_DEEP_TROLL:
    case MONS_FIRE_GIANT:
    case MONS_FROST_GIANT:
    case MONS_GREAT_ORB_OF_EYES:
    case MONS_IRON_TROLL:
    case MONS_OOZE:
    case MONS_ORC_PRIEST:
    case MONS_PLANT:
    case MONS_RED_WASP:
    case MONS_SIMULACRUM_SMALL:
    case MONS_SIMULACRUM_LARGE:
    // item mimics
    case MONS_ARMOUR_MIMIC:
    case MONS_GOLD_MIMIC:
    case MONS_POTION_MIMIC:
    case MONS_SCROLL_MIMIC:
    case MONS_WEAPON_MIMIC:
    // feature mimics
    // case MONS_PORTAL_MIMIC:
    case MONS_STAIR_MIMIC:
    case MONS_SHOP_MIMIC:
    case MONS_FOUNTAIN_MIMIC:
        return 25;

    case MONS_BUTTERFLY:
    case MONS_FUNGUS:
    case MONS_AGATE_SNAIL:
    case MONS_ICE_DRAGON:
    case MONS_INSUBSTANTIAL_WISP:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_RAKSHASA:
    case MONS_REDBACK:
    case MONS_SHADOW_DRAGON:
    case MONS_SPHINX:
    case MONS_STEAM_DRAGON:
    case MONS_STORM_DRAGON:
    case MONS_VERY_UGLY_THING:
    case MONS_WIZARD:
    case MONS_HARPY:
    case MONS_FIRE_CRAB:
        return 20;

    case MONS_BORING_BEETLE:
    case MONS_LICH:
    case MONS_TENTACLED_MONSTROSITY:
        return 17;

    case MONS_BLINK_FROG:
    case MONS_CLAY_GOLEM:
    case MONS_EFREET:
    case MONS_EYE_OF_DEVASTATION:
    case MONS_NECROMANCER:
    case MONS_WOOD_GOLEM:
        return 15;

    case MONS_KOBOLD_DEMONOLOGIST:
        return 13;

    case MONS_BUMBLEBEE:
    case MONS_ORC_HIGH_PRIEST:
        return 12;

    case MONS_DEEP_ELF_SOLDIER:
    case MONS_GIANT_ORANGE_BRAIN:
    case MONS_HELL_KNIGHT:
    case MONS_IRON_GOLEM:
    case MONS_OKLOB_PLANT:
    case MONS_ORC_SORCERER:
    case MONS_SHADOW_WRAITH:
    case MONS_STONE_GOLEM:
    case MONS_TITAN:
    case MONS_WANDERING_MUSHROOM:
        return 10;

    case MONS_GOLDEN_DRAGON:
    case MONS_ORC_WARLORD:
        return 7;

    case MONS_GARGOYLE:
        return 6;

    case MONS_BONE_DRAGON:
    case MONS_CRYSTAL_GOLEM:
    case MONS_DANCING_WEAPON:
    case MONS_DEEP_ELF_HIGH_PRIEST:
    case MONS_DEEP_ELF_MAGE:
    case MONS_DEEP_ELF_SUMMONER:
    case MONS_IRON_DRAGON:
    case MONS_NAGA_MAGE:
    case MONS_NAGA_WARRIOR:
    case MONS_QUICKSILVER_DRAGON:
    case MONS_VAPOUR:
        return 5;

    case MONS_AIR_ELEMENTAL:
    case MONS_DEEP_ELF_CONJURER:
    case MONS_EARTH_ELEMENTAL:
    case MONS_FIRE_ELEMENTAL:
        return 4;

    case MONS_ANCIENT_LICH:
    case MONS_DEEP_ELF_ANNIHILATOR:
    case MONS_DEEP_ELF_DEATH_MAGE:
    case MONS_DEEP_ELF_DEMONOLOGIST:
    case MONS_DEEP_ELF_PRIEST:
    case MONS_DEEP_ELF_SORCERER:
    case MONS_GUARDIAN_SERPENT:
        return 3;

    case MONS_PULSATING_LUMP:
    case MONS_SHINING_EYE:
    case MONS_TOENAIL_GOLEM:
        return 2;

    default:
        return 0;
    }
}

// The Dwarven Hall
int mons_dwarf_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_DWARVEN_HALL, 1);

    if (!mons_dwarf_rare(mcls))
        return mlev + 99;
    // Depths are irrelevant for a depth-1 branch.
    return mlev + 1;
}

int mons_dwarf_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_DEEP_DWARF:
        return 100;
    case MONS_DEEP_DWARF_SCION:
    case MONS_DEEP_DWARF_ARTIFICER:
        return 70;
    case MONS_DEEP_DWARF_NECROMANCER:
    case MONS_DEEP_DWARF_BERSERKER:
    case MONS_DEEP_DWARF_DEATH_KNIGHT:
        return 60;
    case MONS_UNBORN_DEEP_DWARF:
        return 40;
    case MONS_WRAITH:
        return 30;
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_EIDOLON:
    case MONS_FIRE_GIANT:
    case MONS_FROST_GIANT:
    case MONS_STONE_GIANT:
        return 5;
    case MONS_DEEP_TROLL:
    case MONS_IRON_TROLL:
    case MONS_ROCK_TROLL:
    case MONS_SHADOW_WRAITH:
        return 2;

    default:
        return 0;
    }
}

// The Orcish Mines
int mons_mineorc_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_ORCISH_MINES, 1);

    switch (mcls)
    {
    case MONS_HOBGOBLIN:
    case MONS_ORC_PRIEST:
    case MONS_ORC_WARRIOR:
        mlev++;
        break;

    case MONS_GNOLL:
    case MONS_OGRE:
    case MONS_WARG:
    case MONS_ORC_KNIGHT:
    case MONS_ORC_WIZARD:
        mlev += 2;
        break;

    case MONS_CYCLOPS:
    case MONS_IRON_TROLL:
    case MONS_OGRE_MAGE:
    case MONS_ORC_HIGH_PRIEST:
    case MONS_ORC_SORCERER:
    case MONS_ORC_WARLORD:
    case MONS_ROCK_TROLL:
    case MONS_STONE_GIANT:
    case MONS_TROLL:
    case MONS_TWO_HEADED_OGRE:
    case MONS_ETTIN:
        mlev += 3;
        break;

    case MONS_FUNGUS:
    case MONS_GOBLIN:
    case MONS_ORC:
    default:
        mlev += 0;
    }

    return (mlev);
}

int mons_mineorc_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_ORC:
        return 300;

    case MONS_GOBLIN:
    case MONS_ORC_WARRIOR:
        return 30;

    case MONS_HOBGOBLIN:
    case MONS_OGRE:
        return 20;

    case MONS_TROLL:
    case MONS_WARG:
        return 13;

    case MONS_FUNGUS:
    case MONS_ORC_KNIGHT:
    case MONS_ORC_PRIEST:
    case MONS_ORC_SORCERER:
    case MONS_ORC_WIZARD:
        return 10;

    case MONS_ORC_WARLORD:
    case MONS_ORC_HIGH_PRIEST:
    case MONS_CYCLOPS:
    case MONS_TWO_HEADED_OGRE:
        return 5;

    case MONS_ETTIN:
    case MONS_IRON_TROLL:
    case MONS_ROCK_TROLL:
    case MONS_STONE_GIANT:
        return 3;

    case MONS_GNOLL:
        return 2;

    case MONS_OGRE_MAGE:
        return 1;

    default:
        return 0;
    }
}

// The Elven Halls
int mons_hallelf_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_ELVEN_HALLS, 1);

    switch (mcls)
    {
    case MONS_DEEP_ELF_SOLDIER:
    case MONS_DEEP_ELF_FIGHTER:
    case MONS_ORC:
    case MONS_ORC_WARRIOR:
    case MONS_ORC_WIZARD:
    case MONS_DEEP_ELF_MAGE:
    case MONS_DEEP_ELF_SUMMONER:
    case MONS_FUNGUS:
    case MONS_DEEP_ELF_CONJURER:
    case MONS_SHAPESHIFTER:
    case MONS_ORC_KNIGHT:
        mlev += 1;
        break;

    case MONS_ORC_SORCERER:
    case MONS_DEEP_ELF_PRIEST:
    case MONS_GLOWING_SHAPESHIFTER:
    case MONS_DEEP_ELF_KNIGHT:
        mlev += 2;
        break;

    case MONS_ORC_PRIEST:
    case MONS_ORC_HIGH_PRIEST:
        mlev += 3;
        break;

    case MONS_DEEP_ELF_HIGH_PRIEST:
    case MONS_DEEP_ELF_DEMONOLOGIST:
    case MONS_DEEP_ELF_ANNIHILATOR:
    case MONS_DEEP_ELF_SORCERER:
    case MONS_DEEP_ELF_DEATH_MAGE:
        mlev += 5;
        break;

    case MONS_DEEP_ELF_BLADEMASTER:
    case MONS_DEEP_ELF_MASTER_ARCHER:
        mlev += 8;
        break;

    default:
        mlev += 99;
        break;
    }

    return (mlev);
}

int mons_hallelf_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_FUNGUS:
        return 300;

    case MONS_DEEP_ELF_SOLDIER:
    case MONS_DEEP_ELF_FIGHTER:
    case MONS_DEEP_ELF_MAGE:
        return 100;

    case MONS_DEEP_ELF_KNIGHT:
        return 80;

    case MONS_DEEP_ELF_SUMMONER:
        return 72;

    case MONS_DEEP_ELF_CONJURER:
        return 63;

    case MONS_DEEP_ELF_PRIEST:
        return 44;

    case MONS_SHAPESHIFTER:
        return 25;

    case MONS_ORC:
        return 20;

    case MONS_DEEP_ELF_DEMONOLOGIST:
    case MONS_DEEP_ELF_SORCERER:
        return 17;

    case MONS_DEEP_ELF_ANNIHILATOR:
    case MONS_DEEP_ELF_DEATH_MAGE:
    case MONS_ORC_WIZARD:
        return 13;

    case MONS_ORC_WARRIOR:
        return 11;

    case MONS_DEEP_ELF_HIGH_PRIEST:
    case MONS_ORC_SORCERER:
    case MONS_GLOWING_SHAPESHIFTER:
        return 10;

    case MONS_ORC_KNIGHT:
    case MONS_ORC_PRIEST:
    case MONS_ORC_HIGH_PRIEST:
        return 5;

    case MONS_DEEP_ELF_BLADEMASTER:
    case MONS_DEEP_ELF_MASTER_ARCHER:
        return 1;

    default:
        return 0;
    }
}

// The Lair
int mons_lair_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_LAIR, 1);

    switch (mcls)
    {
    case MONS_GIANT_GECKO:
    case MONS_MEGABAT:
    case MONS_JACKAL:
    case MONS_GIANT_NEWT:
    case MONS_RAT:
    case MONS_QUOKKA:
    case MONS_GIANT_CENTIPEDE:
    case MONS_IGUANA:
        mlev += 0;
        break;

    case MONS_GIANT_FROG:
    case MONS_GILA_MONSTER:
    case MONS_PORCUPINE:
    case MONS_HOUND:
    case MONS_BLACK_BEAR:
    case MONS_WORM:
    case MONS_WOLF:
        mlev++;
        break;

    case MONS_FUNGUS:
    case MONS_GIANT_TOAD:
    case MONS_CROCODILE:
    case MONS_GIANT_MITE:
    case MONS_GREEN_RAT:
    case MONS_SCORPION:
    case MONS_SNAKE:
        mlev += 2;
        break;

    case MONS_WATER_MOCCASIN:
    case MONS_BUTTERFLY:
    case MONS_GOLIATH_BEETLE:
    case MONS_GIANT_SLUG:
    case MONS_HIPPOGRIFF:
    case MONS_PLANT:
    case MONS_SPINY_FROG:
    case MONS_WAR_DOG:
    case MONS_YELLOW_WASP:
        mlev += 3;
        break;

    case MONS_BLINK_FROG:
    case MONS_AGATE_SNAIL:
    case MONS_GIANT_SPORE:
    case MONS_KOMODO_DRAGON:
    case MONS_ORANGE_RAT:
    case MONS_SHEEP:
    case MONS_STEAM_DRAGON:
    case MONS_WOLF_SPIDER:
    case MONS_YAK:
    case MONS_GRIZZLY_BEAR:
        mlev += 4;
        break;

    case MONS_BLACK_MAMBA:
    case MONS_BRAIN_WORM:
    case MONS_BUMBLEBEE:
    case MONS_FIRE_DRAKE:
    case MONS_HYDRA:
    case MONS_OKLOB_PLANT:
    case MONS_WYVERN:
    case MONS_TRAPDOOR_SPIDER:
    case MONS_ROCK_WORM:
        mlev += 5;
        break;

    case MONS_ELEPHANT_SLUG:
    case MONS_POLAR_BEAR:
    case MONS_GRIFFON:
    case MONS_LINDWURM:
    case MONS_REDBACK:
    case MONS_WANDERING_MUSHROOM:
    case MONS_ELEPHANT:
        mlev += 6;
        break;

    case MONS_BORING_BEETLE:
    case MONS_BOULDER_BEETLE:
    case MONS_DEATH_YAK:
    case MONS_SPINY_WORM:
    case MONS_STAIR_MIMIC:
    case MONS_FIRE_CRAB:
        mlev += 7;
        break;

    default:
        return 99;
    }

    return (mlev);
}

int mons_lair_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_RAT:
        return 200;

    case MONS_MEGABAT:
    case MONS_GIANT_TOAD:
    case MONS_GIANT_FROG:
    case MONS_PORCUPINE:
    case MONS_QUOKKA:
        return 99;

    case MONS_WATER_MOCCASIN:
    case MONS_CROCODILE:
        return 90;

    case MONS_PLANT:
    case MONS_SNAKE:
        return 80;

    case MONS_SPINY_FROG:
        return 75;

    case MONS_JACKAL:
    case MONS_IGUANA:
    case MONS_GILA_MONSTER:
        return 70;

    case MONS_GREEN_RAT:
        return 64;

    case MONS_HOUND:
        return 60;

    case MONS_AGATE_SNAIL:
        return 56;

    case MONS_GIANT_SLUG:
        return 55;

    case MONS_FUNGUS:
    case MONS_GIANT_GECKO:
    case MONS_GIANT_CENTIPEDE:
    case MONS_HIPPOGRIFF:
    case MONS_HYDRA:
    case MONS_KOMODO_DRAGON:
    case MONS_YAK:
        return 50;

    case MONS_BLACK_MAMBA:
        return 47;

    case MONS_BLINK_FROG:
        return 45;

    case MONS_SHEEP:
    case MONS_ELEPHANT:
    case MONS_FIRE_DRAKE:
        return 36;

    case MONS_WAR_DOG:
        return 35;

    case MONS_WORM:
    case MONS_GIANT_MITE:
    case MONS_GRIFFON:
    case MONS_DEATH_YAK:
    case MONS_ELEPHANT_SLUG:
        return 30;

    case MONS_BORING_BEETLE:
        return 29;

    case MONS_BOULDER_BEETLE:
    case MONS_GIANT_NEWT:
    case MONS_WOLF:
    case MONS_WYVERN:
        return 20;

    case MONS_BLACK_BEAR:
    case MONS_GRIZZLY_BEAR:
    case MONS_POLAR_BEAR:
        return 15;

    case MONS_GOLIATH_BEETLE:
    case MONS_SCORPION:
    case MONS_OKLOB_PLANT:
    case MONS_STEAM_DRAGON:
    case MONS_LINDWURM:
    case MONS_ORANGE_RAT:
        return 10;

    case MONS_SPINY_WORM:
        return 9;

    case MONS_WANDERING_MUSHROOM:
    case MONS_REDBACK:
        return 8;

    case MONS_BRAIN_WORM:
    case MONS_BUMBLEBEE:
        return 7;

    case MONS_WOLF_SPIDER:
        return 6;

    case MONS_YELLOW_WASP:
    case MONS_BUTTERFLY:
    case MONS_TRAPDOOR_SPIDER:
    case MONS_ROCK_WORM:
        return 5;

    case MONS_GIANT_SPORE:
        return 2;

    case MONS_STAIR_MIMIC:
    case MONS_FIRE_CRAB:
        return 1;

    default:
        return 0;
    }
}

// The Swamp
int mons_swamp_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_SWAMP, 1);

    switch (mcls)
    {
    case MONS_MEGABAT:
    case MONS_GIANT_FROG:
    case MONS_GIANT_AMOEBA:
    case MONS_GIANT_SLUG:
    case MONS_GIANT_NEWT:
    case MONS_GIANT_GECKO:
    case MONS_RAT:
    case MONS_SWAMP_DRAKE:
    case MONS_WORM:
    case MONS_SWAMP_WORM:
    case MONS_GIANT_LEECH:
    case MONS_ALLIGATOR:
        mlev++;
        break;

    case MONS_GIANT_TOAD:
    case MONS_FUNGUS:
    case MONS_NECROPHAGE:
    case MONS_PLANT:
    case MONS_SNAKE:
    case MONS_BUTTERFLY:
    case MONS_CROCODILE:
    case MONS_VAMPIRE_MOSQUITO:
    case MONS_AGATE_SNAIL:
    case MONS_HYDRA:
    case MONS_BOG_MUMMY:
        mlev += 2;
        break;

    case MONS_WATER_MOCCASIN:
    case MONS_HUNGRY_GHOST:
    case MONS_INSUBSTANTIAL_WISP:
    case MONS_JELLY:
    case MONS_KOMODO_DRAGON:
    case MONS_PHANTOM:
    case MONS_RED_WASP:
    case MONS_SPINY_FROG:
    case MONS_SWAMP_DRAGON:
    // case MONS_PORTAL_MIMIC:
    case MONS_UGLY_THING:
        mlev += 3;
        break;

    case MONS_BLINK_FROG:
    case MONS_SLIME_CREATURE:
    case MONS_VERY_UGLY_THING:
    case MONS_VAPOUR:
    case MONS_TENTACLED_MONSTROSITY:
        mlev += 4;
        break;

    default:
        mlev += 99;
    }

    return (mlev);
}

int mons_swamp_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_VAMPIRE_MOSQUITO:
        return 250;

    case MONS_PLANT:
        return 200;

    case MONS_GIANT_FROG:
    case MONS_ALLIGATOR:
        return 150;

    case MONS_MEGABAT:
    case MONS_FUNGUS:
        return 99;

    case MONS_GIANT_TOAD:
        return 90;

    case MONS_SWAMP_DRAKE:
        return 80;

    case MONS_HYDRA:
        return 70;

    case MONS_RAT:
    case MONS_GIANT_LEECH:
    case MONS_BOG_MUMMY:
        return 61;

    case MONS_SLIME_CREATURE:
        return 54;

    case MONS_SNAKE:
        return 52;

    case MONS_INSUBSTANTIAL_WISP:
        return 43;

    case MONS_WATER_MOCCASIN:
        return 33;

    case MONS_RED_WASP:
    case MONS_SWAMP_DRAGON:
    case MONS_SPINY_FROG:
        return 30;

    case MONS_JELLY:
    case MONS_BUTTERFLY:
    case MONS_CROCODILE:
        return 25;

    case MONS_WORM:
    // case MONS_PORTAL_MIMIC:
        return 20;

    case MONS_KOMODO_DRAGON:
    case MONS_VERY_UGLY_THING:
    case MONS_VAPOUR:
        return 15;

    case MONS_PHANTOM:
    case MONS_UGLY_THING:
    case MONS_HUNGRY_GHOST:
        return 13;

    case MONS_NECROPHAGE:
        return 12;

    case MONS_SIREN:
    case MONS_BLINK_FROG:
    case MONS_GIANT_AMOEBA:
    case MONS_GIANT_GECKO:
    case MONS_GIANT_NEWT:
    case MONS_GIANT_SLUG:
    case MONS_AGATE_SNAIL:
    case MONS_TENTACLED_MONSTROSITY:
        return 10;

    default:
        return 0;
    }
}

// The Shoals
int mons_shoals_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_SHOALS, 1);
    switch (mcls)
    {
    case MONS_BUTTERFLY:
    case MONS_MEGABAT:
        break;

    case MONS_MERFOLK:
    case MONS_MERMAID:
    case MONS_HIPPOGRIFF:
    case MONS_CENTAUR:
    case MONS_SEA_SNAKE:
        mlev++;
        break;

    case MONS_MANTICORE:
    case MONS_SNAPPING_TURTLE:
    case MONS_HARPY:
        mlev += 2;
        break;

    case MONS_CYCLOPS:          // will have a sheep band
    case MONS_SIREN:
    case MONS_OKLOB_PLANT:
    case MONS_SHARK:
    case MONS_KRAKEN:
        mlev += 3;
        break;

    case MONS_ALLIGATOR_SNAPPING_TURTLE:
    case MONS_MERFOLK_JAVELINEER:
    case MONS_MERFOLK_IMPALER:
    case MONS_MERFOLK_AQUAMANCER:
    // case MONS_PORTAL_MIMIC:
        mlev += 4;
        break;

    default:
        mlev += 99;
    }
    return mlev;
}

int mons_shoals_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_MERFOLK:
        return 90;

    case MONS_SNAPPING_TURTLE:
        return 45;

    case MONS_MERMAID:
    case MONS_MANTICORE:
    case MONS_SEA_SNAKE:
        return 40;

    case MONS_HIPPOGRIFF:
    case MONS_MEGABAT:
    case MONS_BUTTERFLY:
    case MONS_SHARK:
        return 35;

    case MONS_SIREN:
    case MONS_HARPY:
        return 24;

    case MONS_CYCLOPS:
    case MONS_CENTAUR:
        return 20;

    case MONS_KRAKEN:
        return 18;

    case MONS_MERFOLK_AQUAMANCER:
    case MONS_MERFOLK_IMPALER:
        return 15;

    case MONS_OKLOB_PLANT:
    case MONS_ALLIGATOR_SNAPPING_TURTLE:
    case MONS_MERFOLK_JAVELINEER:
    // case MONS_PORTAL_MIMIC:
        return 10;

    default:
        return 0;
    }
}

// The Snake Pit
int mons_pitsnake_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_SNAKE_PIT, 1);

    switch (mcls)
    {
    case MONS_SMALL_SNAKE:
    case MONS_SNAKE:
        mlev++;
        break;

    case MONS_WATER_MOCCASIN:
    case MONS_BLACK_MAMBA:
    case MONS_VIPER:
    case MONS_ANACONDA:
    case MONS_NAGA:
        mlev += 2;
        break;

    case MONS_NAGA_WARRIOR:
    case MONS_NAGA_MAGE:
        mlev += 3;
        break;

    case MONS_GUARDIAN_SERPENT:
        mlev += 4;
        break;

    case MONS_GREATER_NAGA:
    // case MONS_PORTAL_MIMIC:
        mlev += 5;
        break;

    default:
        mlev += 99;
    }

    return (mlev);
}

int mons_pitsnake_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_SNAKE:
    case MONS_WATER_MOCCASIN:
        return 99;

    case MONS_BLACK_MAMBA:
        return 72;

    case MONS_NAGA:
        return 53;

    case MONS_NAGA_WARRIOR:
    case MONS_NAGA_MAGE:
        return 34;

    case MONS_VIPER:
    case MONS_ANACONDA:
        return 32;

    case MONS_GREATER_NAGA:
    case MONS_GUARDIAN_SERPENT:
    case MONS_SMALL_SNAKE:
    // case MONS_PORTAL_MIMIC:
        return 15;

    default:
        return 0;
    }
}

// The Spider Nest
int mons_spidernest_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_SPIDER_NEST, 1);

    switch (mcls)
    {
    case MONS_GIANT_COCKROACH:
    case MONS_GIANT_MITE:
    case MONS_SCORPION:
    case MONS_REDBACK:
    case MONS_WOLF_SPIDER:
    case MONS_TRAPDOOR_SPIDER:
    case MONS_VAMPIRE_MOSQUITO:
    case MONS_SPIDER:
        mlev++;
        break;

    case MONS_YELLOW_WASP:
    case MONS_RED_WASP:
    case MONS_GOLIATH_BEETLE:
        mlev += 2;
        break;

    case MONS_BORING_BEETLE:
    case MONS_BOULDER_BEETLE:
    case MONS_GIANT_CENTIPEDE:
        mlev += 3;
        break;

    case MONS_EMPEROR_SCORPION:
    case MONS_JUMPING_SPIDER:
        mlev += 4;
        break;

    case MONS_TARANTELLA:
    case MONS_GHOST_MOTH:
    case MONS_DEMONIC_CRAWLER:
        mlev += 5;
        break;

    default:
        mlev += 99;
    }

    return (mlev);
}

int mons_spidernest_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_GIANT_COCKROACH:
    case MONS_GIANT_MITE:
        return 60;

    case MONS_SCORPION:
    case MONS_REDBACK:
    case MONS_WOLF_SPIDER:
    case MONS_TRAPDOOR_SPIDER:
        return 50;

    case MONS_VAMPIRE_MOSQUITO:
        return 40;

    case MONS_YELLOW_WASP:
    case MONS_RED_WASP:
        return 20;

    case MONS_GOLIATH_BEETLE:
    case MONS_BORING_BEETLE:
    case MONS_BOULDER_BEETLE:
    case MONS_GIANT_CENTIPEDE:
    case MONS_EMPEROR_SCORPION:
    case MONS_SPIDER:
        return 15;

    case MONS_JUMPING_SPIDER:
    case MONS_TARANTELLA:
        return 10;

    case MONS_GHOST_MOTH:
    case MONS_DEMONIC_CRAWLER:
        return 5;

    default:
        return 0;
    }
}

// The Slime Pits
int mons_pitslime_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_SLIME_PITS, 1);

    switch (mcls)
    {
    case MONS_JELLY:
    case MONS_OOZE:
    case MONS_ACID_BLOB:
    case MONS_GIANT_EYEBALL:
        mlev++;
        break;

    case MONS_BROWN_OOZE:
    case MONS_SLIME_CREATURE:
    case MONS_EYE_OF_DRAINING:
        mlev += 2;
        break;

    case MONS_GIANT_AMOEBA:
    case MONS_AZURE_JELLY:
    case MONS_SHINING_EYE:
    case MONS_GOLDEN_EYE:
        mlev += 3;
        break;

    case MONS_PULSATING_LUMP:
    case MONS_GREAT_ORB_OF_EYES:
    case MONS_EYE_OF_DEVASTATION:
        mlev += 4;
        break;

    case MONS_DEATH_OOZE:
    case MONS_TENTACLED_MONSTROSITY:
    case MONS_GIANT_ORANGE_BRAIN:
        mlev += 5;
        break;

    case MONS_ROYAL_JELLY:
        mlev += 6;

    default:
        mlev += 0;
        break;
    }

    return (mlev);
}

int mons_pitslime_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_JELLY:
        return 300;

    case MONS_SLIME_CREATURE:
        return 200;

    case MONS_BROWN_OOZE:
        return 150;

    case MONS_GIANT_AMOEBA:
    case MONS_ACID_BLOB:
        return 100;

    case MONS_OOZE:
    case MONS_AZURE_JELLY:
    case MONS_GIANT_EYEBALL:
    case MONS_EYE_OF_DRAINING:
    case MONS_SHINING_EYE:
        return 50;

    case MONS_DEATH_OOZE:
    case MONS_GREAT_ORB_OF_EYES:
    case MONS_EYE_OF_DEVASTATION:
    case MONS_GOLDEN_EYE:
        return 30;

    case MONS_PULSATING_LUMP:
    case MONS_GIANT_ORANGE_BRAIN:
        return 20;

    case MONS_TENTACLED_MONSTROSITY:
        return 2;

    default:
        return 0;
    }
}

// The Hive
int mons_hive_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_HIVE, 1);

    switch (mcls)
    {
    case MONS_PLANT:
    case MONS_KILLER_BEE:
        mlev += 0;
        break;

    case MONS_KILLER_BEE_LARVA:
        mlev++;
        break;

    default:
        return 99;
    }

    return (mlev);
}

int mons_hive_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_KILLER_BEE:
        return 300;

    case MONS_PLANT:
        return 100;

    case MONS_KILLER_BEE_LARVA:
        return 50;

    default:
        return 0;
    }
}

// The Hall of Blades
int mons_hallblade_level(int mcls)
{
    if (mcls == MONS_DANCING_WEAPON)
        return absdungeon_depth(BRANCH_HALL_OF_BLADES, 1);
    else
        return 0;
}

int mons_hallblade_rare(int mcls)
{
    return ((mcls == MONS_DANCING_WEAPON) ? 1000 : 0);
}

// The Crypt
int mons_crypt_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_CRYPT, 1);

    switch (mcls)
    {
    case MONS_ZOMBIE_SMALL:
        mlev += 0;
        break;

    case MONS_PHANTOM:
    case MONS_SKELETON_SMALL:
    case MONS_SKELETON_LARGE:
    case MONS_ZOMBIE_LARGE:
    case MONS_WIGHT:
        mlev++;
        break;

    case MONS_SHADOW:
    case MONS_HUNGRY_GHOST:
    case MONS_NECROPHAGE:
    case MONS_SKELETAL_WARRIOR:
    case MONS_SIMULACRUM_SMALL:
    case MONS_SIMULACRUM_LARGE:
        mlev += 2;
        break;

    case MONS_NECROMANCER:
    case MONS_PULSATING_LUMP:
    case MONS_FLAYED_GHOST:
    case MONS_FREEZING_WRAITH:
    case MONS_GHOUL:
    case MONS_ROTTING_HULK:
    case MONS_WRAITH:
    case MONS_FLYING_SKULL:
    case MONS_SILENT_SPECTRE:
        mlev += 3;
        break;

    case MONS_BONE_DRAGON:
    case MONS_FLAMING_CORPSE:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_SHADOW_WRAITH:
    case MONS_VAMPIRE_KNIGHT:
    case MONS_VAMPIRE_MAGE:
    case MONS_ABOMINATION_SMALL:
    case MONS_MUMMY:
    case MONS_VAMPIRE:
    case MONS_ABOMINATION_LARGE:
        mlev += 4;
        break;

    case MONS_REAPER:
    case MONS_ANCIENT_LICH:
    case MONS_LICH:
    case MONS_CURSE_SKULL:
    // feature mimics
    // case MONS_PORTAL_MIMIC:
    case MONS_STAIR_MIMIC:
    case MONS_SHOP_MIMIC:
    case MONS_FOUNTAIN_MIMIC:
        mlev += 5;
        break;

    default:
        mlev += 99;
    }

    return (mlev);
}

int mons_crypt_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_ZOMBIE_SMALL:
    case MONS_SKELETON_SMALL:
    case MONS_SKELETON_LARGE:
        return 410;

    case MONS_ZOMBIE_LARGE:
        return 300;

    case MONS_SKELETAL_WARRIOR:
        return 75;

    case MONS_NECROPHAGE:
        return 50;

    case MONS_WIGHT:
        return 35;

    case MONS_WRAITH:
        return 33;

    case MONS_SHADOW:
    case MONS_SILENT_SPECTRE:
        return 30;

    case MONS_NECROMANCER:
    case MONS_GHOUL:
        return 25;

    case MONS_BONE_DRAGON:
    case MONS_MUMMY:
        return 24;

    case MONS_VAMPIRE:
    case MONS_PHANTOM:
        return 21;

    case MONS_VAMPIRE_KNIGHT:
    case MONS_VAMPIRE_MAGE:
        return 20;

    case MONS_ROTTING_HULK:
        return 17;

    case MONS_PHANTASMAL_WARRIOR:
        return 14;

    case MONS_FLYING_SKULL:
    case MONS_FLAYED_GHOST:
    case MONS_FREEZING_WRAITH:
        return 13;

    case MONS_FLAMING_CORPSE:
    case MONS_HUNGRY_GHOST:
        return 12;

    case MONS_SHADOW_WRAITH:
    case MONS_SIMULACRUM_SMALL:
    case MONS_SIMULACRUM_LARGE:
        return 10;

    case MONS_ANCIENT_LICH:
    // feature mimics
    // case MONS_PORTAL_MIMIC:
    case MONS_STAIR_MIMIC:
    case MONS_SHOP_MIMIC:
    case MONS_FOUNTAIN_MIMIC:
        return 8;

    case MONS_ABOMINATION_SMALL:
    case MONS_LICH:
    case MONS_REAPER:
        return 5;

    case MONS_ABOMINATION_LARGE:
        return 4;

    case MONS_PULSATING_LUMP:
        return 3;

    default:
        return 0;
    }
}

// The Tomb
int mons_tomb_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_TOMB, 1);

    switch (mcls)
    {
    case MONS_ZOMBIE_SMALL:
        mlev += 0;
        break;

    case MONS_MUMMY:
    case MONS_ZOMBIE_LARGE:
    case MONS_SKELETON_SMALL:
    case MONS_SKELETON_LARGE:
    case MONS_TRAPDOOR_SPIDER:
        mlev++;
        break;

    case MONS_GUARDIAN_MUMMY:
    case MONS_FLYING_SKULL:
    case MONS_SIMULACRUM_SMALL:
    case MONS_SIMULACRUM_LARGE:
        mlev += 2;
        break;

    case MONS_LICH:
    case MONS_ANCIENT_LICH:
    case MONS_MUMMY_PRIEST:
        mlev += 3;
        break;

    case MONS_GREATER_MUMMY:
        mlev += 4;
        break;

    default:
        mlev += 99;
    }

    return (mlev);
}

int mons_tomb_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_MUMMY:
        return 300;

    case MONS_GUARDIAN_MUMMY:
        return 202;

    case MONS_MUMMY_PRIEST:
        return 40;

    case MONS_FLYING_SKULL:
        return 33;

    case MONS_ZOMBIE_LARGE:
    case MONS_SKELETON_SMALL:
    case MONS_SKELETON_LARGE:
        return 21;

    case MONS_ZOMBIE_SMALL:
        return 20;

    case MONS_SIMULACRUM_SMALL:
    case MONS_SIMULACRUM_LARGE:
        return 10;

    case MONS_LICH:
        return 4;

    case MONS_ANCIENT_LICH:
        return 2;

    // A nod to the fabled pyramid traps, these should be really rare.
    case MONS_TRAPDOOR_SPIDER:
        return 1;

    default:
        return 0;
    }
}

// The Enchanted Forest
int mons_forest_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_FOREST, 1);

    switch (mcls)
    {
    case MONS_SPRIGGAN:
    case MONS_SPRIGGAN_DRUID:
    case MONS_GRIZZLY_BEAR:
    case MONS_BLACK_BEAR:
    case MONS_WOLF:
        mlev++;
        break;

    case MONS_SPRIGGAN_RIDER:
        mlev += 2;
        break;

    case MONS_SPRIGGAN_AIR_MAGE:
        mlev += 3;

    case MONS_SPRIGGAN_DEFENDER:
        mlev += 5;
        break;

    default:
        mlev += 99;
    }

    return (mlev);
}

int mons_forest_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_SPRIGGAN:
        return 99;

    case MONS_SPRIGGAN_DRUID:
    case MONS_SPRIGGAN_RIDER:
    case MONS_GRIZZLY_BEAR:
    case MONS_WOLF:
        return 40;

    case MONS_SPRIGGAN_AIR_MAGE:
    case MONS_BLACK_BEAR:
        return 20;

    case MONS_SPRIGGAN_DEFENDER:
        return 10;

    default:
        return 0;
    }
}

// The Halls of Zot
int mons_hallzot_level(int mcls)
{
    int mlev = absdungeon_depth(BRANCH_HALL_OF_ZOT, 0);

    switch (mcls)
    {
    case MONS_GOLDEN_DRAGON:
    case MONS_GUARDIAN_MUMMY:
        mlev += 6;
        break;
    case MONS_BONE_DRAGON:
    case MONS_KILLER_KLOWN:
    case MONS_SHADOW_DRAGON:
    case MONS_STORM_DRAGON:
    case MONS_CURSE_TOE:
    case MONS_ORB_GUARDIAN:
    case MONS_GHOST_MOTH:
        mlev += 5;
        break;
    case MONS_DEATH_COB:
    case MONS_DRAGON:
    case MONS_ICE_DRAGON:
        mlev += 4;
        break;
    case MONS_MOTTLED_DRACONIAN:
    case MONS_YELLOW_DRACONIAN:
    case MONS_BLACK_DRACONIAN:
    case MONS_WHITE_DRACONIAN:
    case MONS_RED_DRACONIAN:
    case MONS_PURPLE_DRACONIAN:
    case MONS_PALE_DRACONIAN:
    case MONS_GREEN_DRACONIAN:
    case MONS_GREY_DRACONIAN:
    case MONS_DRACONIAN_CALLER:
    case MONS_DRACONIAN_MONK:
    case MONS_DRACONIAN_SCORCHER:
    case MONS_DRACONIAN_KNIGHT:
    case MONS_DRACONIAN_ANNIHILATOR:
    case MONS_DRACONIAN_ZEALOT:
    case MONS_DRACONIAN_SHIFTER:
    case MONS_TENTACLED_MONSTROSITY:
        mlev += 3;
        break;
    case MONS_MOTH_OF_WRATH:
        mlev += 2;
        break;
    case MONS_ORB_OF_FIRE:
    case MONS_ELECTRIC_GOLEM:
        mlev += 1;
        break;
    default:
        mlev += 99;             // I think this won't be a problem {dlb}
        break;
    }

    return (mlev);
}

int mons_hallzot_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_MOTH_OF_WRATH:
        return 88;
    case MONS_STORM_DRAGON:
    case MONS_TENTACLED_MONSTROSITY:
        return 50;
    case MONS_GOLDEN_DRAGON:
        return 42;
    case MONS_BONE_DRAGON:
    case MONS_DEATH_COB:
    case MONS_DRAGON:
    case MONS_ICE_DRAGON:
        return 40;
    case MONS_SHADOW_DRAGON:
    case MONS_GHOST_MOTH:
        return 30;
    case MONS_GUARDIAN_MUMMY:
    case MONS_ELECTRIC_GOLEM:
    case MONS_CURSE_TOE:
        return 20;

    case MONS_MOTTLED_DRACONIAN:
    case MONS_YELLOW_DRACONIAN:
    case MONS_BLACK_DRACONIAN:
    case MONS_WHITE_DRACONIAN:
    case MONS_RED_DRACONIAN:
    case MONS_PURPLE_DRACONIAN:
    case MONS_PALE_DRACONIAN:
    case MONS_GREEN_DRACONIAN:
    case MONS_GREY_DRACONIAN:
        return 18;

    case MONS_DRACONIAN_CALLER:
    case MONS_DRACONIAN_MONK:
    case MONS_DRACONIAN_SCORCHER:
    case MONS_DRACONIAN_KNIGHT:
    case MONS_DRACONIAN_ANNIHILATOR:
    case MONS_DRACONIAN_ZEALOT:
    case MONS_DRACONIAN_SHIFTER:
        return 16;

    case MONS_KILLER_KLOWN:
    case MONS_ORB_OF_FIRE:
        return 15;
    default:
        return 0;
    }
}

// The Hells

// The Iron City of Dis
int mons_dis_level(int mcls)
{
    int mlev = 26;

    switch (mcls)
    {
    case MONS_CLAY_GOLEM:
    case MONS_IMP:
    case MONS_NECROPHAGE:
    case MONS_RED_DEVIL:
    case MONS_SKELETAL_WARRIOR:
    case MONS_ZOMBIE_LARGE:
        mlev++;
        break;

    case MONS_HELL_HOUND:
    case MONS_HELL_KNIGHT:
    case MONS_SKELETON_LARGE:
    case MONS_PHANTOM:
    case MONS_ROTTING_DEVIL:
    case MONS_SHADOW:
    case MONS_SKELETON_SMALL:
    case MONS_STONE_GOLEM:
    case MONS_TORMENTOR:
    case MONS_WIGHT:
    case MONS_ZOMBIE_SMALL:
        mlev += 2;
        break;

    case MONS_EFREET:
    case MONS_FLYING_SKULL:
    case MONS_HELLION:
    case MONS_HELL_HOG:
    case MONS_IRON_GOLEM:
    case MONS_MUMMY:
        mlev += 3;
        break;

    case MONS_FLAYED_GHOST:
    case MONS_FREEZING_WRAITH:
    case MONS_DEATH_DRAKE:
    case MONS_HAIRY_DEVIL:
    case MONS_IRON_DEVIL:
    case MONS_IRON_IMP:
    case MONS_VAMPIRE:
    case MONS_WRAITH:
        mlev += 3;
        break;

    case MONS_BLUE_DEVIL:
    case MONS_DANCING_WEAPON:
    case MONS_FLAMING_CORPSE:
    case MONS_ICE_DEVIL:
    case MONS_ICE_DRAGON:
    case MONS_LICH:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_REAPER:
    case MONS_SOUL_EATER:
        mlev += 5;
        break;

    case MONS_ANCIENT_LICH:
    case MONS_BONE_DRAGON:
    case MONS_FIEND:
    case MONS_IRON_DRAGON:
        mlev += 6;
        break;

    default:
        return 0;
    }

    return (mlev);
}

int mons_dis_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_IMP:
    case MONS_IRON_DEVIL:
    case MONS_IRON_IMP:
    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
        return 99;

    case MONS_REAPER:
        return 77;

    case MONS_TORMENTOR:
        return 66;

    case MONS_RED_DEVIL:
    case MONS_SKELETAL_WARRIOR:
        return 50;

    case MONS_WRAITH:
        return 48;

    case MONS_SHADOW:
        return 56;

    case MONS_HELL_HOUND:
        return 46;

    case MONS_MUMMY:
    case MONS_WIGHT:
        return 45;

    case MONS_HELLION:
    case MONS_BLUE_DEVIL:
        return 40;

    case MONS_FLYING_SKULL:
        return 35;

    case MONS_FREEZING_WRAITH:
    case MONS_ICE_DEVIL:
        return 30;

    case MONS_FLAMING_CORPSE:
    case MONS_FLAYED_GHOST:
    case MONS_SKELETON_LARGE:
    case MONS_NECROPHAGE:
    case MONS_SKELETON_SMALL:
        return 25;

    case MONS_BONE_DRAGON:
    case MONS_HELL_HOG:
        return 20;

    case MONS_VAMPIRE:
        return 19;

    case MONS_PHANTOM:
        return 17;

    case MONS_HAIRY_DEVIL:
        return 15;

    case MONS_CLAY_GOLEM:
    case MONS_DANCING_WEAPON:
    case MONS_EFREET:
    case MONS_HELL_KNIGHT:
    case MONS_IRON_GOLEM:
    case MONS_LICH:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_ROTTING_DEVIL:
    case MONS_SOUL_EATER:
    case MONS_STONE_GOLEM:
        return 10;

    case MONS_IRON_DRAGON:
        return 5;

    case MONS_ANCIENT_LICH:
    case MONS_FIEND:
        return 3;

    default:
        return 0;
    }
}

// Gehenna - the fire hell
int mons_gehenna_level(int mcls)
{
    int mlev = 26;

    switch (mcls)
    {
    case MONS_CLAY_GOLEM:
    case MONS_SKELETON_LARGE:
    case MONS_RED_DEVIL:
    case MONS_SKELETON_SMALL:
    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
        mlev++;
        break;

    case MONS_HELL_HOG:
    case MONS_HELL_HOUND:
    case MONS_IMP:
    case MONS_NECROPHAGE:
    case MONS_STONE_GOLEM:
        mlev += 2;
        break;

    case MONS_FLYING_SKULL:
    case MONS_IRON_GOLEM:
    case MONS_MUMMY:
    case MONS_PHANTOM:
    case MONS_ROTTING_DEVIL:
    case MONS_SHADOW:
    case MONS_WIGHT:
        mlev += 3;
        break;

    case MONS_HAIRY_DEVIL:
    case MONS_HELL_KNIGHT:
    case MONS_VAMPIRE:
    case MONS_WRAITH:
        mlev += 4;
        break;

    case MONS_EFREET:
    case MONS_FLAMING_CORPSE:
    case MONS_FLAYED_GHOST:
    case MONS_HELLION:
    case MONS_TORMENTOR:
    case MONS_FIRE_CRAB:
        mlev += 5;
        break;

    case MONS_ANCIENT_LICH:
    case MONS_BONE_DRAGON:
    case MONS_FIEND:
    case MONS_LICH:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_PIT_FIEND:
    case MONS_REAPER:
    case MONS_SOUL_EATER:
        mlev += 6;
        break;

    default:
        return 0;
    }

    return (mlev);
}

int mons_gehenna_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_SKELETON_LARGE:
    case MONS_SKELETON_SMALL:
    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
        return 99;

    case MONS_MUMMY:
        return 70;

    case MONS_SHADOW:
        return 61;

    case MONS_RED_DEVIL:
    case MONS_WIGHT:
        return 60;

    case MONS_HELLION:
        return 54;

    case MONS_WRAITH:
        return 53;

    case MONS_NECROPHAGE:
    case MONS_ROTTING_DEVIL:
        return 50;

    case MONS_VAMPIRE:
        return 44;

    case MONS_FLYING_SKULL:
    case MONS_REAPER:
        return 43;

    case MONS_TORMENTOR:
        return 42;

    case MONS_HELL_HOUND:
        return 41;

    case MONS_FLAMING_CORPSE:
    case MONS_FLAYED_GHOST:
    case MONS_PHANTOM:
        return 32;

    case MONS_HELL_HOG:
    case MONS_IMP:
    case MONS_IRON_DEVIL:
        return 30;

    case MONS_LICH:
        return 25;

    case MONS_HELL_KNIGHT:
        return 21;

    case MONS_HAIRY_DEVIL:
    case MONS_PHANTASMAL_WARRIOR:
        return 20;

    case MONS_BONE_DRAGON:
    case MONS_CLAY_GOLEM:
        return 10;

    case MONS_STONE_GOLEM:
        return 8;

    case MONS_PIT_FIEND:
        return 7;

    case MONS_EFREET:
    case MONS_FIEND:
    case MONS_IRON_GOLEM:
    case MONS_SOUL_EATER:
    case MONS_FIRE_CRAB:
        return 5;

    case MONS_ANCIENT_LICH:
        return 4;

    default:
        return 0;
    }
}

// Cocytus - the ice hell
int mons_cocytus_level(int mcls)
{
    int mlev = 26;

    switch (mcls)
    {
    case MONS_SKELETON_LARGE:
    case MONS_NECROPHAGE:
    case MONS_SKELETAL_WARRIOR:
    case MONS_SKELETON_SMALL:
    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
    case MONS_SIMULACRUM_LARGE:
    case MONS_SIMULACRUM_SMALL:
        mlev++;
        break;

    case MONS_BLUE_DEVIL:
    case MONS_ICE_BEAST:
    case MONS_PHANTOM:
    case MONS_SHADOW:
        mlev += 2;
        break;

    case MONS_FLYING_SKULL:
    case MONS_ROTTING_DEVIL:
    case MONS_VAMPIRE:
    case MONS_WIGHT:
        mlev += 3;
        break;

    case MONS_FREEZING_WRAITH:
    case MONS_HAIRY_DEVIL:
    case MONS_HUNGRY_GHOST:
    case MONS_MUMMY:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_WRAITH:
        mlev += 4;
        break;

    case MONS_ICE_DEVIL:
    case MONS_ICE_DRAGON:
    case MONS_TORMENTOR:
    case MONS_WHITE_IMP:
        mlev += 5;
        break;

    case MONS_ANCIENT_LICH:
    case MONS_BONE_DRAGON:
    case MONS_LICH:
    case MONS_REAPER:
    case MONS_SOUL_EATER:
        mlev += 6;
        break;

    default:
        return 0;
    }

    return (mlev);
}

int mons_cocytus_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_FREEZING_WRAITH:
        return 87;

    case MONS_ICE_BEAST:
    case MONS_SKELETON_LARGE:
    case MONS_SKELETON_SMALL:
    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
        return 85;

    case MONS_BLUE_DEVIL:
    case MONS_ICE_DEVIL:
    case MONS_WHITE_IMP:
        return 76;

    case MONS_FLYING_SKULL:
        return 57;

    case MONS_SHADOW:
        return 56;

    case MONS_SKELETAL_WARRIOR:
        return 50;

    case MONS_REAPER:
        return 47;

    case MONS_WIGHT:
    case MONS_WRAITH:
        return 45;

    case MONS_ICE_DRAGON:
        return 38;

    case MONS_ROTTING_DEVIL:
    case MONS_TORMENTOR:
        return 37;

    case MONS_MUMMY:
        return 35;

    case MONS_VAMPIRE:
        return 34;

    case MONS_HAIRY_DEVIL:
    case MONS_HUNGRY_GHOST:
        return 26;

    case MONS_NECROPHAGE:
    case MONS_PHANTOM:
        return 25;

    case MONS_PHANTASMAL_WARRIOR:
        return 20;

    case MONS_SOUL_EATER:
        return 19;

    case MONS_BONE_DRAGON:
    case MONS_LICH:
    case MONS_SIMULACRUM_LARGE:
    case MONS_SIMULACRUM_SMALL:
        return 12;

    case MONS_ANCIENT_LICH:
        return 5;

    default:
        return 0;
    }
}

// Tartarus - the undead hell
int mons_tartarus_level(int mcls)
{
    int mlev = 26;

    switch (mcls)
    {
    case MONS_IMP:
    case MONS_SKELETON_LARGE:
    case MONS_RED_DEVIL:
    case MONS_SHADOW_IMP:
    case MONS_SKELETAL_WARRIOR:
    case MONS_SKELETON_SMALL:
        mlev++;
        break;

    case MONS_HELL_KNIGHT:
    case MONS_NECROPHAGE:
    case MONS_PHANTOM:
    case MONS_WIGHT:
    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
        mlev += 2;
        break;

    case MONS_FREEZING_WRAITH:
    case MONS_HELL_HOUND:
    case MONS_NECROMANCER:
    case MONS_SHADOW:
    case MONS_SHADOW_DEMON:
    case MONS_WRAITH:
    case MONS_SILENT_SPECTRE:
        mlev += 3;
        break;

    case MONS_BLUE_DEVIL:
    case MONS_BONE_DRAGON:
    case MONS_FLAYED_GHOST:
    case MONS_HUNGRY_GHOST:
    case MONS_ICE_DEVIL:
    case MONS_MUMMY:
    case MONS_PHANTASMAL_WARRIOR:
    case MONS_TORMENTOR:
    case MONS_SIMULACRUM_LARGE:
    case MONS_SIMULACRUM_SMALL:
        mlev += 4;
        break;

    case MONS_FLYING_SKULL:
    case MONS_HELLION:
    case MONS_REAPER:
    case MONS_ROTTING_DEVIL:
    case MONS_SHADOW_DRAGON:
    case MONS_VAMPIRE:
        mlev += 5;
        break;

    case MONS_ANCIENT_LICH:
    case MONS_HAIRY_DEVIL:
    case MONS_LICH:
    case MONS_SOUL_EATER:
        mlev += 6;
        break;

    default:
        return 0;
    }

    return (mlev);
}

int mons_tartarus_rare(int mcls)
{
    switch (mcls)
    {
    case MONS_SKELETON_LARGE:
    case MONS_SHADOW_IMP:
    case MONS_SKELETAL_WARRIOR:
    case MONS_SKELETON_SMALL:
    case MONS_ZOMBIE_LARGE:
    case MONS_ZOMBIE_SMALL:
        return 99;

    case MONS_SHADOW:
        return 92;

    case MONS_REAPER:
        return 73;

    case MONS_NECROPHAGE:
        return 72;

    case MONS_WIGHT:
        return 71;

    case MONS_ROTTING_DEVIL:
        return 62;

    case MONS_FREEZING_WRAITH:
        return 60;

    case MONS_FLYING_SKULL:
        return 53;

    case MONS_HELL_HOUND:
    case MONS_PHANTOM:
    case MONS_WRAITH:
        return 52;

    case MONS_SHADOW_DEMON:
        return 50;

    case MONS_PHANTASMAL_WARRIOR:
        return 45;

    case MONS_VAMPIRE:
        return 44;

    case MONS_HELLION:
    case MONS_TORMENTOR:
        return 42;

    case MONS_BONE_DRAGON:
        return 40;

    case MONS_SOUL_EATER:
    case MONS_SILENT_SPECTRE:
        return 35;

    case MONS_ICE_DEVIL:
        return 34;

    case MONS_MUMMY:
        return 33;

    case MONS_BLUE_DEVIL:
    case MONS_HUNGRY_GHOST:
        return 32;

    case MONS_FLAYED_GHOST:
    case MONS_HAIRY_DEVIL:
        return 30;

    case MONS_LICH:
        return 24;

    case MONS_IMP:
    case MONS_SHADOW_DRAGON:
    case MONS_DEATH_DRAKE:
        return 20;

    case MONS_RED_DEVIL:
        return 13;

    case MONS_HELL_KNIGHT:
        return 14;

    case MONS_NECROMANCER:
    case MONS_SIMULACRUM_LARGE:
    case MONS_SIMULACRUM_SMALL:
        return 12;

    case MONS_ANCIENT_LICH:
        return 6;

    default:
        return 0;
    }
}
