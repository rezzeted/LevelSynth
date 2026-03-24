# ImGui + SDL3 + OpenGL3

Минимальное приложение на C++20 с Dear ImGui, SDL3 и OpenGL 3. Библиотека **edgar** (порт Edgar-DotNet) для процедурной раскладки комнат. Сборка через **CMake**; зависимости задаются **манифестом vcpkg** ([`vcpkg.json`](vcpkg.json)), сам **vcpkg** — **git submodule** в [`toolchain/vcpkg`](toolchain/vcpkg).

---

## Требования

- **CMake** 3.20+
- **Git** (для submodule `toolchain/vcpkg`)
- **Компилятор** с поддержкой C++20 (на Windows — Visual Studio 2022 или новее, x64; [`build_vs.bat`](build_vs.bat) ориентирован на **Visual Studio 18 2026**)
- **OpenGL**
- После клона: `git submodule update --init --recursive` и один раз `bootstrap-vcpkg.bat` / `bootstrap-vcpkg.sh` в `toolchain/vcpkg`

---

## Сборка (vcpkg)

Проще всего: из корня репозитория запустить [`build_vs.bat`](build_vs.bat) — он вызывает **`cmake --preset vs2026`** и **`cmake --build --preset debug`** (все пути vcpkg, triplet и overlay заданы в [`CMakePresets.json`](CMakePresets.json)).

Ручная конфигурация (эквивалент по смыслу):

```batch
cmake -S . -B _build -G "Visual Studio 18 2026" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=%CD%\toolchain\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
  -DVCPKG_OVERLAY_PORTS=%CD%\toolchain\vcpkg-overlay\ports
cmake --build _build --config Debug
```

Либо пресеты [`CMakePresets.json`](CMakePresets.json): `cmake --preset vs2026`, затем `cmake --build --preset release` (triplet **`x64-windows-static`**, toolchain и overlay в пресете). Для Ninja без VS: `cmake --preset default`. Для VS 2022: пресет `vs2022` и build `debug-vs2022` / `release-vs2022`.

Исполняемые файлы: `_build/bin/<Config>/main.exe`, тесты: `_build/bin/<Config>/edgar_tests.exe`.

Пакеты из манифеста устанавливаются в каталог **`vcpkg_installed/`** рядом с билдом (в `.gitignore`).

### Clipper2

Пересечение полигонов в edgar использует **Clipper2** той же версии, что и порт vcpkg (**2.0.1**), но библиотека **собирается из исходников** через FetchContent в [`src/libs/edgar/CMakeLists.txt`](src/libs/edgar/CMakeLists.txt), чтобы статический бинарник совпадал с вашим MSVC (предсобранный `Clipper2.lib` из vcpkg на другой машине может давать `LNK2019 __std_rotate` при смешении версий toolset).

---

## Зависимости (vcpkg.json)

| Порт | Назначение |
|------|------------|
| nlohmann-json, fmt, spdlog, stb | edgar: JSON, логи, PNG |
| gtest | тесты |
| sdl3, imgui (+ docking, sdl3, opengl3) | окно и ImGui |

---

## Структура проекта

```
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── toolchain/vcpkg/          # git submodule vcpkg
├── thirdparty/CMakeLists.txt # find_package + imgui_impl INTERFACE
├── src/libs/edgar/           # библиотека edgar
├── src/apps/main/
├── src/tests/
├── resources/edgar_gui/      # копия из референса (см. ниже)
└── docs/EDGAR_PORT_INVENTORY.md
```

### Ресурсы Edgar.GUI (копия из референса)

Каталог [`resources/edgar_gui/`](resources/edgar_gui) — это **не** самостоятельные ассеты проекта, а **копия** дерева **`src/Resources`** из upstream [Edgar-DotNet](https://github.com/OndrejNepozitek/Edgar-DotNet) (WinForms-проект `Edgar.GUI`: `MapDescriptions`, `Maps`, `Images`, `Rooms`, `RandomGraphs` и т.д.). У себя их можно заново скопировать из локального клона референса, например:

```batch
robocopy %CD%\_edgar_ref\src\Resources %CD%\resources\edgar_gui /E
```

При сборке `main` CMake **копирует** `resources/edgar_gui` рядом с `main.exe` в `resources/edgar_gui/`, чтобы пути относительно исполняемого файла совпадали с ожидаемой раскладкой папок.

---

## Технические особенности приложения

### Статическая линковка SDL3 на Windows

- Точка входа — свой `main()`, не SDL_main.
- Перед любыми включениями SDL задаётся **`SDL_MAIN_HANDLED`**.
- Подключается **`<SDL3/SDL_main.h>`** и перед `SDL_Init()` вызывается **`SDL_SetMainReady()`**.
- В SDL3 **`SDL_Init()`** при успехе возвращает **`true`**, при ошибке — **`false`**.

### Окно и OpenGL

- Окно: `SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY`.
- HiDPI: `ImGuiConfigFlags_DpiEnableScaleFonts` и `ImGuiConfigFlags_DpiEnableScaleViewports`.
- Контекст OpenGL: Core Profile 3.0.

### Шрифты

- Windows: Segoe UI / Arial из `C:\Windows\Fonts\`.
- Linux: DejaVu / Liberation в типичных путях.

---

## Тесты

- `enable_testing()` в корневом CMake, цели в `src/tests/`.
- Запуск: `ctest -C Debug` из каталога сборки.

---

## Стиль и документация

- Код на **C++20**.
- Порт Edgar: см. [`docs/EDGAR_PORT_INVENTORY.md`](docs/EDGAR_PORT_INVENTORY.md).
