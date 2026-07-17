#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

fn main() {
    let _ = std::fs::create_dir_all(r"C:\UDisk\Codes\WindowsCleanerNext-Tauri\tmp");
    let _ = std::fs::OpenOptions::new()
        .create(true)
        .append(true)
        .open(r"C:\UDisk\Codes\WindowsCleanerNext-Tauri\tmp\app-startup.log")
        .and_then(|mut file| {
            use std::io::Write;
            writeln!(file, "main:start")
        });
    windows_cleaner_next_tauri_lib::run()
}
