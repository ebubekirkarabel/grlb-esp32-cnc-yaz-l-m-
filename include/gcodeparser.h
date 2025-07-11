#ifndef GCODEPARSER_H
#define GCODEPARSER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct GCodeCommand {
    QString originalLine;
    QString command;
    QMap<QChar, double> parameters;
    int lineNumber;
    bool isValid;
    QString errorMessage;
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
    
    // Yardımcı fonksiyonlar
    QStringList getSupportedCommands() const;
    QString getCommandDescription(const QString &command) const;
    double getParameter(const GCodeCommand &command, QChar param, double defaultValue = 0.0) const;
    
    // Hata yönetimi
    QStringList getErrors() const;
    void clearErrors();
    
signals:
    void parsingProgress(int current, int total);
    void parsingError(int line, const QString &error);
    void parsingCompleted(int totalCommands);

private:
    QStringList supportedCommands;
    QStringList errors;
    
    void initializeSupportedCommands();
    QString extractCommand(const QString &line);
    QMap<QChar, double> extractParameters(const QString &line);
    bool isComment(const QString &line);
    QString removeComments(const QString &line);
    bool validateGCommand(GCodeCommand &command);
    bool validateMCommand(GCodeCommand &command);
};

#endif // GCODEPARSER_H 