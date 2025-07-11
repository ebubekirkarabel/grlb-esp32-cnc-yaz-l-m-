#include "gcodeparser.h"
#include <QDebug>
#include <QRegularExpression>
#include <Qt>

GCodeParser::GCodeParser(QObject *parent)
    : QObject(parent)
{
    initializeSupportedCommands();
}

QVector<GCodeCommand> GCodeParser::parseFile(const QString &content)
{
    clearErrors();
    QVector<GCodeCommand> commands;
    QStringList lines = content.split('\n', Qt::SkipEmptyParts);
    
    for (int i = 0; i < lines.size(); ++i) {
        GCodeCommand command = parseLine(lines[i], i + 1);
        commands.append(command);
        
        emit parsingProgress(i + 1, lines.size());
        
        if (!command.isValid) {
            emit parsingError(i + 1, command.errorMessage);
        }
    }
    
    emit parsingCompleted(commands.size());
    return commands;
}

GCodeCommand GCodeParser::parseLine(const QString &line, int lineNumber)
{
    GCodeCommand command;
    command.originalLine = line.trimmed();
    command.lineNumber = lineNumber;
    command.isValid = false;
    
    // Boş satır veya sadece yorum
    if (command.originalLine.isEmpty() || isComment(command.originalLine)) {
        command.isValid = true; // Boş satırlar geçerli
        return command;
    }
    
    // Yorumları kaldır
    QString cleanLine = removeComments(command.originalLine);
    if (cleanLine.isEmpty()) {
        command.isValid = true;
        return command;
    }
    
    // Komutu çıkar
    command.command = extractCommand(cleanLine);
    if (command.command.isEmpty()) {
        command.errorMessage = "Geçersiz komut formatı";
        return command;
    }
    
    // Parametreleri çıkar
    command.parameters = extractParameters(cleanLine);
    
    // Komutu doğrula
    command.isValid = validateCommand(command);
    
    return command;
}

bool GCodeParser::validateCommand(GCodeCommand &command)
{
    if (command.command.isEmpty()) {
        return true; // Boş komutlar geçerli
    }
    
    // Desteklenen komutları kontrol et
    if (!supportedCommands.contains(command.command)) {
        command.errorMessage = QString("Desteklenmeyen komut: %1").arg(command.command);
        return false;
    }
    
    // G-code komutları için özel doğrulama
    if (command.command.startsWith('G')) {
        return validateGCommand(command);
    }
    
    // M-code komutları için özel doğrulama
    if (command.command.startsWith('M')) {
        return validateMCommand(command);
    }
    
    return true;
}

QStringList GCodeParser::getSupportedCommands() const
{
    return supportedCommands;
}

QString GCodeParser::getCommandDescription(const QString &command) const
{
    QMap<QString, QString> descriptions;
    descriptions["G0"] = "Hızlı hareket";
    descriptions["G1"] = "Doğrusal hareket";
    descriptions["G2"] = "Saat yönünde dairesel hareket";
    descriptions["G3"] = "Saat yönünün tersine dairesel hareket";
    descriptions["G20"] = "İnç birimi";
    descriptions["G21"] = "Milimetre birimi";
    descriptions["G28"] = "Ana pozisyona dön";
    descriptions["G90"] = "Mutlak koordinat";
    descriptions["G91"] = "Göreceli koordinat";
    descriptions["M0"] = "Programı durdur";
    descriptions["M1"] = "Koşullu durdurma";
    descriptions["M2"] = "Programı sonlandır";
    descriptions["M3"] = "Spindli saat yönünde çalıştır";
    descriptions["M4"] = "Spindli saat yönünün tersine çalıştır";
    descriptions["M5"] = "Spindli durdur";
    descriptions["M6"] = "Takım değiştir";
    descriptions["M8"] = "Soğutma sıvısını aç";
    descriptions["M9"] = "Soğutma sıvısını kapat";
    
    return descriptions.value(command, "Bilinmeyen komut");
}

double GCodeParser::getParameter(const GCodeCommand &command, QChar param, double defaultValue) const
{
    return command.parameters.value(param, defaultValue);
}

QStringList GCodeParser::getErrors() const
{
    return errors;
}

void GCodeParser::clearErrors()
{
    errors.clear();
}

void GCodeParser::initializeSupportedCommands()
{
    supportedCommands = {
        "G0", "G1", "G2", "G3", "G20", "G21", "G28", "G90", "G91",
        "M0", "M1", "M2", "M3", "M4", "M5", "M6", "M8", "M9"
    };
}

QString GCodeParser::extractCommand(const QString &line)
{
    QRegularExpression cmdRegex(R"(([GM]\d+))");
    QRegularExpressionMatch match = cmdRegex.match(line);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return QString();
}

QMap<QChar, double> GCodeParser::extractParameters(const QString &line)
{
    QMap<QChar, double> params;
    QRegularExpression paramRegex(R"(([XYZIJKFSR])(-?\d*\.?\d+))");
    QRegularExpressionMatchIterator it = paramRegex.globalMatch(line);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QChar param = match.captured(1)[0];
        double value = match.captured(2).toDouble();
        params[param] = value;
    }
    
    return params;
}

bool GCodeParser::isComment(const QString &line)
{
    return line.trimmed().startsWith(';') || line.trimmed().startsWith('(');
}

QString GCodeParser::removeComments(const QString &line)
{
    QString result = line;
    
    // Parantez içi yorumları kaldır
    QRegularExpression parenComment(R"(\([^)]*\))");
    result.remove(parenComment);
    
    // Noktalı virgül sonrası yorumları kaldır
    int semicolonIndex = result.indexOf(';');
    if (semicolonIndex != -1) {
        result = result.left(semicolonIndex);
    }
    
    return result.trimmed();
}

bool GCodeParser::validateGCommand(GCodeCommand &command)
{
    QString cmd = command.command;
    
    if (cmd == "G0" || cmd == "G1") {
        // G0/G1 için X, Y, Z, F parametreleri geçerli
        QList<QChar> validParams = {'X', 'Y', 'Z', 'F'};
        for (auto it = command.parameters.begin(); it != command.parameters.end(); ++it) {
            if (!validParams.contains(it.key())) {
                command.errorMessage = QString("G0/G1 için geçersiz parametre: %1").arg(it.key());
                return false;
            }
        }
    }
    else if (cmd == "G2" || cmd == "G3") {
        // G2/G3 için X, Y, Z, I, J, K, F parametreleri geçerli
        QList<QChar> validParams = {'X', 'Y', 'Z', 'I', 'J', 'K', 'F'};
        for (auto it = command.parameters.begin(); it != command.parameters.end(); ++it) {
            if (!validParams.contains(it.key())) {
                command.errorMessage = QString("G2/G3 için geçersiz parametre: %1").arg(it.key());
                return false;
            }
        }
    }
    
    return true;
}

bool GCodeParser::validateMCommand(GCodeCommand &command)
{
    QString cmd = command.command;
    
    if (cmd == "M3" || cmd == "M4") {
        // M3/M4 için S parametresi geçerli
        QList<QChar> validParams = {'S'};
        for (auto it = command.parameters.begin(); it != command.parameters.end(); ++it) {
            if (!validParams.contains(it.key())) {
                command.errorMessage = QString("M3/M4 için geçersiz parametre: %1").arg(it.key());
                return false;
            }
        }
    }
    
    return true;
} 