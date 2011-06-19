#ifndef LOS_DEF_H
#define LOS_DEF_H

#include "coord-circle.h"
#include "los.h"
#include "losglobal.h"
#include "losparam.h"

class los_base
{
public:
    virtual ~los_base() {}

    virtual coord_def get_center() const = 0;
    virtual circle_def get_bounds() const = 0;

    virtual bool in_bounds(const coord_def& p) const = 0;
    virtual bool see_cell(const coord_def& p) const = 0;
};

class los_glob : public los_base
{
    los_type lt;
    coord_def center;
    circle_def bds;

public:
    los_glob() {}
    los_glob(const coord_def& c, los_type l,
             const circle_def &b = BDS_DEFAULT)
        : lt(l), center(c), bds(b) {}

    los_glob& operator=(const los_glob& other);

    coord_def get_center() const;
    circle_def get_bounds() const;

    bool in_bounds(const coord_def& p) const;
    bool see_cell(const coord_def& p) const;
};

class los_def : public los_base
{
    los_grid show;
    coord_def center;
    opacity_func const * opc;
    circle_def bds;
    bool arena;

public:
    los_def();
    los_def(const coord_def& c, const opacity_func &o = opc_default,
                                const circle_def &b = BDS_DEFAULT);
    los_def(const los_def& l);
    ~los_def();
    los_def& operator=(const los_def& l);
    void init(const coord_def& center, const opacity_func& o,
                                       const circle_def& b);
    void init_arena(const coord_def& center);
    void set_center(const coord_def& center);
    coord_def get_center() const;
    void set_opacity(const opacity_func& o);
    void set_bounds(const circle_def& b);
    circle_def get_bounds() const;

    void update();
    bool in_bounds(const coord_def& p) const;
    bool see_cell(const coord_def& p) const;
};

#endif
