# v8asm 静态反汇编尝试(V8 15.2.124.28,运行时无关)

[aynakeya/v8asm](https://github.com/aynakeya/v8asm) 是**纯 Python 静态**反汇编器:
它直接解析 V8 code-cache 的序列化流,**不加载/不链接 V8 运行时**。这从原理上
绕开了 d8 路线的死穴——不需要匹配 Electron 的运行时,Electron 专属的 external
references 也不会导致反序列化崩溃(只会变成占位符)。因此这是本案最有希望的方向。

## 已取得的进展

1. **确认版本**:`checkversion` 读出 `version_hash=0x472058a6`;`--calculate 15.2.124.28`
   得 `0xaf965af5`。两者不同仅因 **embedder 字符串**(Electron 的 V8 版本串带
   `-electron` 后缀),这与"版本就是 15.2.124.28"(已从二进制读到)一致。
2. **生成了 15.2.124.28 的 profile**(`15.2.124.28.json`,本目录)。v8asm 的
   profile 由 V8 源码头文件生成,不需要编译或运行 V8——只需对 v8/v8 的 tag
   `15.2.124.28` 执行 `git show`。生成时遇到一处 API 漂移并已修复:
   见 `generate_profiles-15.2.patch`(V8 15.2 的 `runtime.h` 用可变参数宏,
   把 `OFFLINE_F/OFFLINE_I` 改成可变参数即可)。

## 当前卡点(工具前沿)

用生成的 profile 静态反汇编时:

```
disassembler: unable to parse V8 cached data:
  tagged_size=4: no BytecodeArray candidates;
  tagged_size=8: unsupported serializer tag 0x0f at 0x12
```

即 v8asm 的**解析器本身**尚未完全支持 V8 15.2 的序列化格式(指针压缩路径找不到
BytecodeArray;非压缩路径遇到未知序列化 tag)。这与 v8asm README 的 TODO
"broaden Electron/private-build object-layout coverage" 一致——**14/15 的解析
逻辑还没补齐**,不是 profile 的问题。

## 三个公开工具的共同结论

| 工具 | 路线 | 对本文件(V8 15.2.124.28 / Electron 44.5.0)的结果 |
|---|---|---|
| suleram/View8 + jsc2js d8 | 跑 d8 反序列化再反汇编 | d8 在 `Deserialize` 内部越界(Electron 编译配置/外部引用不匹配) |
| NullString1/View8 | 同上(需匹配 node/d8 oracle) | 同样受运行时匹配限制 |
| **aynakeya/v8asm** | **纯静态解析,运行时无关** | profile 可生成;**解析器暂不支持 15.2 序列化格式** |

截至目前,**没有任何公开工具完整支持 V8 15.2 的 `.jsc` 反编译**。v8asm 的静态
路线最有前途,差的是解析器对 15.2 的适配。

## 可行的下一步

1. **给 v8asm 提 Issue/PR**(最有价值):附上本目录的 `15.2.124.28.json`(现成
   profile,省去作者生成)、`generate_profiles-15.2.patch`,以及卡点报错
   (`unsupported serializer tag 0x0f`)。作者正在做 14/15 覆盖,这些能直接推进。
2. 等 v8asm 补齐 15.2 解析后,用本目录的 profile 即可静态反汇编,再接 View8 出 JS。
3. 若只需了解 engine 的功能/联网/授权:见 `../../engine-analysis.md`,已完整,不依赖反编译。

## 复现

```bash
git clone https://github.com/aynakeya/v8asm && cd v8asm
git apply /path/to/generate_profiles-15.2.patch      # 修 15.2 API 漂移
# 拉 V8 源码该 tag(按需取 blob):
git init v8src && cd v8src && git remote add origin https://github.com/v8/v8.git
git fetch --depth 1 --filter=blob:none origin refs/tags/15.2.124.28:refs/tags/15.2.124.28
cd ..
# 生成 profile 到 disassembler/profiles/15.2.124.28.json，并在 index.json 的 versions 里加该版本
python3 -m disassembler engine.jsc --version 15.2.124.28 --format json   # 当前会在解析器处报错
```
