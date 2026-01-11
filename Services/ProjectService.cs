using System.Collections.Generic;
using System.IO;
using Microsoft.Data.Sqlite;
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

        // Путь к файлу базы данных SQLite
        private static readonly string DbPath = Path.Combine(AppDataFolder, "projects.db");
        private static readonly string ConnectionString = $"Data Source={DbPath}";

        // Инициализация базы данных
        private static void InitializeDatabase()
        {
            // Если папка не существует, создаем её
            if (!Directory.Exists(AppDataFolder))
            {
                Directory.CreateDirectory(AppDataFolder);
            }

            // Создаем таблицу, если она не существует
            using var connection = new SqliteConnection(ConnectionString);
            connection.Open();

            const string createTableSql = @"
                CREATE TABLE IF NOT EXISTS Projects (
                    Id INTEGER PRIMARY KEY AUTOINCREMENT,
                    Name TEXT NOT NULL,
                    Description TEXT,
                    Status TEXT,
                    FolderPath TEXT,
                    Language TEXT,
                    LastInteraction TEXT
                )";

            using var command = new SqliteCommand(createTableSql, connection);
            command.ExecuteNonQuery();
        }

        // Метод для загрузки проектов
        public static List<Project> LoadProjects()
        {
            InitializeDatabase();

            var projects = new List<Project>();

            using var connection = new SqliteConnection(ConnectionString);
            connection.Open();

            const string selectSql = "SELECT Id, Name, Description, Status, FolderPath, Language, LastInteraction FROM Projects";
            using var command = new SqliteCommand(selectSql, connection);
            using var reader = command.ExecuteReader();

            while (reader.Read())
            {
                DateTimeOffset? lastInteraction = null;
                if (!reader.IsDBNull(6))
                {
                    var dateStr = reader.GetString(6);
                    if (DateTimeOffset.TryParse(dateStr, out var parsed))
                    {
                        lastInteraction = parsed;
                    }
                }

                projects.Add(new Project
                {
                    Id = reader.GetInt32(0),
                    Name = reader.GetString(1),
                    Description = reader.IsDBNull(2) ? string.Empty : reader.GetString(2),
                    Status = reader.IsDBNull(3) ? string.Empty : reader.GetString(3),
                    FolderPath = reader.IsDBNull(4) ? string.Empty : reader.GetString(4),
                    Language = reader.IsDBNull(5) ? string.Empty : reader.GetString(5),
                    LastInteraction = lastInteraction
                });
            }

            return projects;
        }

        // Метод для сохранения проектов (полная перезапись)
        public static void SaveProjects(List<Project> projects)
        {
            InitializeDatabase();

            using var connection = new SqliteConnection(ConnectionString);
            connection.Open();

            using var transaction = connection.BeginTransaction();

            try
            {
                // Очищаем таблицу и сбрасываем счётчик autoincrement
                using (var deleteCommand = new SqliteCommand("DELETE FROM Projects", connection, transaction))
                {
                    deleteCommand.ExecuteNonQuery();
                }

                // Вставляем все проекты без указания Id (AUTOINCREMENT сгенерирует новые)
                const string insertSql = @"
                    INSERT INTO Projects (Name, Description, Status, FolderPath, Language, LastInteraction) 
                    VALUES ($Name, $Description, $Status, $FolderPath, $Language, $LastInteraction);
                    SELECT last_insert_rowid();";

                foreach (var project in projects)
                {
                    using var insertCommand = new SqliteCommand(insertSql, connection, transaction);
                    insertCommand.Parameters.AddWithValue("$Name", project.Name);
                    insertCommand.Parameters.AddWithValue("$Description", project.Description);
                    insertCommand.Parameters.AddWithValue("$Status", project.Status);
                    insertCommand.Parameters.AddWithValue("$FolderPath", project.FolderPath);
                    insertCommand.Parameters.AddWithValue("$Language", project.Language);
                    insertCommand.Parameters.AddWithValue("$LastInteraction", 
                        project.LastInteraction.HasValue ? project.LastInteraction.Value.ToString("o") : DBNull.Value);
                    
                    // Обновляем Id проекта сгенерированным значением
                    project.Id = Convert.ToInt32(insertCommand.ExecuteScalar());
                }

                transaction.Commit();
            }
            catch
            {
                transaction.Rollback();
                throw;
            }
        }

        // Метод для добавления одного проекта
        public static int AddProject(Project project)
        {
            InitializeDatabase();

            using var connection = new SqliteConnection(ConnectionString);
            connection.Open();

            const string insertSql = @"
                INSERT INTO Projects (Name, Description, Status, FolderPath, Language, LastInteraction) 
                VALUES ($Name, $Description, $Status, $FolderPath, $Language, $LastInteraction);
                SELECT last_insert_rowid();";

            using var command = new SqliteCommand(insertSql, connection);
            command.Parameters.AddWithValue("$Name", project.Name);
            command.Parameters.AddWithValue("$Description", project.Description);
            command.Parameters.AddWithValue("$Status", project.Status);
            command.Parameters.AddWithValue("$FolderPath", project.FolderPath);
            command.Parameters.AddWithValue("$Language", project.Language);
            command.Parameters.AddWithValue("$LastInteraction", 
                project.LastInteraction.HasValue ? project.LastInteraction.Value.ToString("o") : DBNull.Value);

            return Convert.ToInt32(command.ExecuteScalar());
        }

        // Метод для обновления проекта
        public static void UpdateProject(Project project)
        {
            InitializeDatabase();

            using var connection = new SqliteConnection(ConnectionString);
            connection.Open();

            const string updateSql = @"
                UPDATE Projects 
                SET Name = $Name, Description = $Description, Status = $Status, FolderPath = $FolderPath, Language = $Language, LastInteraction = $LastInteraction 
                WHERE Id = $Id";

            using var command = new SqliteCommand(updateSql, connection);
            command.Parameters.AddWithValue("$Id", project.Id);
            command.Parameters.AddWithValue("$Name", project.Name);
            command.Parameters.AddWithValue("$Description", project.Description);
            command.Parameters.AddWithValue("$Status", project.Status);
            command.Parameters.AddWithValue("$FolderPath", project.FolderPath);
            command.Parameters.AddWithValue("$Language", project.Language);
            command.Parameters.AddWithValue("$LastInteraction", 
                project.LastInteraction.HasValue ? project.LastInteraction.Value.ToString("o") : DBNull.Value);
            command.ExecuteNonQuery();
        }

        // Метод для удаления проекта
        public static void DeleteProject(int id)
        {
            InitializeDatabase();

            using var connection = new SqliteConnection(ConnectionString);
            connection.Open();

            const string deleteSql = "DELETE FROM Projects WHERE Id = $Id";
            using var command = new SqliteCommand(deleteSql, connection);
            command.Parameters.AddWithValue("$Id", id);
            command.ExecuteNonQuery();
        }
    }
}
