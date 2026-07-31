cmake_minimum_required(VERSION 3.14)
project(ia_training_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Incluir la carpeta de cabeceras de tu proyecto y subcarpetas comunes de librerías
include_directories(
    include
    ${Boost_INCLUDE_DIRS}
)

# Buscar dependencias
find_package(Boost REQUIRED COMPONENTS system)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Añadir todos los archivos fuente
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Crear ejecutable
add_executable(server_app ${SOURCES})

# Vincular librerías
target_link_libraries(server_app PRIVATE 
    Boost::system 
    CURL::libcurl 
    Threads::Threads
)
