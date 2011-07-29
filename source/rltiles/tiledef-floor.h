// This file has been automatically generated.

#ifndef TILEDEF_FLOOR_H
#define TILEDEF_FLOOR_H

#include "tiledef_defines.h"



enum tile_floor_type
{
    TILE_DNGN_UNSEEN = 0,
    TILE_DNGN_ERROR,
    TILE_FLOOR_GREY_DIRT,
    TILE_FLOOR_NORMAL = TILE_FLOOR_GREY_DIRT,
    TILE_FLOOR_GREY_DIRT_1,
    TILE_FLOOR_GREY_DIRT_2,
    TILE_FLOOR_GREY_DIRT_3,
    TILE_FLOOR_GREY_DIRT_4,
    TILE_FLOOR_GREY_DIRT_5,
    TILE_FLOOR_GREY_DIRT_6,
    TILE_FLOOR_GREY_DIRT_7,
    TILE_FLOOR_PEBBLE,
    TILE_FLOOR_PEBBLE_LIGHTGRAY = TILE_FLOOR_PEBBLE,
    TILE_FLOOR_PEBBLE_1,
    TILE_FLOOR_PEBBLE_2,
    TILE_FLOOR_PEBBLE_3,
    TILE_FLOOR_PEBBLE_4,
    TILE_FLOOR_PEBBLE_5,
    TILE_FLOOR_PEBBLE_6,
    TILE_FLOOR_PEBBLE_7,
    TILE_FLOOR_PEBBLE_8,
    TILE_FLOOR_PEBBLE_BROWN,
    TILE_FLOOR_PEBBLE_BROWN_1,
    TILE_FLOOR_PEBBLE_BROWN_2,
    TILE_FLOOR_PEBBLE_BROWN_3,
    TILE_FLOOR_PEBBLE_BROWN_4,
    TILE_FLOOR_PEBBLE_BROWN_5,
    TILE_FLOOR_PEBBLE_BROWN_6,
    TILE_FLOOR_PEBBLE_BROWN_7,
    TILE_FLOOR_PEBBLE_BROWN_8,
    TILE_FLOOR_PEBBLE_BLUE,
    TILE_FLOOR_PEBBLE_BLUE_1,
    TILE_FLOOR_PEBBLE_BLUE_2,
    TILE_FLOOR_PEBBLE_BLUE_3,
    TILE_FLOOR_PEBBLE_BLUE_4,
    TILE_FLOOR_PEBBLE_BLUE_5,
    TILE_FLOOR_PEBBLE_BLUE_6,
    TILE_FLOOR_PEBBLE_BLUE_7,
    TILE_FLOOR_PEBBLE_BLUE_8,
    TILE_FLOOR_PEBBLE_GREEN,
    TILE_FLOOR_PEBBLE_GREEN_1,
    TILE_FLOOR_PEBBLE_GREEN_2,
    TILE_FLOOR_PEBBLE_GREEN_3,
    TILE_FLOOR_PEBBLE_GREEN_4,
    TILE_FLOOR_PEBBLE_GREEN_5,
    TILE_FLOOR_PEBBLE_GREEN_6,
    TILE_FLOOR_PEBBLE_GREEN_7,
    TILE_FLOOR_PEBBLE_GREEN_8,
    TILE_FLOOR_PEBBLE_CYAN,
    TILE_FLOOR_PEBBLE_CYAN_1,
    TILE_FLOOR_PEBBLE_CYAN_2,
    TILE_FLOOR_PEBBLE_CYAN_3,
    TILE_FLOOR_PEBBLE_CYAN_4,
    TILE_FLOOR_PEBBLE_CYAN_5,
    TILE_FLOOR_PEBBLE_CYAN_6,
    TILE_FLOOR_PEBBLE_CYAN_7,
    TILE_FLOOR_PEBBLE_CYAN_8,
    TILE_FLOOR_PEBBLE_RED,
    TILE_FLOOR_PEBBLE_RED_1,
    TILE_FLOOR_PEBBLE_RED_2,
    TILE_FLOOR_PEBBLE_RED_3,
    TILE_FLOOR_PEBBLE_RED_4,
    TILE_FLOOR_PEBBLE_RED_5,
    TILE_FLOOR_PEBBLE_RED_6,
    TILE_FLOOR_PEBBLE_RED_7,
    TILE_FLOOR_PEBBLE_RED_8,
    TILE_FLOOR_PEBBLE_MAGENTA,
    TILE_FLOOR_PEBBLE_MAGENTA_1,
    TILE_FLOOR_PEBBLE_MAGENTA_2,
    TILE_FLOOR_PEBBLE_MAGENTA_3,
    TILE_FLOOR_PEBBLE_MAGENTA_4,
    TILE_FLOOR_PEBBLE_MAGENTA_5,
    TILE_FLOOR_PEBBLE_MAGENTA_6,
    TILE_FLOOR_PEBBLE_MAGENTA_7,
    TILE_FLOOR_PEBBLE_MAGENTA_8,
    TILE_FLOOR_PEBBLE_DARKGRAY,
    TILE_FLOOR_PEBBLE_DARKGRAY_1,
    TILE_FLOOR_PEBBLE_DARKGRAY_2,
    TILE_FLOOR_PEBBLE_DARKGRAY_3,
    TILE_FLOOR_PEBBLE_DARKGRAY_4,
    TILE_FLOOR_PEBBLE_DARKGRAY_5,
    TILE_FLOOR_PEBBLE_DARKGRAY_6,
    TILE_FLOOR_PEBBLE_DARKGRAY_7,
    TILE_FLOOR_PEBBLE_DARKGRAY_8,
    TILE_FLOOR_PEBBLE_LIGHTBLUE,
    TILE_FLOOR_PEBBLE_LIGHTBLUE_1,
    TILE_FLOOR_PEBBLE_LIGHTBLUE_2,
    TILE_FLOOR_PEBBLE_LIGHTBLUE_3,
    TILE_FLOOR_PEBBLE_LIGHTBLUE_4,
    TILE_FLOOR_PEBBLE_LIGHTBLUE_5,
    TILE_FLOOR_PEBBLE_LIGHTBLUE_6,
    TILE_FLOOR_PEBBLE_LIGHTBLUE_7,
    TILE_FLOOR_PEBBLE_LIGHTBLUE_8,
    TILE_FLOOR_PEBBLE_LIGHTGREEN,
    TILE_FLOOR_PEBBLE_LIGHTGREEN_1,
    TILE_FLOOR_PEBBLE_LIGHTGREEN_2,
    TILE_FLOOR_PEBBLE_LIGHTGREEN_3,
    TILE_FLOOR_PEBBLE_LIGHTGREEN_4,
    TILE_FLOOR_PEBBLE_LIGHTGREEN_5,
    TILE_FLOOR_PEBBLE_LIGHTGREEN_6,
    TILE_FLOOR_PEBBLE_LIGHTGREEN_7,
    TILE_FLOOR_PEBBLE_LIGHTGREEN_8,
    TILE_FLOOR_PEBBLE_LIGHTCYAN,
    TILE_FLOOR_PEBBLE_LIGHTCYAN_1,
    TILE_FLOOR_PEBBLE_LIGHTCYAN_2,
    TILE_FLOOR_PEBBLE_LIGHTCYAN_3,
    TILE_FLOOR_PEBBLE_LIGHTCYAN_4,
    TILE_FLOOR_PEBBLE_LIGHTCYAN_5,
    TILE_FLOOR_PEBBLE_LIGHTCYAN_6,
    TILE_FLOOR_PEBBLE_LIGHTCYAN_7,
    TILE_FLOOR_PEBBLE_LIGHTCYAN_8,
    TILE_FLOOR_PEBBLE_LIGHTRED,
    TILE_FLOOR_PEBBLE_LIGHTRED_1,
    TILE_FLOOR_PEBBLE_LIGHTRED_2,
    TILE_FLOOR_PEBBLE_LIGHTRED_3,
    TILE_FLOOR_PEBBLE_LIGHTRED_4,
    TILE_FLOOR_PEBBLE_LIGHTRED_5,
    TILE_FLOOR_PEBBLE_LIGHTRED_6,
    TILE_FLOOR_PEBBLE_LIGHTRED_7,
    TILE_FLOOR_PEBBLE_LIGHTRED_8,
    TILE_FLOOR_PEBBLE_LIGHTMAGENTA,
    TILE_FLOOR_PEBBLE_LIGHTMAGENTA_1,
    TILE_FLOOR_PEBBLE_LIGHTMAGENTA_2,
    TILE_FLOOR_PEBBLE_LIGHTMAGENTA_3,
    TILE_FLOOR_PEBBLE_LIGHTMAGENTA_4,
    TILE_FLOOR_PEBBLE_LIGHTMAGENTA_5,
    TILE_FLOOR_PEBBLE_LIGHTMAGENTA_6,
    TILE_FLOOR_PEBBLE_LIGHTMAGENTA_7,
    TILE_FLOOR_PEBBLE_LIGHTMAGENTA_8,
    TILE_FLOOR_PEBBLE_YELLOW,
    TILE_FLOOR_PEBBLE_YELLOW_1,
    TILE_FLOOR_PEBBLE_YELLOW_2,
    TILE_FLOOR_PEBBLE_YELLOW_3,
    TILE_FLOOR_PEBBLE_YELLOW_4,
    TILE_FLOOR_PEBBLE_YELLOW_5,
    TILE_FLOOR_PEBBLE_YELLOW_6,
    TILE_FLOOR_PEBBLE_YELLOW_7,
    TILE_FLOOR_PEBBLE_YELLOW_8,
    TILE_FLOOR_PEBBLE_WHITE,
    TILE_FLOOR_PEBBLE_WHITE_1,
    TILE_FLOOR_PEBBLE_WHITE_2,
    TILE_FLOOR_PEBBLE_WHITE_3,
    TILE_FLOOR_PEBBLE_WHITE_4,
    TILE_FLOOR_PEBBLE_WHITE_5,
    TILE_FLOOR_PEBBLE_WHITE_6,
    TILE_FLOOR_PEBBLE_WHITE_7,
    TILE_FLOOR_PEBBLE_WHITE_8,
    TILE_FLOOR_HALL,
    TILE_FLOOR_HALL_1,
    TILE_FLOOR_HALL_2,
    TILE_FLOOR_HALL_3,
    TILE_FLOOR_HIVE,
    TILE_FLOOR_ORC = TILE_FLOOR_HIVE,
    TILE_FLOOR_HIVE_1,
    TILE_FLOOR_HIVE_2,
    TILE_FLOOR_HIVE_3,
    TILE_FLOOR_ICE,
    TILE_FLOOR_ICE_1,
    TILE_FLOOR_ICE_2,
    TILE_FLOOR_ICE_3,
    TILE_FLOOR_LAIR,
    TILE_FLOOR_LAIR_1,
    TILE_FLOOR_LAIR_2,
    TILE_FLOOR_LAIR_3,
    TILE_FLOOR_LAIR_4,
    TILE_FLOOR_LAIR_5,
    TILE_FLOOR_LAIR_6,
    TILE_FLOOR_LAIR_7,
    TILE_FLOOR_LAIR_8,
    TILE_FLOOR_LAIR_9,
    TILE_FLOOR_LAIR_10,
    TILE_FLOOR_LAIR_11,
    TILE_FLOOR_LAIR_12,
    TILE_FLOOR_LAIR_13,
    TILE_FLOOR_LAIR_14,
    TILE_FLOOR_LAIR_15,
    TILE_FLOOR_MOSS,
    TILE_FLOOR_MOSS_1,
    TILE_FLOOR_MOSS_2,
    TILE_FLOOR_MOSS_3,
    TILE_FLOOR_SLIME,
    TILE_FLOOR_SLIME_1,
    TILE_FLOOR_SLIME_2,
    TILE_FLOOR_SLIME_3,
    TILE_FLOOR_SNAKE,
    TILE_FLOOR_SNAKE_1,
    TILE_FLOOR_SNAKE_2,
    TILE_FLOOR_SNAKE_3,
    TILE_FLOOR_SWAMP,
    TILE_FLOOR_SWAMP_1,
    TILE_FLOOR_SWAMP_2,
    TILE_FLOOR_SWAMP_3,
    TILE_FLOOR_TOMB,
    TILE_FLOOR_TOMB_1,
    TILE_FLOOR_TOMB_2,
    TILE_FLOOR_TOMB_3,
    TILE_FLOOR_VAULT,
    TILE_FLOOR_VAULT_1,
    TILE_FLOOR_VAULT_2,
    TILE_FLOOR_VAULT_3,
    TILE_FLOOR_VINES,
    TILE_FLOOR_VINES_1,
    TILE_FLOOR_VINES_2,
    TILE_FLOOR_VINES_3,
    TILE_FLOOR_VINES_4,
    TILE_FLOOR_VINES_5,
    TILE_FLOOR_VINES_6,
    TILE_FLOOR_ROUGH,
    TILE_FLOOR_ROUGH_RED = TILE_FLOOR_ROUGH,
    TILE_FLOOR_ROUGH_1,
    TILE_FLOOR_ROUGH_2,
    TILE_FLOOR_ROUGH_3,
    TILE_FLOOR_ROUGH_BLUE,
    TILE_FLOOR_ROUGH_BLUE_1,
    TILE_FLOOR_ROUGH_BLUE_2,
    TILE_FLOOR_ROUGH_BLUE_3,
    TILE_FLOOR_ROUGH_GREEN,
    TILE_FLOOR_ROUGH_GREEN_1,
    TILE_FLOOR_ROUGH_GREEN_2,
    TILE_FLOOR_ROUGH_GREEN_3,
    TILE_FLOOR_ROUGH_CYAN,
    TILE_FLOOR_ROUGH_CYAN_1,
    TILE_FLOOR_ROUGH_CYAN_2,
    TILE_FLOOR_ROUGH_CYAN_3,
    TILE_FLOOR_ROUGH_MAGENTA,
    TILE_FLOOR_ROUGH_MAGENTA_1,
    TILE_FLOOR_ROUGH_MAGENTA_2,
    TILE_FLOOR_ROUGH_MAGENTA_3,
    TILE_FLOOR_ROUGH_BROWN,
    TILE_FLOOR_ROUGH_BROWN_1,
    TILE_FLOOR_ROUGH_BROWN_2,
    TILE_FLOOR_ROUGH_BROWN_3,
    TILE_FLOOR_ROUGH_LIGHTGRAY,
    TILE_FLOOR_ROUGH_LIGHTGRAY_1,
    TILE_FLOOR_ROUGH_LIGHTGRAY_2,
    TILE_FLOOR_ROUGH_LIGHTGRAY_3,
    TILE_FLOOR_ROUGH_DARKGRAY,
    TILE_FLOOR_ROUGH_DARKGRAY_1,
    TILE_FLOOR_ROUGH_DARKGRAY_2,
    TILE_FLOOR_ROUGH_DARKGRAY_3,
    TILE_FLOOR_ROUGH_LIGHTBLUE,
    TILE_FLOOR_ROUGH_LIGHTBLUE_1,
    TILE_FLOOR_ROUGH_LIGHTBLUE_2,
    TILE_FLOOR_ROUGH_LIGHTBLUE_3,
    TILE_FLOOR_ROUGH_LIGHTGREEN,
    TILE_FLOOR_ROUGH_LIGHTGREEN_1,
    TILE_FLOOR_ROUGH_LIGHTGREEN_2,
    TILE_FLOOR_ROUGH_LIGHTGREEN_3,
    TILE_FLOOR_ROUGH_LIGHTCYAN,
    TILE_FLOOR_ROUGH_LIGHTCYAN_1,
    TILE_FLOOR_ROUGH_LIGHTCYAN_2,
    TILE_FLOOR_ROUGH_LIGHTCYAN_3,
    TILE_FLOOR_ROUGH_LIGHTRED,
    TILE_FLOOR_ROUGH_LIGHTRED_1,
    TILE_FLOOR_ROUGH_LIGHTRED_2,
    TILE_FLOOR_ROUGH_LIGHTRED_3,
    TILE_FLOOR_ROUGH_LIGHTMAGENTA,
    TILE_FLOOR_ROUGH_LIGHTMAGENTA_1,
    TILE_FLOOR_ROUGH_LIGHTMAGENTA_2,
    TILE_FLOOR_ROUGH_LIGHTMAGENTA_3,
    TILE_FLOOR_ROUGH_YELLOW,
    TILE_FLOOR_ROUGH_YELLOW_1,
    TILE_FLOOR_ROUGH_YELLOW_2,
    TILE_FLOOR_ROUGH_YELLOW_3,
    TILE_FLOOR_ROUGH_WHITE,
    TILE_FLOOR_ROUGH_WHITE_1,
    TILE_FLOOR_ROUGH_WHITE_2,
    TILE_FLOOR_ROUGH_WHITE_3,
    TILE_FLOOR_SAND_STONE,
    TILE_FLOOR_SAND_STONE_1,
    TILE_FLOOR_SAND_STONE_2,
    TILE_FLOOR_SAND_STONE_3,
    TILE_FLOOR_SAND_STONE_4,
    TILE_FLOOR_SAND_STONE_5,
    TILE_FLOOR_SAND_STONE_6,
    TILE_FLOOR_SAND_STONE_7,
    TILE_FLOOR_COBBLE_BLOOD,
    TILE_FLOOR_COBBLE_BLOOD_1,
    TILE_FLOOR_COBBLE_BLOOD_2,
    TILE_FLOOR_COBBLE_BLOOD_3,
    TILE_FLOOR_COBBLE_BLOOD_4,
    TILE_FLOOR_COBBLE_BLOOD_5,
    TILE_FLOOR_COBBLE_BLOOD_6,
    TILE_FLOOR_COBBLE_BLOOD_7,
    TILE_FLOOR_COBBLE_BLOOD_8,
    TILE_FLOOR_COBBLE_BLOOD_9,
    TILE_FLOOR_COBBLE_BLOOD_10,
    TILE_FLOOR_COBBLE_BLOOD_11,
    TILE_FLOOR_MARBLE,
    TILE_FLOOR_MARBLE_1,
    TILE_FLOOR_MARBLE_2,
    TILE_FLOOR_MARBLE_3,
    TILE_FLOOR_MARBLE_4,
    TILE_FLOOR_MARBLE_5,
    TILE_FLOOR_SANDSTONE,
    TILE_FLOOR_SANDSTONE_1,
    TILE_FLOOR_SANDSTONE_2,
    TILE_FLOOR_SANDSTONE_3,
    TILE_FLOOR_SANDSTONE_4,
    TILE_FLOOR_SANDSTONE_5,
    TILE_FLOOR_SANDSTONE_6,
    TILE_FLOOR_SANDSTONE_7,
    TILE_FLOOR_SANDSTONE_8,
    TILE_FLOOR_SANDSTONE_9,
    TILE_FLOOR_VOLCANIC,
    TILE_FLOOR_VOLCANIC_1,
    TILE_FLOOR_VOLCANIC_2,
    TILE_FLOOR_VOLCANIC_3,
    TILE_FLOOR_VOLCANIC_4,
    TILE_FLOOR_VOLCANIC_5,
    TILE_FLOOR_VOLCANIC_6,
    TILE_FLOOR_CRYSTAL_SQUARES,
    TILE_FLOOR_CRYSTAL_SQUARES_1,
    TILE_FLOOR_CRYSTAL_SQUARES_2,
    TILE_FLOOR_CRYSTAL_SQUARES_3,
    TILE_FLOOR_CRYSTAL_SQUARES_4,
    TILE_FLOOR_CRYSTAL_SQUARES_5,
    TILE_FLOOR_GRASS,
    TILE_FLOOR_GRASS_1,
    TILE_FLOOR_GRASS_2,
    TILE_FLOOR_GRASS_3,
    TILE_FLOOR_GRASS_4,
    TILE_FLOOR_GRASS_5,
    TILE_FLOOR_GRASS_6,
    TILE_FLOOR_GRASS_7,
    TILE_FLOOR_GRASS_8,
    TILE_FLOOR_GRASS_9,
    TILE_FLOOR_GRASS_10,
    TILE_FLOOR_GRASS_11,
    TILE_HALO_GRASS,
    TILE_HALO_GRASS_1,
    TILE_HALO_GRASS_2,
    TILE_HALO_GRASS_3,
    TILE_HALO_GRASS_4,
    TILE_HALO_GRASS_5,
    TILE_HALO_GRASS_6,
    TILE_HALO_GRASS_7,
    TILE_HALO_GRASS_8,
    TILE_FLOOR_GRASS_DIRT_MIX,
    TILE_FLOOR_GRASS_DIRT_MIX_1,
    TILE_FLOOR_GRASS_DIRT_MIX_2,
    TILE_FLOOR_NERVES,
    TILE_FLOOR_NERVES_1,
    TILE_FLOOR_NERVES_2,
    TILE_FLOOR_NERVES_3,
    TILE_FLOOR_NERVES_4,
    TILE_FLOOR_NERVES_5,
    TILE_FLOOR_NERVES_6,
    TILE_HALO_GRASS2,
    TILE_HALO_GRASS2_1,
    TILE_HALO_GRASS2_2,
    TILE_HALO_GRASS2_3,
    TILE_HALO_GRASS2_4,
    TILE_HALO_GRASS2_5,
    TILE_HALO_GRASS2_6,
    TILE_HALO_GRASS2_7,
    TILE_HALO_GRASS2_8,
    TILE_HALO_VAULT,
    TILE_HALO_VAULT_1,
    TILE_HALO_VAULT_2,
    TILE_HALO_VAULT_3,
    TILE_HALO_VAULT_4,
    TILE_HALO_VAULT_5,
    TILE_HALO_VAULT_6,
    TILE_HALO_VAULT_7,
    TILE_HALO_VAULT_8,
    TILE_FLOOR_DIRT,
    TILE_FLOOR_DIRT_1,
    TILE_FLOOR_DIRT_2,
    TILE_HALO_DIRT,
    TILE_HALO_DIRT_1,
    TILE_HALO_DIRT_2,
    TILE_HALO_DIRT_3,
    TILE_HALO_DIRT_4,
    TILE_HALO_DIRT_5,
    TILE_HALO_DIRT_6,
    TILE_HALO_DIRT_7,
    TILE_HALO_DIRT_8,
    TILE_TUTORIAL_PAD,
    TILE_FLOOR_LIMESTONE,
    TILE_FLOOR_LIMESTONE_1,
    TILE_FLOOR_LIMESTONE_2,
    TILE_FLOOR_LIMESTONE_3,
    TILE_FLOOR_LIMESTONE_4,
    TILE_FLOOR_LIMESTONE_5,
    TILE_FLOOR_LIMESTONE_6,
    TILE_FLOOR_LIMESTONE_7,
    TILE_FLOOR_LIMESTONE_8,
    TILE_FLOOR_LIMESTONE_9,
    TILE_SIGIL_CURVE_N_E,
    TILE_SIGIL_CURVE_N_W,
    TILE_SIGIL_CURVE_S_E,
    TILE_SIGIL_CURVE_S_W,
    TILE_SIGIL_STRAIGHT_E_W,
    TILE_SIGIL_STRAIGHT_N_S,
    TILE_SIGIL_STRAIGHT_NE_SW,
    TILE_SIGIL_STRAIGHT_NW_SE,
    TILE_SIGIL_CROSS,
    TILE_SIGIL_CIRCLE,
    TILE_SIGIL_RHOMBUS,
    TILE_SIGIL_Y,
    TILE_SIGIL_Y_INVERTED,
    TILE_SIGIL_Y_RIGHT,
    TILE_SIGIL_Y_LEFT,
    TILE_SIGIL_Y_LEFTLEANING,
    TILE_SIGIL_Y_RIGHTLEANING,
    TILE_SIGIL_ALGIZ_LEFT,
    TILE_SIGIL_ALGIZ_RIGHT,
    TILE_SIGIL_STRAIGHT_E_NW,
    TILE_SIGIL_STRAIGHT_E_SW,
    TILE_SIGIL_STRAIGHT_W_NE,
    TILE_SIGIL_STRAIGHT_W_SE,
    TILE_SIGIL_STRAIGHT_N_SE,
    TILE_SIGIL_STRAIGHT_N_SW,
    TILE_SIGIL_STRAIGHT_S_NE,
    TILE_SIGIL_STRAIGHT_S_NW,
    TILE_SIGIL_FOURWAY,
    TILE_SIGIL_SHARP_E_NE,
    TILE_SIGIL_SHARP_W_SW,
    TILE_SIGIL_STRAIGHT_E_NE_SW,
    TILE_FLOOR_MAX
};

unsigned int tile_floor_count(tileidx_t idx);
tileidx_t tile_floor_basetile(tileidx_t idx);
int tile_floor_probs(tileidx_t idx);
const char *tile_floor_name(tileidx_t idx);
tile_info &tile_floor_info(tileidx_t idx);
bool tile_floor_index(const char *str, tileidx_t *idx);
bool tile_floor_equal(tileidx_t tile, tileidx_t idx);
tileidx_t tile_floor_coloured(tileidx_t idx, int col);

#endif

