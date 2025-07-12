using Avalonia.Controls;
using Avalonia.Interactivity;
using MsBox.Avalonia;
using MsBox.Avalonia.Enums;
using ProjectManagerApp.Models;
using ProjectManagerApp.Services;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Windows.Input;

namespace ProjectManagerApp
{
    public partial class MainWindow : Window
    {
        public ObservableCollection<Project> Projects { get; set; }
        public Project? SelectedProject { get; set; } // Добавлено свойство SelectedProject
        public static ICommand? OpenFolderCommand { get; private set; }
        public ICommand RemoveProjectCommand { get; private set; }
        public static ICommand? RunBatchCommand { get; private set; }

        public MainWindow()
        {
            InitializeComponent();
            // Загрузка проектов
            Projects = new ObservableCollection<Project>(ProjectService.LoadProjects());
            DataContext = this;

            // Команды
            OpenFolderCommand = new RelayCommand<string>(OpenFolderAsync);
            RemoveProjectCommand = new RelayCommand<Project>(RemoveProject);
            RunBatchCommand = new RelayCommand<string>(RunBatchAsync);
        }

        #region Public Methods
        // Метод для добавления нового проекта в список
        public void AddProject(object sender, RoutedEventArgs args)
        {
            Projects.Add(new Project
            {
                Name = "Новый проект",
                Description = "Описание",
                Status = "Новый",
                FolderPath = string.Empty
            });
        }

        // Метод сохранения
        public void SaveProjects(object sender, RoutedEventArgs args)
        {
            ProjectService.SaveProjects(new List<Project>(Projects));
            var box = MessageBoxManager
                .GetMessageBoxStandard("Успех", "Проекты успешно сохранены!",
                ButtonEnum.Ok, MsBox.Avalonia.Enums.Icon.Success);
            box.ShowAsync();
        }

        // Метод открытия папки конфигурации
        public void OpenConfigFolder(object sender, RoutedEventArgs args)
        {
            var folderPath = ProjectService.AppDataFolder;
            if (Directory.Exists(folderPath))
            {
                Process.Start(new ProcessStartInfo
                {
                    FileName = folderPath,
                    UseShellExecute = true
                });
            }
            else
            {
                var box = MessageBoxManager
                    .GetMessageBoxStandard("Ошибка", "Папка конфигурации не найдена!",
                    ButtonEnum.Ok, MsBox.Avalonia.Enums.Icon.Error);
                box.ShowAsync();
            }
        }
        #endregion

        #region Private Methods
        // Новый метод для запуска bat-файла
        private async void RunBatchAsync(string folderPath)
        {
            if (string.IsNullOrEmpty(folderPath) || !Directory.Exists(folderPath))
            {
                var box = MessageBoxManager.GetMessageBoxStandard(
                    "Ошибка", 
                    "Папка проекта не найдена!",
                    ButtonEnum.Ok, 
                    MsBox.Avalonia.Enums.Icon.Error);
                await box.ShowAsync();
                return;
            }

            var batPath = Path.Combine(folderPath, "run.bat");
            if (!File.Exists(batPath))
            {
                var box = MessageBoxManager.GetMessageBoxStandard(
                    "Ошибка", 
                    "Файл run.bat не найден!",
                    ButtonEnum.Ok, 
                    MsBox.Avalonia.Enums.Icon.Error);
                await box.ShowAsync();
                return;
            }

            try
            {
                Process.Start(new ProcessStartInfo
                {
                    FileName = batPath,
                    WorkingDirectory = folderPath,
                    UseShellExecute = true
                });
            }
            catch (Exception ex)
            {
                var box = MessageBoxManager.GetMessageBoxStandard(
                    "Ошибка запуска", 
                    $"Ошибка: {ex.Message}",
                    ButtonEnum.Ok, 
                    MsBox.Avalonia.Enums.Icon.Error);
                await box.ShowAsync();
            }
        }
        // Метод для удаления проекта из списка
        private void RemoveProject(Project project)
        {
            Projects.Remove(project);
        }
        // Метод для открытия папки
        private void OpenFolderAsync(string folderPath)
        {
            if (!string.IsNullOrEmpty(folderPath) && Directory.Exists(folderPath))
            {
                Process.Start(new ProcessStartInfo
                {
                    FileName = folderPath,
                    UseShellExecute = true
                });
            }
            else
            {
                // Показать сообщение об ошибке, если папка недоступна
                var box = MessageBoxManager
                    .GetMessageBoxStandard("Ошибка", "Папка не найдена!",
                    ButtonEnum.Ok, MsBox.Avalonia.Enums.Icon.Error);
                box.ShowAsync();
            }
        }
        #endregion

        #region Override
        // Метод для сохранения проектов, например, при закрытии окна
        protected override void OnClosing(WindowClosingEventArgs e)
        {
            base.OnClosing(e);
            ProjectService.SaveProjects(new List<Project>(Projects));
        }
        #endregion

    }

    // Реализация команды
    public class RelayCommand<T> : ICommand
    {
        private readonly Action<T> _execute;
        private readonly Func<T, bool>? _canExecute;

        public RelayCommand(Action<T> execute, Func<T, bool>? canExecute = null)
        {
            _execute = execute;
            _canExecute = canExecute;
        }

        public event EventHandler? CanExecuteChanged;

        public bool CanExecute(object? parameter)
        {
            return _canExecute == null || _canExecute((T)parameter!);
        }

        public void Execute(object? parameter)
        {
            _execute((T)parameter!);
        }
    }
}
