#include "AppHdr.h"

#ifdef USE_TILE
#ifdef USE_GL

#include "glwrapper-ogl.h"

// How do we get access to the GL calls?
// If other UI types use the -ogl wrapper they should
// include more conditional includes here.
#ifdef USE_SDL
#ifdef USE_GLES
#include <SDL.h>
#include <SDL_gles.h>
#include <GLES/gl.h>
#else
#include <SDL_opengl.h>
#endif
#endif

#include "debug.h"

/////////////////////////////////////////////////////////////////////////////
// Static functions from GLStateManager

GLStateManager *glmanager = NULL;

void GLStateManager::init()
{
    if (glmanager)
        return;

    glmanager = new OGLStateManager();
}

void GLStateManager::shutdown()
{
    delete glmanager;
    glmanager = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// Static functions from GLShapeBuffer

GLShapeBuffer *GLShapeBuffer::create(bool texture, bool colour,
                                     drawing_modes prim)
{
    return (new OGLShapeBuffer(texture, colour, prim));
}

/////////////////////////////////////////////////////////////////////////////
// OGLStateManager

OGLStateManager::OGLStateManager()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 1.0f);
    glDepthFunc(GL_LEQUAL);
}

void OGLStateManager::set(const GLState& state)
{
    if (state.array_vertex != m_current_state.array_vertex)
    {
        if (state.array_vertex)
            glEnableClientState(GL_VERTEX_ARRAY);
        else
            glDisableClientState(GL_VERTEX_ARRAY);
    }

    if (state.array_texcoord != m_current_state.array_texcoord)
    {
        if (state.array_texcoord)
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        else
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if (state.array_colour != m_current_state.array_colour)
    {
        if (state.array_colour)
        {
            glEnableClientState(GL_COLOR_ARRAY);
        }
        else
        {
            glDisableClientState(GL_COLOR_ARRAY);

            // [enne] This should *not* be necessary, but the Linux OpenGL
            // driver that I'm using sets this to the last colour of the
            // colour array.  So, we need to unset it here.
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    if (state.texture != m_current_state.texture)
    {
        if (state.texture)
            glEnable(GL_TEXTURE_2D);
        else
            glDisable(GL_TEXTURE_2D);
    }

    if (state.blend != m_current_state.blend)
    {
        if (state.blend)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
    }

    if (state.depthtest != m_current_state.depthtest)
    {
        if (state.depthtest)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    if (state.alphatest != m_current_state.alphatest
        || state.alpharef != m_current_state.alpharef)
    {
        if (state.alphatest)
        {
            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_NOTEQUAL, state.alpharef);
        }
        else
            glDisable(GL_ALPHA_TEST);
    }

    if (state.colour != m_current_state.colour)
    {
        glColor4f(state.colour.r, state.colour.g,
                  state.colour.b, state.colour.a);
    }

    m_current_state = state;
}

void OGLStateManager::set_transform(const GLW_3VF &trans, const GLW_3VF &scale)
{
    glLoadIdentity();
    glTranslatef(trans.x, trans.y, trans.z);
    glScalef(scale.x, scale.y, scale.z);
}

void OGLStateManager::reset_view_for_resize(const coord_def &m_windowsz)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // For ease, vertex positions are pixel positions.
#ifdef USE_GLES
    glOrthox(0, m_windowsz.x, m_windowsz.y, 0, -1000, 1000);
#else
    glOrtho(0, m_windowsz.x, m_windowsz.y, 0, -1000, 1000);
#endif
}

void OGLStateManager::reset_transform()
{
    glLoadIdentity();
    glTranslatef(0,0,0);
    glScalef(1,1,1);
}

void OGLStateManager::pixelstore_unpack_alignment(unsigned int bpp)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, bpp);
}

void OGLStateManager::delete_textures(size_t count, unsigned int *textures)
{
    glDeleteTextures(count, (GLuint*)textures);
}

void OGLStateManager::generate_textures(size_t count, unsigned int *textures)
{
    glGenTextures(count, (GLuint*)textures);
}

void OGLStateManager::bind_texture(unsigned int texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
}

void OGLStateManager::load_texture(unsigned char *pixels, unsigned int width,
                                   unsigned int height, MipMapOptions mip_opt)
{
    // Assumptions...
    const unsigned int bpp = 4;
    const GLenum texture_format = GL_RGBA;
    const GLenum format = GL_UNSIGNED_BYTE;
    // Also assume that the texture is already bound using bind_texture

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#ifdef GL_CLAMP
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#else
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
#ifndef USE_GLES
    if (mip_opt == MIPMAP_CREATE)
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, bpp, width, height,
                          texture_format, format, pixels);
    }
    else
#endif
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, bpp, width, height, 0,
                     texture_format, format, pixels);
    }
}

void OGLStateManager::reset_view_for_redraw(float x, float y)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(x, y , 1.0f);
}

/////////////////////////////////////////////////////////////////////////////
// OGLShapeBuffer

OGLShapeBuffer::OGLShapeBuffer(bool texture, bool colour, drawing_modes prim) :
    m_prim_type(prim),
    m_texture_verts(texture),
    m_colour_verts(colour)
{
    ASSERT(prim == GLW_RECTANGLE || prim == GLW_LINES);
}

const char *OGLShapeBuffer::print_statistics() const
{
    return (NULL);
}

