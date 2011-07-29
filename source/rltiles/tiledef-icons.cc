// This file has been automatically generated.

#include "tiledef-icons.h"

#include <string>
#include <cstring>
#include <cassert>
using namespace std;

unsigned int _tile_icons_count[TILEI_ICONS_MAX - 0] =
{
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
};

unsigned int tile_icons_count(tileidx_t idx)
{
    assert(idx >= 0 && idx < TILEI_ICONS_MAX);
    return _tile_icons_count[idx - 0];
}

tileidx_t _tile_icons_basetiles[TILEI_ICONS_MAX - 0] =
{
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    27,
    28,
    29,
    30,
    31,
    32,
    33,
    34,
    35,
    36,
    37,
    38,
    39,
    40,
    41,
    42,
    43,
    44,
    45,
    46,
    47,
    48,
    49,
    50,
    51,
    52,
    53,
    54,
    55,
    56,
    57,
    58,
    59,
    60,
    61,
    62,
    63,
    64,
    65,
    66,
    67,
    68,
    69,
    70,
    71,
    72,
};

tileidx_t tile_icons_basetile(tileidx_t idx)
{
    assert(idx >= 0 && idx < TILEI_ICONS_MAX);
    return _tile_icons_basetiles[idx - 0];
}

int _tile_icons_probs[TILEI_ICONS_MAX - 0] =
{
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
};

int tile_icons_probs(tileidx_t idx)
{
    assert(idx >= 0 && idx < TILEI_ICONS_MAX);
    return _tile_icons_probs[idx - 0];
}

const char *_tile_icons_name[TILEI_ICONS_MAX - 0] =
{
    "ICONS_FILLER_0",
    "TRAP_NET",
    "MASK_DEEP_WATER",
    "MASK_SHALLOW_WATER",
    "MASK_DEEP_WATER_MURKY",
    "MASK_SHALLOW_WATER_MURKY",
    "MASK_DEEP_WATER_SHOALS",
    "MASK_SHALLOW_WATER_SHOALS",
    "MASK_LAVA",
    "CURSOR",
    "CURSOR2",
    "CURSOR3",
    "TUTORIAL_CURSOR",
    "HEART",
    "GOOD_NEUTRAL",
    "NEUTRAL",
    "ANIMATED_WEAPON",
    "MIMIC",
    "POISON",
    "FLAME",
    "BERSERK",
    "MAY_STAB_BRAND",
    "STAB_BRAND",
    "SOMETHING_UNDER",
    "TRIED",
    "NEW_STAIR",
    "MESH",
    "OOR_MESH",
    "MAGIC_MAP_MESH",
    "TRAVEL_EXCLUSION_FG",
    "TRAVEL_EXCLUSION_CENTRE_FG",
    "NUM0",
    "NUM1",
    "NUM2",
    "NUM3",
    "NUM4",
    "NUM5",
    "NUM6",
    "NUM7",
    "NUM8",
    "NUM9",
    "NUM0_OUTLINE",
    "NUM1_OUTLINE",
    "NUM2_OUTLINE",
    "NUM3_OUTLINE",
    "NUM4_OUTLINE",
    "NUM5_OUTLINE",
    "NUM6_OUTLINE",
    "NUM7_OUTLINE",
    "NUM8_OUTLINE",
    "NUM9_OUTLINE",
    "NUM_MINUS5",
    "NUM_MINUS4",
    "NUM_MINUS3",
    "NUM_MINUS2",
    "NUM_MINUS1",
    "NUM_ZERO",
    "NUM_PLUS1",
    "NUM_PLUS2",
    "NUM_PLUS3",
    "NUM_PLUS4",
    "NUM_PLUS5",
    "DEMON_NUM1",
    "DEMON_NUM2",
    "DEMON_NUM3",
    "DEMON_NUM4",
    "DEMON_NUM5",
    "ITEM_SLOT_SELECTED",
    "MDAM_LIGHTLY_DAMAGED",
    "MDAM_MODERATELY_DAMAGED",
    "MDAM_HEAVILY_DAMAGED",
    "MDAM_SEVERELY_DAMAGED",
    "MDAM_ALMOST_DEAD",
};

