cmake_minimum_required(VERSION 3.20)

project(Lume3DConsumer LANGUAGES C)

find_package(Lume3D 1 CONFIG REQUIRED)

add_executable(lume_consumer main.c)
target_link_libraries(lume_consumer PRIVATE Lume3D::lume3d)
