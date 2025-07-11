#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>

class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject *parent = nullptr);
    ~Settings();
    
    // Genel ayarlar
    void setValue(const QString &key, const QVariant &value);
    QVariant getValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    
    // Seri port ayarları
    void setSerialPort(const QString &port);
    QString getSerialPort() const;
    void setBaudRate(int baudRate);
    int getBaudRate() const;
    void setDataBits(int dataBits);
    int getDataBits() const;
    void setParity(int parity);
    int getParity() const;
    void setStopBits(int stopBits);
    int getStopBits() const;
    
    // Eksen ayarları
    void setAxisLimits(char axis, double minLimit, double maxLimit);
    double getAxisMinLimit(char axis) const;
    double getAxisMaxLimit(char axis) const;
    void setAxisEnabled(char axis, bool enabled);
    bool isAxisEnabled(char axis) const;
    
    // Jog ayarları
    void setJogStep(double step);
    double getJogStep() const;
    void setJogSpeed(double speed);
    double getJogSpeed() const;
    void setJogAcceleration(double acceleration);
    double getJogAcceleration() const;
    
    // G-code ayarları
    void setDefaultFeedRate(double feedRate);
    double getDefaultFeedRate() const;
    void setMaxFeedRate(double maxFeedRate);
    double getMaxFeedRate() const;
    void setGCodeEditorFont(const QString &fontFamily, int fontSize);
    QString getGCodeEditorFontFamily() const;
    int getGCodeEditorFontSize() const;
    
    // UI ayarları
    void setWindowGeometry(const QByteArray &geometry);
    QByteArray getWindowGeometry() const;
    void setWindowState(const QByteArray &state);
    QByteArray getWindowState() const;
    void setTheme(const QString &theme);
    QString getTheme() const;
    
    // Dosya ayarları
    void setLastDirectory(const QString &directory);
    QString getLastDirectory() const;
    void setRecentFiles(const QStringList &files);
    QStringList getRecentFiles() const;
    void addRecentFile(const QString &file);
    
    // Ayarları kaydetme/yükleme
    void saveSettings();
    void loadSettings();
    void resetToDefaults();
    
    // Ayarları dışa/içe aktarma
    bool exportSettings(const QString &filename);
    bool importSettings(const QString &filename);
    
signals:
    void settingsChanged(const QString &key, const QVariant &value);
    void settingsLoaded();
    void settingsSaved();

private:
    QSettings *settings;
    
    void initializeDefaults();
    QString getAxisKey(char axis, const QString &property) const;
};

// Ayarlar için sabitler
namespace SettingsKeys {
    // Seri port
    const QString SERIAL_PORT = "Serial/Port";
    const QString SERIAL_BAUD_RATE = "Serial/BaudRate";
    const QString SERIAL_DATA_BITS = "Serial/DataBits";
    const QString SERIAL_PARITY = "Serial/Parity";
    const QString SERIAL_STOP_BITS = "Serial/StopBits";
    
    // Eksenler
    const QString AXIS_X_MIN_LIMIT = "Axis/X/MinLimit";
    const QString AXIS_X_MAX_LIMIT = "Axis/X/MaxLimit";
    const QString AXIS_X_ENABLED = "Axis/X/Enabled";
    const QString AXIS_Y_MIN_LIMIT = "Axis/Y/MinLimit";
    const QString AXIS_Y_MAX_LIMIT = "Axis/Y/MaxLimit";
    const QString AXIS_Y_ENABLED = "Axis/Y/Enabled";
    const QString AXIS_Z_MIN_LIMIT = "Axis/Z/MinLimit";
    const QString AXIS_Z_MAX_LIMIT = "Axis/Z/MaxLimit";
    const QString AXIS_Z_ENABLED = "Axis/Z/Enabled";
    
    // Jog
    const QString JOG_STEP = "Jog/Step";
    const QString JOG_SPEED = "Jog/Speed";
    const QString JOG_ACCELERATION = "Jog/Acceleration";
    
    // G-code
    const QString GCODE_DEFAULT_FEED_RATE = "GCode/DefaultFeedRate";
    const QString GCODE_MAX_FEED_RATE = "GCode/MaxFeedRate";
    const QString GCODE_EDITOR_FONT_FAMILY = "GCode/EditorFontFamily";
    const QString GCODE_EDITOR_FONT_SIZE = "GCode/EditorFontSize";
    
    // UI
    const QString UI_WINDOW_GEOMETRY = "UI/WindowGeometry";
    const QString UI_WINDOW_STATE = "UI/WindowState";
    const QString UI_THEME = "UI/Theme";
    
    // Dosya
    const QString FILE_LAST_DIRECTORY = "File/LastDirectory";
    const QString FILE_RECENT_FILES = "File/RecentFiles";
}

#endif // SETTINGS_H 