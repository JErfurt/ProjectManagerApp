# ProjectManagerApp

A cross-platform project management application built on the Avalonia UI framework.

## 🚀 Description

ProjectManagerApp is a modern desktop application for project management developed using the Avalonia UI Framework. The application provides a user-friendly interface for creating, editing, and tracking projects.

## ✨ Features

- 🖥️ Cross-platform (Windows, Linux, macOS)
- 🎨 Modern user interface based on Avalonia UI
- 📝 Project and task management
- 💾 Local data storage
- 🔧 Extensible architecture

## 🛠️ Technologies

- **.NET 9.0** - Development platform
- **Avalonia UI** - Cross-platform UI Framework
- **C#** - Programming language
- **MVVM** - Architectural pattern

## 📋 Requirements

- .NET 9.0 SDK or higher
- Visual Studio 2022 / JetBrains Rider / Visual Studio Code

## 🚀 Quick Start

### Cloning the Repository
```bash
git clone https://github.com/your-username/ProjectManagerApp.git
cd ProjectManagerApp
```

### Building and Running
```bash
# Restore dependencies
dotnet restore

# Build the project
dotnet build

# Run the application
dotnet run
```

### Creating a Distributable Package
```bash
# For Windows (x64)
dotnet publish -c Release -r win-x64 --self-contained true

# For Windows (ARM64)
dotnet publish -c Release -r win-arm64 --self-contained true

# For Linux (x64)
dotnet publish -c Release -r linux-x64 --self-contained true

# For Linux (ARM64)
dotnet publish -c Release -r linux-arm64 --self-contained true

# For macOS (Intel)
dotnet publish -c Release -r osx-x64 --self-contained true

# For macOS (Apple Silicon)
dotnet publish -c Release -r osx-arm64 --self-contained true
```

## 📁 Project Structure

```
ProjectManagerApp/
├── App.axaml              # Main application file
├── App.axaml.cs           # Code for the main application file
├── MainWindow.axaml       # Main application window
├── MainWindow.axaml.cs    # Code for the main window
├── Models/                # Data models
│   └── Project.cs         # Project model
├── Services/              # Application services
│   └── ProjectService.cs  # Service for managing projects
├── Properties/            # Project settings
├── Program.cs             # Application entry point
└── ProjectManagerApp.csproj # Project file
```

## 🤝 Contributing to the Project

We welcome contributions to the development of the project! If you would like to contribute:

1. Fork the repository
2. Create a branch for your new feature (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push changes to the branch (`git push origin feature/AmazingFeature`)
5. Create a Pull Request

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 📞 Contacts

- Creator: [JErfurt](https://github.com/JErfurt)
- Project link: [https://github.com/JErfurt/ProjectManagerApp](https://github.com/JErfurt/ProjectManagerApp)

## 🙏 Acknowledgments

- [Avalonia UI](https://avaloniaui.net/) - for the excellent cross-platform UI Framework
- [.NET](https://dotnet.microsoft.com/) - for the powerful development platform
- All contributors and users of the project

---

⭐ If you like the project, please give it a star!