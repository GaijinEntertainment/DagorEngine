using Microsoft.Build.Framework;
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Management;
using System.Text.RegularExpressions;
using System.Threading;

namespace Gaijin.Utilities
{
    public class Exec : Microsoft.Build.Tasks.Exec
    {
        readonly Regex regexrg;
        readonly Regex regexUnit;
        readonly Regex regexProgress;

        private int targetCount = 0;
        private int targetIndex = 0;

        public Exec() : base()
        {
            regexrg = new Regex("(^\\.\\.?\\/[^\\n\" ?: *<>|]+\\.[A-z0 - 9]+)(.*)");
            regexUnit = new Regex(@"\.(?:cpp|c|cc|inl|lib|asm|masm|das|rc|exe)$");
            regexProgress = new Regex(@"^\.{3}on \d+th target\.{3}$");
        }

        protected override void LogEventsFromTextOutput(string singleLine, MessageImportance messageImportance)
        {
            if (singleLine.StartsWith("...") || singleLine.StartsWith("\f..."))
            {
                if (singleLine.StartsWith("...patience..."))
                    return;
                if (regexProgress.IsMatch(singleLine))
                    return;

                if (singleLine.StartsWith("...updating") && singleLine.EndsWith("target(s)..."))
                {
                    var count = singleLine.Substring(12);
                    var space = count.IndexOf(' ');
                    int.TryParse(count.Substring(0, space), out int tmpTargetCount);
                    targetCount += tmpTargetCount;
                }

                Log.LogMessageFromText(string.Join(" ", "JAM:", singleLine.Substring(3, singleLine.Length - 6)), messageImportance);
                return;
            }
            else if (singleLine.StartsWith("../"))
            {
                var groups = regexrg.Split(singleLine);
                if (groups.Length == 4)
                {
                    var fullPath = Path.GetFullPath(Path.Combine(GetWorkingDirectory(), groups[1]));
                    Log.LogMessageFromText(string.Join("", fullPath, groups[2]), messageImportance);
                    return;
                }
            }
            else if (singleLine.StartsWith(@"..\"))
            {
                int parentIndex = singleLine.IndexOf('(') - 1;
                if (parentIndex > 0)
                {
                    var fullPath = Path.GetFullPath(Path.Combine(GetWorkingDirectory(), singleLine.Substring(0, parentIndex)));
                    Log.LogMessageFromText(string.Join("", fullPath, singleLine.Substring(parentIndex)), messageImportance);
                    return;
                }
            }

            if (regexUnit.IsMatch(singleLine) && !singleLine.Equals("buildStamp.c"))
                singleLine = string.Join("", "[", ++targetIndex, "/", targetCount, "] ", singleLine);
            else if (singleLine.Equals("."))
                singleLine = "";
            else if (singleLine.StartsWith(@"* copy file to:"))
                ++targetIndex;

            Log.LogMessageFromText(singleLine, messageImportance);
        }

        public static void StopProgram(uint pid)
        {
            // It's impossible to be attached to 2 consoles at the same time,
            // so release the current one.
            NativeMethods.FreeConsole();

            // This does not require the console window to be visible.
            if (NativeMethods.AttachConsole(pid))
            {
                // Disable Ctrl-C handling for our program
                NativeMethods.SetConsoleCtrlHandler(null, true);
                NativeMethods.GenerateConsoleCtrlEvent(NativeMethods.CtrlTypes.CTRL_C_EVENT, 0);

                // Must wait here. If we don't and re-enable Ctrl-C
                // handling below too fast, we might terminate ourselves.
                Thread.Sleep(2000);

                NativeMethods.FreeConsole();

                // Re-enable Ctrl-C handling or any subsequently started
                // programs will inherit the disabled state.
                NativeMethods.SetConsoleCtrlHandler(null, false);
            }
        }

        public override void Cancel()
        {
            ManagementObjectSearcher mos = new ManagementObjectSearcher(
                string.Format("Select * From Win32_Process Where ParentProcessID={0}", Process.GetCurrentProcess().Id));

            foreach (ManagementObject mo in mos.Get().Cast<ManagementObject>())
            {
                var name = Convert.ToString(mo["Name"]);
                if (name == "cmd.exe")
                {
                    int childProcessID = Convert.ToInt32(mo["ProcessID"]);
                    var childProcess = Process.GetProcessById(childProcessID);
                    StopProgram((uint)childProcessID);
                    childProcess.WaitForExit();

                    break;
                }
            }

            base.Cancel();
        }
    }
}
