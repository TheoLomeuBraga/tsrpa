project(TSRPA CXX)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(SDL_STATIC ON)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(glm)
add_subdirectory(examples_test/libs/SDL)
add_subdirectory(examples_test/libs/SDL_image)
add_subdirectory(examples_test/libs/SDL_ttf)
include_directories( ./ examples_test/libs/SDL/include examples_test/libs/SDL_image/include examples_test/libs/SDL_ttf/include examples_test/libs/tiny_obj_loader)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/examples_test/assets/AlienCyborg.ttf DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_executable(hello_sdl3_render_atempt examples_test/hello_sdl3_render_atempt.cpp)
target_link_options(hello_sdl3_render_atempt PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(hello_sdl3_render_atempt PRIVATE SDL3::SDL3-static SDL3_image::SDL3_image-static SDL3_ttf::SDL3_ttf-static glm::glm)


