cmake_minimum_required(VERSION 3.15)
project(ACC CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cartella con gli header condivisi (dichiarazioni delle funzioni thread)
include_directories(${CMAKE_SOURCE_DIR}/cpp_client/include)

# --- Aggiungi qui ogni nuovo file .cpp che implementa un algoritmo ---
# Basta mettere il file dentro src/ e aggiungerlo a questa lista.
set(SOURCES
    cpp_client/main.cpp
    cpp_client/SensorThread.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Necessario per std::thread su Linux (pthread)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

target_include_directories(${PROJECT_NAME} PRIVATE ${ZMQ_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${ZMQ_LIBRARIES})

# Metti l'eseguibile in una cartella prevedibile (utile per lo script Python)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