const char *tile_icons_name(tileidx_t idx)
{
    assert(idx >= 0 && idx < TILEI_ICONS_MAX);
    return _tile_icons_name[idx - 0];
}

tile_info _tile_icons_info[TILEI_ICONS_MAX - 0] =
{
    tile_info(32, 32, 0, 0, 0, 0, 32, 32),
    tile_info(32, 32, 1, 1, 32, 0, 62, 30),
    tile_info(32, 32, 0, 16, 62, 0, 94, 16),
    tile_info(32, 32, 0, 16, 94, 0, 126, 16),
    tile_info(32, 32, 0, 16, 126, 0, 158, 16),
    tile_info(32, 32, 0, 16, 158, 0, 190, 16),
    tile_info(32, 32, 0, 16, 190, 0, 222, 16),
    tile_info(32, 32, 0, 16, 222, 0, 254, 16),
    tile_info(32, 32, 0, 16, 254, 0, 286, 16),
    tile_info(32, 32, 0, 0, 286, 0, 318, 32),
    tile_info(32, 32, 0, 0, 318, 0, 350, 32),
    tile_info(32, 32, 0, 0, 350, 0, 382, 32),
    tile_info(32, 32, 0, 0, 382, 0, 414, 32),
    tile_info(32, 32, 21, 0, 414, 0, 425, 9),
    tile_info(32, 32, 23, 0, 414, 9, 423, 17),
    tile_info(32, 32, 23, 0, 414, 17, 423, 25),
    tile_info(32, 32, 0, 0, 425, 0, 437, 12),
    tile_info(32, 32, 7, 12, 425, 12, 444, 31),
    tile_info(32, 32, 26, 0, 444, 0, 450, 12),
    tile_info(32, 32, 26, 0, 444, 12, 450, 21),
    tile_info(32, 32, 18, 18, 450, 0, 464, 14),
    tile_info(32, 32, 24, 0, 450, 14, 458, 28),
    tile_info(32, 32, 17, 0, 464, 0, 479, 9),
    tile_info(32, 32, 0, 17, 464, 9, 474, 24),
    tile_info(8, 16, 0, 2, 479, 0, 486, 12),
    tile_info(32, 32, 24, 1, 479, 12, 486, 21),
    tile_info(32, 32, 0, 0, 486, 0, 518, 32),
    tile_info(32, 32, 0, 0, 518, 0, 550, 32),
    tile_info(32, 32, 0, 0, 550, 0, 582, 32),
    tile_info(32, 32, 0, 0, 582, 0, 614, 32),
    tile_info(32, 32, 0, 0, 614, 0, 646, 32),
    tile_info(8, 16, 1, 2, 646, 0, 653, 12),
    tile_info(8, 16, 2, 2, 646, 12, 650, 24),
    tile_info(8, 16, 1, 2, 653, 0, 660, 12),
    tile_info(8, 16, 1, 2, 653, 12, 660, 24),
    tile_info(8, 16, 0, 2, 660, 0, 668, 12),
    tile_info(8, 16, 1, 2, 660, 12, 667, 24),
    tile_info(8, 16, 1, 2, 668, 0, 675, 12),
    tile_info(8, 16, 1, 2, 668, 12, 675, 24),
    tile_info(8, 16, 1, 2, 675, 0, 682, 12),
    tile_info(8, 16, 1, 2, 675, 12, 682, 24),
    tile_info(8, 16, 0, 1, 682, 0, 690, 13),
    tile_info(8, 16, 1, 1, 682, 13, 687, 26),
    tile_info(8, 16, 0, 1, 690, 0, 698, 13),
    tile_info(8, 16, 0, 1, 690, 13, 698, 26),
    tile_info(8, 16, 0, 1, 698, 0, 706, 13),
    tile_info(8, 16, 0, 1, 698, 13, 706, 26),
    tile_info(8, 16, 0, 1, 706, 0, 714, 13),
    tile_info(8, 16, 0, 1, 706, 13, 714, 26),
    tile_info(8, 16, 0, 1, 714, 0, 722, 13),
    tile_info(8, 16, 0, 1, 714, 13, 722, 26),
    tile_info(32, 32, 18, 18, 722, 0, 735, 13),
    tile_info(32, 32, 18, 18, 722, 13, 735, 26),
    tile_info(32, 32, 18, 18, 735, 0, 748, 13),
    tile_info(32, 32, 18, 18, 735, 13, 748, 26),
    tile_info(32, 32, 21, 18, 748, 0, 758, 13),
    tile_info(32, 32, 23, 18, 748, 13, 756, 26),
    tile_info(32, 32, 22, 18, 758, 0, 767, 13),
    tile_info(32, 32, 19, 18, 758, 13, 770, 26),
    tile_info(32, 32, 19, 18, 770, 0, 782, 13),
    tile_info(32, 32, 18, 18, 770, 13, 783, 26),
    tile_info(32, 32, 18, 18, 783, 0, 796, 13),
    tile_info(32, 32, 0, 20, 783, 13, 796, 25),
    tile_info(32, 32, 0, 20, 796, 0, 809, 12),
    tile_info(32, 32, 0, 20, 796, 12, 809, 24),
    tile_info(32, 32, 0, 20, 809, 0, 822, 12),
    tile_info(32, 32, 0, 20, 809, 12, 822, 24),
    tile_info(32, 32, 0, 0, 822, 0, 854, 32),
    tile_info(32, 13, 19, 0, 854, 0, 867, 13),
    tile_info(32, 13, 19, 0, 854, 13, 867, 26),
    tile_info(32, 13, 19, 0, 867, 0, 880, 13),
    tile_info(32, 13, 19, 0, 867, 13, 880, 26),
    tile_info(32, 13, 19, 0, 880, 0, 893, 13),
};

