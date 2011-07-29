// This file has been automatically generated.

#ifndef TILEDEF_DNGN_H
#define TILEDEF_DNGN_H

#include "tiledef_defines.h"

#include "tiledef-floor.h"
#include "tiledef-wall.h"
#include "tiledef-feat.h"


enum tile_dngn_type
{
    TILE_DNGN_MAX = TILE_FEAT_MAX
};

unsigned int tile_dngn_count(tileidx_t idx);
tileidx_t tile_dngn_basetile(tileidx_t idx);
int tile_dngn_probs(tileidx_t idx);
const char *tile_dngn_name(tileidx_t idx);
tile_info &tile_dngn_info(tileidx_t idx);
bool tile_dngn_index(const char *str, tileidx_t *idx);
bool tile_dngn_equal(tileidx_t tile, tileidx_t idx);
tileidx_t tile_dngn_coloured(tileidx_t idx, int col);

#endif

