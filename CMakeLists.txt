project(TSRPA CXX)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_executable(hello examples_test/hello.cpp)
target_link_options(hello PRIVATE -static-libgcc -static-libstdc++ -static)



set(SDL_STATIC ON)
set(BUILD_SHARED_LIBS OFF)


add_subdirectory(glm)
add_subdirectory(examples_test/libs/SDL)
add_subdirectory(examples_test/libs/SDL_image)
include_directories( ./ examples_test/libs/SDL/include examples_test/libs/SDL_image/include examples_test/libs/tiny_obj_loader)

add_executable(hello_sdl3 examples_test/hello_sdl3.cpp)
target_link_options(hello_sdl3 PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(hello_sdl3 PRIVATE SDL3::SDL3-static)

add_executable(hello_sdl3_image examples_test/hello_sdl3_image.cpp)
target_link_options(hello_sdl3_image PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(hello_sdl3_image PRIVATE SDL3::SDL3-static SDL3_image::SDL3_image-static)

add_executable(hello_sdl3_dynamic_texture examples_test/hello_sdl3_dynamic_texture.cpp)
target_link_options(hello_sdl3_dynamic_texture PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(hello_sdl3_dynamic_texture PRIVATE SDL3::SDL3-static)

add_executable(hello_sdl3_render_atempt examples_test/hello_sdl3_render_atempt.cpp)
target_link_options(hello_sdl3_render_atempt PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(hello_sdl3_render_atempt PRIVATE SDL3::SDL3-static SDL3_image::SDL3_image-static glm::glm)


