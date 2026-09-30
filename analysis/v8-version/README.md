# engine.jsc 的 V8 / Node 版本分析

分析对象：仓库根目录的 `engine.jsc`（bytenode 编译的 V8 字节码），配套 `engine.cjs`
加载器与 `engine.jsc.meta.json`。

## 结论

| 项目 | 值 |
|---|---|
| Electron | **44.5.0**（来自 `engine.jsc.meta.json`） |
| V8 | **15.2**（Electron 44 系列内置，官方发行说明） |
| Node.js | **24.x**（Electron 44 系列内置） |
| 平台 / 架构 | win32 / x64 |
| 运行上下文 | Electron utility 进程 |

`engine.jsc.meta.json` 内容：

```json
{"electron":"44.5.0","platform":"win32","arch":"x64","context":"utility",
 "sha256":"56b2191feb9abdcf16dd38699ebf6a799c2c28cf785bd8a2948f41a2c1bd0577"}
```

该 sha256 与 `engine.jsc` 的实际哈希一致，文件未被篡改。

Electron→运行时版本对应（官方发行说明）：

- Electron 44.0.0 → Chromium 152、**V8 15.2**、Node 24.18.1
- Electron 44.1.0 → Chromium 152.0.7977.65、**V8 15.2.124.18**、Node 24.19.0
- 本文件为 44.5.0，属同一 15.2 分支的后续补丁，完整号约为 **15.2.124.x**

`engine.cjs` 在加载前强制校验运行时，进一步印证该约束：

```js
if (process.versions.electron !== build.electron ||
    process.platform !== build.platform ||
    process.arch !== build.arch)
  throw new Error('Incompatible application runtime. Reinstall this version of the app.');
```

## 字节码头（V8 code cache header）

`engine.jsc` 前 32 字节（小端）：

```
0xc0de06cf  magic_number       (^ 0xC0DE0000 = 1743, V8 code cache 版本标记)
0x472058a6  version_hash       (V8 版本字符串的哈希)
0x00138a24  source_hash
0xdae6e9eb  flag_hash
0x318e262a  (payload/checksum 区)
0x001dbde0
0x00000000
0x00000000
```

## 为什么不能直接从 version_hash 反推出确切版本

V8 生成 `version_hash` 的算法近期变动过两次：

1. **2026-06-08**（commit `164cdee2b5`）：整数哈希从 Thomas Wang 的
   mix 函数改为 rapidhash 的 mum 混合器。
2. **2026-09-25**（commit `c68946f709`）：版本哈希开始纳入 embedder 字符串。

Electron 44 的 V8 15.2 正处在这段变动区间，逐版还原不稳。

### 尝试记录（本目录内的脚本）

- `v8hash.cpp`、`vr.c`：按 V8 当前源码（rapidhash + Murmur 合并，含/不含
  embedder 字符串）暴力比对 `0x472058A6`，在 10.x–16.x 范围内**未**得到
  形如 `15.2.124.x` 的合理版本号。
- `vd.c`、`vd_hash.py`：移植自 View8 工具自带 `Bin/VersionDetector.exe` 的
  哈希函数（反汇编见 `VersionDetector.hashfn.disasm.txt`）。该 exe 用的是
  **旧算法**（Wang32 常量 `0x809`=2057、移位 15/12/4/16，配 MurmurHash
  混合器 `0xC6A4A7935BD1E995` / 右移 47）。反查同样得不到合理 V8 版本号。

两者都得不到合理结果，恰好印证该文件由 **2026 年的新版 V8（15.2）**序列化，
与 Electron 44 一致。因此版本以 `meta.json` 声明 + Electron 官方对应关系为准，
而非从 hash 反推。

## View8 反编译的可行性

View8（github.com/suleram/View8）可把 `.jsc` 反编译成类 JS 代码，但它依赖与目标
**完全匹配**的已打补丁 V8 反汇编器。其发行版仅提供：

- V8 9.4.146.24（Node 16）
- V8 10.2.154.26（Node 18）
- V8 11.3.244.8（Node 20）

没有 V8 15.2（Node 24）的反汇编器，因此在不自行编译一份打补丁的 V8 15.2 的
前提下，无法在本环境用 View8 完整反编译 `engine.jsc`。要走这条路，需要：

1. 拉取 V8 15.2.124.x 源码；
2. 应用 View8 的反汇编补丁并编译出 `15.2.124.x.exe`；
3. `python view8.py -i engine.jsc -p 15.2.124.x.exe -o out.js`。

## 复现方法

```bash
# 读字节码头
python3 - <<'PY'
import struct; b=open('engine.jsc','rb').read()
print([hex(x) for x in struct.unpack('<8I', b[:32])])
PY

# 旧算法反查（View8 VersionDetector 移植）
gcc -O2 -o vd vd.c && ./vd

# 新算法（rapidhash）反查
gcc -O2 -o vr vr.c && ./vr
```
