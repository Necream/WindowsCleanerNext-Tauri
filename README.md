# Windows Cleaner Next - Tauri

一款基于 **Tauri 2 + Rust + C++** 的 Windows 清理工具，侧边栏布局，面向新手使用。

本项目从 [DaYe 的 Windows Cleaner](https://github.com/darkmatter2048/WindowsCleaner) 重构而来，目标是：
- 更轻量
- 更清晰
- 更少操作步骤
- 尽量避免捆绑第三方二进制带来的协议风险

## 主要功能

### 清理
- Windows 更新缓存
- Windows 临时文件
- 系统日志
- 系统还原点
- 预取缓存
- 内存清理
- NVIDIA 驱动日志清理
- 用户临时文件
- 用户自定义脚本
- TMP / cache / msp 文件扫描清理
- 浏览器缓存

### 工具
- `mklink` 链接创建工具
- Mem Reduct CLI 下载引导
- 打开 `Tools` 目录

## 界面说明

- 左侧是功能分类
- 右侧是具体内容
- 当前所有清理项都放在「清理」
- 「工具」页放一些辅助功能和外部工具入口
- 「设置」页放主题、语言和窗口行为等基础选项

## 使用前提

- 需要 **Windows 10 / 11**
- 需要 **管理员权限**
- 本软件会自动请求 UAC 管理员权限

## 首次使用

1. 启动软件
2. 允许管理员权限
3. 进入「清理」页
4. 选择你要清理的项目
5. 点击「运行选中」

## Mem Reduct CLI

本项目的「内存清理」功能依赖用户自行下载的 `memreduct-cli.exe`。

请把它放到软件安装目录下的 `Tools` 文件夹中。

下载地址：

- https://github.com/Necream/memreduct-cli/releases

说明：
- 本项目**不捆绑**该二进制文件
- 用户自行下载后放入 `Tools` 目录即可使用

## 外部工具链接

有些工具只提供下载入口，不参与本项目内部调用流程。

这类工具的处理方式是：
- 只在「工具」页提供官网 / Release 链接
- 不把外部二进制打包进主程序
- 不在清理流程里直接调用它们

这样可以尽量避免协议冲突，也降低维护成本。

## NVIDIA 清理

该功能会自动检测 NVIDIA 相关组件，仅在检测到相关软件时才会推荐。

清理目标只包含安全的日志或缓存位置，不会去删除驱动安装源文件。

## `mklink` 链接工具

这个工具用于创建 Windows 链接，让文件或目录看起来仍然存在于原位置，但实际存放在其他磁盘。

常见用途：
- 把大文件夹迁移到 D 盘 / E 盘
- 不改软件配置继续使用原路径

## 运行开发版

```bash
npm install
npm run tauri dev
```

## 构建发布版

```bash
npm run tauri build
```

构建产物会输出到 `src-tauri/target/release/`。

Release builds and downloadable artifacts are code signed through the SignPath Foundation program.
If SignPath approval is still pending, the release workflow publishes the unsigned MSI and NSIS assets first.

## Microsoft Store 发布

如果你想把这个项目提交到 Microsoft Store，可以先看这里：

- [STORE_PUBLISH.md](./STORE_PUBLISH.md)

当前仓库保留了现有的 Windows 安装包发布方式，同时也预留了商店提交所需的发布资料与流程说明。

## 目录说明

- `src/` 前端页面
- `src-tauri/` Rust 后端和打包配置
- `src-tauri/Tools/` C++ 清理工具
- `src-tauri/Config/` 配置文件

## 注意事项

- 某些清理项会删除系统缓存，属于正常行为
- 如果文件被占用，可能会出现个别错误计数
- 不要手动删除不熟悉的系统目录
- 建议先看输出日志再决定是否执行某些清理项

## 许可证

本项目采用 **Apache-2.0**。

其中部分工具通过外部方式提供，不直接捆绑到主程序中，以避免协议冲突。
