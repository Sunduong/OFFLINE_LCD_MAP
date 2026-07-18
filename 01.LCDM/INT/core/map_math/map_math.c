#include "map_math.h"
#include <math.h>

static const char *TAG = "MAP_MATH";

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif // M_PI

/**
 * @brief Convert longtitude to tile X coordinate (float, has fractional part).
 *
 * @param lon Longtitude in decimal degrees (-180 to +180)
 * @param zoom zoom level (0 - 19)
 * @return Tile X as double
 */
double lon_to_tile_x(double lon, int zoom)
{
    return (lon + 180.0) / 360.0 * (double)(1 << zoom);
}

/**
 * @brief Convert latitude to tile Y coordinate (float, has fractional part).
 *
 * @param lon Latitude in decimal degrees (-180 to +180)
 * @param zoom zoom level (0 - 19)
 * @return Tile Y as double
 */
double lat_to_tile_y(double lat, int zoom)
{
    double lat_rad = lat * M_PI / 180.0;
    return (1.0 - asinh(tan(lat_rad)) / M_PI) /2.0 * (double)(1 << zoom);
}

/**
 * @brief Convert GPS coordinates to a complete map_position_t struct.
 *
 * @param lat Latitude in decimal degrees (-180 to +180)
 * @param lon Longitude in decimal degrees (-180 to +180)
 * @param zoom zoom level (0 - 19)
 * @param pos   Output: filled map_position_t struct
 */
void gps_to_map_position(double lat, double lon, int zoom, map_position_t *pos)
{
    if (pos == NULL)    return;
    pos->zoom = zoom;

    pos->tile_x = lon_to_tile_x(lon, zoom);
    pos->tile_y = lat_to_tile_y(lat, zoom);

    pos->base_tile_x = (int)(pos->tile_x);
    pos->base_tile_y = (int)(pos->tile_y);

    pos->pixel_offset_x = (int)(pos->tile_x - pos->base_tile_x);
    pos->pixel_offset_y = (int)(pos->tile_y - pos->base_tile_y);
}