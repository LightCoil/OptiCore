# CMakeLists.txt для OptiCore
cmake_minimum_required(VERSION 3.10)
project(OptiCore)
set(CMAKE_CXX_STANDARD 17)

# --- Qt5 ---
find_package(Qt5 COMPONENTS Widgets Svg OpenGL REQUIRED)

# --- ImGui ---
find_path(IMGUI_INCLUDE_DIR imgui.h)
find_library(IMGUI_LIB NAMES imgui)
if(NOT IMGUI_INCLUDE_DIR OR NOT IMGUI_LIB)
    message(FATAL_ERROR "Не найдена библиотека ImGui! Установите её через пакетный менеджер или соберите из исходников.")
endif()

# --- ALSA ---
find_package(ALSA REQUIRED)

# --- PulseAudio ---
find_path(PULSEAUDIO_INCLUDE_DIR pulse/pulseaudio.h)
find_library(PULSEAUDIO_LIB NAMES pulse)
if(NOT PULSEAUDIO_INCLUDE_DIR OR NOT PULSEAUDIO_LIB)
    message(FATAL_ERROR "Не найдена библиотека PulseAudio! Установите libpulse-dev (Debian/Ubuntu) или pulseaudio (Arch).")
endif()

# --- NVML (NVIDIA) ---
find_path(NVML_INCLUDE_DIR nvml.h)
find_library(NVML_LIB NAMES nvidia-ml)
# Не критично, если нет NVML — просто не будет мониторинга NVIDIA
if(NOT NVML_INCLUDE_DIR OR NOT NVML_LIB)
    message(WARNING "NVML не найден: мониторинг NVIDIA GPU будет недоступен.")
endif()

# --- OpenGL ---
find_package(OpenGL REQUIRED)

# --- X11 ---
find_package(X11 REQUIRED)

# --- Исходники ---
file(GLOB_RECURSE SRC
    CPU/*.cpp
    Memory/*.cpp
    IO/*.cpp
    Video/*.cpp
    Audio/*.cpp
    Power/*.cpp
    UI/OptiCorePanel/*.cpp
    plugins/*.cpp
)

include_directories(
    ${IMGUI_INCLUDE_DIR}
    ${PULSEAUDIO_INCLUDE_DIR}
    ${NVML_INCLUDE_DIR}
)

add_executable(OptiCorePanel ${SRC})

# Линковка библиотек
set(LIBS
    Qt5::Widgets Qt5::Svg Qt5::OpenGL
    ${IMGUI_LIB}
    ${ALSA_LIBRARIES}
    ${PULSEAUDIO_LIB}
    ${NVML_LIB}
    ${OPENGL_LIBRARIES}
    ${X11_LIBRARIES}
)

target_link_libraries(OptiCorePanel PRIVATE ${LIBS})

# Для Qt5
set_target_properties(OptiCorePanel PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Сообщение об успешной конфигурации
message(STATUS "OptiCore готов к сборке! Все основные зависимости найдены.") 