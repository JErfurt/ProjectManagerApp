using System;
using System.Diagnostics;
using System.Threading.Tasks;

namespace ProjectManagerApp.Services
{
    public static class CodeEditorService
    {
        public static async Task OpenFolderInCodeEditor(string folderPath)
        {
            // Добавление кавычек вокруг пути
            string quotedPath = $"\"{folderPath}\"";

            // Создание нового процесса для открытия VSCode
            ProcessStartInfo startInfo = new ProcessStartInfo
            {
                FileName = "code",
                Arguments = quotedPath,
                UseShellExecute = true // Открывает в стандартном приложении
            };

            try
            {
                Process.Start(startInfo);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Ошибка: {ex.Message}");
            }
        }
    }
}
