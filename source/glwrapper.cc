#include "AppHdr.h"

#ifdef USE_TILE

#include "glwrapper.h"

/////////////////////////////////////////////////////////////////////////////
// VColour

VColour VColour::white(255, 255, 255, 255);
VColour VColour::black(0, 0, 0, 255);
VColour VColour::transparent(0, 0, 0, 0);

bool VColour::operator==(const VColour &vc) const
{
    return (r == vc.r && g == vc.g && b == vc.b && a == vc.a);
}

bool VColour::operator!=(const VColour &vc) const
{
    return (r != vc.r || g != vc.g || b != vc.b || a != vc.a);
}

/////////////////////////////////////////////////////////////////////////////
// GLState

// Note: these defaults should match the OpenGL defaults
GLState::GLState() :
    array_vertex(false),
    array_texcoord(false),
    array_colour(false),
    blend(false),
    texture(false),
    depthtest(false),
    alphatest(false),
    alpharef(0),
    colour(VColour::white)
{
}

GLState::GLState(const GLState &state) :
    array_vertex(state.array_vertex),
    array_texcoord(state.array_texcoord),
    array_colour(state.array_colour),
    blend(state.blend),
    texture(state.texture),
    depthtest(state.depthtest),
    alphatest(state.alphatest),
    alpharef(state.alpharef),
    colour(state.colour)
{
}

const GLState &GLState::operator=(const GLState &state)
{
    array_vertex = state.array_vertex;
    array_texcoord = state.array_texcoord;
    array_colour = state.array_colour;
    blend = state.blend;
    texture = state.texture;
    depthtest = state.depthtest;
    alphatest = state.alphatest;
    alpharef = state.alpharef;
    colour = state.colour;

    return (*this);
}

bool GLState::operator==(const GLState &state) const
{
    return (array_vertex == state.array_vertex
            && array_texcoord == state.array_texcoord
            && array_colour == state.array_colour
            && blend == state.blend
            && texture == state.texture
            && depthtest == state.depthtest
            && alphatest == state.alphatest
            && alpharef == state.alpharef
            && colour == state.colour);
}

/////////////////////////////////////////////////////////////////////////////
// GLStateManager

#ifdef ASSERTS
bool GLStateManager::_valid(int num_verts, drawing_modes mode)
{
    switch (mode)
    {
    case GLW_RECTANGLE:
        return (num_verts % 4 == 0);
    case GLW_LINES:
        return (num_verts % 2 == 0);
    default:
        return (false);
    }
}
#endif // ASSERTS

#endif // USE_TILE
