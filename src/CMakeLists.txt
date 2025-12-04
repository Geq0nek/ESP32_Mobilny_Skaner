# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources})

# idf_component_register(
#     SRCS ${app_sources}
#     INCLUDE_DIRS "src"
#     REQUIRES esp-mqtt
# )

target_add_binary_data(${COMPONENT_TARGET} "cert.pem" TEXT)