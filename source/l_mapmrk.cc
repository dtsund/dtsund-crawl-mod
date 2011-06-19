#include "AppHdr.h"

#include "cluautil.h"
#include "l_libs.h"

#include "env.h"
#include "mapmark.h"

static int mapmarker_pos(lua_State *ls)
{
    MAPMARKER(ls, 1, mark);
    lua_pushnumber(ls, mark->pos.x);
    lua_pushnumber(ls, mark->pos.y);
    return (2);
}

static int mapmarker_move(lua_State *ls)
{
    MAPMARKER(ls, 1, mark);
    const coord_def dest(luaL_checkint(ls, 2), luaL_checkint(ls, 3));
    env.markers.move_marker(mark, dest);
    return (0);
}

static int mapmarker_remove(lua_State *ls)
{
    MAPMARKER(ls, 1, mark);
    env.markers.remove(mark);
    return (0);
}

const struct luaL_reg mapmarker_dlib[] =
{
{ "pos", mapmarker_pos },
{ "move", mapmarker_move },
{ "remove", mapmarker_remove },
{ NULL, NULL }
};

void luaopen_mapmarker(lua_State *ls)
{
    luaopen_setmeta(ls, "mapmarker", mapmarker_dlib, MAPMARK_METATABLE);
}
