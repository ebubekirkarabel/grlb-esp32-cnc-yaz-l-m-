#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

// Log seviyeleri
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

// Log kategorileri
enum class LogCategory {
    General,
    GCode,
    Communication,
    Machine,
    Safety,
    Performance,
    User,
    System
};

class Logger : public QObject
{
    Q_OBJECT

public:
    static Logger* instance();
    
    // Logging fonksiyonları
    void log(LogLevel level, LogCategory category, const QString &message);
    void debug(LogCategory category, const QString &message);
    void info(LogCategory category, const QString &message);
    void warning(LogCategory category, const QString &message);
    void error(LogCategory category, const QString &message);
    void critical(LogCategory category, const QString &message);
    
    // Log yönetimi
    void setLogLevel(LogLevel level);
    void setLogFile(const QString &filePath);
    void enableConsoleOutput(bool enable);
    void enableFileOutput(bool enable);
    
    // Log analizi (gelecekteki özellikler için)
    QString getLogsByCategory(LogCategory category, int maxLines = 100);
    QString getLogsByLevel(LogLevel level, int maxLines = 100);
    QString getLogsByTimeRange(const QDateTime &start, const QDateTime &end);
    
    // Performans metrikleri
    void logPerformance(const QString &operation, qint64 duration);
    void logMachineStatus(const QString &status);
    void logUserAction(const QString &action);

signals:
    void logMessageAdded(LogLevel level, LogCategory category, const QString &message);

private:
    Logger(QObject *parent = nullptr);
    ~Logger();
    
    static Logger* m_instance;
    QFile m_logFile;
    QTextStream m_logStream;
    LogLevel m_currentLevel;
    bool m_consoleEnabled;
    bool m_fileEnabled;
    QMutex m_mutex;
    
    QString levelToString(LogLevel level);
    QString categoryToString(LogCategory category);
    QString formatMessage(LogLevel level, LogCategory category, const QString &message);
};

// Kolay kullanım için makrolar
#define LOG_DEBUG(category, message) Logger::instance()->debug(category, message)
#define LOG_INFO(category, message) Logger::instance()->info(category, message)
#define LOG_WARNING(category, message) Logger::instance()->warning(category, message)
#define LOG_ERROR(category, message) Logger::instance()->error(category, message)
#define LOG_CRITICAL(category, message) Logger::instance()->critical(category, message)

#define LOG_PERFORMANCE(operation, duration) Logger::instance()->logPerformance(operation, duration)
#define LOG_MACHINE_STATUS(status) Logger::instance()->logMachineStatus(status)
#define LOG_USER_ACTION(action) Logger::instance()->logUserAction(action)

#endif // LOGGER_H 