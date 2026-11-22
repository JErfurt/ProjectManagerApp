using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Layout;
using Avalonia.Media;

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
                await ShowErrorAsync("Ошибка", "Папка не найдена!");
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
                await ShowErrorAsync("Ошибка", $"Не удалось открыть папку: {ex.Message}");
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
                await ShowErrorAsync("Ошибка", "Папка проекта не найдена!");
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
                        await ShowErrorAsync("Ошибка", "Файл run.bat не найден!");
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
                        await ShowErrorAsync("Ошибка", "Файл run.sh не найден!");
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
                await ShowErrorAsync("Ошибка запуска", $"Ошибка: {ex.Message}");
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

        private static Task ShowErrorAsync(string title, string message)
        {
            var owner = (Application.Current?.ApplicationLifetime as IClassicDesktopStyleApplicationLifetime)?.MainWindow;
            var close = new Button
            {
                Content = "ОК",
                IsDefault = true,
                HorizontalAlignment = HorizontalAlignment.Right,
                MinWidth = 80
            };
            var dialog = new Window
            {
                Title = title,
                Width = 420,
                SizeToContent = SizeToContent.Height,
                MinHeight = 140,
                CanResize = false,
                WindowStartupLocation = owner is null
                    ? WindowStartupLocation.CenterScreen
                    : WindowStartupLocation.CenterOwner,
                Content = new StackPanel
                {
                    Margin = new Thickness(20),
                    Spacing = 16,
                    Children =
                    {
                        new TextBlock
                        {
                            Text = message,
                            TextWrapping = TextWrapping.Wrap
                        },
                        close
                    }
                }
            };
            close.Click += (_, _) => dialog.Close();

            if (owner is null)
            {
                dialog.Show();
                return Task.CompletedTask;
            }

            return dialog.ShowDialog(owner);
        }
    }
} 