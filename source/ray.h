/**
 * @file
 * @brief Ray definition
**/

#ifndef RAY_H
#define RAY_H

#include "fixedarray.h"
#include "geom2d.h"

typedef SquareArray<bool,1> reflect_grid;

struct ray_def
{
    geom::ray r;
    bool on_corner;
    int cycle_idx;

    ray_def() : on_corner(false), cycle_idx(-1) {}
    ray_def(const geom::ray& _r)
        : r(_r), on_corner(false), cycle_idx(-1) {}

    coord_def pos() const;
    bool advance();
    void bounce(const reflect_grid &rg);
    void regress();

    // Get and set the direction.
    double get_degrees() const;
    void set_degrees(double d);

    bool _valid() const;
};

#endif
