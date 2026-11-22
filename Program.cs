using Avalonia;
using Avalonia.Win32;
using System;
using System.Runtime.InteropServices;

namespace ProjectManagerApp;

class Program
{
    // Initialization code. Don't use any Avalonia, third-party APIs or any
    // SynchronizationContext-reliant code before AppMain is called: things aren't initialized
    // yet and stuff might break.
    // STAThread нужен для Windows, но просто игнорируется на других платформах
    [STAThread]
    public static void Main(string[] args) => BuildAvaloniaApp()
        .StartWithClassicDesktopLifetime(args);

    // Avalonia configuration, don't remove; also used by visual designer.
    public static AppBuilder BuildAvaloniaApp()
        => AppBuilder.Configure<App>()
            .UsePlatformDetect()
            .With(new Win32PlatformOptions
            {
                // Программная отрисовка не поднимает ANGLE и драйвер GPU,
                // которые в этом окне добавляли десятки мегабайт приватной памяти.
                RenderingMode = [Win32RenderingMode.Software]
            })
            .LogToTrace();
}
