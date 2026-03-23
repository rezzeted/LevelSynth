# ImGui + SDL3 + OpenGL3

Минимальное приложение на C++20 с Dear ImGui, SDL3 и OpenGL 3. Библиотека **edgar** (порт Edgar-DotNet) для процедурной раскладки комнат. Сборка через **CMake**; зависимости задаются **манифестом vcpkg** ([`vcpkg.json`](vcpkg.json)), сам **vcpkg** — **git submodule** в [`toolchain/vcpkg`](toolchain/vcpkg).

---

## Требования

- **CMake** 3.20+
- **Git** (для submodule `toolchain/vcpkg`)
- **Компилятор** с поддержкой C++20 (на Windows — Visual Studio 2022, x64)
- **OpenGL**
- После клона: `git submodule update --init --recursive` и один раз `bootstrap-vcpkg.bat` / `bootstrap-vcpkg.sh` в `toolchain/vcpkg`

---

## Сборка (vcpkg)

Укажите toolchain vcpkg при конфигурации CMake:

```batch
cmake -S . -B _build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=%CD%\toolchain\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build _build --config Debug
```

Либо используйте пресеты [`CMakePresets.json`](CMakePresets.json): `cmake --preset vs2026`, затем `cmake --build --preset release` (triplet **`x64-windows-static`**, `CMAKE_TOOLCHAIN_FILE` и `VCPKG_OVERLAY_PORTS` заданы в пресете). Для Ninja без VS: `cmake --preset default`. Для VS 2022: пресет `vs2022` и build `debug-vs2022` / `release-vs2022`.

Скрипт [`build_vs.bat`](build_vs.bat) передаёт тот же `CMAKE_TOOLCHAIN_FILE`.

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
└── docs/EDGAR_PORT_INVENTORY.md
```

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
