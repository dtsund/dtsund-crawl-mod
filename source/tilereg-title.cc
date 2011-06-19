#include "AppHdr.h"

#ifdef USE_TILE

#include "tilereg-title.h"

#include "files.h"
#include "libutil.h"
#include "macro.h"

static const std::string _get_title_image()
{
    std::vector<std::string> files = get_title_files();
    return files[random2(files.size())];
}

TitleRegion::TitleRegion(int width, int height, FontWrapper* font) :
    m_buf(true, false), m_font_buf(font)
{
    // set the texture for the title image
    m_buf.set_tex(&m_img);

    sx = sy = 0;
    dx = dy = 1;

    if (!m_img.load_texture(_get_title_image().c_str(), MIPMAP_NONE))
        return;

    // Center
    wx = width;
    wy = height;
    ox = (wx - m_img.orig_width()) / 2;
    oy = (wy - m_img.orig_height()) / 2;

    GLWPrim rect(0, 0, m_img.width(), m_img.height());
    rect.set_tex(0, 0, 1, 1);
    m_buf.add_primitive(rect);
}

void TitleRegion::render()
{
#ifdef DEBUG_TILES_REDRAW
    cprintf("rendering TitleRegion\n");
#endif
    set_transform();
    m_buf.draw();
    m_font_buf.draw();
}

void TitleRegion::run()
{
    mouse_control mc(MOUSE_MODE_MORE);
    getchm();
}

/**
 * We only want to show one line of message by default so clear the
 * font buffer before adding the new message.
 */
void TitleRegion::update_message(std::string message)
{
    m_font_buf.clear();
    m_font_buf.add(message, VColour::white, 0, 0);
}

#endif
