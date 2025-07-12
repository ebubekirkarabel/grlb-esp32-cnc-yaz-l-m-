#ifndef GCODEPARSER_H
#define GCODEPARSER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QQueue>

struct GCodeCommand {
    QString originalLine;
    QString command;
    QMap<QChar, double> parameters;
    int lineNumber;
    bool isValid;
    QString errorMessage;
    // Yeni alanlar
    double estimatedTime;
    double distance;
    bool requiresSlowdown;
};

struct LookAheadBuffer {
    QQueue<GCodeCommand> commands;
    int maxBufferSize;
    double corneringSpeed;
    double acceleration;
};

class GCodeParser : public QObject
{
    Q_OBJECT

public:
    explicit GCodeParser(QObject *parent = nullptr);
    
    // Ana parsing fonksiyonları
    QVector<GCodeCommand> parseFile(const QString &content);
    GCodeCommand parseLine(const QString &line, int lineNumber = 0);
    bool validateCommand(GCodeCommand &command);
    
    // Yeni: Look-ahead ve optimizasyon
    void enableLookAhead(bool enabled);
    void setLookAheadBufferSize(int size);
    void setCorneringSpeed(double speed);
    void setAcceleration(double acceleration);
    QVector<GCodeCommand> optimizeCommands(const QVector<GCodeCommand> &commands);
    double calculateCorneringSpeed(const GCodeCommand &prev, const GCodeCommand &current, const GCodeCommand &next);
    
    // Yardımcı fonksiyonlar
    QStringList getSupportedCommands() const;
    QString getCommandDescription(const QString &command) const;
    double getParameter(const GCodeCommand &command, QChar param, double defaultValue = 0.0) const;
    
    // Hata yönetimi
    QStringList getErrors() const;
    void clearErrors();
    
    // Yeni: İstatistikler
    double getTotalEstimatedTime() const;
    double getTotalDistance() const;
    int getOptimizationCount() const;

signals:
    void parsingProgress(int current, int total);
    void parsingError(int line, const QString &error);
    void parsingCompleted(int totalCommands);
    void optimizationCompleted(int optimizedCommands, double timeSaved);

private:
    QStringList supportedCommands;
    QStringList errors;
    LookAheadBuffer lookAheadBuffer;
    bool lookAheadEnabled;
    double totalEstimatedTime;
    double totalDistance;
    int optimizationCount;
    
    void initializeSupportedCommands();
    QString extractCommand(const QString &line);
    QMap<QChar, double> extractParameters(const QString &line);
    bool isComment(const QString &line);
    QString removeComments(const QString &line);
    bool validateGCommand(GCodeCommand &command);
    bool validateMCommand(GCodeCommand &command);
    
    // Yeni yardımcı fonksiyonlar
    double calculateCommandTime(const GCodeCommand &command);
    double calculateCommandDistance(const GCodeCommand &command);
    bool needsCorneringSlowdown(const GCodeCommand &prev, const GCodeCommand &current, const GCodeCommand &next);
    double calculateOptimalSpeed(const GCodeCommand &command, double corneringSpeed);
};

#endif // GCODEPARSER_H 