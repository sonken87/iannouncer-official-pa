using System.Text;

namespace JscTool;

// Stage ⑥: extract the string constants embedded in the serialized payload.
//
// Full constant-pool reconstruction needs V8 deserialization, but the string
// contents (SeqOneByteString / SeqTwoByteString bodies) sit in the payload as
// readable byte runs. We scan for one-byte (Latin-1/ASCII) and two-byte
// (UTF-16LE) runs. This reliably surfaces routes, symbols, and identifiers.
public sealed record ExtractedString(long Offset, string Kind, string Value);

public static class ConstantPoolExtractor
{
    public static List<ExtractedString> Extract(ReadOnlySpan<byte> payload, int minLen = 4)
    {
        var list = new List<ExtractedString>();

        // One-byte printable runs.
        int start = -1;
        for (int i = 0; i <= payload.Length; i++)
        {
            bool printable = i < payload.Length && IsPrintable(payload[i]);
            if (printable)
            {
                if (start < 0) start = i;
            }
            else if (start >= 0)
            {
                int len = i - start;
                if (len >= minLen)
                    list.Add(new ExtractedString(start, "1b",
                        Encoding.Latin1.GetString(payload.Slice(start, len))));
                start = -1;
            }
        }

        // Two-byte (UTF-16LE) printable runs: ASCII char followed by 0x00.
        int i2 = 0;
        while (i2 + 1 < payload.Length)
        {
            if (IsPrintable(payload[i2]) && payload[i2 + 1] == 0)
            {
                int s = i2;
                var sb = new StringBuilder();
                while (i2 + 1 < payload.Length && IsPrintable(payload[i2]) && payload[i2 + 1] == 0)
                {
                    sb.Append((char)payload[i2]);
                    i2 += 2;
                }
                if (sb.Length >= minLen)
                    list.Add(new ExtractedString(s, "2b", sb.ToString()));
            }
            else i2++;
        }

        return list;
    }

    private static bool IsPrintable(byte b) => b >= 0x20 && b < 0x7F || b == 0x09;
}
