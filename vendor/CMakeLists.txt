# ImGui library with SDL3 backends
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
	imgui/backends/imgui_impl_sdl3.cpp
	imgui/backends/imgui_impl_sdl3.h
	imgui/backends/imgui_impl_sdlrenderer3.cpp
	imgui/backends/imgui_impl_sdlrenderer3.h
)

target_include_directories(imgui PUBLIC
    imgui/
    imgui/backends/
    sdl/include/
)

target_link_libraries(imgui PUBLIC
    SDL3::SDL3
)