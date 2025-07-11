#include "logger.h"
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QTextStream>

Logger* Logger::m_instance = nullptr;

Logger* Logger::instance()
{
    if (!m_instance) {
        m_instance = new Logger();
    }
    return m_instance;
}

Logger::Logger(QObject *parent)
    : QObject(parent)
    , logFile(nullptr)
    , logStream(nullptr)
    , currentLogLevel(LogLevel::Info)
    , maxFileSize(10 * 1024 * 1024) // 10MB
    , maxLogFiles(5)
    , timestampFormat("yyyy-MM-dd hh:mm:ss.zzz")
    , includeTimestamp(true)
    , includeCategory(true)
    , includeLevel(true)
{
    setLogFile("cnc_controller.log");
}

Logger::~Logger()
{
    if (logStream) {
        delete logStream;
    }
    if (logFile) {
        delete logFile;
    }
}

void Logger::log(LogLevel level, const QString &message, const QString &category)
{
    if (!shouldLog(level, category)) {
        return;
    }
    
    QString formattedMessage = formatMessage(level, message, category);
    
    writeToFile(formattedMessage);
    writeToConsole(formattedMessage);
    
    emit messageLogged(level, message, category);
}

void Logger::debug(const QString &message, const QString &category)
{
    log(LogLevel::Debug, message, category);
}

void Logger::info(const QString &message, const QString &category)
{
    log(LogLevel::Info, message, category);
}

void Logger::warning(const QString &message, const QString &category)
{
    log(LogLevel::Warning, message, category);
}

void Logger::error(const QString &message, const QString &category)
{
    log(LogLevel::Error, message, category);
}

void Logger::critical(const QString &message, const QString &category)
{
    log(LogLevel::Critical, message, category);
}

void Logger::setLogFile(const QString &filename)
{
    QMutexLocker locker(&logMutex);
    
    if (logStream) {
        delete logStream;
        logStream = nullptr;
    }
    if (logFile) {
        delete logFile;
        logFile = nullptr;
    }
    
    logFilename = filename;
    logFile = new QFile(filename);
    
    if (logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        logStream = new QTextStream(logFile);
        emit logFileChanged(filename);
    } else {
        qDebug() << "Log dosyası açılamadı:" << filename;
    }
}

void Logger::setMaxFileSize(qint64 maxSize)
{
    maxFileSize = maxSize;
}

void Logger::setMaxLogFiles(int maxFiles)
{
    maxLogFiles = maxFiles;
}

void Logger::rotateLogFiles()
{
    if (!logFile || !logFile->exists()) {
        return;
    }
    
    if (logFile->size() < maxFileSize) {
        return;
    }
    
    backupLogFile();
}

void Logger::setLogLevel(LogLevel level)
{
    currentLogLevel = level;
}

void Logger::setCategoryFilter(const QStringList &categories)
{
    categoryFilter = categories;
}

void Logger::enableCategory(const QString &category, bool enabled)
{
    if (enabled) {
        if (!enabledCategories.contains(category)) {
            enabledCategories.append(category);
        }
    } else {
        enabledCategories.removeAll(category);
    }
}

void Logger::setTimestampFormat(const QString &format)
{
    timestampFormat = format;
}

void Logger::setIncludeTimestamp(bool include)
{
    includeTimestamp = include;
}

void Logger::setIncludeCategory(bool include)
{
    includeCategory = include;
}

void Logger::setIncludeLevel(bool include)
{
    includeLevel = include;
}

QString Logger::levelToString(LogLevel level) const
{
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

LogLevel Logger::stringToLevel(const QString &level) const
{
    QString upperLevel = level.toUpper();
    if (upperLevel == "DEBUG") return LogLevel::Debug;
    if (upperLevel == "INFO") return LogLevel::Info;
    if (upperLevel == "WARNING") return LogLevel::Warning;
    if (upperLevel == "ERROR") return LogLevel::Error;
    if (upperLevel == "CRITICAL") return LogLevel::Critical;
    return LogLevel::Info;
}

QString Logger::getCurrentLogFile() const
{
    return logFilename;
}

qint64 Logger::getCurrentFileSize() const
{
    if (logFile) {
        return logFile->size();
    }
    return 0;
}

void Logger::clearLog()
{
    QMutexLocker locker(&logMutex);
    
    if (logStream) {
        delete logStream;
        logStream = nullptr;
    }
    if (logFile) {
        logFile->close();
        logFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
        logStream = new QTextStream(logFile);
    }
}

void Logger::exportLog(const QString &filename)
{
    if (!logFile || !logFile->exists()) {
        return;
    }
    
    QFile::copy(logFilename, filename);
}

void Logger::writeToFile(const QString &message)
{
    QMutexLocker locker(&logMutex);
    
    if (logStream) {
        *logStream << message << Qt::endl;
        logStream->flush();
        
        checkFileSize();
    }
}

void Logger::writeToConsole(const QString &message)
{
    qDebug().noquote() << message;
}

bool Logger::shouldLog(LogLevel level, const QString &category) const
{
    // Log seviyesi kontrolü
    if (level < currentLogLevel) {
        return false;
    }
    
    // Kategori filtresi kontrolü
    if (!categoryFilter.isEmpty() && !categoryFilter.contains(category)) {
        return false;
    }
    
    // Etkin kategori kontrolü
    if (!enabledCategories.isEmpty() && !enabledCategories.contains(category)) {
        return false;
    }
    
    return true;
}

QString Logger::formatMessage(LogLevel level, const QString &message, const QString &category) const
{
    QString formattedMessage;
    
    if (includeTimestamp) {
        formattedMessage += QDateTime::currentDateTime().toString(timestampFormat) + " ";
    }
    
    if (includeLevel) {
        formattedMessage += "[" + levelToString(level) + "] ";
    }
    
    if (includeCategory && !category.isEmpty()) {
        formattedMessage += "[" + category + "] ";
    }
    
    formattedMessage += message;
    
    return formattedMessage;
}

void Logger::checkFileSize()
{
    if (logFile && logFile->size() >= maxFileSize) {
        backupLogFile();
    }
}

void Logger::backupLogFile()
{
    if (!logFile || !logFile->exists()) {
        return;
    }
    
    QString oldFilename = logFilename;
    QString backupFilename = oldFilename + ".1";
    
    // Eski yedek dosyaları kaydır
    for (int i = maxLogFiles - 1; i > 0; --i) {
        QString currentBackup = oldFilename + "." + QString::number(i);
        QString nextBackup = oldFilename + "." + QString::number(i + 1);
        
        if (QFile::exists(currentBackup)) {
            if (i == maxLogFiles - 1) {
                QFile::remove(currentBackup);
            } else {
                QFile::rename(currentBackup, nextBackup);
            }
        }
    }
    
    // Mevcut log dosyasını yedekle
    if (QFile::exists(oldFilename)) {
        QFile::rename(oldFilename, backupFilename);
    }
    
    // Yeni log dosyası oluştur
    if (logStream) {
        delete logStream;
        logStream = nullptr;
    }
    if (logFile) {
        delete logFile;
        logFile = nullptr;
    }
    
    logFile = new QFile(oldFilename);
    if (logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        logStream = new QTextStream(logFile);
        emit logFileRotated(backupFilename, oldFilename);
    }
} 