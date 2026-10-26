add_executable(wireframe_demo ${CMAKE_CURRENT_SOURCE_DIR}/wireframe_demo/main.cpp)
target_link_libraries(wireframe_demo PRIVATE prism_core)
prism_apply_warnings(wireframe_demo)

add_executable(rasterizer_demo ${CMAKE_CURRENT_SOURCE_DIR}/rasterizer_demo/main.cpp)
target_link_libraries(rasterizer_demo PRIVATE prism_core)
prism_apply_warnings(rasterizer_demo)

add_executable(dungeon_crawler ${CMAKE_CURRENT_SOURCE_DIR}/dungeon_crawler/main.cpp)
target_link_libraries(dungeon_crawler PRIVATE prism_core)
prism_apply_warnings(dungeon_crawler)
