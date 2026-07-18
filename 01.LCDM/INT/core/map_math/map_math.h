#ifndef MAP_MATH_H
#define MAP_MATH_H

#include "map_define.h"

double lon_to_tile_x(double lon, int zoom);
double lat_to_tile_y(double lat, int zoom);
void gps_to_map_position(double lat, double lon, int zoom, map_position_t *pos);
#endif  // MAP_MATH_H