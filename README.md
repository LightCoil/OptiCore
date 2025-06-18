# OptiCore — мультиплатформенный оптимизационный фреймворк

## Сборка и запуск на Linux (antiX, Debian, Ubuntu, Arch)

### 1. Установка зависимостей

#### Для Debian/Ubuntu/antiX:
```bash
sudo apt update
sudo apt install -y build-essential cmake git pkg-config libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libasound2-dev libpulse-dev libnvidia-ml-dev qtbase5-dev qtbase5-dev-tools qtchooser qt5-qmake libqt5svg5-dev libqt5opengl5-dev
# Для ImGui (если нет в репо):
git clone https://github.com/ocornut/imgui.git
cd imgui && make && sudo make install && cd ..
# Для NVML (NVIDIA):
sudo apt install -y libnvidia-ml-dev || echo 'NVML не найден, GPU-мониторинг NVIDIA будет недоступен'
```

#### Для Arch Linux:
```bash
sudo pacman -Syu --noconfirm
sudo pacman -S --needed base-devel cmake git pkgconf mesa qt5-base alsa-lib pulseaudio nvidia-utils
# ImGui (AUR):
yay -S imgui
```

#### Добавление репозиториев (если что-то не находится):
- **Debian/Ubuntu:**
  - Для Qt5: `sudo add-apt-repository ppa:beineri/opt-qt-5.15.2-bionic -y && sudo apt update`
  - Для PulseAudio: `sudo add-apt-repository ppa:ubuntu-audio-dev/ppa -y && sudo apt update`
- **Arch:** используйте AUR (yay, paru) для ImGui, если нет в официальных.

### 2. Клонирование репозитория OptiCore
```bash
git clone <URL_ВАШЕГО_РЕПОЗИТОРИЯ>
cd Project\ Future
```

### 3. Сборка проекта
```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### 4. Запуск OptiCorePanel
```bash
./OptiCorePanel
```

### 5. Примечания
- Для управления governor, питания и некоторых функций может потребоваться запуск от root:
  ```bash
  sudo ./OptiCorePanel
  ```
- Для мониторинга NVIDIA GPU необходим установленный драйвер NVIDIA и библиотека NVML.
- Для PulseAudio/ALSA — убедитесь, что демоны запущены.
- Если что-то не находится — проверьте, что все репозитории добавлены и обновлены.

### 6. Структура проекта
- `CPU/`, `Memory/`, `IO/`, `Video/`, `Audio/`, `Power/` — оптимизаторы
- `UI/OptiCorePanel/` — интерфейс ImGui
- `plugins/` — интеграция модулей
- `docs/` — документация

### 7. Контакты и поддержка
- Вопросы и баги — в Issues репозитория или на почту автора.

---

**OptiCore — NASA-grade оптимизация для вашего Linux!** 