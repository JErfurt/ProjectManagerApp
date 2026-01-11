using Avalonia.Controls;
using Avalonia.Interactivity;
using ProjectManagerApp.Models;
using ProjectManagerApp.Services;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Windows.Input;

namespace ProjectManagerApp
{
    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        private string _selectedLanguageFilter = "";
        private string _searchQuery = "";
        private List<Project> _allProjects = new();
        
        public ObservableCollection<Project> Projects { get; set; }
        public Project? SelectedProject { get; set; }
        public static ICommand? OpenFolderCommand { get; private set; }
        public static ICommand? OpenInEditorCommand { get; private set; }
        public ICommand RemoveProjectCommand { get; private set; }
        public static ICommand? RunBatchCommand { get; private set; }
        
        // Список языков программирования для выбора
        public static List<string> AvailableLanguages { get; } = new()
        {
            "",
            "C#",
            "Python",
            "JavaScript",
            "TypeScript",
            "ASM",
            "Java",
            "C++",
            "C",
            "Go",
            "Rust",
            "Ruby",
            "PHP",
            "Swift",
            "Kotlin",
            "Dart",
            "R",
            "Scala",
            "Lua",
            "Shell",
            "PowerShell",
            "SQL",
            "HTML/CSS",
            "Other"
        };
        
        // Список языков для фильтра (с опцией "Все")
        public static List<string> FilterLanguages { get; } = new List<string> { "Все" }
            .Concat(AvailableLanguages.Where(l => !string.IsNullOrEmpty(l)))
            .ToList();
        
        // Выбранный язык для фильтра
        public string SelectedLanguageFilter
        {
            get => _selectedLanguageFilter;
            set
            {
                if (_selectedLanguageFilter != value)
                {
                    _selectedLanguageFilter = value;
                    OnPropertyChanged();
                    ApplyFilter();
                }
            }
        }
        
        // Поисковый запрос
        public string SearchQuery
        {
            get => _searchQuery;
            set
            {
                if (_searchQuery != value)
                {
                    _searchQuery = value;
                    OnPropertyChanged();
                    ApplyFilter();
                }
            }
        }

        public MainWindow()
        {
            InitializeComponent();
            
            // Загрузка проектов
            _allProjects = ProjectService.LoadProjects();
            Projects = new ObservableCollection<Project>(_allProjects);
            
            // Подписываемся на изменения свойств существующих проектов
            foreach (var project in _allProjects)
            {
                project.PropertyChanged += OnProjectPropertyChanged;
            }
            
            _selectedLanguageFilter = "Все";
            
            DataContext = this;

            // Команды — принимают Project и обновляют LastInteraction
            OpenFolderCommand = new RelayCommand<Project>(async (project) =>
            {
                if (project != null && !string.IsNullOrEmpty(project.FolderPath))
                {
                    project.LastInteraction = DateTimeOffset.Now;
                    await PlatformService.OpenFolderAsync(project.FolderPath);
                }
            });
            
            OpenInEditorCommand = new RelayCommand<Project>(async (project) =>
            {
                if (project != null && !string.IsNullOrEmpty(project.FolderPath))
                {
                    project.LastInteraction = DateTimeOffset.Now;
                    await CodeEditorService.OpenFolderInCodeEditor(project.FolderPath);
                }
            });
            
            RemoveProjectCommand = new RelayCommand<Project>(RemoveProject);
            
            RunBatchCommand = new RelayCommand<Project>(async (project) =>
            {
                if (project != null && !string.IsNullOrEmpty(project.FolderPath))
                {
                    project.LastInteraction = DateTimeOffset.Now;
                    await PlatformService.RunScriptAsync(project.FolderPath);
                }
            });
        }

        #region Public Methods
        // Метод для добавления нового проекта в список
        public void AddProject(object sender, RoutedEventArgs args)
        {
            var newProject = new Project
            {
                Name = "Новый проект",
                Description = "Описание",
                Status = "Новый",
                FolderPath = string.Empty,
                LastInteraction = null
            };
            
            // Сохраняем в БД и получаем Id
            newProject.Id = ProjectService.AddProject(newProject);
            
            // Подписываемся на изменения
            newProject.PropertyChanged += OnProjectPropertyChanged;
            
            // Добавляем в полный список
            _allProjects.Add(newProject);
            
            // Добавляем в отображаемый список (если проходит фильтр)
            if (SelectedLanguageFilter == "Все" || string.IsNullOrEmpty(newProject.Language))
            {
                Projects.Add(newProject);
            }
        }

        // Метод открытия папки конфигурации
        public async void OpenConfigFolder(object sender, RoutedEventArgs args)
        {
            var folderPath = ProjectService.AppDataFolder;
            await PlatformService.OpenFolderAsync(folderPath);
        }
        #endregion

        #region Private Methods
        // Применение фильтра и поиска
        private void ApplyFilter()
        {
            Projects.Clear();
            
            var filtered = _allProjects.AsEnumerable();
            
            // Фильтр по языку
            if (SelectedLanguageFilter != "Все")
            {
                filtered = filtered.Where(p => p.Language == SelectedLanguageFilter);
            }
            
            // Поиск по названию и описанию (без учёта регистра)
            if (!string.IsNullOrWhiteSpace(SearchQuery))
            {
                var query = SearchQuery.Trim();
                filtered = filtered.Where(p => 
                    p.Name.Contains(query, StringComparison.OrdinalIgnoreCase) ||
                    p.Description.Contains(query, StringComparison.OrdinalIgnoreCase));
            }
            
            foreach (var project in filtered)
            {
                Projects.Add(project);
            }
        }
        
        // Метод для удаления проекта из списка
        private void RemoveProject(Project project)
        {
            // Отписываемся от изменений
            project.PropertyChanged -= OnProjectPropertyChanged;
            
            // Удаляем из БД
            ProjectService.DeleteProject(project.Id);
            
            // Удаляем из полного списка и отображаемого
            _allProjects.Remove(project);
            Projects.Remove(project);
        }
        
        // Обработчик изменения свойств проекта
        private void OnProjectPropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            if (sender is Project project && project.Id > 0 
                && e.PropertyName != nameof(Project.Id) 
                && e.PropertyName != nameof(Project.LastInteractionDisplay))
            {
                // Обновляем в БД при изменении любого свойства (кроме Id и вычисляемого LastInteractionDisplay)
                ProjectService.UpdateProject(project);
                
                // Если изменился язык — переприменяем фильтр
                if (e.PropertyName == nameof(Project.Language))
                {
                    ApplyFilter();
                }
            }
        }
        #endregion
        
        #region INotifyPropertyChanged
        public new event PropertyChangedEventHandler? PropertyChanged;
        
        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
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

        public event EventHandler? CanExecuteChanged
        {
            add { }
            remove { }
        }

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
