# ProjectManagerApp (FLTK / C++)

Лёгкое кроссплатформенное настольное приложение для управления проектами, переписанное на **C++17** и **FLTK 1.4** с хранением данных в **SQLite**.

## Особенности

- Минимальное потребление оперативной памяти (~3 МБ приватной памяти) и малый размер исполняемого файла (~3.5 МБ при полной статической сборке, без внешних DLL).
- Кроссплатформенность (Windows, Linux, macOS).
- Автоматическая поддержка светлой и тёмной системной темы (на Windows, включая тёмный заголовок окна) и Per-Monitor HiDPI.
- Полная совместимость с существующей базой данных `projects.db` (та же схема SQLite и путь в `AppData/jerfurt/ProjectManagerApp`).
- Фильтрация по языку программирования и мгновенный регистронезависимый поиск по названию и описанию (с поддержкой кириллицы).
- Инлайн-редактирование полей таблицы (двойной клик / `Enter` / `F2`, переход по `Tab`), выпадающий список выбора языка, контекстное меню удаления строки (ПКМ).
- Быстрые действия для каждого проекта: открыть папку, открыть в VS Code, запустить `run.bat` / `run.sh`.

## Сборка

### Требования

- CMake 3.20+
- Компилятор C++17 (MinGW GCC, MSVC, Clang)
- Ninja (для `build.bat` на Windows)
- FLTK 1.4 и SQLite3 подтягиваются автоматически через CMake `FetchContent`, если не найдены в системе.

### Windows (`build.bat`)

```bat
build.bat
```

Итоговый самодостаточный исполняемый файл появится в `publish\ProjectManagerApp.exe`.

### CMake (Windows / Linux / macOS)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Структура проекта

```text
ProjectManagerApp/
├── CMakeLists.txt
├── build.bat
├── app.manifest
├── resources.rc
└── src/
    ├── main.cpp
    ├── Models/
    │   └── Project.hpp
    ├── Services/
    │   ├── CodeEditorService.hpp/.cpp
    │   ├── PlatformService.hpp/.cpp
    │   └── ProjectService.hpp/.cpp
    └── UI/
        ├── Icons.hpp/.cpp
        ├── MainWindow.hpp/.cpp
        ├── ProjectTable.hpp/.cpp
        ├── SearchInput.hpp
        └── Theme.hpp
```

## Лицензия

MIT — см. [LICENSE](LICENSE).
