cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_SOURCE_DIR}/cmake/conan.cmake)
project(Renderer
        VERSION 0.1.0)
#list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
#include(conan)

include(CTest)
enable_testing()
include(GoogleTest)


#conan_cmake_run(CONANFILE conanfile.py
#        BASIC_SETUP
#        GENERATORS CMakeDeps)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake
        )
list (APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 20)

set(SHADER_INPUT_FILES
        ${CMAKE_SOURCE_DIR}/resources/shaders/fwd-diffuse-map.vert
        ${CMAKE_SOURCE_DIR}/resources/shaders/fwd-diffuse-map.frag
        ${CMAKE_SOURCE_DIR}/resources/shaders/fwd-parallax-map.vert
        ${CMAKE_SOURCE_DIR}/resources/shaders/fwd-parallax-map.frag
        )

set(SHADER_OUTPUT_FILES
        ${CMAKE_BINARY_DIR}/resources/shaders/fwd-diffuse-map.vert.spv
        ${CMAKE_BINARY_DIR}/resources/shaders/fwd-diffuse-map.frag.spv
        ${CMAKE_BINARY_DIR}/resources/shaders/fwd-parallax-map.vert.spv
        ${CMAKE_BINARY_DIR}/resources/shaders/fwd-parallax-map.frag.spv)

# drawback of this method is that it is always considered out-of-date, and therefore always recompiles.
#add_custom_target(compile-shaders
#        echo "Compiling shaders using GLSLC"
#        COMMAND ${CMAKE_CURRENT_LIST_DIR}/compile-shaders.sh ${CMAKE_CURRENT_LIST_DIR}/workspace/assets/shaders ${CMAKE_BINARY_DIR}/workspace/assets/shaders)

# a better solution is as follows:
#file(GLOB SHADER_INPUT_FILES   ${CMAKE_CURRENT_LIST_DIR}/workspace/assets/shaders/*.vert ${CMAKE_CURRENT_LIST_DIR}/workspace/assets/shaders/*.frag)
#file(GLOB SHADER_OUTPUT_FILES  ${CMAKE_BINARY_DIR}/resources/shaders/*.spv)
#add_custom_command(
#        OUTPUT ${SHADER_OUTPUT_FILES}
#        COMMAND echo "Compiling shaders"
#        COMMAND ${CMAKE_CURRENT_LIST_DIR}/compile-shaders.sh ${CMAKE_CURRENT_LIST_DIR}/resources/shaders ${CMAKE_BINARY_DIR}/resources/shaders
#        DEPENDS ${SHADER_INPUT_FILES})
#add_custom_target(compile-shaders ALL DEPENDS ${SHADER_OUTPUT_FILES})

####################################################################################
### Resources
####################################################################################
install(FILES ${SHADER_OUTPUT_FILES} DESTINATION resources/shaders)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/compile-shaders.sh DESTINATION tools)

configure_file(${CMAKE_SOURCE_DIR}/resources/materials.json ${CMAKE_BINARY_DIR}/resources/materials.json COPYONLY)
file(COPY resources/textures DESTINATION ${CMAKE_BINARY_DIR}/resources)

# configure file will copy the file <1> to the destination <2>, only if file <1> has changed or at first run.
#configure_file(WELCOME.md $<TARGET_FILE_DIR:normalmap>/WELCOME.md COPYONLY)

# The file(...) command can use generator expressions. It makes it possible to copy (or generate) the file in the same
# location as, in this case, the normalmap target executable. Useful for unit tests.
#file(GENERATE
#        OUTPUT $<TARGET_FILE_DIR:normalmap>/WELCOME.md
#        INPUT ${CMAKE_CURRENT_LIST_DIR}/WELCOME.md)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(examples)

#add_dependencies(normalmap compile-shaders)

#install(FILES LICENSE.md README.md DESTINATION ${CMAKE_BINARY_DIR})



