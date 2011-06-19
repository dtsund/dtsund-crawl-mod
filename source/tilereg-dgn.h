#ifdef USE_TILE
#ifndef TILEREG_DGN_H
#define TILEREG_DGN_H

#include "tilereg.h"
#include "tiledgnbuf.h"
#include "viewgeom.h"
#include <vector>

class mcache_entry;

struct TextTag
{
    std::string tag;
    coord_def gc;
};

bool tile_dungeon_tip(const coord_def &gc, std::string &tip);
int tile_click_cell(const coord_def &gc, unsigned char mod);

class DungeonRegion : public TileRegion
{
public:
    DungeonRegion(const TileRegionInit &init);
    virtual ~DungeonRegion();

    virtual void render();
    virtual void clear();
    virtual int handle_mouse(MouseEvent &event);
    virtual bool update_tip_text(std::string &tip);
    virtual bool update_alt_text(std::string &alt);
    virtual void on_resize();

    void load_dungeon(const crawl_view_buffer &vbuf, const coord_def &gc);
    void place_cursor(cursor_type type, const coord_def &gc);
    bool on_screen(const coord_def &gc) const;

    void clear_text_tags(text_tag_type type);
    void add_text_tag(text_tag_type type, const std::string &tag,
                      const coord_def &gc);

    const coord_def &get_cursor() const { return m_cursor[CURSOR_MOUSE]; }

    void add_overlay(const coord_def &gc, int idx);
    void clear_overlays();

protected:
    void pack_buffers();
    void pack_cursor(cursor_type type, unsigned int tile);

    void draw_minibars();

    int get_buffer_index(const coord_def &gc);
    void to_screen_coords(const coord_def &gc, coord_def *pc) const;

    crawl_view_buffer m_vbuf;
    int m_cx_to_gx;
    int m_cy_to_gy;
    coord_def m_cursor[CURSOR_MAX];
    coord_def m_last_clicked_grid;
    std::vector<TextTag> m_tags[TAG_MAX];

    DungeonCellBuffer m_buf_dngn;
    ShapeBuffer m_buf_flash;

    struct tile_overlay
    {
        coord_def gc;
        tileidx_t idx;
    };
    std::vector<tile_overlay> m_overlays;
};

#endif
#endif
