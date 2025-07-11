#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QString>

// Ayar kategorileri
enum class SettingsCategory {
    General,        // Genel ayarlar
    Machine,        // Makine parametreleri
    Communication,  // İletişim ayarları
    GCode,          // G-code ayarları
    UI,             // Kullanıcı arayüzü
    Safety,         // Güvenlik ayarları
    Performance,    // Performans ayarları
    Advanced        // Gelişmiş ayarlar
};

class Settings : public QObject
{
    Q_OBJECT

public:
    static Settings* instance();
    
    // Temel ayar işlemleri
    void setValue(SettingsCategory category, const QString &key, const QVariant &value);
    QVariant getValue(SettingsCategory category, const QString &key, const QVariant &defaultValue = QVariant());
    
    // Makine ayarları
    void setMachineLimits(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
    void getMachineLimits(double &xMin, double &xMax, double &yMin, double &yMax, double &zMin, double &zMax);
    
    void setMachineSpeeds(double maxFeedRate, double maxJogSpeed, double maxSpindleSpeed);
    void getMachineSpeeds(double &maxFeedRate, double &maxJogSpeed, double &maxSpindleSpeed);
    
    // İletişim ayarları
    void setSerialSettings(const QString &port, int baudRate, int dataBits, int stopBits, const QString &parity);
    void getSerialSettings(QString &port, int &baudRate, int &dataBits, int &stopBits, QString &parity);
    
    void setNetworkSettings(const QString &ip, int port, const QString &protocol);
    void getNetworkSettings(QString &ip, int &port, QString &protocol);
    
    // G-code ayarları
    void setGCodeSettings(bool autoLeveling, bool toolCompensation, double defaultFeedRate);
    void getGCodeSettings(bool &autoLeveling, bool &toolCompensation, double &defaultFeedRate);
    
    // Güvenlik ayarları
    void setSafetySettings(bool hardLimits, bool softLimits, bool emergencyStop, double maxAcceleration);
    void getSafetySettings(bool &hardLimits, bool &softLimits, bool &emergencyStop, double &maxAcceleration);
    
    // Performans ayarları
    void setPerformanceSettings(int updateInterval, bool realTimeMonitoring, bool performanceLogging);
    void getPerformanceSettings(int &updateInterval, bool &realTimeMonitoring, bool &performanceLogging);
    
    // UI ayarları
    void setUISettings(const QString &theme, bool darkMode, int fontSize, bool showTooltips);
    void getUISettings(QString &theme, bool &darkMode, int &fontSize, bool &showTooltips);
    
    // Gelişmiş ayarlar (gelecekteki özellikler için)
    void setAdvancedSettings(bool pluginSystem, bool webInterface, bool cloudSync, bool aiFeatures);
    void getAdvancedSettings(bool &pluginSystem, bool &webInterface, bool &cloudSync, bool &aiFeatures);
    
    // Ayar yönetimi
    void saveSettings();
    void loadSettings();
    void resetToDefaults();
    void exportSettings(const QString &filePath);
    void importSettings(const QString &filePath);
    
    // Ayar değişiklik sinyalleri
    void notifySettingChanged(SettingsCategory category, const QString &key, const QVariant &value);

signals:
    void settingChanged(SettingsCategory category, const QString &key, const QVariant &value);
    void settingsSaved();
    void settingsLoaded();

private:
    Settings(QObject *parent = nullptr);
    ~Settings();
    
    static Settings* m_instance;
    QSettings* m_settings;
    
    QString categoryToString(SettingsCategory category);
    QString getFullKey(SettingsCategory category, const QString &key);
};

// Kolay kullanım için makrolar
#define SETTINGS Settings::instance()
#define SET_VALUE(category, key, value) SETTINGS->setValue(category, key, value)
#define GET_VALUE(category, key, defaultValue) SETTINGS->getValue(category, key, defaultValue)

#endif // SETTINGS_H 