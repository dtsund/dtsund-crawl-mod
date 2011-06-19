#ifdef USE_TILE
#ifndef TILEREG_TAB_H
#define TILEREG_TAB_H

#include "tilereg-grid.h"

// A region that contains multiple region, selectable by tabs.
class TabbedRegion : public GridRegion
{
public:
    TabbedRegion(const TileRegionInit &init);

    virtual ~TabbedRegion();

    enum
    {
        TAB_OFS_UNSELECTED,
        TAB_OFS_MOUSEOVER,
        TAB_OFS_SELECTED,
        TAB_OFS_MAX
    };

    void set_tab_region(int idx, GridRegion *reg, tileidx_t tile_tab);
    GridRegion *get_tab_region(int idx);
    tileidx_t get_tab_tile(int idx);
    void activate_tab(int idx);
    int active_tab() const;
    int num_tabs() const;
    void enable_tab(int idx);
    void disable_tab(int idx);
    int find_tab(std::string tab_name) const;

    virtual void update();
    virtual void clear();
    virtual void render();
    virtual void on_resize();
    virtual int handle_mouse(MouseEvent &event);
    virtual bool update_tip_text(std::string &tip);
    virtual bool update_tab_tip_text(std::string &tip, bool active);
    virtual bool update_alt_text(std::string &alt);

    virtual const std::string name() const { return ""; }

protected:
    virtual void pack_buffers();
    virtual void draw_tag();
    virtual void activate() {}

    bool invalid_index(int idx) const;
    bool active_is_valid() const;
    // Returns the tab the mouse is over, -1 if none.
    int get_mouseover_tab(MouseEvent &event) const;
    void set_icon_pos(int idx);
    void reset_icons(int from_idx);


    int m_active;
    int m_mouse_tab;
    TileBuffer m_buf_gui;

    struct TabInfo
    {
        GridRegion *reg;
        tileidx_t tile_tab;
        int ofs_y;
        int min_y;
        int max_y;
        int height;
        bool enabled;
    };
    std::vector<TabInfo> m_tabs;
};

#endif
#endif
