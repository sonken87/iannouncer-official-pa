namespace JscTool;

// Stage ②: compute V8's version-string hash and scan a version range to find
// the one whose hash matches the header's version_hash.
//
// V8 (post-2026-06 "rapidhash" integer mixing, e.g. 15.2) computes the version
// hash as base::Hasher over {major, minor, build, patch} (each an int), then a
// static_cast<uint32_t>. Older V8 used Thomas Wang integer hashing. Some builds
// also fold the embedder string into the hash. We implement both integer
// mixers and optional embedder folding so the scanner can match either era.
public static class V8VersionHash
{
    private const ulong M = 0xC6A4A7935BD1E995UL;              // MurmurHash2 64A mixer (hash_combine)
    private const ulong S1 = 0x2D358DCCAA6C78A5UL;             // rapidhash default secret[0]
    private const ulong S2 = 0x8BB84B93962EACC9UL;             // rapidhash default secret[1]

    public enum IntMixer { Rapidhash, Wang }

    // hash_combine(seed, hash) — MurmurHash2 64A finalizer, V8 64-bit branch.
    private static ulong Combine(ulong seed, ulong h)
    {
        h *= M; h ^= h >> 47; h *= M;
        seed ^= h; seed *= M;
        return seed;
    }

    // hash32 for a 32-bit key: V8 hash64(uint32) via rapidhash "mum" mixer,
    // truncated to 32 bits.
    private static uint HashRapid(uint key)
    {
        ulong a = key ^ S1, b = key ^ S2;
        System.UInt128 prod = (System.UInt128)a * b;
        ulong lo = (ulong)prod, hi = (ulong)(prod >> 64);
        return (uint)(lo ^ hi);
    }

    // Thomas Wang 32-bit integer hash (pre-rapidhash V8).
    private static uint HashWang(uint h)
    {
        unchecked
        {
            h = ~h + (h << 15);
            h ^= h >> 12;
            h += h << 2;
            h ^= h >> 4;
            h *= 2057;
            h ^= h >> 16;
            return h;
        }
    }

    public static uint Compute(int major, int minor, int build, int patch,
                               IntMixer mixer, string embedder = "")
    {
        Func<uint, uint> h32 = mixer == IntMixer.Rapidhash ? HashRapid : HashWang;
        ulong seed = 0;
        seed = Combine(seed, h32((uint)major));
        seed = Combine(seed, h32((uint)minor));
        seed = Combine(seed, h32((uint)build));
        seed = Combine(seed, h32((uint)patch));
        foreach (char c in embedder)               // AddRange(OneByteVector(embedder))
            seed = Combine(seed, (byte)c);          // hash_value(unsigned char) is identity
        return (uint)seed;
    }

    public record Match(int Major, int Minor, int Build, int Patch, IntMixer Mixer, string Embedder)
    {
        public override string ToString() =>
            $"{Major}.{Minor}.{Build}.{Patch}  (mixer={Mixer}, embedder=\"{Embedder}\")";
    }

    // Scan a version cube against a target hash. Embedder candidates cover plain
    // builds and common Node/Electron suffixes.
    public static List<Match> Scan(uint target,
                                   (int lo, int hi) major,
                                   (int lo, int hi) minor,
                                   (int lo, int hi) build,
                                   (int lo, int hi) patch,
                                   IEnumerable<string>? embedders = null)
    {
        var embs = (embedders ?? new[] { "", "-electron.0", "-electron", "-node.0" }).ToArray();
        var results = new List<Match>();
        foreach (var mixer in new[] { IntMixer.Rapidhash, IntMixer.Wang })
        foreach (var emb in embs)
            for (int ma = major.lo; ma <= major.hi; ma++)
            for (int mi = minor.lo; mi <= minor.hi; mi++)
            for (int b = build.lo; b <= build.hi; b++)
            for (int p = patch.lo; p <= patch.hi; p++)
                if (Compute(ma, mi, b, p, mixer, emb) == target)
                    results.Add(new Match(ma, mi, b, p, mixer, emb));
        return results;
    }
}
