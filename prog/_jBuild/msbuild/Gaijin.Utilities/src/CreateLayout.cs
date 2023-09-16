using Microsoft.Build.Framework;
using System.IO;
using System.Runtime.InteropServices;

namespace Gaijin.Utilities
{
    public class CreateLayout : Microsoft.Build.Utilities.Task
    {
        [Required]
        public ITaskItem[] SourceFiles { get; set; }
        [Required]
        public string LooseImageDir { get; set; }

        public sealed override bool Execute()
        {
            string lastLinkDir = @"";
            for (int i = 0; i < SourceFiles.Length; i++)
            {
                ref var sourceFile = ref SourceFiles[i];
                if (!File.Exists(sourceFile.ItemSpec))
                {
                    Log.LogError(string.Format("Creating Loose Image Dir failed, the '{0}' is missing!", sourceFile.ItemSpec));
                    return false;
                }

                var linkDir = sourceFile.GetMetadata("LinkDir");
                if (lastLinkDir != linkDir)
                {
                    lastLinkDir = linkDir;
                    linkDir = Path.Combine(LooseImageDir, linkDir);
                    if (!Directory.Exists(linkDir))
                        Directory.CreateDirectory(linkDir);
                }

                var linkName = Path.Combine(LooseImageDir, sourceFile.GetMetadata("LinkName"));
                if (!File.Exists(linkName))
                {
                    if (!NativeMethods.CreateSymbolicLink(linkName, sourceFile.ItemSpec, NativeMethods.SymbolicLink.AllowUnprivilegedCreate))
                    {
                        Log.LogError(string.Format("Creating symbolic link for {0} to {1} is failed. Error code: {2}", sourceFile.ItemSpec, linkName, Marshal.GetLastWin32Error()));
                        return false;
                    }
                }
            }

            return true;
        }
    }
}