unsigned int OGLShapeBuffer::size() const
{
    return (m_position_buffer.size());
}

void OGLShapeBuffer::add(const GLWPrim &rect)
{
    switch (m_prim_type)
    {
    case GLW_RECTANGLE:
        add_rect(rect);
        break;
    case GLW_LINES:
        add_line(rect);
        break;
    default:
        die("Invalid primitive type");
        break;
    }
}

void OGLShapeBuffer::add_rect(const GLWPrim &rect)
{
    // Copy vert positions
    size_t last = m_position_buffer.size();
    m_position_buffer.resize(last + 4);
    m_position_buffer[last    ].set(rect.pos_sx, rect.pos_sy, rect.pos_z);
    m_position_buffer[last + 1].set(rect.pos_sx, rect.pos_ey, rect.pos_z);
    m_position_buffer[last + 2].set(rect.pos_ex, rect.pos_sy, rect.pos_z);
    m_position_buffer[last + 3].set(rect.pos_ex, rect.pos_ey, rect.pos_z);

    // Copy texture coords if necessary
    if (m_texture_verts)
    {
        last = m_texture_buffer.size();
        m_texture_buffer.resize(last + 4);
        m_texture_buffer[last    ].set(rect.tex_sx, rect.tex_sy);
        m_texture_buffer[last + 1].set(rect.tex_sx, rect.tex_ey);
        m_texture_buffer[last + 2].set(rect.tex_ex, rect.tex_sy);
        m_texture_buffer[last + 3].set(rect.tex_ex, rect.tex_ey);
    }

    // Copy vert colours if necessary
    if (m_colour_verts)
    {
        last = m_colour_buffer.size();
        m_colour_buffer.resize(last + 4);
        m_colour_buffer[last    ].set(rect.col_s);
        m_colour_buffer[last + 1].set(rect.col_e);
        m_colour_buffer[last + 2].set(rect.col_s);
        m_colour_buffer[last + 3].set(rect.col_e);
    }

    // build indices
    last = m_ind_buffer.size();

    if (last > 3)
    {
        // This is not the first box so make FOUR degenerate triangles
        m_ind_buffer.resize(last + 6);
        unsigned short int val = m_ind_buffer[last - 1];

        // the first three degens finish the previous box and move
        // to the first position of the new one we just added and
        // the fourth degen creates a triangle that is a line from p1 to p3
        m_ind_buffer[last    ] = val++;
        m_ind_buffer[last + 1] = val;

        // Now add as normal
        m_ind_buffer[last + 2] = val++;
        m_ind_buffer[last + 3] = val++;
        m_ind_buffer[last + 4] = val++;
        m_ind_buffer[last + 5] = val;
    }
    else
    {
        // This is the first box so don't bother making any degenerate triangles
        m_ind_buffer.resize(last + 4);
        m_ind_buffer[0] = 0;
        m_ind_buffer[1] = 1;
        m_ind_buffer[2] = 2;
        m_ind_buffer[3] = 3;
    }
}

void OGLShapeBuffer::add_line(const GLWPrim &rect)
{
    // Copy vert positions
    size_t last = m_position_buffer.size();
    m_position_buffer.resize(last + 2);
    m_position_buffer[last    ].set(rect.pos_sx, rect.pos_sy, rect.pos_z);
    m_position_buffer[last + 1].set(rect.pos_ex, rect.pos_ey, rect.pos_z);

    // Copy texture coords if necessary
    if (m_texture_verts)
    {
        last = m_texture_buffer.size();
        m_texture_buffer.resize(last + 2);
        m_texture_buffer[last    ].set(rect.tex_sx, rect.tex_sy);
        m_texture_buffer[last + 1].set(rect.tex_ex, rect.tex_ey);
    }

    // Copy vert colours if necessary
    if (m_colour_verts)
    {
        last = m_colour_buffer.size();
        m_colour_buffer.resize(last + 2);
        m_colour_buffer[last    ].set(rect.col_s);
        m_colour_buffer[last + 1].set(rect.col_e);
    }
}

// Draw the buffer
#if 0
void OGLShapeBuffer::draw(const GLState &state, const GLW_3VF *pt, const GLW_3VF *ps)
#endif
void OGLShapeBuffer::draw(const GLState &state)
{
    if (m_position_buffer.size() == 0)
        return;

    if (!state.array_vertex)
        return;

    glmanager->set(state);

    glVertexPointer(3, GL_FLOAT, 0, &m_position_buffer[0]);

    if (state.array_texcoord && m_texture_verts)
        glTexCoordPointer(2, GL_FLOAT, 0, &m_texture_buffer[0]);

    if (state.array_colour && m_colour_verts)
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, &m_colour_buffer[0]);

    switch (m_prim_type)
    {
    case GLW_RECTANGLE:
        glDrawElements(GL_TRIANGLE_STRIP, m_ind_buffer.size(),
                       GL_UNSIGNED_SHORT, &m_ind_buffer[0]);
        break;
    case GLW_LINES:
        glDrawArrays(GL_LINES, 0, m_position_buffer.size());
        break;
    default:
        die("Invalid primitive type");
        break;
    }
}

void OGLShapeBuffer::clear()
{
    m_position_buffer.clear();
    m_ind_buffer.clear();
    m_texture_buffer.clear();
    m_colour_buffer.clear();
}

#endif // USE_GL
#endif // USE_TILE