tile_info &tile_icons_info(tileidx_t idx)
{
    assert(idx >= 0 && idx < TILEI_ICONS_MAX);
    return _tile_icons_info[idx - 0];
}


typedef std::pair<const char*, tileidx_t> _name_pair;

_name_pair icons_name_pairs[] =
{
    _name_pair("animated_weapon", 16 + 0),
    _name_pair("berserk", 20 + 0),
    _name_pair("cursor", 9 + 0),
    _name_pair("cursor2", 10 + 0),
    _name_pair("cursor3", 11 + 0),
    _name_pair("demon_num1", 62 + 0),
    _name_pair("demon_num2", 63 + 0),
    _name_pair("demon_num3", 64 + 0),
    _name_pair("demon_num4", 65 + 0),
    _name_pair("demon_num5", 66 + 0),
    _name_pair("flame", 19 + 0),
    _name_pair("good_neutral", 14 + 0),
    _name_pair("heart", 13 + 0),
    _name_pair("item_slot_selected", 67 + 0),
    _name_pair("magic_map_mesh", 28 + 0),
    _name_pair("mask_deep_water", 2 + 0),
    _name_pair("mask_deep_water_murky", 4 + 0),
    _name_pair("mask_deep_water_shoals", 6 + 0),
    _name_pair("mask_lava", 8 + 0),
    _name_pair("mask_shallow_water", 3 + 0),
    _name_pair("mask_shallow_water_murky", 5 + 0),
    _name_pair("mask_shallow_water_shoals", 7 + 0),
    _name_pair("may_stab_brand", 21 + 0),
    _name_pair("mdam_almost_dead", 72 + 0),
    _name_pair("mdam_heavily_damaged", 70 + 0),
    _name_pair("mdam_lightly_damaged", 68 + 0),
    _name_pair("mdam_moderately_damaged", 69 + 0),
    _name_pair("mdam_severely_damaged", 71 + 0),
    _name_pair("mesh", 26 + 0),
    _name_pair("mimic", 17 + 0),
    _name_pair("neutral", 15 + 0),
    _name_pair("new_stair", 25 + 0),
    _name_pair("num0", 31 + 0),
    _name_pair("num0_outline", 41 + 0),
    _name_pair("num1", 32 + 0),
    _name_pair("num1_outline", 42 + 0),
    _name_pair("num2", 33 + 0),
    _name_pair("num2_outline", 43 + 0),
    _name_pair("num3", 34 + 0),
    _name_pair("num3_outline", 44 + 0),
    _name_pair("num4", 35 + 0),
    _name_pair("num4_outline", 45 + 0),
    _name_pair("num5", 36 + 0),
    _name_pair("num5_outline", 46 + 0),
    _name_pair("num6", 37 + 0),
    _name_pair("num6_outline", 47 + 0),
    _name_pair("num7", 38 + 0),
    _name_pair("num7_outline", 48 + 0),
    _name_pair("num8", 39 + 0),
    _name_pair("num8_outline", 49 + 0),
    _name_pair("num9", 40 + 0),
    _name_pair("num9_outline", 50 + 0),
    _name_pair("num_minus1", 55 + 0),
    _name_pair("num_minus2", 54 + 0),
    _name_pair("num_minus3", 53 + 0),
    _name_pair("num_minus4", 52 + 0),
    _name_pair("num_minus5", 51 + 0),
    _name_pair("num_plus1", 57 + 0),
    _name_pair("num_plus2", 58 + 0),
    _name_pair("num_plus3", 59 + 0),
    _name_pair("num_plus4", 60 + 0),
    _name_pair("num_plus5", 61 + 0),
    _name_pair("num_zero", 56 + 0),
    _name_pair("oor_mesh", 27 + 0),
    _name_pair("poison", 18 + 0),
    _name_pair("something_under", 23 + 0),
    _name_pair("stab_brand", 22 + 0),
    _name_pair("trap_net", 1 + 0),
    _name_pair("travel_exclusion_centre_fg", 30 + 0),
    _name_pair("travel_exclusion_fg", 29 + 0),
    _name_pair("tried", 24 + 0),
    _name_pair("tutorial_cursor", 12 + 0),
};

