# NetworkMonitoringSystem

**Windows desktop application for LAN device monitoring and connection key management.**

**Настольное приложение Windows для мониторинга устройств ЛВС и управления ключами подключения.**

[![Language](https://img.shields.io/badge/language-C%2B%2B%2FCLI-00599C?logo=c%2B%2B&logoColor=white)](https://learn.microsoft.com/cpp/dotnet/dotnet-programming-with-cpp-cli-visual-cpp)
[![Platform](https://img.shields.io/badge/platform-Windows-0078D6?logo=windows&logoColor=white)](https://www.microsoft.com/windows)
[![UI](https://img.shields.io/badge/UI-Windows%20Forms-512BD4)](https://learn.microsoft.com/dotnet/desktop/winforms/)
[![Database](https://img.shields.io/badge/database-SQLite-003B57?logo=sqlite&logoColor=white)](https://www.sqlite.org/)
[![IDE](https://img.shields.io/badge/build-Visual%20Studio-C94545?logo=visualstudio&logoColor=white)](https://visualstudio.microsoft.com/)

---

## Overview / Обзор

**English.** NetworkMonitoringSystem is a **LAN monitoring desktop application** for Windows, aimed at operator-style workflows. It helps you maintain a catalog of network devices, **check IP reachability with ICMP ping**, see **online/offline status** in a clear table, and **manage connection keys** (free/busy state, assignment to a named target, release). All operational data is stored locally in **SQLite**—no web backend or cloud services are involved.

**Русский.** NetworkMonitoringSystem — это **настольная система мониторинга локальной сети** под Windows для сценариев работы оператора. Приложение ведёт список сетевых устройств, **проверяет доступность по IP через ping**, отображает **статус «в сети / не в сети»** в таблице и **управляет ключами подключения** (свободен/занят, назначение на объект, освобождение). Данные хранятся локально в **SQLite**; веб-сервер и облачные интеграции не используются.

---

## Features / Возможности

**English**

- **Device list management** — add, edit, delete, and refresh records; fields include name, IP address, device type, and last known reachability status.
- **Ping-based availability** — `System.Net.NetworkInformation.Ping` with a bounded timeout; invalid or empty IPs are treated as offline.
- **Single-device and bulk checks** — check the selected row or run a full pass over all devices.
- **Status display** — grid columns for device type and reachability (Russian labels in the UI).
- **Connection key management** — CRUD for keys; columns show whether a key is free or busy and assignment details.
- **Assign and release keys** — assign a key to a device name or custom target via a dedicated dialog; release busy keys when work is done.
- **SQLite persistence** — devices, keys, and assignment history tables with foreign keys where applicable.
- **Progress UI** — modal progress window with status text during lengthy check operations.
- **Russian desktop UI** — main window, tabs, dialogs, and status strings are in Russian.

**Русский**

- **Управление списком устройств** — добавление, изменение, удаление, обновление; имя, IP, тип устройства, статус доступности.
- **Проверка доступности по ping** — ICMP через API .NET с ограничением по времени ожидания; некорректный или пустой IP считается недоступным.
- **Проверка одного и всех** — проверка выбранной строки или проход по всему списку.
- **Отображение статуса** — колонки типа устройства и доступности (подписи на русском).
- **Ключи подключения** — создание, редактирование, удаление; отображение занятости и назначения.
- **Назначение и освобождение** — диалог выбора/ввода цели назначения; освобождение занятого ключа.
- **Хранение в SQLite** — устройства, ключи, история назначений.
- **Окно прогресса** — отдельное окно с индикатором и текстом статуса при массовой проверке.
- **Русскоязычный интерфейс** — основное окно, вкладки и диалоги на русском.

---

## Interface overview / Интерфейс

**English.** The main form uses a **tab control** with two tabs: **Network devices** and **Connection keys**. Each tab shows a **read-only data grid** (list/table view, not a topology map) and a toolbar for actions. **Device edit** and **key edit** use separate dialog forms. **Key assignment** opens a dedicated dialog (combo with device names plus free text). During checks, a **fixed progress dialog** shows completion percentage and the current device being probed. **Screenshots:** there is no `screenshots/` folder in the repository yet; you can add images under e.g. `docs/screenshots/` later and link them here.

**Русский.** Главное окно — **вкладки**: «Сетевые устройства» и «Ключи подключения». На каждой вкладке — **таблица (DataGridView)** и панель кнопок. Редактирование устройства и ключа — **отдельные формы**. **Назначение ключа** — отдельный диалог. При проверке показывается **окно прогресса**. **Скриншотов** в репозитории пока нет; при необходимости можно добавить каталог (например, `docs/screenshots/`) и вставить ссылки в этот раздел.

---

## Tech stack / Технологии

- **Language:** C++/CLI  
- **UI:** Windows Forms (.NET)  
- **Runtime:** .NET Framework 4.8 (see `NetworkMonitoringSystem.vcxproj`)  
- **Data:** SQLite via **System.Data.SQLite** (NuGet package under `src/packages`)  
- **Tooling:** Visual Studio (v143 toolset, C++/CLI enabled)  
- **Target:** Windows desktop (**x64** and Win32 configurations)

---

## Project structure / Структура проекта

```
NetworkMonitoringSystem/
├── LICENSE
├── README.md
├── docs/                          # доп. материалы (напр. architecture_stage2.md, tz.txt)
└── src/
    ├── packages/                  # восстановленные NuGet-зависимости (SQLite)
    └── NetworkMonitoringSystem/
        ├── NetworkMonitoringSystem.sln
        ├── NetworkMonitoringSystem.vcxproj
        ├── packages.config
        ├── Program.cpp            # точка входа
        ├── MainForm.*             # главное окно, вкладки
        ├── DeviceEditForm.*       # карточка устройства
        ├── KeyEditForm.*          # карточка ключа
        ├── KeyAssignForm.*        # назначение ключа
        ├── ProgressForm.*         # прогресс проверки
        ├── Models/                # модели данных
        ├── Services/              # устройства, ключи, мониторинг (ping)
        └── Data/                  # DatabaseManager, схема SQLite
```

---

## How to run / Запуск

**English**

1. Install **Visual Studio** with workloads/components for **Desktop development with C++** and support for **C++/CLI** and **.NET desktop development** (so that .NET Framework 4.8 targeting is available).
2. Open `src/NetworkMonitoringSystem/NetworkMonitoringSystem.sln`.
3. Allow **NuGet restore** so `System.Data.SQLite` and related files populate `src/packages` (restore runs on build if configured).
4. Select configuration **Debug** or **Release** and platform **x64** (recommended).
5. Build the solution, then run **NetworkMonitoringSystem**.

**Русский**

1. Установите **Visual Studio** с поддержкой **разработки классических приложений на C++**, **C++/CLI** и **.NET для настольных приложений** (в т.ч. целевой профиль **.NET Framework 4.8**).
2. Откройте файл `src/NetworkMonitoringSystem/NetworkMonitoringSystem.sln`.
3. Дождитесь восстановления **NuGet** (пакет SQLite в `src/packages`).
4. Выберите **x64** (рекомендуется) и конфигурацию **Debug** или **Release**.
5. Соберите решение и запустите проект **NetworkMonitoringSystem**.

---

## What the system delivers / Что делает система

**English.** In operational terms, the tool **monitors a defined set of network devices by IP**, surfaces **availability** in the UI, and **coordinates connection keys** (who or what a key is tied to, and when it is free again). **Data stays on the local machine** in a SQLite file suitable for a small control-room or admin desk setup.

**Русский.** По смыслу это **наблюдение за перечнем устройств по IP**, **наглядный статус доступности** и **учёт ключей подключения** с назначением и возвратом. **Данные сохраняются локально** в SQLite — для рабочего места администратора или оператора без внешних сервисов.

---

## Keywords / Ключевые слова

**English (for discovery).** network monitoring system, LAN monitoring, Windows desktop application, C++/CLI WinForms project, SQLite desktop database, device availability check, IP ping monitoring, connection key management, network operator software, local network monitoring tool, ICMP reachability, .NET Framework Windows Forms.

**Русский.** система мониторинга сети, мониторинг ЛВС, настольное приложение Windows, C++/CLI и Windows Forms, SQLite локальная база, проверка доступности устройств, мониторинг по IP и ping, управление ключами подключения, ПО для оператора сети, локальный мониторинг без облака.

---

## Contact / Контакты

Telegram: https://t.me/VadikQA

---

## License / Лицензия

This project is licensed under the **MIT License** — see the [`LICENSE`](LICENSE) file.

Проект распространяется на условиях **лицензии MIT** — текст в файле [`LICENSE`](LICENSE).
