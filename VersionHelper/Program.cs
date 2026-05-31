using System.Text;

namespace VersionHelper;

public class Program
{
    static int Main(string[] args)
    {
        if (args.Length != 3)
        {
            Console.Error.WriteLine("Usage: VersionHelper <filename> <description> <path>");
            return 1;
        }

        var template = new VersionInfo
        {
            Filename = args[0],
            Description = args[1]
        };
        var fullPath = Path.GetFullPath(args[2]);
        _ = Directory.CreateDirectory(Directory.GetParent(fullPath)!.FullName);
        File.WriteAllText(fullPath, template.TransformText(), Encoding.UTF8);
        return 0;
    }
}
