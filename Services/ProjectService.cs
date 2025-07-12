using System.Collections.Generic;
using System.IO;
using Newtonsoft.Json;
using ProjectManagerApp.Models;
using System;

namespace ProjectManagerApp.Services
{
    public static class ProjectService
    {
        // Путь к папке приложения в AppData
        public static readonly string AppDataFolder = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            "jerfurt",
            "ProjectManagerApp"
        );

        // Путь к файлу списка проектов
        private static readonly string FilePath = Path.Combine(AppDataFolder, "projects.json");

        // Метод для загрузки проектов
        public static List<Project> LoadProjects()
        {
            // Если папка не существует, создаем её
            if (!Directory.Exists(AppDataFolder))
            {
                Directory.CreateDirectory(AppDataFolder);
            }

            // Если файл не существует, создаем пустой файл
            if (!File.Exists(FilePath))
            {
                File.WriteAllText(FilePath, "[]"); // Пустой JSON массив
            }

            // Читаем файл и десериализуем проекты
            var json = File.ReadAllText(FilePath);
            return JsonConvert.DeserializeObject<List<Project>>(json) ?? new List<Project>();
        }

        // Метод для сохранения проектов
        public static void SaveProjects(List<Project> projects)
        {
            // Сериализуем список проектов и записываем в файл
            var json = JsonConvert.SerializeObject(projects, Formatting.Indented);
            File.WriteAllText(FilePath, json);
        }
    }
}
