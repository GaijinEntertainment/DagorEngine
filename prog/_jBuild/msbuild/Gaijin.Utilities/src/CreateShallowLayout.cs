using Microsoft.Build.Framework;
using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace Gaijin.Utilities
{
    public class CreateShallowLayout : Microsoft.Build.Utilities.Task
    {
        [Required]
        public string[] SourceDirs { get; set; }
        [Required]
        public string LooseImageDir { get; set; }
        public string LastBuildStateFile { get; set; }

        private static DateTime GetLastWriteTimeUtc(FileSystemInfo fsInfo)
        {
            if ((fsInfo.Attributes & FileAttributes.ReparsePoint) != 0)
            {
                var file = NativeMethods.CreateFile(fsInfo.FullName, NativeMethods.FILE_READ_EA, FileShare.ReadWrite | FileShare.Delete, IntPtr.Zero, FileMode.Open, NativeMethods.FILE_FLAG_BACKUP_SEMANTICS, IntPtr.Zero);
                if (file == NativeMethods.INVALID_HANDLE_VALUE)
                    throw new Win32Exception();

                try
                {
                    var sb = new StringBuilder(1024);
                    var res = NativeMethods.GetFinalPathNameByHandle(file, sb, 1024, 0);
                    if (res == 0)
                        throw new Win32Exception();

                    return Directory.GetLastWriteTimeUtc(sb.ToString());
                }
                finally
                {
                    NativeMethods.CloseHandle(file);
                }
            }

            return fsInfo.LastWriteTimeUtc;
        }

        private bool CheckUptoDate()
        {
            var looseImageInfo = new DirectoryInfo(LooseImageDir);
            if (!looseImageInfo.Exists)
                return false;

            var looseImageDirTime = GetLastWriteTimeUtc(looseImageInfo);

            var lastBuildStateInfo = new FileInfo(LastBuildStateFile);
            if (lastBuildStateInfo.Exists && GetLastWriteTimeUtc(lastBuildStateInfo) < looseImageDirTime)
                return false;

            for (int i = 0; i < SourceDirs.Length; i++)
            {
                var sourceDirInfo = new DirectoryInfo(SourceDirs[i]);
                if (!sourceDirInfo.Exists || looseImageDirTime < GetLastWriteTimeUtc(sourceDirInfo))
                    return false;
            }

            return true;
        }

        public sealed override bool Execute()
        {
            if (CheckUptoDate())
                return true;

            if (Directory.Exists(LooseImageDir))
                Directory.Delete(LooseImageDir, true);

            Directory.CreateDirectory(LooseImageDir);

            for (int i = 0; i < SourceDirs.Length; i++)
            {
                ref var sourceDir = ref SourceDirs[i];
                if (!Directory.Exists(sourceDir))
                    continue;

                var directories = Directory.GetDirectories(sourceDir, "*", SearchOption.TopDirectoryOnly);
                for (int j = 0; j < directories.Length; j++)
                {
                    ref var dir = ref directories[j];
                    var targetDir = Path.Combine(LooseImageDir, Path.GetFileName(dir));
                    if (Directory.Exists(targetDir))
                        continue;

                    if (NativeMethods.CreateSymbolicLink(targetDir, dir, NativeMethods.SymbolicLink.Directory | NativeMethods.SymbolicLink.AllowUnprivilegedCreate))
                        continue;

                    Log.LogError(string.Format("Creating symbolic link for {0} to {1} is failed. Error code: {2}", targetDir, dir, Marshal.GetLastWin32Error()));
                    return false;
                }

                var files = Directory.GetFiles(sourceDir, "*", SearchOption.TopDirectoryOnly);
                for (int j = 0; j < files.Length; j++)
                {
                    ref var file = ref files[j];
                    var targetFile = Path.Combine(LooseImageDir, Path.GetFileName(file));
                    if (File.Exists(targetFile))
                        continue;

                    if (NativeMethods.CreateSymbolicLink(targetFile, file, NativeMethods.SymbolicLink.File | NativeMethods.SymbolicLink.AllowUnprivilegedCreate))
                        continue;

                    Log.LogError(string.Format("Creating symbolic link for {0} to {1} is failed. Error code: {2}", targetFile, file, Marshal.GetLastWin32Error()));
                    return false;
                }
            }

            return true;
        }
    }
}
