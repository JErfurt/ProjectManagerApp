using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using MsBox.Avalonia;
using MsBox.Avalonia.Enums;

namespace ProjectManagerApp.Services
{
    public static class PlatformService
    {        
        public static bool IsWindows => RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
        public static bool IsLinux   => RuntimeInformation.IsOSPlatform(OSPlatform.Linux);
        public static bool IsMacOS   => RuntimeInformation.IsOSPlatform(OSPlatform.OSX);

        /// <summary>
        /// Открывает папку в файловом менеджере операционной системы
        /// </summary>
        /// <param name="folderPath">Путь к папке</param>
        public static async Task OpenFolderAsync(string folderPath)
        {
            if (string.IsNullOrEmpty(folderPath) || !Directory.Exists(folderPath))
            {
                var box = MessageBoxManager
                    .GetMessageBoxStandard("Ошибка", "Папка не найдена!",
                    ButtonEnum.Ok, Icon.Error);
                await box.ShowAsync();
                return;
            }

            try
            {
                if (IsWindows)
                {
                    // Windows - используем explorer
                    Process.Start(new ProcessStartInfo
                    {
                        FileName = "explorer",
                        Arguments = folderPath,
                        UseShellExecute = true
                    });
                }
                else if (IsMacOS)
                {
                    // macOS - используем open
                    Process.Start("open", folderPath);
                }
                else if (IsLinux)
                {
                    // Linux - пробуем разные файловые менеджеры
                    if (IsCommandAvailable("xdg-open"))
                    {
                        Process.Start("xdg-open", folderPath);
                    }
                    else if (IsCommandAvailable("nautilus"))
                    {
                        Process.Start("nautilus", folderPath);
                    }
                    else if (IsCommandAvailable("dolphin"))
                    {
                        Process.Start("dolphin", folderPath);
                    }
                    else
                    {
                        throw new NotSupportedException("Файловый менеджер не найден");
                    }
                }
            }
            catch (Exception ex)
            {
                var box = MessageBoxManager
                    .GetMessageBoxStandard("Ошибка", $"Не удалось открыть папку: {ex.Message}",
                    ButtonEnum.Ok, Icon.Error);
                await box.ShowAsync();
            }
        }

        /// <summary>
        /// Запускает скрипт в зависимости от операционной системы
        /// </summary>
        /// <param name="folderPath">Путь к папке проекта</param>
        public static async Task RunScriptAsync(string folderPath)
        {
            if (string.IsNullOrEmpty(folderPath) || !Directory.Exists(folderPath))
            {
                var box = MessageBoxManager.GetMessageBoxStandard(
                    "Ошибка", 
                    "Папка проекта не найдена!",
                    ButtonEnum.Ok, 
                    Icon.Error);
                await box.ShowAsync();
                return;
            }

            string scriptPath = string.Empty;
            
            try
            {
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                {
                    // Windows - ищем run.bat
                    scriptPath = Path.Combine(folderPath, "run.bat");
                    if (!File.Exists(scriptPath))
                    {
                        var box = MessageBoxManager.GetMessageBoxStandard(
                            "Ошибка", 
                            "Файл run.bat не найден!",
                            ButtonEnum.Ok, 
                            Icon.Error);
                        await box.ShowAsync();
                        return;
                    }

                    Process.Start(new ProcessStartInfo
                    {
                        FileName = scriptPath,
                        WorkingDirectory = folderPath,
                        UseShellExecute = true
                    });
                }
                else
                {
                    // macOS/Linux - ищем run.sh
                    scriptPath = Path.Combine(folderPath, "run.sh");
                    if (!File.Exists(scriptPath))
                    {
                        var box = MessageBoxManager.GetMessageBoxStandard(
                            "Ошибка", 
                            "Файл run.sh не найден!",
                            ButtonEnum.Ok, 
                            Icon.Error);
                        await box.ShowAsync();
                        return;
                    }

                    // Делаем скрипт исполняемым
                    Process.Start("chmod", $"+x {scriptPath}");
                    
                    // Запускаем скрипт
                    Process.Start(new ProcessStartInfo
                    {
                        FileName = "/bin/bash",
                        Arguments = scriptPath,
                        WorkingDirectory = folderPath,
                        UseShellExecute = false
                    });
                }
            }
            catch (Exception ex)
            {
                var box = MessageBoxManager.GetMessageBoxStandard(
                    "Ошибка запуска", 
                    $"Ошибка: {ex.Message}",
                    ButtonEnum.Ok, 
                    Icon.Error);
                await box.ShowAsync();
            }
        }

        /// <summary>
        /// Проверяет доступность команды в системе
        /// </summary>
        /// <param name="command">Команда для проверки</param>
        /// <returns>true если команда доступна</returns>
        private static bool IsCommandAvailable(string command)
        {
            try
            {
                var process = Process.Start(new ProcessStartInfo
                {
                    FileName = "which",
                    Arguments = command,
                    RedirectStandardOutput = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                });
                process?.WaitForExit();
                return process?.ExitCode == 0;
            }
            catch
            {
                return false;
            }
        }
    }
} 