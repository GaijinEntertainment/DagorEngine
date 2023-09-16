using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace Gaijin.Utilities
{
    public class GetCompileCmdOfSelectedFiles : Microsoft.Build.Utilities.Task
    {
        [Required]
        public string BuildCommand { get; set; }
        [Required]
        public string WorkDirectory { get; set; }
        [Required]
        public string[] SelectedFiles { get; set; }
        public string[] ExcludeArgs { get; set; }

        [Output]
        public ITaskItem[] Commands { get; set; }
        [Output]
        public ITaskItem[] SkippedFiles { get; set; }

        public sealed override bool Execute()
        {
            Process jamBuild = new Process();
            jamBuild.StartInfo.UseShellExecute = false;
            jamBuild.StartInfo.RedirectStandardOutput = true;
            jamBuild.StartInfo.FileName = "jam";
            jamBuild.StartInfo.Arguments = BuildCommand + " -a -n";
            jamBuild.StartInfo.WorkingDirectory = WorkDirectory;

            var selectedFiles = new List<string>();
            foreach (var selectedFile in SelectedFiles)
            {
                selectedFiles.Add(selectedFile.Replace('\\', '/'));
            }

            jamBuild.Start();
            string buildLog = jamBuild.StandardOutput.ReadToEnd();
            jamBuild.WaitForExit();

            var commands = new List<TaskItem>();

            using (StringReader reader = new StringReader(buildLog))
            {
                string line;
                while ((line = reader.ReadLine()) != null)
                {
                    bool executionLine = false;
                    int skipLeadingChar = 0;

                    if (line.StartsWith("  call_filtered "))
                    {
                        executionLine = true;
                        skipLeadingChar = 16;
                    }
                    else if (line.StartsWith("  call "))
                    {
                        executionLine = true;
                        skipLeadingChar = 7;
                    }

                    if (executionLine)
                    {
                        foreach (var selectedFile in selectedFiles)
                        {
                            bool rightLine = false;

                            if (line.EndsWith(selectedFile + ")\\#"))
                            {
                                rightLine = true;
                                line = line.Substring(0, line.Length - 3).Replace("#\\(", "");
                            }
                            else if (line.EndsWith(selectedFile))
                            {
                                rightLine = true;
                            }

                            if (rightLine)
                            {
                                selectedFiles.Remove(selectedFile);

                                int outputPosition = line.LastIndexOf("-Fo") + 3;
                                if (outputPosition == 2)
                                    outputPosition = line.LastIndexOf(" -o ") + 4;
                                if (outputPosition == 3)
                                    return false;
                                int lastSpace = line.LastIndexOf(" ");

                                var outDir = Path.GetDirectoryName(new Uri(WorkDirectory + line.Substring(outputPosition, lastSpace - outputPosition)).LocalPath);
                                if (!Directory.Exists(outDir))
                                {
                                    Directory.CreateDirectory(outDir);
                                }

                                var command = line.Substring(skipLeadingChar, line.Length - skipLeadingChar);

                                foreach (var excludeArg in ExcludeArgs)
                                {
                                    command = command.Replace(excludeArg, "");
                                }

                                var commandItem = new TaskItem(command);
                                commandItem.SetMetadata("File", selectedFile.Replace('/', '\\'));
                                commands.Add(commandItem);

                                break;
                            }
                        }

                        if (selectedFiles.Count == 0)
                        {
                            break;
                        }
                    }
                }
            }

            Commands = commands.ToArray();

            if (selectedFiles.Count > 0)
            {
                SkippedFiles = new TaskItem[selectedFiles.Count];
                for (int i = 0; i < selectedFiles.Count; i++)
                {
                    SkippedFiles[i] = new TaskItem(selectedFiles[i].Replace('/', '\\'));
                }
            }

            return true;
        }
    }
}
