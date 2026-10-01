# LittleFS 使用说明（ESP32 / Arduino / PlatformIO）

## 1. 目标

本说明面向 ESP32 开发中对 LittleFS 的常用读取、保存、目录扫描和手动格式化场景，适合用于保存校准数据、配置数据、日志文件等。

## 2. 基本用法

### 2.1 挂载文件系统

```cpp
#include <FS.h>
#include <LittleFS.h>

if (!LittleFS.begin(false)) {
  Serial.println("LittleFS mount failed");
  return;
}
```

说明：
- `LittleFS.begin(false)` 表示“正常挂载，不强制格式化”
- 如果你要手动清空文件系统，才调用 `LittleFS.format()`

### 2.2 直接写入 JSON 文件

```cpp
File file = LittleFS.open("/cali_123456789.json", "w");
if (!file) {
  Serial.println("Open failed");
  return;
}

file.print("{\"test\":123}");
file.close();
```

### 2.3 读取 JSON 文件

```cpp
File file = LittleFS.open("/cali_123456789.json", "r");
if (!file) {
  Serial.println("Read failed");
  return;
}

String content = file.readString();
Serial.println(content);
file.close();
```

### 2.4 判断文件是否存在

```cpp
if (LittleFS.exists("/cali_123456789.json")) {
  Serial.println("File exists");
}
```

### 2.5 删除文件

```cpp
if (LittleFS.remove("/cali_123456789.json")) {
  Serial.println("Removed");
}
```

## 3. 遍历根目录并查看文件列表

遍历目录最可靠的方式是：

```cpp
File root = LittleFS.open("/");
if (!root || !root.isDirectory()) {
  Serial.println("Root open failed");
  return;
}

while (true) {
  File entry = root.openNextFile();
  if (!entry) {
    break;
  }

  String fileName = entry.name();
  String fullPath = fileName.startsWith("/") ? fileName : "/" + fileName;

  if (!entry.isDirectory()) {
    Serial.print(fullPath);
    Serial.print(" | size=");
    Serial.println(entry.size());
  }

  entry.close();
}

root.close();
```

注意：
- `entry.name()` 在不同实现下可能返回 `cali_xxx.json` 或 `/cali_xxx.json`
- 因此推荐先做统一处理：

```cpp
String fullPath = fileName.startsWith("/") ? fileName : "/" + fileName;
```

## 4. 生成随机文件名并保留历史文件

如果需求是：
- 每次重启都生成一个新的随机文件名
- 不删除旧文件
- 只保留历史文件列表

可以采用这种方式：

```cpp
String buildRandomTestFilePath() {
  char buffer[32];
  const uint32_t randomNumber = random(100000000u, 1000000000u);
  snprintf(buffer, sizeof(buffer), "/cali_%09u.json", randomNumber);
  return String(buffer);
}
```

然后在 `begin()` 或启动时调用：

```cpp
String gCurrentFilePath;
if (gCurrentFilePath.length() == 0) {
  do {
    gCurrentFilePath = buildRandomTestFilePath();
  } while (LittleFS.exists(gCurrentFilePath.c_str()));
}
```

这样可以避免重复文件名，并且不会主动删除历史文件。

## 5. 手动格式化逻辑

推荐的设计：
- 默认不自动格式化
- 只有在遇到挂载失败、文件不可读，或者用户明确要求时才格式化

示例：

```cpp
bool formatFilesystem() {
  Serial.println("Formatting LittleFS...");
  if (!LittleFS.format()) {
    Serial.println("LittleFS format failed");
    return false;
  }

  if (!LittleFS.begin(false)) {
    Serial.println("LittleFS mount after formatting failed");
    return false;
  }

  return true;
}
```

在 `loop()` 中处理：

```cpp
void loop() {
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == '1') {
      Serial.println("Manual format requested.");
      if (formatFilesystem()) {
        Serial.println("Format successful. Restarting...");
        ESP.restart();
      }
    }
  }
}
```

这是最稳妥的模式：不会因为重启就无条件清空数据。

## 6. 典型写入/读取流程

```cpp
void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin(false)) {
    Serial.println("LittleFS mount failed");
    return;
  }

  String path = "/cali_123456789.json";

  // 1. 写入
  File file = LittleFS.open(path, "w");
  if (file) {
    file.print("{\"value\":42}\n");
    file.close();
  }

  // 2. 读取
  File in = LittleFS.open(path, "r");
  if (in) {
    Serial.println(in.readString());
    in.close();
  }
}
```

## 7. 注意事项

### 7.1 不要在每次启动时 blindly 调用 `LittleFS.begin(true)`

`LittleFS.begin(true)` 表示“挂载失败时格式化”。
如果程序在每次启动时都执行它，历史文件会被重置，用户会看到目录被清空。

### 7.2 目录遍历时要兼容路径格式

有时 `entry.name()` 为：
- `/cali_123.json`
- `cali_123.json`

所以应统一转换为：

```cpp
String fullPath = entry.name();
if (!fullPath.startsWith("/")) fullPath = "/" + fullPath;
```

### 7.3 手动格式化必须要经过确认

用户确认后再执行 `LittleFS.format()`，这样能避免无意间删除全部测试文件。

## 8. 常见日志示例

```text
Boot: selecting a new active file -> /cali_708627262.json
Current LittleFS root before create:
All calibration files currently stored in LittleFS:
  /cali_782023518.json  size=1402
Write OK: /cali_708627262.json
Current LittleFS root after save:
  /cali_708627262.json  size=1402
  /cali_782023518.json  size=1402
Created and saved test calibration JSON
```

这说明：
- 每次启动都生成新文件
- 历史文件保留
- 目录扫描正常
- 写入和读回都成功

## 9. 适合的使用场景

这个模式适用于：
- 校准记录持久化
- 生产参数保存
- 温度/电流/压力曲线采样记录
- 需要保留历史文件并在重启后继续记录的新数据文件场景

## 10. 总结

本方案的核心原则是：

- 默认不格式化
- 只在失败或用户确认时格式化
- 每次都新建随机文件名
- 保存历史数据文件
- 目录遍历显示全部 `cali_*.json` 文件
- 通过打开当前文件路径来验证写入是否成功

这对需要“保留多个测试文件记录”非常适用。
