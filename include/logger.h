#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger : public QObject
{
    Q_OBJECT

public:
    static Logger* instance();
    
    // Logging fonksiyonları
    void log(LogLevel level, const QString &message, const QString &category = "");
    void debug(const QString &message, const QString &category = "");
    void info(const QString &message, const QString &category = "");
    void warning(const QString &message, const QString &category = "");
    void error(const QString &message, const QString &category = "");
    void critical(const QString &message, const QString &category = "");
    
    // Dosya yönetimi
    void setLogFile(const QString &filename);
    void setMaxFileSize(qint64 maxSize);
    void setMaxLogFiles(int maxFiles);
    void rotateLogFiles();
    
    // Filtreleme
    void setLogLevel(LogLevel level);
    void setCategoryFilter(const QStringList &categories);
    void enableCategory(const QString &category, bool enabled = true);
    
    // Ayarlar
    void setTimestampFormat(const QString &format);
    void setIncludeTimestamp(bool include);
    void setIncludeCategory(bool include);
    void setIncludeLevel(bool include);
    
    // Yardımcı fonksiyonlar
    QString levelToString(LogLevel level) const;
    LogLevel stringToLevel(const QString &level) const;
    QString getCurrentLogFile() const;
    qint64 getCurrentFileSize() const;
    
    // Log temizleme
    void clearLog();
    void exportLog(const QString &filename);
    
signals:
    void messageLogged(LogLevel level, const QString &message, const QString &category);
    void logFileChanged(const QString &filename);
    void logFileRotated(const QString &oldFile, const QString &newFile);

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();
    
    static Logger* m_instance;
    
    QFile *logFile;
    QTextStream *logStream;
    QMutex logMutex;
    
    LogLevel currentLogLevel;
    QStringList enabledCategories;
    QStringList categoryFilter;
    
    QString logFilename;
    qint64 maxFileSize;
    int maxLogFiles;
    
    QString timestampFormat;
    bool includeTimestamp;
    bool includeCategory;
    bool includeLevel;
    
    void writeToFile(const QString &message);
    void writeToConsole(const QString &message);
    bool shouldLog(LogLevel level, const QString &category) const;
    QString formatMessage(LogLevel level, const QString &message, const QString &category) const;
    void checkFileSize();
    void backupLogFile();
};

// Kolay kullanım için makrolar
#define LOG_DEBUG(msg, cat) Logger::instance()->debug(msg, cat)
#define LOG_INFO(msg, cat) Logger::instance()->info(msg, cat)
#define LOG_WARNING(msg, cat) Logger::instance()->warning(msg, cat)
#define LOG_ERROR(msg, cat) Logger::instance()->error(msg, cat)
#define LOG_CRITICAL(msg, cat) Logger::instance()->critical(msg, cat)

// Kategori sabitleri
namespace LogCategories {
    const QString MAIN = "Main";
    const QString UI = "UI";
    const QString SERIAL = "Serial";
    const QString GCODE = "GCode";
    const QString AXIS = "Axis";
    const QString SIMULATION = "Simulation";
    const QString SETTINGS = "Settings";
    const QString EMERGENCY = "Emergency";
}

#endif // LOGGER_H 