use std::fs;
use std::os::windows::process::CommandExt;
use std::process::Command;
use tauri::menu::{MenuBuilder, MenuItemBuilder};
use tauri::tray::{MouseButton, TrayIconBuilder, TrayIconEvent};
use tauri::Manager;

const CREATE_NO_WINDOW: u32 = 0x08000000;

#[tauri::command]
fn get_app_path(app: tauri::AppHandle) -> Result<String, String> {
    let resource_dir = app.path().resource_dir().map_err(|e| e.to_string())?;
    Ok(resource_dir.to_string_lossy().to_string())
}

#[tauri::command]
async fn run_with_wait(path: String) -> Result<String, String> {
    let ext = std::path::Path::new(&path)
        .extension()
        .and_then(|e| e.to_str())
        .unwrap_or("")
        .to_lowercase();

    let output = if ext == "bat" || ext == "cmd" {
        Command::new("cmd")
            .args(["/c", &path])
            .creation_flags(CREATE_NO_WINDOW)
            .output()
            .map_err(|e| format!("Run Error: {}", e))?
    } else {
        Command::new(&path)
            .creation_flags(CREATE_NO_WINDOW)
            .output()
            .map_err(|e| format!("Run Error: {}", e))?
    };

    let mut result = String::new();
    if !output.stdout.is_empty() {
        result.push_str(&String::from_utf8_lossy(&output.stdout));
    }
    if !output.stderr.is_empty() {
        if !result.is_empty() {
            result.push('\n');
        }
        result.push_str(&String::from_utf8_lossy(&output.stderr));
    }
    Ok(result)
}

#[tauri::command]
fn run_without_wait(path: String) -> Result<String, String> {
    Command::new("cmd")
        .args(["/c", "start", "", &path])
        .creation_flags(CREATE_NO_WINDOW)
        .spawn()
        .map_err(|e| format!("Run Error: {}", e))?;
    Ok("OK".to_string())
}

#[tauri::command]
fn show_message(app: tauri::AppHandle, msg: String) -> Result<String, String> {
    use tauri_plugin_dialog::DialogExt;
    app.dialog()
        .message(msg)
        .title("WindowsCleanerNext-Tauri")
        .show(|_| {});
    Ok("OK".to_string())
}

#[tauri::command]
fn read_file_content(path: String) -> Result<String, String> {
    fs::read_to_string(&path).map_err(|e| e.to_string())
}

#[tauri::command]
fn write_file_content(path: String, data: String) -> Result<String, String> {
    if let Some(parent) = std::path::Path::new(&path).parent() {
        fs::create_dir_all(parent).map_err(|e| e.to_string())?;
    }
    fs::write(&path, &data).map_err(|e| e.to_string())?;
    Ok("OK".to_string())
}

fn read_program_name(base_path: &str) -> String {
    let config_path = std::path::Path::new(base_path)
        .join("Config")
        .join("Configs.NCc");
    if let Ok(data) = fs::read_to_string(&config_path) {
        if let Ok(json) = serde_json::from_str::<serde_json::Value>(&data) {
            if let Some(name) = json.get("ProgramName").and_then(|v| v.as_str()) {
                return name.to_string();
            }
        }
    }
    "Program".to_string()
}

pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_shell::init())
        .invoke_handler(tauri::generate_handler![
            get_app_path,
            run_with_wait,
            run_without_wait,
            show_message,
            read_file_content,
            write_file_content,
        ])
        .setup(move |app| {
            let base_path = if cfg!(debug_assertions) {
                std::env::current_dir().unwrap_or_default()
            } else {
                app.path().resource_dir().unwrap_or_default()
            };
            let program_name = read_program_name(&base_path.to_string_lossy());
            let title = program_name.clone();
            let tray_label = program_name.clone();

            let show_item = MenuItemBuilder::with_id("show", "显示窗口").build(app)?;
            let quit_item = MenuItemBuilder::with_id(
                "quit",
                format!("退出 {}", program_name),
            )
            .build(app)?;
            let menu = MenuBuilder::new(app)
                .item(&show_item)
                .separator()
                .item(&quit_item)
                .build()?;

            let _tray = TrayIconBuilder::new()
                .icon(app.default_window_icon().unwrap().clone())
                .tooltip(tray_label)
                .menu(&menu)
                .on_menu_event(|app, event| match event.id().as_ref() {
                    "show" => {
                        if let Some(w) = app.get_webview_window("main") {
                            let _ = w.show();
                            let _ = w.set_focus();
                        }
                    }
                    "quit" => {
                        app.exit(0);
                    }
                    _ => {}
                })
                .on_tray_icon_event(|tray, event| {
                    if let TrayIconEvent::Click {
                        button: MouseButton::Left,
                        ..
                    } = event
                    {
                        let app = tray.app_handle();
                        if let Some(w) = app.get_webview_window("main") {
                            let _ = w.show();
                            let _ = w.set_focus();
                        }
                    }
                })
                .build(app)?;

            let window = app.get_webview_window("main").unwrap();
            window.set_title(&title)?;

            let app_handle = window.app_handle().clone();
            window.on_window_event(move |event| {
                if let tauri::WindowEvent::CloseRequested { .. } = event {
                    app_handle.exit(0);
                }
            });

            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
