using System.Text.RegularExpressions;

namespace JscTool;

// Stages ⑧/⑨ (string-level): classify extracted constants into the categories
// that matter for this kind of program — HTTP routes, license symbols, module
// requires, and domain (weather) API symbols — so the report auto-locates the
// business and license logic without full decompilation.
public static class Detectors
{
    public sealed record Group(string Title, List<string> Items);

    private static readonly Regex Route = new(@"^/[A-Za-z0-9._~\-/]*$", RegexOptions.Compiled);
    private static readonly Regex Url = new(@"^(https?|wss?)://", RegexOptions.Compiled);
    private static readonly Regex Ident = new(@"^[A-Za-z_$][A-Za-z0-9_$]{2,}$", RegexOptions.Compiled);

    public static List<Group> Classify(IEnumerable<string> strings)
    {
        var uniq = strings.Distinct().ToList();

        List<string> Pick(Func<string, bool> f) =>
            uniq.Where(f).Distinct().OrderBy(s => s, StringComparer.Ordinal).ToList();

        var routes = Pick(s => Route.IsMatch(s) && s.Length > 1 && s.Contains('/') &&
                               !s.Contains(' ') && s.Length <= 120);
        var urls = Pick(s => Url.IsMatch(s));
        var license = Pick(s => s.Contains("LICENSE", StringComparison.OrdinalIgnoreCase) ||
                                s.Contains("license", StringComparison.Ordinal) ||
                                s.Contains("MACHINE_ID", StringComparison.OrdinalIgnoreCase) ||
                                s.StartsWith("x-stratawx", StringComparison.OrdinalIgnoreCase) ||
                                s.Contains("seat", StringComparison.OrdinalIgnoreCase) ||
                                s.Contains("activation", StringComparison.OrdinalIgnoreCase));
        var weather = Pick(s => Ident.IsMatch(s) && (
                                s.StartsWith("weather", StringComparison.OrdinalIgnoreCase) ||
                                s.StartsWith("metar", StringComparison.OrdinalIgnoreCase) ||
                                s.StartsWith("taf", StringComparison.OrdinalIgnoreCase) ||
                                s.Contains("SimConnect", StringComparison.OrdinalIgnoreCase) ||
                                s.Contains("wind", StringComparison.OrdinalIgnoreCase) ||
                                s.Contains("sigmet", StringComparison.OrdinalIgnoreCase) ||
                                s.Contains("pirep", StringComparison.OrdinalIgnoreCase) ||
                                s.Contains("cloud", StringComparison.OrdinalIgnoreCase) ||
                                s.Contains("simbrief", StringComparison.OrdinalIgnoreCase)));
        var envVars = Pick(s => Regex.IsMatch(s, @"^[A-Z][A-Z0-9]+(_[A-Z0-9]+)+$"));

        return new List<Group>
        {
            new("HTTP routes / paths", routes),
            new("URLs / hosts", urls),
            new("License / seat symbols", license),
            new("Weather / SimConnect symbols", weather),
            new("ENV / constant-style identifiers", envVars),
        };
    }
}
