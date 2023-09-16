using Microsoft.Build.Framework;
using System;
using System.Collections.Generic;

namespace Gaijin.Utilities
{
    public class CreateCppItems : Microsoft.Build.Utilities.Task
    {
        [Output]
        [Required]
        public ITaskItem[] CppSource { get; set; }
        public ITaskItem[] ConfigPerDirectory { get; set; }
        public ITaskItem GenericCppSource { get; set; }

        public sealed override bool Execute()
        {
            if (CppSource == null)
            {
                CppSource = Array.Empty<ITaskItem>();
                return true;
            }

            Array.Sort(CppSource, delegate (ITaskItem left, ITaskItem right)
            {
                return left.ToString().CompareTo(right.ToString());
            });

            Array.Sort(ConfigPerDirectory, delegate (ITaskItem left, ITaskItem right)
            {
                return left.ToString().CompareTo(right.ToString());
            });

            List<ITaskItem> newCppSource = new List<ITaskItem>();

            string configurationOptions = GenericCppSource.GetMetadata("ConfigurationOptions");
            GenericCppSource.RemoveMetadata("ConfigurationOptions");

            Span<ITaskItem> cppSource = CppSource.AsSpan();
            for (int i = 0; i < cppSource.Length; i++)
            {
                ref ITaskItem item = ref cppSource[i];
                string itemsDir = item.ToString();
                bool modified = false;

                string previousDir = "";
                Span<ITaskItem> configPerDirectory = ConfigPerDirectory.AsSpan();
                for (int j = 0; j < configPerDirectory.Length; j++)
                {
                    ref ITaskItem configPerDir = ref configPerDirectory[j];
                    string actualDir = configPerDir.ToString();
                    if (itemsDir.StartsWith(actualDir))
                    {
                        previousDir = actualDir;

                        if (!modified)
                        {
                            modified = true;
                            GenericCppSource.CopyMetadataTo(item);
                            item.RemoveMetadata("OriginalItemSpec");
                            newCppSource.Add(item);
                        }

                        {
                            string additionalIncludeDirectories = configPerDir.GetMetadata("AdditionalIncludeDirectories");
                            if (additionalIncludeDirectories.Length > 0)
                            {
                                string _additionalIncludeDirectories = item.GetMetadata("AdditionalIncludeDirectories");
                                item.SetMetadata("AdditionalIncludeDirectories",
                                    additionalIncludeDirectories.EndsWith("#")
                                    ? additionalIncludeDirectories.Replace("#", _additionalIncludeDirectories)
                                    : (_additionalIncludeDirectories.Length > 0
                                        ? string.Join(";", _additionalIncludeDirectories, additionalIncludeDirectories)
                                        : additionalIncludeDirectories)
                                    );
                            }
                        }

                        {
                            string forcedIncludeFiles = configPerDir.GetMetadata("ForcedIncludeFiles");
                            if (forcedIncludeFiles.Length > 0)
                            {
                                string _forcedIncludeFiles = item.GetMetadata("ForcedIncludeFiles");
                                item.SetMetadata("ForcedIncludeFiles",
                                    forcedIncludeFiles.EndsWith("#")
                                    ? forcedIncludeFiles.Replace("#", _forcedIncludeFiles)
                                    : (_forcedIncludeFiles.Length > 0
                                        ? string.Join(";", _forcedIncludeFiles, forcedIncludeFiles)
                                        : forcedIncludeFiles)
                                    );
                            }
                        }

                        {
                            string preprocessorDefinitions = configPerDir.GetMetadata("PreprocessorDefinitions");
                            if (preprocessorDefinitions.Length > 0)
                            {
                                string _preprocessorDefinitions = item.GetMetadata("PreprocessorDefinitions");
                                item.SetMetadata("PreprocessorDefinitions", _preprocessorDefinitions.Length > 0
                                    ? string.Join(";", _preprocessorDefinitions, preprocessorDefinitions)
                                    : preprocessorDefinitions);
                            }
                        }

                        {
                            string undefinePreprocessorDefinitions = configPerDir.GetMetadata("UndefinePreprocessorDefinitions");
                            if (undefinePreprocessorDefinitions.Length > 0)
                            {
                                string _undefinePreprocessorDefinitions = item.GetMetadata("UndefinePreprocessorDefinitions");
                                item.SetMetadata("UndefinePreprocessorDefinitions", _undefinePreprocessorDefinitions.Length > 0
                                    ? string.Join(";", _undefinePreprocessorDefinitions, undefinePreprocessorDefinitions)
                                    : undefinePreprocessorDefinitions);
                            }
                        }
                    }
                    else if (actualDir.Contains(previousDir))
                        previousDir = "";
                    else
                        break;
                }
            }

            GenericCppSource.SetMetadata("ConfigurationOptions", configurationOptions);
            CppSource = newCppSource.Count != 0 ? newCppSource.ToArray() : Array.Empty<ITaskItem>();

            return true;
        }
    }
}
