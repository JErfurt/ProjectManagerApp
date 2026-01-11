using System;
using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;

namespace ProjectManagerApp.Services
{
    public static class CodeEditorService
    {
        static string? FindVSCodeWindows()
        {
            string[] paths =
            {
                @"%LOCALAPPDATA%\Programs\Microsoft VS Code\Code.exe",
                @"%ProgramFiles%\Microsoft VS Code\Code.exe",
                @"%ProgramFiles(x86)%\Microsoft VS Code\Code.exe"
            };

            foreach (var path in paths)
            {
                var full = Environment.ExpandEnvironmentVariables(path);
                if (File.Exists(full))
                    return full;
            }

            return null;
        }

        static string? FindVSCodeLinux()
        {
            string[] candidates = { "code", "code-insiders", "codium" };

            foreach (var cmd in candidates)
            {
                try
                {
                    var psi = new ProcessStartInfo
                    {
                        FileName = "which",
                        Arguments = cmd,
                        RedirectStandardOutput = true,
                        UseShellExecute = false,
                        CreateNoWindow = true
                    };

                    using var p = Process.Start(psi);
                    var result = p?.StandardOutput.ReadLine();
                    if (!string.IsNullOrWhiteSpace(result))
                        return result;
                }
                catch { }
            }

            return null;
        }

        public static async Task OpenFolderInCodeEditor(string folderPath)
        {
            try
            {
                if (PlatformService.IsWindows)
                {
                    var vscode = FindVSCodeWindows();
                    if (vscode != null)
                    {
                        Process.Start(new ProcessStartInfo
                        {
                            FileName = vscode,
                            Arguments = $"\"{folderPath}\"",
                            UseShellExecute = true
                        });
                        return;
                    }
                }

                if (PlatformService.IsMacOS)
                {
                    Process.Start(new ProcessStartInfo
                    {
                        FileName = "open",
                        Arguments = $"-a \"Visual Studio Code\" \"{folderPath}\"",
                        UseShellExecute = true
                    });
                    return;
                }

                if (PlatformService.IsLinux)
                {
                    var vscode = FindVSCodeLinux();
                    if (vscode != null)
                    {
                        Process.Start(new ProcessStartInfo
                        {
                            FileName = vscode,
                            Arguments = $"\"{folderPath}\"",
                            UseShellExecute = true
                        });
                        return;
                    }
                }

                // 🔁 Fallback — открыть папку системно
                Process.Start(new ProcessStartInfo
                {
                    FileName = folderPath,
                    UseShellExecute = true
                });
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
            }
        }
    }
}
