#include "settings.h"
#include <QDebug>
#include <QDir>

Settings::Settings(QObject *parent)
    : QObject(parent)
    , settings(new QSettings("CNCController", "CNCController", this))
{
    initializeDefaults();
}

Settings::~Settings()
{
    saveSettings();
}

void Settings::setValue(const QString &key, const QVariant &value)
{
    settings->setValue(key, value);
    emit settingsChanged(key, value);
}

QVariant Settings::getValue(const QString &key, const QVariant &defaultValue) const
{
    return settings->value(key, defaultValue);
}

void Settings::setSerialPort(const QString &port)
{
    setValue(SettingsKeys::SERIAL_PORT, port);
}

QString Settings::getSerialPort() const
{
    return getValue(SettingsKeys::SERIAL_PORT, "COM1").toString();
}

void Settings::setBaudRate(int baudRate)
{
    setValue(SettingsKeys::SERIAL_BAUD_RATE, baudRate);
}

int Settings::getBaudRate() const
{
    return getValue(SettingsKeys::SERIAL_BAUD_RATE, 115200).toInt();
}

void Settings::setDataBits(int dataBits)
{
    setValue(SettingsKeys::SERIAL_DATA_BITS, dataBits);
}

int Settings::getDataBits() const
{
    return getValue(SettingsKeys::SERIAL_DATA_BITS, 8).toInt();
}

void Settings::setParity(int parity)
{
    setValue(SettingsKeys::SERIAL_PARITY, parity);
}

int Settings::getParity() const
{
    return getValue(SettingsKeys::SERIAL_PARITY, 0).toInt();
}

void Settings::setStopBits(int stopBits)
{
    setValue(SettingsKeys::SERIAL_STOP_BITS, stopBits);
}

int Settings::getStopBits() const
{
    return getValue(SettingsKeys::SERIAL_STOP_BITS, 1).toInt();
}

void Settings::setAxisLimits(char axis, double minLimit, double maxLimit)
{
    QString minKey = getAxisKey(axis, "MinLimit");
    QString maxKey = getAxisKey(axis, "MaxLimit");
    
    setValue(minKey, minLimit);
    setValue(maxKey, maxLimit);
}

double Settings::getAxisMinLimit(char axis) const
{
    QString key = getAxisKey(axis, "MinLimit");
    switch (axis) {
        case 'X': return getValue(key, -50.0).toDouble();
        case 'Y': return getValue(key, -50.0).toDouble();
        case 'Z': return getValue(key, -15.0).toDouble();
        default: return -50.0;
    }
}

double Settings::getAxisMaxLimit(char axis) const
{
    QString key = getAxisKey(axis, "MaxLimit");
    switch (axis) {
        case 'X': return getValue(key, 50.0).toDouble();
        case 'Y': return getValue(key, 50.0).toDouble();
        case 'Z': return getValue(key, 15.0).toDouble();
        default: return 50.0;
    }
}

void Settings::setAxisEnabled(char axis, bool enabled)
{
    QString key = getAxisKey(axis, "Enabled");
    setValue(key, enabled);
}

bool Settings::isAxisEnabled(char axis) const
{
    QString key = getAxisKey(axis, "Enabled");
    return getValue(key, true).toBool();
}

void Settings::setJogStep(double step)
{
    setValue(SettingsKeys::JOG_STEP, step);
}

double Settings::getJogStep() const
{
    return getValue(SettingsKeys::JOG_STEP, 1.0).toDouble();
}

void Settings::setJogSpeed(double speed)
{
    setValue(SettingsKeys::JOG_SPEED, speed);
}

double Settings::getJogSpeed() const
{
    return getValue(SettingsKeys::JOG_SPEED, 1000.0).toDouble();
}

void Settings::setJogAcceleration(double acceleration)
{
    setValue(SettingsKeys::JOG_ACCELERATION, acceleration);
}

double Settings::getJogAcceleration() const
{
    return getValue(SettingsKeys::JOG_ACCELERATION, 1.0).toDouble();
}

void Settings::setDefaultFeedRate(double feedRate)
{
    setValue(SettingsKeys::GCODE_DEFAULT_FEED_RATE, feedRate);
}

double Settings::getDefaultFeedRate() const
{
    return getValue(SettingsKeys::GCODE_DEFAULT_FEED_RATE, 1000.0).toDouble();
}

void Settings::setMaxFeedRate(double maxFeedRate)
{
    setValue(SettingsKeys::GCODE_MAX_FEED_RATE, maxFeedRate);
}

double Settings::getMaxFeedRate() const
{
    return getValue(SettingsKeys::GCODE_MAX_FEED_RATE, 5000.0).toDouble();
}

void Settings::setGCodeEditorFont(const QString &fontFamily, int fontSize)
{
    setValue(SettingsKeys::GCODE_EDITOR_FONT_FAMILY, fontFamily);
    setValue(SettingsKeys::GCODE_EDITOR_FONT_SIZE, fontSize);
}

