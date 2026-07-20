use encoding_rs::Encoding;
use std::fs;
use std::os::windows::process::CommandExt;
use std::process::Command;
use std::time::Duration;
use tauri::menu::{MenuBuilder, MenuItemBuilder};
use tauri::tray::{MouseButton, TrayIconBuilder, TrayIconEvent};
use tauri::{Manager, WebviewUrl, WebviewWindowBuilder};

const CREATE_NO_WINDOW: u32 = 0x08000000;

#[cfg(windows)]
#[link(name = "kernel32")]
extern "system" {
    fn GetACP() -> u32;
    fn GetConsoleOutputCP() -> u32;
}

fn decode_with_encoding(bytes: &[u8], label: &[u8]) -> String {
    if let Some(encoding) = Encoding::for_label(label) {
        let (decoded, _, _) = encoding.decode(bytes);
        return decoded.into_owned();
    }
    String::from_utf8_lossy(bytes).into_owned()
}

fn decode_tool_output(bytes: &[u8]) -> String {
    if bytes.is_empty() {
        return String::new();
    }

    if let Ok(text) = std::str::from_utf8(bytes) {
        return text.to_owned();
    }

    #[cfg(windows)]
    {
        let console_cp = unsafe { GetConsoleOutputCP() };
        match console_cp {
            65001 => return String::from_utf8_lossy(bytes).into_owned(),
            936 => return decode_with_encoding(bytes, b"gbk"),
            _ => {}
        }

        let acp = unsafe { GetACP() };
        match acp {
            65001 => return String::from_utf8_lossy(bytes).into_owned(),
            936 => return decode_with_encoding(bytes, b"gbk"),
            _ => {}
        }
    }

    decode_with_encoding(bytes, b"gb18030")
}

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
        result.push_str(&decode_tool_output(&output.stdout));
    }
    if !output.stderr.is_empty() {
        if !result.is_empty() {
            result.push('\n');
        }
        result.push_str(&decode_tool_output(&output.stderr));
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

#[tauri::command]
fn get_settings_path(app: tauri::AppHandle) -> Result<String, String> {
    let settings_dir = app.path().app_config_dir().map_err(|e| e.to_string())?;
    Ok(settings_dir
        .join("settings.json")
        .to_string_lossy()
        .to_string())
}

#[tauri::command]
fn nvidia_installed() -> Result<bool, String> {
    let check_paths = [
        r"C:\Program Files\NVIDIA Corporation\NVIDIA App\CEF\NVIDIA Overlay.exe",
        r"C:\Program Files\NVIDIA Corporation\NVIDIA app\CEF\NVIDIA Overlay.exe",
        r"C:\Windows\System32\nvapi64.dll",
        r"C:\Program Files\NVIDIA Corporation\NVSMI\nvidia-smi.exe",
    ];
    Ok(check_paths.iter().any(|p| std::path::Path::new(p).exists()))
}
#[tauri::command]
fn get_hibernate_status() -> Result<bool, String> {
    // When hibernate is enabled, Windows creates C:\hiberfil.sys
    // When disabled, the file is removed
    Ok(std::path::Path::new("C:\\hiberfil.sys").exists())
}

#[tauri::command]
fn set_hibernate(enabled: bool) -> Result<String, String> {
    let arg = if enabled { "on" } else { "off" };
    let output = Command::new("powercfg.exe")
        .args(["/h", arg])
        .output()
        .map_err(|e| format!("设置休眠状态失败: {}", e))?;

    if output.status.success() {
        Ok(if enabled {
            "休眠已启用".to_string()
        } else {
            "休眠已禁用".to_string()
        })
    } else {
        let err = decode_tool_output(&output.stderr);
        Err(format!("操作失败（可能需要管理员权限）: {}", err))
    }
}

#[tauri::command]
async fn get_virtual_memory_info() -> Result<String, String> {
    let ps_script = "Get-CimInstance Win32_PageFileSetting | Select-Object Name, InitialSize, MaximumSize | ConvertTo-Json -Compress";
    let output = Command::new("powershell")
        .args(["-NoProfile", "-Command", ps_script])
        .creation_flags(CREATE_NO_WINDOW)
        .output()
        .map_err(|e| format!("查询虚拟内存失败: {}", e))?;

    let stdout = decode_tool_output(&output.stdout);
    if stdout.trim().is_empty() || stdout.trim() == "null" {
        // No page file configured — return empty array
        return Ok("[]".to_string());
    }
    Ok(stdout)
}

#[tauri::command]
async fn get_c_drive_usage() -> Result<String, String> {
    let ps_script = "Get-CimInstance Win32_LogicalDisk -Filter \"DeviceID='C:'\" | Select-Object Size,FreeSpace | ConvertTo-Json -Compress";
    let output = Command::new("powershell")
        .args(["-NoProfile", "-Command", ps_script])
        .creation_flags(CREATE_NO_WINDOW)
        .output()
        .map_err(|e| format!("查询磁盘空间失败: {}", e))?;

    let stdout = decode_tool_output(&output.stdout);
    if stdout.trim().is_empty() || stdout.trim() == "null" {
        return Err("未能读取 C 盘空间信息".to_string());
    }
    Ok(stdout)
}

#[tauri::command]
async fn set_virtual_memory(drive: String, initial_size: u32, maximum_size: u32) -> Result<String, String> {
    // Sanitize drive — ensure it ends with :
    let drive = if drive.ends_with(':') { drive } else { format!("{}:", drive) };

    let ps_script = format!(
        "$ErrorActionPreference='Stop'; \
         $drive='{0}'; \
         $pf=Get-CimInstance Win32_PageFileSetting -Filter \"Name='{{0}}'\" -ErrorAction SilentlyContinue; \
         if (-not $pf) {{ \
           $pf=New-CimInstance Win32_PageFileSetting -Property @{{Name=$drive; InitialSize=0; MaximumSize=0}} -ClientOnly; \
           Set-CimInstance -InputObject $pf -Property @{{InitialSize={1}; MaximumSize={2}}} | Out-Null; \
         }} else {{ \
           Set-CimInstance -InputObject $pf -Property @{{InitialSize={1}; MaximumSize={2}}} | Out-Null; \
         }}; \
         Write-Output 'OK'",
        drive, initial_size, maximum_size
    );

    let output = Command::new("powershell")
        .args(["-NoProfile", "-Command", &ps_script])
        .creation_flags(CREATE_NO_WINDOW)
        .output()
        .map_err(|e| format!("设置虚拟内存失败: {}", e))?;

    if output.status.success() {
        Ok("虚拟内存设置已更新（可能需要重启生效）".to_string())
    } else {
        let err = decode_tool_output(&output.stderr);
        Err(format!("设置失败（可能需要管理员权限）: {}", err))
    }
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
    "WindowsCleanerNext-Tauri".to_string()
}

fn append_startup_log(line: &str) {
    let log_dir = r"C:\UDisk\Codes\WindowsCleanerNext-Tauri\tmp";
    let _ = fs::create_dir_all(log_dir);
    if let Ok(mut file) = fs::OpenOptions::new()
        .create(true)
        .append(true)
        .open(format!(r"{}\app-startup.log", log_dir))
    {
        use std::io::Write;
        let _ = writeln!(file, "{}", line);
    }
}

pub fn run() {
    append_startup_log("run:start");
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
            get_settings_path,
            nvidia_installed,
            get_hibernate_status,
            set_hibernate,
            get_virtual_memory_info,
            get_c_drive_usage,
            set_virtual_memory,
        ])
        .setup(move |app| {
            append_startup_log("setup:start");
            let base_path = app.path().resource_dir().unwrap_or_default();
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

            let window = match app.get_webview_window("main") {
                Some(window) => window,
                None => WebviewWindowBuilder::new(app, "main", WebviewUrl::App("index.html".into()))
                    .title(&title)
                    .inner_size(1000.0, 800.0)
                    .visible(true)
                    .resizable(true)
                    .decorations(true)
                    .center()
                    .build()?,
            };
            append_startup_log("window:acquired");
            window.set_title(&title)?;
            let _ = window.center();
            let _ = window.unminimize();
            let _ = window.show();
            let _ = window.set_focus();
            let window_for_show = window.clone();
            std::thread::spawn(move || {
                std::thread::sleep(Duration::from_millis(500));
                let _ = window_for_show.center();
                let _ = window_for_show.unminimize();
                let _ = window_for_show.show();
                let _ = window_for_show.set_focus();
            });

            let app_handle = window.app_handle().clone();
            window.on_window_event(move |event| {
                if let tauri::WindowEvent::CloseRequested { .. } = event {
                    app_handle.exit(0);
                }
            });
            append_startup_log("setup:done");

            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}







