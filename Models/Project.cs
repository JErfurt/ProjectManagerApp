using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace ProjectManagerApp.Models
{
    public class Project : INotifyPropertyChanged
    {
        private int _id;
        private string _name = string.Empty;
        private string _description = string.Empty;
        private string _status = string.Empty;
        private string _folderPath = string.Empty;
        private string _language = string.Empty;
        private DateTimeOffset? _lastInteraction;

        public int Id
        {
            get => _id;
            set
            {
                if (_id != value)
                {
                    _id = value;
                    OnPropertyChanged();
                }
            }
        }

        public string Name
        {
            get => _name;
            set
            {
                if (_name != value)
                {
                    _name = value;
                    OnPropertyChanged();
                }
            }
        }

        public string Description
        {
            get => _description;
            set
            {
                if (_description != value)
                {
                    _description = value;
                    OnPropertyChanged();
                }
            }
        }

        public string Status
        {
            get => _status;
            set
            {
                if (_status != value)
                {
                    _status = value;
                    OnPropertyChanged();
                }
            }
        }

        public string FolderPath
        {
            get => _folderPath;
            set
            {
                if (_folderPath != value)
                {
                    _folderPath = value;
                    OnPropertyChanged();
                }
            }
        }

        public string Language
        {
            get => _language;
            set
            {
                if (_language != value)
                {
                    _language = value;
                    OnPropertyChanged();
                }
            }
        }

        public DateTimeOffset? LastInteraction
        {
            get => _lastInteraction;
            set
            {
                if (_lastInteraction != value)
                {
                    _lastInteraction = value;
                    OnPropertyChanged();
                    OnPropertyChanged(nameof(LastInteractionDisplay));
                }
            }
        }

        // Форматированное отображение даты
        public string LastInteractionDisplay => LastInteraction?.ToLocalTime().ToString("dd.MM.yyyy HH:mm:ss zzz") ?? "—";

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
