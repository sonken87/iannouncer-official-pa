# jsctool — V8 `.jsc`(CachedData)分析器

**导入 `.jsc` → 自动输出分析报告**的单文件 Windows 工具。针对 V8 15.2 /
Electron 44 这类新版本(现有反编译器尚不支持)提供**当前可达的最好结果**。

## 用法

- 把 `engine.jsc` **拖到 `jsctool.exe`** 上;或命令行 `jsctool.exe engine.jsc`;
  或双击 exe 后按提示输入路径。
- 在输入文件同目录**自动生成**:
  - `engine.report.txt` — 结构化报告(头 / 版本 / 分类符号)
  - `engine.strings.txt` — 去重字符串常量清单
  - `engine.constants.json` — 带偏移的原始提取(机读)
- 自包含,目标机**无需安装 .NET**。

## 它能做什么(不依赖 V8 运行时,纯静态)

| 阶段 | 功能 | 状态 |
|---|---|---|
| ① | 解析 32 字节 V8 CachedData 头(magic / version_hash / flags / payload / checksum) | ✅ |
| ② | 版本哈希扫描(rapidhash/Wang + 常见 embedder,匹配 version_hash) | ✅ |
| ⑥ | 常量池 / 字符串提取(1-byte 与 UTF-16LE) | ✅ |
| ⑧/⑨ | 分类检测:HTTP 路由、URL/host、License/seat 符号、天气/SimConnect 符号、ENV 常量 | ✅ |

## 它**不能**做什么(与语言无关的硬限制)

| 阶段 | 功能 | 状态 |
|---|---|---|
| ④/⑤ | 加载 CachedData → V8 Deserialize → Dump Bytecode | ❌ 需匹配的 V8 运行时 |
| ⑦/⑧ | Bytecode → CFG → 伪 JS | ❌ 依赖 ④/⑤ |

**为什么**:反序列化 V8 CachedData 必须由**编译它的同款 V8**完成(涉及版本专属的
bytecode 表、对象布局、以及 Electron 专属 external references)。这一步无法脱离
V8 实现——用 C#、Python、C++ 都一样。截至目前没有任何公开工具完整支持 V8 15.2
的 `.jsc` 反序列化(详见 `../analysis/decompile/`)。因此本工具专注于**不依赖运行时
也能拿到的、对定位业务/授权逻辑最有用的信息**。

## 对本样本(StrataWx engine.jsc)的实测输出

见 `sample-output/engine.report.txt`。摘要:

- 头:`magic=0xc0de06cf ver_hash=0x472058a6`,payload+header==file ✅
- URL/host:`127.0.0.1:5173`、`www.simbrief.com`、`wx.stratawxfs.org/wxE`
- License:`STRATAWX_LICENSE_KEY`、`STRATAWX_MACHINE_ID`、`x-stratawx-license`、
  `x-stratawx-machine`、`no license key in env - weather unavailable (licensed proxy only)`
- 天气/SimConnect:115 个符号(`SimConnectConnection`、`WeatherMode`、
  `cloudLayers`、`exportHistoricWinds` 等)
- HTTP 路由:`/api/v1/weather/current`、`/export-historic-winds`、`/metar/`、
  `/atis`、`/gfs`、`/health`、`/route` 等

## 从源码构建

```bash
dotnet publish -c Release -r win-x64 --self-contained true \
  -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true \
  -o publish/win-x64
# 产物:publish/win-x64/jsctool.exe
# Linux/macOS 换 -r linux-x64 / osx-x64
```

## 源码结构

```
src/V8CacheHeader.cs        ① 头解析
src/V8VersionHash.cs        ② 版本哈希算法与扫描
src/ConstantPoolExtractor.cs ⑥ 字符串/常量提取
src/Detectors.cs            ⑧⑨ 路由/URL/License/天气/ENV 分类
src/Program.cs             CLI + 拖拽导入 + 自动输出
```
