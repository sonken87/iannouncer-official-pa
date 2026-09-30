using System.Text;
using JscTool;

// jsctool — V8 .jsc (CachedData) analyzer.
//
// Usage:
//   jsctool <file.jsc>            分析文件，在同目录自动输出报告
//   拖动 .jsc 到 jsctool.exe 上   同上（Explorer 会把路径作为参数传入）
//   双击 jsctool.exe             提示输入/拖入文件路径
//
// 自动输出（与输入文件同目录）:
//   <name>.report.txt       结构化分析报告（头/版本/分类符号/函数概览）
//   <name>.strings.txt      去重后的字符串常量清单
//   <name>.constants.json   带偏移的原始提取结果（机读）

Console.OutputEncoding = Encoding.UTF8;

string? path = args.Length > 0 ? args[0] : null;
if (string.IsNullOrWhiteSpace(path))
{
    Console.Write("拖入 .jsc 文件到本窗口，或输入路径后回车: ");
    path = Console.ReadLine()?.Trim().Trim('"');
}
if (string.IsNullOrWhiteSpace(path) || !File.Exists(path))
{
    Console.Error.WriteLine($"找不到文件: {path}");
    Pause();
    return 2;
}

byte[] data = File.ReadAllBytes(path);
var header = V8CacheHeader.Parse(data, data.LongLength);

// Payload begins after the 32-byte header.
int payloadStart = V8CacheHeader.HeaderSize;
int payloadLen = (int)Math.Min(data.LongLength - payloadStart,
                               header.PayloadLength == 0 ? data.LongLength : header.PayloadLength);
var payload = data.AsSpan(payloadStart, Math.Max(0, payloadLen));

var extracted = ConstantPoolExtractor.Extract(payload, minLen: 4);
var strings = extracted.Select(e => e.Value).ToList();
var groups = Detectors.Classify(strings);

// ---- Build report ----
var r = new StringBuilder();
r.AppendLine("================ jsctool 分析报告 ================");
r.AppendLine($"文件: {Path.GetFullPath(path)}");
r.AppendLine($"大小: {data.LongLength} 字节");
r.AppendLine($"时间: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
r.AppendLine();
r.AppendLine("── ① V8 CachedData 头 ──");
r.Append(header.Describe());
r.AppendLine();

r.AppendLine("── ② V8 版本 ──");
r.AppendLine($"version_hash = 0x{header.VersionHash:x8}");
r.AppendLine("扫描 8.x–16.x 匹配 version_hash（含 rapidhash/Wang 两种混合、常见 embedder）...");
var matches = V8VersionHash.Scan(
    header.VersionHash,
    major: (8, 16), minor: (0, 12), build: (0, 400), patch: (0, 60),
    embedders: new[] { "", "-electron.0", "-electron", "-node.0" });
if (matches.Count == 0)
    r.AppendLine("  未在扫描范围内找到匹配（embedder 后缀可能不在候选内；已知本类文件为 Electron，version_hash 含 Electron 专属 embedder 串）。");
else
    foreach (var m in matches.Take(20)) r.AppendLine($"  候选: {m}");
r.AppendLine();

r.AppendLine("── ⑥/⑧ 提取的字符串常量分类 ──");
r.AppendLine($"共提取字符串 {strings.Count} 条（去重 {strings.Distinct().Count()} 条）。");
r.AppendLine();
foreach (var g in groups)
{
    r.AppendLine($"【{g.Title}】 {g.Items.Count} 条");
    foreach (var it in g.Items.Take(200)) r.AppendLine($"    {it}");
    if (g.Items.Count > 200) r.AppendLine($"    ... 其余 {g.Items.Count - 200} 条见 .strings.txt");
    r.AppendLine();
}

r.AppendLine("── ④/⑤/⑦ 完整 Bytecode → 伪 JS ──");
r.AppendLine("需要匹配的 V8 运行时来反序列化 CachedData（这一步无法脱离 V8 完成，与语言无关）。");
r.AppendLine("本工具已完成不依赖运行时的阶段：头解析、版本扫描、常量池/字符串提取与分类。");
r.AppendLine("如需继续，请见仓库 analysis/decompile/ 中 v8asm 静态路线（待 V8 15.2 解析器补齐）。");

// ---- Write outputs next to input ----
string baseName = Path.Combine(
    Path.GetDirectoryName(Path.GetFullPath(path)) ?? ".",
    Path.GetFileNameWithoutExtension(path));
File.WriteAllText(baseName + ".report.txt", r.ToString(), new UTF8Encoding(false));

var sb = new StringBuilder();
foreach (var s in strings.Distinct().OrderBy(x => x, StringComparer.Ordinal))
    sb.AppendLine(s);
File.WriteAllText(baseName + ".strings.txt", sb.ToString(), new UTF8Encoding(false));

var js = new StringBuilder();
js.Append('[');
for (int k = 0; k < extracted.Count; k++)
{
    var e = extracted[k];
    if (k > 0) js.Append(',');
    js.Append("{\"offset\":").Append(e.Offset)
      .Append(",\"kind\":\"").Append(e.Kind).Append("\",\"value\":")
      .Append(JsonString(e.Value)).Append('}');
}
js.Append(']');
File.WriteAllText(baseName + ".constants.json", js.ToString(), new UTF8Encoding(false));

Console.WriteLine(r.ToString());
Console.WriteLine("已生成:");
Console.WriteLine($"  {baseName}.report.txt");
Console.WriteLine($"  {baseName}.strings.txt");
Console.WriteLine($"  {baseName}.constants.json");
Pause();
return 0;

static string JsonString(string s)
{
    var b = new StringBuilder("\"");
    foreach (char c in s)
    {
        switch (c)
        {
            case '\"': b.Append("\\\""); break;
            case '\\': b.Append("\\\\"); break;
            case '\n': b.Append("\\n"); break;
            case '\r': b.Append("\\r"); break;
            case '\t': b.Append("\\t"); break;
            default:
                if (c < 0x20) b.Append("\\u").Append(((int)c).ToString("x4"));
                else b.Append(c);
                break;
        }
    }
    b.Append('\"');
    return b.ToString();
}

static void Pause()
{
    if (!Console.IsInputRedirected)
    {
        Console.WriteLine();
        Console.Write("按回车键退出...");
        try { Console.ReadLine(); } catch { }
    }
}
