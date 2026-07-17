fn main() {
    let profile = std::env::var("PROFILE").unwrap_or_default();
    let elevation_level = if profile == "debug" {
        "asInvoker"
    } else {
        "requireAdministrator"
    };

    let manifest = r#"
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <dependency>
    <dependentAssembly>
      <assemblyIdentity
        type="win32"
        name="Microsoft.Windows.Common-Controls"
        version="6.0.0.0"
        processorArchitecture="*"
        publicKeyToken="6595b64144ccf1df"
        language="*"
      />
    </dependentAssembly>
  </dependency>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="__ELEVATION_LEVEL__" uiAccess="false" />
      </requestedPrivileges>
    </security>
  </trustInfo>
</assembly>
"#.replace("__ELEVATION_LEVEL__", elevation_level);

    let windows = tauri_build::WindowsAttributes::new()
        .app_manifest(manifest);
    let attrs = tauri_build::Attributes::new()
        .windows_attributes(windows);
    tauri_build::try_build(attrs).expect("failed to run build script");
}
