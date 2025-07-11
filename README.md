# CNC Controller v1.0

Modern, modüler CNC kontrol yazılımı. Qt6 ve C++17 kullanılarak geliştirilmiştir.

## Özellikler

### Mevcut Özellikler
- ✅ **3 Eksen Kontrolü (X, Y, Z)**: Jog ve sürekli hareket
- ✅ **G-Code Desteği**: Dosya yükleme, kaydetme ve parsing
- ✅ **3D Simülasyon**: OpenGL tabanlı görselleştirme
- ✅ **Emergency Stop**: Acil durum durdurma sistemi
- ✅ **Hız Kontrolü**: Jog hızı ve feedrate ayarları
- ✅ **Soft Limits**: Eksen limit kontrolü
- ✅ **Run from Line**: Belirli satırdan başlatma
- ✅ **Klavye Kısayolları**: Hızlı kontrol için

### Modüler Mimari
- 🔧 **GCodeParser**: G-code parsing ve doğrulama
- 🔧 **SerialCommunication**: ESP32/GRBL iletişimi
- 🔧 **AxisController**: Eksen hareket kontrolü
- 🔧 **Settings**: Uygulama ayarları yönetimi
- 🔧 **Logger**: Kapsamlı loglama sistemi

## Kurulum

### Gereksinimler
- Qt6 (Core, Widgets, OpenGL, SerialPort)
- C++17 uyumlu derleyici
- CMake 3.16+ veya qmake

### Derleme

#### CMake ile:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

#### qmake ile:
```bash
qmake CNC_Controller.pro
make
```

## Modüler Yapı

### 1. GCodeParser Modülü
G-code dosyalarını parse eder ve doğrular.

```cpp
GCodeParser parser;
QVector<GCodeCommand> commands = parser.parseFile(gcodeContent);
```

**Desteklenen Komutlar:**
- G0/G1: Doğrusal hareket
- G2/G3: Dairesel hareket
- G20/G21: Birim ayarları
- G28: Ana pozisyona dön
- G90/G91: Koordinat modu
- M0-M9: Yardımcı fonksiyonlar

### 2. SerialCommunication Modülü
ESP32/GRBL ile seri port üzerinden iletişim kurar.

```cpp
SerialCommunication serial;
serial.connectToDevice("COM3", 115200);
serial.sendGCodeCommand("G1 X10 Y10 F1000");
```

**Özellikler:**
- Otomatik komut kuyruğu
- Timeout yönetimi
- Hata yakalama
- GRBL protokol desteği

### 3. AxisController Modülü
Eksen hareketlerini kontrol eder ve limit kontrolü yapar.

```cpp
AxisController controller;
controller.setAxisLimits('X', -50, 50);
controller.startContinuousJog('X', true);
```

**Özellikler:**
- Jog ve sürekli hareket
- Soft limit kontrolü
- Hızlanma kontrolü
- Emergency stop entegrasyonu

### 4. Settings Modülü
Uygulama ayarlarını kalıcı olarak saklar.

```cpp
Settings settings;
settings.setJogSpeed(1000.0);
settings.setSerialPort("COM3");
```

**Saklanan Ayarlar:**
- Seri port konfigürasyonu
- Eksen limitleri
- Jog ayarları
- G-code ayarları
- UI tercihleri

### 5. Logger Modülü
Kapsamlı loglama sistemi.

```cpp
Logger::instance()->info("Uygulama başlatıldı", "Main");
Logger::instance()->error("Bağlantı hatası", "Serial");
```

**Özellikler:**
- Dosya ve konsol loglama
- Log seviyesi kontrolü
- Kategori filtreleme
- Otomatik dosya rotasyonu

## Kullanım

### Temel Kontroller
- **X, Y, Z**: Eksen jog (Shift+tuş = pozitif yön)
- **Space**: Tüm hareketleri durdur
- **E**: Emergency stop
- **Ctrl+R**: Reset

### G-Code İşleme
1. Sol panelden G-code dosyası açın
2. "Satırdan Başlat" ile belirli satırdan başlatın
3. Tek komut göndermek için alt paneli kullanın

### Seri Port Bağlantısı
1. ESP32/GRBL cihazını bağlayın
2. Ayarlardan port seçin
3. Bağlantıyı test edin

## Geliştirme

### Yeni Modül Ekleme
1. Header dosyası oluşturun (`include/modulename.h`)
2. Implementasyon dosyası oluşturun (`src/modulename.cpp`)
3. Proje dosyalarına ekleyin
4. MainWindow'da başlatın ve bağlayın

### Kod Standartları
- Qt naming conventions kullanın
- Signal-slot mekanizmasını tercih edin
- Hata yönetimi için Logger kullanın
- Ayarlar için Settings modülünü kullanın

## Gelecek Özellikler

### Planlanan Modüller
- **ToolpathVisualizer**: Gelişmiş 3D görselleştirme
- **PerformanceMonitor**: Performans izleme
- **CollisionDetection**: Çarpışma algılama
- **ToolManagement**: Takım yönetimi

### Geliştirme Öncelikleri
1. Gerçek G-code execution
2. Gelişmiş 3D simülasyon
3. Çoklu dil desteği
4. Plugin sistemi

## Lisans

Bu proje MIT lisansı altında lisanslanmıştır.

## Katkıda Bulunma

1. Fork yapın
2. Feature branch oluşturun
3. Değişikliklerinizi commit edin
4. Pull request gönderin

## İletişim

Sorularınız için issue açabilir veya pull request gönderebilirsiniz.
