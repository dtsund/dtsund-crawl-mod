#include "AppHdr.h"

#include "coord-circle.h"

#include "coordit.h"
#include "los.h"

#include <cmath>

bool rect_def::contains(const coord_def& p) const
{
    return (p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y);
}

rect_def rect_def::intersect(const rect_def& other) const
{
    rect_def res;
    res.min.x = std::max(min.x, other.min.x);
    res.min.y = std::max(min.y, other.min.y);
    res.max.x = std::min(max.x, other.max.x);
    res.max.y = std::min(max.y, other.max.y);
    return (res);
}

rectangle_iterator rect_def::iter() const
{
    return (rectangle_iterator(min, max));
}

circle_def::circle_def()
    : los_radius(true), origin(coord_def(0,0)), check_bounds(false)
{
    // Set up bounding box and shape.
    init(LOS_MAX_RADIUS, C_ROUND);
}

circle_def::circle_def(const coord_def& origin_, const circle_def& bds)
    : los_radius(bds.los_radius), shape(bds.shape),
      origin(origin_), check_bounds(true),
      radius(bds.radius), radius_sq(bds.radius_sq)
{
    // Set up bounding box.
    init_bbox();
}

circle_def::circle_def(int param, circle_type ctype)
    : los_radius(false), origin(coord_def(0,0)), check_bounds(false)
{
    init(param, ctype);
}

circle_def::circle_def(const coord_def &origin_, int param,
                       circle_type ctype)
    : los_radius(false), origin(origin_), check_bounds(true)
{
    init(param, ctype);
}

void circle_def::init(int param, circle_type ctype)
{
    switch (ctype)
    {
    case C_SQUARE:
        shape = SH_SQUARE;
        radius = param;
        break;
    case C_CIRCLE:
        shape = SH_CIRCLE;
        radius_sq = param;
        radius = ceil(sqrt((float)radius_sq));
        break;
    case C_ROUND:
        shape = SH_CIRCLE;
        radius = param;
        radius_sq = radius * radius + 1;
        break;
    case C_POINTY:
        shape = SH_CIRCLE;
        radius = param;
        radius_sq = radius * radius;
    }
    init_bbox();
}

void circle_def::init_bbox()
{
    bbox = rect_def(origin - coord_def(radius, radius),
                    origin + coord_def(radius, radius));
    if (check_bounds)
        bbox = bbox.intersect(RECT_MAP_BOUNDS);
}

const rect_def& circle_def::get_bbox() const
{
    return (bbox);
}

const coord_def& circle_def::get_center() const
{
    return (origin);
}

circle_iterator circle_def::iter() const
{
    return (circle_iterator(*this));
}

bool circle_def::contains(const coord_def &p) const
{
    if (!bbox.contains(p))
        return (false);
    switch (shape)
    {
    case SH_SQUARE:
        return ((p - origin).rdist() <= radius);
    case SH_CIRCLE:
    {
        int r_sq = los_radius ? get_los_radius_sq() : radius_sq;
        return ((p - origin).abs() <= r_sq);
    }
    default:
        return (false);
    }
}