bool tile_icons_index(const char *str, tileidx_t *idx)
{
    assert(str);
    if (!str)
        return false;

    string lc = str;
    for (unsigned int i = 0; i < lc.size(); i++)
        lc[i] = tolower(lc[i]);

    int num_pairs = sizeof(icons_name_pairs) / sizeof(icons_name_pairs[0]);
    bool result = binary_search<const char *, tileidx_t>(
       lc.c_str(), &icons_name_pairs[0], num_pairs, &strcmp, idx);
    return (result);
}

bool tile_icons_equal(tileidx_t tile, tileidx_t idx)
{
    assert(tile >= 0 && tile < TILEI_ICONS_MAX);
    return (idx >= tile && idx < tile + tile_icons_count(tile));
}


typedef std::pair<tile_variation, tileidx_t> _colour_pair;

_colour_pair icons_colour_pairs[] =
{
    _colour_pair(tile_variation(0, 0), 0),
};

tileidx_t tile_icons_coloured(tileidx_t idx, int col)
{
    int num_pairs = sizeof(icons_colour_pairs) / sizeof(icons_colour_pairs[0]);
    tile_variation key(idx, col);
    tileidx_t found;
    bool result = binary_search<tile_variation, tileidx_t>(
       key, &icons_colour_pairs[0], num_pairs,
       &tile_variation::cmp, &found);
    return (result ? found : idx);
}

