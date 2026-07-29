idf_component_register(
    SRCS 
        map_math/map_math.c
        map_parser/map_parser.c
        map_renderer/map_renderer.c
        map_cache/map_cache.c
    INCLUDE_DIRS
        "."
        map_math
        map_parser
        map_renderer
        navigation
        map_cache
    REQUIRES 
        gps sd_card lcd esp_mm)
