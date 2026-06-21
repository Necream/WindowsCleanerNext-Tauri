@echo off
chcp 65001 >nul
echo 编译 Windows Cleaner Next 工具集...
echo.

set CPP_FILES=clean_prefetch.cpp clean_windows_update_cache.cpp clean_windows_temp.cpp clean_user_temp.cpp clean_system_logs.cpp clean_browser_cache.cpp clean_app_cache.cpp clean_tmp_files.cpp clean_restore_points.cpp clean_memory.cpp clean_user_paths.cpp

for %%f in (%CPP_FILES%) do (
    echo 编译: %%f
    cl /nologo /EHsc /O2 /D "_UNICODE" /D "UNICODE" /Fe:"%%~nf.exe" "%%f" /link shell32.lib
    if errorlevel 1 (
        echo 编译失败: %%f
    ) else (
        echo 编译成功: %%~nf.exe
    )
    echo.
)

echo 所有工具编译完成！
pause
