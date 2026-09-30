# 反编译 engine.jsc(V8 15.2 / Electron 44 字节码)

目标:把 `engine.jsc`(bytenode 编译的 V8 字节码)还原成可读的近似 JavaScript。

> 版权提示:还原出的源码版权仍属 StrataWx,仅供你自己分析/互操作使用,请勿对外分发。
> 还原是**语义近似**,不等于原始源码(变量名、控制流可能不同)。

## 方案:xqy2006/jsc2js

调研后确定用 **[xqy2006/jsc2js](https://github.com/xqy2006/jsc2js)**,原因:

- 它的"现代兼容层"明确覆盖 **V8 14.7.84–15.3.25**(含 `15.2.124.x`),正是本文件的版本。
- 它**绕过宿主相关的哈希**(version / source / flags + embedder 只读快照身份),
  从而兼容同一 V8 线的不同宿主(upstream d8 与 **Electron**),并明确说明
  "Electron works fine"。本文件正是 Electron 生成。
- 提供多版本**预编译补丁 d8**(带 `loadjsc()` 内建)+ 集成 View8,无需自行编译 V8。

其他候选:suleram/View8(仅 9.4/10.2/11.3)、hasherezade/jsc_deobfuscator
(仅到 13.6)、noelex/v8dasm(需自行编译)、v8-disassembler/ghidra_nodejs
(仅 V8 6.2)——都不覆盖 15.2,故不采用。

## 精确版本链

| 层 | 版本 | 来源 |
|---|---|---|
| Electron | 44.5.0 | `engine.jsc.meta.json` |
| Chromium | 152.0.7977.130 | Electron v44.5.0 的 `DEPS` |
| Node | v24.21.0 | 同上 |
| V8 | 15.2.124.x | Chromium 152 对应线;jsc2js 有 `.13/.18/.19/.28` 等 tag |

`engine.jsc` 头部(32 字节,小端)完整且自洽:

```
magic=0xc0de06cf  version_hash=0x472058a6  source_hash=0x00138a24
flag_hash=0xdae6e9eb  ro_snapshot_checksum=0x318e262a
payload_length=0x001dbde0 (=1,949,152)
文件大小 1,949,184 = 32(头) + 1,949,152(payload) ✓ 边界检查通过
```

## 关键结论:必须用 Windows d8

在本 Linux 环境实测:预编译的 **Linux** d8(`15.2.124.13/.18/.19/.28` 全试过)
在 `loadjsc` 反序列化阶段一律崩溃:

```
vector.h:415: libc++ Hardening assertion __n < size() failed: vector[] index out of bounds
Received signal 6 (Aborted)
```

四个版本报错完全一致,**不是补丁号选错**。这与 hasherezade/jsc_deobfuscator
文档记录的现象吻合:**同一 V8 版本,Linux 运行时会拒绝 Windows 运行时能接受的
code cache**。本文件是 **win32/x64** 序列化的,因此反汇编必须在 **Windows** 上、
用 `d8-windows.exe` 完成。

(ICU 依赖:d8 需要 `icudtl.dat`。Linux 上用系统 Chromium 的 `icudtl.dat` 已解决
ICU 初始化;Windows 上用 Electron 44.5.0 自带的 `icudtl.dat`。)

## 在 GitHub 上运行(推荐)

仓库已内置 workflow:`.github/workflows/decompile-engine-jsc.yml`
(`windows-latest`,手动触发 `workflow_dispatch`)。它会:

1. 下载 jsc2js 对应版本的 `d8-<tag>-windows.zip`;
2. 取 Electron 44.5.0 的 `icudtl.dat`;
3. `d8.exe -e "loadjsc('engine.jsc')"` → `engine.bytecode.txt`;
4. jsc2js 的 View8:`view8.py --disassembled engine.bytecode.txt engine.js`;
5. 上传 `engine.bytecode.txt` / `engine.js` 等为构建产物(artifacts)。

在 GitHub 仓库 **Actions** 页选择 *decompile-engine-jsc* → *Run workflow*
(可改 `v8_tag`;若某版本仍崩溃,换相邻 tag 如 `15.2.124.19`)。

## 本地复现(Linux,预期崩溃,用于佐证平台差异)

见 `decompile_local.sh`。它下载 Linux d8 并尝试反编译,记录崩溃,证明需要
Windows 路径。

## 已验证 / 未决

- 已验证:方案选型、精确版本链、Windows d8 资源存在(35MB + snapshot)、
  Linux 路径崩溃且原因明确、workflow 逻辑。
- 未决:Windows d8 的实际反编译结果——需在 GitHub Actions(Windows runner)
  上运行 workflow 产出;本 Linux 环境无法运行 Windows d8。
