using Microsoft.Build.Framework;

namespace Gaijin.Utilities
{
    public class RemoveDir : Microsoft.Build.Utilities.Task
    {
        [Required]
        public string Directory { get; set; }

        public sealed override bool Execute()
        {
            if (System.IO.Directory.Exists(Directory))
            {
                System.IO.Directory.Delete(Directory, true);
            }
            return !System.IO.Directory.Exists(Directory);
        }
    }
}
