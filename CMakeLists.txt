cmake_minimum_required(VERSION 3.20)

project(Lume3D VERSION 1.5.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Python 3 COMPONENTS Interpreter REQUIRED)
execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import jinja2"
    RESULT_VARIABLE LUME_JINJA2_STATUS
    OUTPUT_QUIET
    ERROR_QUIET
)
if(NOT LUME_JINJA2_STATUS EQUAL 0)
    message(FATAL_ERROR
        "The GLAD generator requires Jinja2. Install the build requirements with: "
        "${Python_EXECUTABLE} -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements-build.txt"
    )
endif()

option(LUME_BUILD_EXAMPLES "Build the Lume3D examples" ON)
option(LUME_BUILD_TESTS "Build the Lume3D tests" ON)
option(LUME_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 70a9bb3881fe80fd483236e2b203cb451c6ecf40
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG 73db193f853e2ee079bf3ca8a64aa2eaf6459043
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(glad)
add_subdirectory(${glad_SOURCE_DIR}/cmake ${glad_BINARY_DIR}/cmake)
glad_add_library(lume_glad STATIC REPRODUCIBLE API gl:core=3.3)
set_property(TARGET lume_glad PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/gladsources/lume_glad/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
    cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG bbeb5b0b070ddacddac6852fb72143eb68454937
    GIT_SHALLOW FALSE
)
FetchContent_Declare(
    fast_obj
    GIT_REPOSITORY https://github.com/thisistherk/fast_obj.git
    GIT_TAG 42629f744269e004907a6fb4f16c6c7f69acc586
    GIT_SHALLOW FALSE
    SOURCE_SUBDIR lume-no-cmake-project
)
FetchContent_Declare(
    tinycthread
    GIT_REPOSITORY https://github.com/tinycthread/tinycthread.git
    GIT_TAG 6b9816f06eaccb39c2c9a4adb75f94a3a63de57a
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(cgltf fast_obj tinycthread)

add_library(lume3d
    src/aplicativo.c
    src/animacao.c
    src/assets.c
    src/cena.c
    src/geometria.c
    src/matematica.c
    src/recursos.c
    src/renderizador.c
    src/stb_imagem.c
    src/terceiros.c
    ${tinycthread_SOURCE_DIR}/source/tinycthread.c
)
add_library(Lume3D::lume3d ALIAS lume3d)

target_compile_features(lume3d PUBLIC c_std_11)
target_include_directories(lume3d
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${stb_SOURCE_DIR}
        ${cgltf_SOURCE_DIR}
        ${fast_obj_SOURCE_DIR}
        ${tinycthread_SOURCE_DIR}/source
)
target_link_libraries(lume3d PRIVATE glfw lume_glad)
target_compile_definitions(lume3d
    PRIVATE LUME_BUILDING_LIBRARY _CRT_SECURE_NO_WARNINGS
    PUBLIC $<$<BOOL:${BUILD_SHARED_LIBS}>:LUME_SHARED>
)
set_target_properties(lume3d PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(MSVC)
    target_compile_options(lume3d PRIVATE /W4)
    set_source_files_properties(
        src/terceiros.c
        ${tinycthread_SOURCE_DIR}/source/tinycthread.c
        PROPERTIES COMPILE_OPTIONS "/wd4100;/wd4244;/wd4996"
    )
    if(LUME_WARNINGS_AS_ERRORS)
        target_compile_options(lume3d PRIVATE /WX)
    endif()
else()
    target_compile_options(lume3d PRIVATE -Wall -Wextra -Wpedantic)
    if(LUME_WARNINGS_AS_ERRORS)
        target_compile_options(lume3d PRIVATE -Werror)
    endif()
    target_link_libraries(lume3d PRIVATE m)
endif()

if(LUME_BUILD_EXAMPLES)
    add_executable(lume_example_ocean examples/oceano.c)
    add_executable(lume_example_black_hole examples/buraco_negro.c)
    target_link_libraries(lume_example_ocean PRIVATE Lume3D::lume3d)
    target_link_libraries(lume_example_black_hole PRIVATE Lume3D::lume3d)
endif()

if(LUME_BUILD_TESTS)
    enable_testing()
    add_executable(lume_tests tests/testes.c)
    target_include_directories(lume_tests PRIVATE src ${tinycthread_SOURCE_DIR}/source)
    target_link_libraries(lume_tests PRIVATE Lume3D::lume3d glfw lume_glad)
    if(NOT MSVC)
        target_link_libraries(lume_tests PRIVATE m)
    endif()
    add_test(NAME lume_unit_tests COMMAND lume_tests)

    add_executable(lume_render_test tests/teste_render.c)
    target_include_directories(lume_render_test PRIVATE src ${tinycthread_SOURCE_DIR}/source)
    target_compile_definitions(lume_render_test PRIVATE LUME_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    target_link_libraries(lume_render_test PRIVATE Lume3D::lume3d glfw lume_glad)
    add_test(NAME lume_render_test COMMAND lume_render_test)
    if(LUME_BUILD_EXAMPLES)
        add_test(NAME lume_ocean_example_smoke COMMAND lume_example_ocean --smoke)
        add_test(NAME lume_black_hole_example_smoke COMMAND lume_example_black_hole --smoke)
    endif()
endif()

install(TARGETS lume3d glfw lume_glad
    EXPORT Lume3DTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY doc-en doc-ptbr DESTINATION ${CMAKE_INSTALL_DATADIR}/Lume3D)
install(DIRECTORY examples/ DESTINATION ${CMAKE_INSTALL_DATADIR}/Lume3D/examples FILES_MATCHING PATTERN "*.c")
install(FILES README.md CHANGELOG.md LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/Lume3D)
install(DIRECTORY ${glfw_SOURCE_DIR}/include/GLFW DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gladsources/lume_glad/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT Lume3DTargets
    FILE Lume3DTargets.cmake
    NAMESPACE Lume3D::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Lume3D
)

configure_package_config_file(
    cmake/Lume3DConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Lume3DConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Lume3D
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/Lume3DConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/Lume3DConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/Lume3DConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Lume3D
)
