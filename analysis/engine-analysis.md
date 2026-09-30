# StrataWx `resources/app/engine` 分析报告

分析对象：用户从 `C:\Users\Admin\AppData\Local\StrataWx\resources\app\engine`
提取并上传到本仓库的文件：

- `engine.cjs` — 明文加载器
- `engine.jsc` — 编译后的 V8 字节码主程序（约 1.9 MB）
- `engine.jsc.meta.json` — 运行时版本元数据
- `airport-db.data.json` — 机场数据库（约 3.5 MB）

分析方式：仅基于上传到 GitHub 的文件，未运行程序。含明文文件通读，
以及从 `engine.jsc` 提取的明文字符串（`strings`，ASCII + UTF-16LE）。

> 注：`engine` 与本仓库 iAnnouncer 项目无关联，是用户单独提交来做安全/功能分析的。

## 结论

`engine` 是一个面向 **微软飞行模拟器（MSFS 2020/2024）** 的**实时天气引擎**。
它获取真实世界航空天气，通过 SimConnect 将云、风、能见度、颠簸等注入模拟器，
使模拟器内天气与现实一致。在上传的文件中**未发现恶意行为迹象**。

## 各文件作用

| 文件 | 作用 |
|---|---|
| `engine.cjs` | 加载器（5 行）：校验运行时为 Electron 44.5.0 / win32 / x64，然后 `require('bytenode')` 加载 `engine.jsc` |
| `engine.jsc.meta.json` | 版本元数据；记录的 sha256 与 `engine.jsc` 实际哈希一致，未被篡改；`context: utility` 表示运行在 Electron 工具进程 |
| `engine.jsc` | 主程序，V8 字节码，打包了 Express、cookie-parser、iconv-lite 等库 |
| `airport-db.data.json` | 机场库，来源标注 OurAirports（公有领域）：25,229 个机场、60,172 个跑道端；字段含名称/坐标/标高/国家/跑道 |

## 主要功能（据明文字符串）

1. **获取真实天气**
   - METAR/TAF 实况与预报（按机场 ID 或经纬度范围）
   - SIGMET/AIRMET/PIREP 危险天气通报与飞行员报告
   - 数值天气模式：GFS（`nomads:gfs`）、ECMWF（`ecmwf_ifs025`）、
     Open-Meteo（模式/集合/历史存档）
   - User-Agent：`StrataWx/1.0 (+https://stratawxfs.org)`
2. **写入模拟器**
   - 通过 SimConnect 连接 MSFS（`SimConnect: connected`、`MSFS 2020`、`MSFS 2024`）
   - 写入云量、云层、疏密、风向风速、颠簸、结冰
   - 提示文字：会沿航迹提前加载前方天气系统；处于 IFR 类通报区内会压低能见度
3. **航路与高空风**
   - 从 SimBrief 取飞行计划（`https://www.simbrief.com/api/xml.fetcher.php`）
   - 计算各航路点高空风并写入 PMDG 737、iFly 737 MAX 的 FMC
     （`pmdg-aircraft-736/737`、`ifly-aircraft-737max8`）
   - 支持导出历史风数据
4. **本地 HTTP 服务**（仅监听 `127.0.0.1`，供模拟器面板/主界面调用）
   - 接口：`/api/v1/weather/current`、`/api/v1/route`、`/health`、`/status`、
     `/weather`、`/simbrief`
   - 需配合装在 MSFS Community 文件夹的 `stratawx-injector` 插件包

## 联网 / 注册表 / 授权

- **对外地址**：`wx.stratawxfs.org`（天气代理）、`www.simbrief.com`，及上述天气数据源
- **授权**：使用环境变量 `STRATAWX_LICENSE_KEY`、`STRATAWX_MACHINE_ID`，
  请求头 `x-stratawx-license`；提示 "no license key in env - weather unavailable
  (licensed proxy only)"——即无授权则拿不到天气，因此会把授权密钥与机器 ID
  发往 `wx.stratawxfs.org`
- **注册表**：仅见**读取**操作，用于查 `SimConnect_Port_IPv4`（定位 SimConnect
  端口），通过 npm 包 `regedit` 调用系统 cscript 实现（`spawnCScriptSucceeded`）
- **未发现**：键盘记录、截屏、剪贴板窃取、钱包、Discord/Telegram、挖矿、
  `eval`/`new Function` 等属于其自身代码的命中；出现的 `cookie`/`crypto` 等
  字样均来自 Express 框架及其 MIME 类型表

## 局限

- 为**字符串级静态分析**，非完整反编译：能说明"有哪些功能、连哪些地址"，
  不能证明"绝无其他隐藏逻辑"
- 发往服务器的确切字段无法从字符串确认（除授权密钥/机器 ID 外，可能含位置等）
- 如需进一步确认，可运行时抓包（Fiddler/Wireshark）核实其仅连上述地址
- `engine.jsc` 需 V8 15.2 / Electron 44 运行时才能加载（见 `v8-version/`）

## 相关目录

- `v8-version/` — V8/Node 版本判定与字节码头分析、复现脚本
