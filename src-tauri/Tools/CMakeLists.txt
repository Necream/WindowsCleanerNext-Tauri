cmake_minimum_required(VERSION 3.10)
project(WindowsCleanerTools LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

set(TOOLS
    clean_prefetch
    clean_windows_update_cache
    clean_windows_temp
    clean_user_temp
    clean_system_logs
    clean_browser_cache
    clean_app_cache
    clean_tmp_files
    clean_restore_points
    clean_memory
    clean_user_paths
)

foreach(TOOL ${TOOLS})
    add_executable(${TOOL} ${TOOL}.cpp)
    target_link_libraries(${TOOL} PRIVATE shell32.lib)
endforeach()
