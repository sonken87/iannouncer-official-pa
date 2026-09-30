using System.Buffers.Binary;

namespace JscTool;

// Stage ①: parse the 32-byte V8 SerializedCodeData header of a .jsc file.
//
// Modern V8 (>= ~9.x, incl. 15.2) layout, all little-endian uint32:
//   [0x00] magic_number                (0xC0DE0000 | code_cache_version)
//   [0x04] version_hash                (hash of the V8 version string)
//   [0x08] source_hash                 (length/hash of the original source)
//   [0x0C] flag_hash                   (hash of the V8 flags at serialization)
//   [0x10] read_only_snapshot_checksum (embedder read-only snapshot identity)
//   [0x14] payload_length              (bytes of serialized payload)
//   [0x18] checksum                    (payload checksum; may be split/2 words)
//   [0x1C] (reserved / second checksum word)
public sealed class V8CacheHeader
{
    public const int HeaderSize = 32;
    public const uint MagicFamilyMask = 0xFFFF0000u; // high 16 bits identify the V8 magic family

    public uint Magic { get; init; }
    public uint VersionHash { get; init; }
    public uint SourceHash { get; init; }
    public uint FlagHash { get; init; }
    public uint ReadOnlySnapshotChecksum { get; init; }
    public uint PayloadLength { get; init; }
    public uint Checksum0 { get; init; }
    public uint Checksum1 { get; init; }

    public long FileLength { get; init; }

    // The high 16 bits are the code-cache magic family; the low 16 bits encode
    // the compile-time ExternalReferenceTable size (embedder-specific).
    public uint MagicFamily => Magic & MagicFamilyMask;
    public uint EmbedderMagicBits => Magic & 0x0000FFFFu;

    public bool LooksLikeV8Cache => MagicFamily == 0xC0DE0000u;
    public long ExpectedPayloadLength => FileLength - HeaderSize;
    public bool PayloadLengthMatchesFile => PayloadLength == ExpectedPayloadLength;

    public static V8CacheHeader Parse(ReadOnlySpan<byte> data, long fileLength)
    {
        if (data.Length < HeaderSize)
            throw new InvalidDataException($"file too small: {data.Length} < {HeaderSize} bytes");
        return new V8CacheHeader
        {
            Magic = BinaryPrimitives.ReadUInt32LittleEndian(data.Slice(0x00, 4)),
            VersionHash = BinaryPrimitives.ReadUInt32LittleEndian(data.Slice(0x04, 4)),
            SourceHash = BinaryPrimitives.ReadUInt32LittleEndian(data.Slice(0x08, 4)),
            FlagHash = BinaryPrimitives.ReadUInt32LittleEndian(data.Slice(0x0C, 4)),
            ReadOnlySnapshotChecksum = BinaryPrimitives.ReadUInt32LittleEndian(data.Slice(0x10, 4)),
            PayloadLength = BinaryPrimitives.ReadUInt32LittleEndian(data.Slice(0x14, 4)),
            Checksum0 = BinaryPrimitives.ReadUInt32LittleEndian(data.Slice(0x18, 4)),
            Checksum1 = BinaryPrimitives.ReadUInt32LittleEndian(data.Slice(0x1C, 4)),
            FileLength = fileLength,
        };
    }

    public string Describe()
    {
        var w = new StringWriter();
        w.WriteLine("V8 CachedData header");
        w.WriteLine($"  magic                     = 0x{Magic:x8}  (family 0x{MagicFamily:x8}, ext-ref-table bits 0x{EmbedderMagicBits:x4})");
        w.WriteLine($"  version_hash              = 0x{VersionHash:x8}");
        w.WriteLine($"  source_hash              = 0x{SourceHash:x8}  ({SourceHash} = original source length)");
        w.WriteLine($"  flag_hash                = 0x{FlagHash:x8}");
        w.WriteLine($"  ro_snapshot_checksum     = 0x{ReadOnlySnapshotChecksum:x8}");
        w.WriteLine($"  payload_length           = 0x{PayloadLength:x8}  ({PayloadLength} bytes)");
        w.WriteLine($"  checksum[0]              = 0x{Checksum0:x8}");
        w.WriteLine($"  checksum[1]              = 0x{Checksum1:x8}");
        w.WriteLine($"  file_length              = {FileLength} bytes");
        w.WriteLine($"  looks_like_v8_cache      = {LooksLikeV8Cache}");
        w.WriteLine($"  payload+header==file     = {PayloadLengthMatchesFile} (header {HeaderSize} + payload {PayloadLength} = {HeaderSize + PayloadLength})");
        return w.ToString();
    }
}
