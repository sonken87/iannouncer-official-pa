# jsc2js Issue 草稿(可直接粘贴到 https://github.com/xqy2006/jsc2js/issues)

## 标题

Electron 44.5.0 (V8 15.2.124.28) code cache — vector OOB inside CodeSerializer::Deserialize

## 正文(中文)

- 目标文件:bytenode 生成的 `.jsc`,来自 **Electron 44.5.0**(utility 进程)。
- 精确 V8 版本:**15.2.124.28**(已从 Electron 二进制中提取版本字符串确认)。
- 文件头(32 字节,小端):

  ```
  magic          = 0xc0de06cf
  version_hash   = 0x472058a6
  source_hash    = 0x00138a24
  flag_hash      = 0xdae6e9eb
  ro_snapshot_ck = 0x318e262a
  payload_length = 0x001dbde0  (= 1,949,152)
  ```

  文件大小 1,949,184 字节 = 32(header) + 1,949,152(payload),边界精确匹配。
- 现象:使用 Releases 里的预编译 `d8-15.2.124.28`(**Linux 与 Windows 均已尝试**),
  `loadjsc()` 的头部检查(magic 家族 / payload 长度)通过,但在
  `CodeSerializer::Deserialize` 内部崩溃:

  ```
  vector.h:415: libc++ Hardening assertion __n < size() failed: vector[] index out of bounds
  Received signal 6 (SIGABRT)
  ```

  相邻版本 `15.2.124.1 / .5 / .13 / .18 / .19` 同样崩溃;加
  `--no-lazy --no-flush-bytecode` 无效。
- 推测:Electron 的 V8 与原版 v8.git 同一 tag,但**编译配置(GN args)/外部引用表
  内容不同**。现代兼容层只规范化了外部引用表的**大小**,未覆盖 Electron 专属的
  external reference 条目,导致反序列化时索引越界。
- 请教:能否为 **Electron 44.5.0 的 V8 15.2.124.28** 提供匹配的 d8,或指点如何
  绕过 embedder 专属的 external references?可提供样本文件。

## 正文(English)

- Target: a bytenode `.jsc` produced by **Electron 44.5.0** (utility process).
- Exact V8: **15.2.124.28** (confirmed from the version string inside the Electron binary).
- Header (32 bytes, little-endian):

  ```
  magic          = 0xc0de06cf
  version_hash   = 0x472058a6
  source_hash    = 0x00138a24
  flag_hash      = 0xdae6e9eb
  ro_snapshot_ck = 0x318e262a
  payload_length = 0x001dbde0  (= 1,949,152)
  ```

  File is 1,949,184 bytes = 32 (header) + 1,949,152 (payload); boundary matches exactly.
- Symptom: with the prebuilt `d8-15.2.124.28` (tried on **both Linux and Windows**),
  the `loadjsc()` header checks pass, but it aborts **inside**
  `CodeSerializer::Deserialize`:

  ```
  vector.h:415: libc++ Hardening assertion __n < size() failed: vector[] index out of bounds
  Received signal 6 (SIGABRT)
  ```

  Neighbouring tags `15.2.124.1/.5/.13/.18/.19` abort the same way;
  `--no-lazy --no-flush-bytecode` does not help.
- Hypothesis: Electron's V8 at this tag differs from upstream v8.git in build
  config / external-reference-table contents. The modern compat layer normalizes
  the external reference table **size** but not Electron-specific external
  reference **entries**, so deserialization indexes out of bounds.
- Question: could you provide a d8 matching **Electron 44.5.0's V8 15.2.124.28**,
  or advise how to bypass embedder-specific external references? Sample available.

## 背景记录(本仓库)

- 引擎功能分析:`../engine-analysis.md`
- V8 版本判定:`../v8-version/README.md`
- 反编译方案与流程:`./README.md`
