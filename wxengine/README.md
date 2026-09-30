# wxengine

一个用 C++ 重新实现的 MSFS **实时天气 / 高空风引擎**，目标平台 **MSFS 2024**。
功能对标从 `engine.jsc`（StrataWx）静态分析得到的能力（见 `../analysis/`），
但为**独立实现**：不反编译、不复制原字节码，也不绕过原产品的授权。

## 功能

- 通过 **SimConnect** 连接 MSFS 2024，读取飞机位置/高度/航向
- 从 **SimBrief** 获取飞行计划（OFP navlog）
- 沿航路各点取**高空风与温度**，插值到 FMC 高度层
- 生成**风数据上传**文件（PMDG 737 / iFly 737 MAX）+ 通用 JSON
- 本地 `127.0.0.1` HTTP API，供机内面板 / UI 调用
- METAR 解码、机场库（复用 OurAirports 的 `airport-db.data.json`）

## 风数据来源（可切换）

`WindProvider` 是抽象接口，`config.json` 的 `wind_source` 选择实现：

| 值 | 实现 | 说明 |
|---|---|---|
| `open-meteo` | `OpenMeteoProvider` | 公开数据源；非商业免费，商业需 Open-Meteo 套餐 |
| `stratawx` | `StrataWxProvider` | 授权代理；**接口待按官方 API 文档补全** |

### StrataWx provider 的状态

`src/stratawx_provider.cpp` 目前是**占位实现**，会抛出"未实现"。要启用，需按
**官方 API 文档**填入：请求路径、请求头名称、返回 JSON 结构。凭据从环境变量读取，
**绝不硬编码、绝不提交仓库**：

```
STRATAWX_LICENSE_KEY   合同要求的授权密钥
STRATAWX_MACHINE_ID    合同要求的机器 ID
```

（合同要求以相同的 KEY / MACHINE_ID 调用。从字符串分析看，请求头疑似
`x-stratawx-license`，但**务必以官方文档为准**，不要照 `engine.jsc` 内部实现推断。）

## 依赖

- CMake ≥ 3.20、支持 C++17 的编译器
- 自动拉取（FetchContent）：nlohmann/json、cpp-httplib、doctest
- Windows：WinHTTP（系统自带）；非 Windows：OpenSSL（供 httplib HTTPS）
- MSFS 2024 SDK（仅 Windows 且开启 `WXE_WITH_SIMCONNECT` 时）

## 构建

```bash
# 开发 / 测试（无模拟器，非 Windows 也可）
cmake -S . -B build -G Ninja -DWXE_WITH_SIMCONNECT=OFF
cmake --build build
ctest --test-dir build --output-on-failure

# Windows + MSFS 2024 SDK
cmake -S . -B build -DMSFS_SDK="C:/MSFS 2024 SDK"
cmake --build build --config Release
```

## 运行

```bash
cp config.example.json config.json   # 按需修改
wxengine --config config.json serve  # 启动本地 API
wxengine --config config.json winds  # 取 SimBrief 航路风并生成上传文件
```

本地 API：

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/health` | 健康检查 |
| GET | `/status` | 模拟器连接状态 + 风源 |
| GET | `/api/v1/weather/current` | 当前机位的各层风 |

## 目录

```
src/
  types.h                 公共数据类型
  config.*                config.json 加载
  http_client*.{h,cpp}    HTTP 客户端（WinHTTP / httplib）
  geo.*                   大圆距离、风矢量、剖面插值
  simbrief.*              SimBrief OFP 解析
  open_meteo.*            Open-Meteo 高空风
  stratawx_provider.*     StrataWx 授权代理（占位，待补全）
  winds.*                 WindProvider 接口与工厂
  airports.*              机场库加载
  metar.*                 METAR 解码
  uplink.*                风数据上传文件生成
  sim*.{h,cpp}            SimConnect（Windows）/ 空实现（其他）
  server.*                本地 HTTP 服务
  main.cpp                入口
tests/                    doctest 单元测试
```

## 说明与限制

- MSFS 2024 已废弃 SimConnect 旧的天气写入函数，实时云/风注入需配合
  Community 插件（原产品用 `stratawx-injector`）。`inject_winds` 是接入点，
  具体注入方式须按目标版本 SDK 确认。
- PMDG / iFly 风上传文件的确切格式须以各厂商 SDK/说明为准；`uplink.cpp` 先写出
  自描述的文本格式作为起点。
- 各数据源使用条款需自行遵守（aviationweather.gov / NOAA 免费；Open-Meteo 免费版
  限非商业；SimBrief 需用户自己的账号）。