QString Settings::getGCodeEditorFontFamily() const
{
    return getValue(SettingsKeys::GCODE_EDITOR_FONT_FAMILY, "Consolas").toString();
}

int Settings::getGCodeEditorFontSize() const
{
    return getValue(SettingsKeys::GCODE_EDITOR_FONT_SIZE, 10).toInt();
}

void Settings::setWindowGeometry(const QByteArray &geometry)
{
    setValue(SettingsKeys::UI_WINDOW_GEOMETRY, geometry);
}

QByteArray Settings::getWindowGeometry() const
{
    return getValue(SettingsKeys::UI_WINDOW_GEOMETRY).toByteArray();
}

void Settings::setWindowState(const QByteArray &state)
{
    setValue(SettingsKeys::UI_WINDOW_STATE, state);
}

QByteArray Settings::getWindowState() const
{
    return getValue(SettingsKeys::UI_WINDOW_STATE).toByteArray();
}

void Settings::setTheme(const QString &theme)
{
    setValue(SettingsKeys::UI_THEME, theme);
}

QString Settings::getTheme() const
{
    return getValue(SettingsKeys::UI_THEME, "default").toString();
}

void Settings::setLastDirectory(const QString &directory)
{
    setValue(SettingsKeys::FILE_LAST_DIRECTORY, directory);
}

QString Settings::getLastDirectory() const
{
    return getValue(SettingsKeys::FILE_LAST_DIRECTORY, QDir::homePath()).toString();
}

void Settings::setRecentFiles(const QStringList &files)
{
    setValue(SettingsKeys::FILE_RECENT_FILES, files);
}

QStringList Settings::getRecentFiles() const
{
    return getValue(SettingsKeys::FILE_RECENT_FILES, QStringList()).toStringList();
}

void Settings::addRecentFile(const QString &file)
{
    QStringList recentFiles = getRecentFiles();
    
    // Dosyayı listeden çıkar (varsa)
    recentFiles.removeAll(file);
    
    // Dosyayı başa ekle
    recentFiles.prepend(file);
    
    // Maksimum 10 dosya tut
    while (recentFiles.size() > 10) {
        recentFiles.removeLast();
    }
    
    setRecentFiles(recentFiles);
}

void Settings::saveSettings()
{
    settings->sync();
    emit settingsSaved();
}

void Settings::loadSettings()
{
    // Ayarlar otomatik olarak yüklenir
    emit settingsLoaded();
}

void Settings::resetToDefaults()
{
    settings->clear();
    initializeDefaults();
    emit settingsLoaded();
}

bool Settings::exportSettings(const QString &filename)
{
    QSettings exportSettings(filename, QSettings::IniFormat);
    
    // Tüm ayarları kopyala
    QStringList keys = settings->allKeys();
    for (const QString &key : keys) {
        exportSettings.setValue(key, settings->value(key));
    }
    
    return true;
}

bool Settings::importSettings(const QString &filename)
{
    QSettings importSettings(filename, QSettings::IniFormat);
    
    // Tüm ayarları kopyala
    QStringList keys = importSettings.allKeys();
    for (const QString &key : keys) {
        settings->setValue(key, importSettings.value(key));
    }
    
    emit settingsLoaded();
    return true;
}

void Settings::initializeDefaults()
{
    // Sadece varsayılan değerler yoksa ayarla
    if (!settings->contains(SettingsKeys::SERIAL_BAUD_RATE)) {
        setBaudRate(115200);
    }
    if (!settings->contains(SettingsKeys::SERIAL_DATA_BITS)) {
        setDataBits(8);
    }
    if (!settings->contains(SettingsKeys::SERIAL_PARITY)) {
        setParity(0);
    }
    if (!settings->contains(SettingsKeys::SERIAL_STOP_BITS)) {
        setStopBits(1);
    }
    
    if (!settings->contains(SettingsKeys::JOG_STEP)) {
        setJogStep(1.0);
    }
    if (!settings->contains(SettingsKeys::JOG_SPEED)) {
        setJogSpeed(1000.0);
    }
    if (!settings->contains(SettingsKeys::JOG_ACCELERATION)) {
        setJogAcceleration(1.0);
    }
    
    if (!settings->contains(SettingsKeys::GCODE_DEFAULT_FEED_RATE)) {
        setDefaultFeedRate(1000.0);
    }
    if (!settings->contains(SettingsKeys::GCODE_MAX_FEED_RATE)) {
        setMaxFeedRate(5000.0);
    }
    if (!settings->contains(SettingsKeys::GCODE_EDITOR_FONT_FAMILY)) {
        setGCodeEditorFont("Consolas", 10);
    }
    
    if (!settings->contains(SettingsKeys::UI_THEME)) {
        setTheme("default");
    }
}

QString Settings::getAxisKey(char axis, const QString &property) const
{
    return QString("Axis/%1/%2").arg(axis).arg(property);
} 