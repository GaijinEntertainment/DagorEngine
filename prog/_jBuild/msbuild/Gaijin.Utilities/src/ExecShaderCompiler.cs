using Microsoft.Build.Framework;
using System.IO;
using System.Text.RegularExpressions;

namespace Gaijin.Utilities
{
    public class ExecShaderCompiler : Microsoft.Build.Tasks.Exec
    {
        readonly Regex regexrg;

        public ExecShaderCompiler() : base()
        {
            regexrg = new Regex("(\\[(ERROR|WARNING)\\]) ([\\S]*.sh)(\\(\\d*,\\d*\\)):(.*)");
        }

        protected override void LogEventsFromTextOutput(string singleLine, MessageImportance messageImportance)
        {
            var groups = regexrg.Split(singleLine);

            if (groups.Length == 7)
            {
                Log.LogMessageFromText(Path.GetFullPath(Path.Combine(GetWorkingDirectory(), groups[3])) + groups[4] + (groups[2].Equals("ERROR") ? ": error :" : ": warning :") + groups[5], MessageImportance.High);
            }
            else
            {
                Log.LogMessageFromText(singleLine, messageImportance);
            }
        }
    }
}
