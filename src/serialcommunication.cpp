#include "serialcommunication.h"
#include <QDebug>
#include <QSerialPortInfo>

SerialCommunication::SerialCommunication(QObject *parent)
    : QObject(parent)
    , serialPort(new QSerialPort(this))
    , timeoutTimer(new QTimer(this))
    , safetyTimer(new QTimer(this))
    , statusTimer(new QTimer(this))
    , isProcessingCommand(false)
    , limitSwitchMonitoringEnabled(false)
    , homingInProgress(false)
    , homingEnabled(true)
    , safetyChecksEnabled(true)
    , safetyTimeout(10000) // 10 saniye
{
    // Timer ayarları
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(5000); // 5 saniye timeout
    
    safetyTimer->setSingleShot(true);
    safetyTimer->setInterval(safetyTimeout);
    
    statusTimer->setInterval(100); // 100ms = 10Hz status polling
    
    // Limit switch durumunu başlat
    limitSwitchStatus = {
        LimitSwitchState::NotTriggered,
        LimitSwitchState::NotTriggered,
        LimitSwitchState::NotTriggered,
        LimitSwitchState::NotTriggered,
        LimitSwitchState::NotTriggered,
        LimitSwitchState::NotTriggered,
        false
    };
    
    // Spindle durumunu başlat
    spindleStatus = {
        SpindleState::Off,
        0.0,
        0.0,
        false,
        false
    };
    
    // Seri port bağlantıları
    connect(serialPort, &QSerialPort::readyRead, this, &SerialCommunication::handleReadyRead);
    connect(serialPort, &QSerialPort::errorOccurred, this, &SerialCommunication::handleError);
    connect(timeoutTimer, &QTimer::timeout, this, &SerialCommunication::handleTimeout);
    connect(safetyTimer, &QTimer::timeout, this, &SerialCommunication::handleSafetyTimeout);
    connect(statusTimer, &QTimer::timeout, this, &SerialCommunication::requestStatus);
}

SerialCommunication::~SerialCommunication()
{
    disconnectFromDevice();
}

bool SerialCommunication::connectToDevice(const QString &portName, int baudRate)
{
    if (serialPort->isOpen()) {
        disconnectFromDevice();
    }
    
    serialPort->setPortName(portName);
    serialPort->setBaudRate(baudRate);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    
    if (serialPort->open(QIODevice::ReadWrite)) {
        emit connected();
        
        // Bağlantı sonrası güvenlik kontrollerini başlat
        if (safetyChecksEnabled) {
            startStatusMonitoring();
            requestLimitSwitchStatus();
        }
        
        return true;
    } else {
        emit errorOccurred("Bağlantı hatası: " + serialPort->errorString());
        return false;
    }
}

void SerialCommunication::disconnectFromDevice()
{
    if (serialPort->isOpen()) {
        stopStatusMonitoring();
        serialPort->close();
        emit disconnected();
    }
    
    // Bekleyen komutları temizle
    commandQueue.clear();
    isProcessingCommand = false;
    timeoutTimer->stop();
    safetyTimer->stop();
}

bool SerialCommunication::isConnected() const
{
    return serialPort->isOpen();
}

// YENİ: Hardware limit switch kontrolü
void SerialCommunication::requestLimitSwitchStatus()
{
    if (isConnected()) {
        sendCommand("$I"); // GRBL limit switch durumu sorgusu
    }
}

LimitSwitchStatus SerialCommunication::getLimitSwitchStatus() const
{
    return limitSwitchStatus;
}

bool SerialCommunication::isAnyLimitSwitchTriggered() const
{
    return limitSwitchStatus.anyTriggered;
}

void SerialCommunication::enableLimitSwitchMonitoring(bool enabled)
{
    limitSwitchMonitoringEnabled = enabled;
    if (enabled && isConnected()) {
        startStatusMonitoring();
    } else {
        stopStatusMonitoring();
    }
}

// YENİ: Spindle kontrolü
bool SerialCommunication::setSpindleSpeed(double speed)
{
    if (!isConnected()) {
        emit errorOccurred("Seri port bağlı değil");
        return false;
    }
    
    spindleStatus.targetSpeed = speed;
    QString command = QString("S%1").arg(static_cast<int>(speed));
    return sendCommand(command);
}

bool SerialCommunication::startSpindle(bool clockwise)
{
    if (!isConnected()) {
        emit errorOccurred("Seri port bağlı değil");
        return false;
    }
    
    QString command = clockwise ? "M3" : "M4";
    bool result = sendCommand(command);
    
    if (result) {
        spindleStatus.state = clockwise ? SpindleState::Clockwise : SpindleState::CounterClockwise;
        emit spindleStateChanged(spindleStatus.state);
    }
    
    return result;
}

bool SerialCommunication::stopSpindle()
{
    if (!isConnected()) {
        emit errorOccurred("Seri port bağlı değil");
        return false;
    }
    
    bool result = sendCommand("M5");
    
    if (result) {
        spindleStatus.state = SpindleState::Off;
        spindleStatus.speed = 0.0;
        emit spindleStateChanged(spindleStatus.state);
        emit spindleSpeedChanged(0.0);
    }
    
    return result;
}

bool SerialCommunication::setCoolant(bool on)
{
    if (!isConnected()) {
        emit errorOccurred("Seri port bağlı değil");
        return false;
    }
    
    QString command = on ? "M8" : "M9";
    bool result = sendCommand(command);
    
    if (result) {
        spindleStatus.coolantOn = on;
        emit coolantStateChanged(on);
    }
    
    return result;
}

bool SerialCommunication::setAirBlast(bool on)
{
    if (!isConnected()) {
        emit errorOccurred("Seri port bağlı değil");
        return false;
    }
    
    QString command = on ? "M7" : "M9"; // M7 = air blast, M9 = tüm sıvıları kapat
    bool result = sendCommand(command);
    
    if (result) {
        spindleStatus.airBlastOn = on;
        emit airBlastStateChanged(on);
    }
    
    return result;
}

SpindleStatus SerialCommunication::getSpindleStatus() const
{
    return spindleStatus;
}

// YENİ: Homing işlemleri
bool SerialCommunication::startHoming()
{
    if (!isConnected()) {
        emit errorOccurred("Seri port bağlı değil");
        return false;
    }
    
    if (!homingEnabled) {
        emit errorOccurred("Homing devre dışı");
        return false;
    }
    
    homingInProgress = true;
    emit homingStarted();
    
    return sendCommand("$H"); // GRBL homing komutu
}

bool SerialCommunication::startHomingAxis(char axis)
{
    if (!isConnected()) {
        emit errorOccurred("Seri port bağlı değil");
        return false;
    }
    
    if (!homingEnabled) {
        emit errorOccurred("Homing devre dışı");
        return false;
    }
    
    homingInProgress = true;
    emit homingStarted();
    
    QString command = QString("$H%1").arg(axis);
    return sendCommand(command);
}

bool SerialCommunication::isHoming() const
{
    return homingInProgress;
}

void SerialCommunication::setHomingEnabled(bool enabled)
{
    homingEnabled = enabled;
}

// YENİ: Güvenlik ayarları
void SerialCommunication::setSafetyTimeout(int milliseconds)
{
    safetyTimeout = milliseconds;
    safetyTimer->setInterval(milliseconds);
}

int SerialCommunication::getSafetyTimeout() const
{
    return safetyTimeout;
}

void SerialCommunication::enableSafetyChecks(bool enabled)
{
    safetyChecksEnabled = enabled;
    if (enabled && isConnected()) {
        startStatusMonitoring();
    } else {
        stopStatusMonitoring();
    }
}

bool SerialCommunication::areSafetyChecksEnabled() const
{
    return safetyChecksEnabled;
}

// YENİ: Yardımcı fonksiyonlar
void SerialCommunication::startStatusMonitoring()
{
    if (!statusTimer->isActive()) {
        statusTimer->start();
    }
}

void SerialCommunication::stopStatusMonitoring()
{
    if (statusTimer->isActive()) {
        statusTimer->stop();
    }
}

void SerialCommunication::checkSafetyConditions()
{
    // Limit switch kontrolü
    if (limitSwitchStatus.anyTriggered) {
        emit errorOccurred("Hardware limit switch tetiklendi!");
        sendEmergencyStop();
    }
    
    // Spindle güvenlik kontrolü
    if (spindleStatus.state != SpindleState::Off && !spindleStatus.coolantOn) {
        emit errorOccurred("Spindle çalışıyor ama soğutucu kapalı!");
    }
}

bool SerialCommunication::validateSafetyCommand(const QString &command)
{
    // Güvenlik komutlarını kontrol et
    if (command.startsWith("G0") || command.startsWith("G1")) {
        // Hareket komutları için limit switch kontrolü
        if (limitSwitchStatus.anyTriggered) {
            emit errorOccurred("Limit switch tetikli - hareket engellendi");
            return false;
        }
    }
    
    return true;
}

// YENİ: Response parsing fonksiyonları
void SerialCommunication::parseLimitSwitchResponse(const QString &response)
{
    // GRBL limit switch response parsing
    if (response.contains("Limit")) {
        // Limit switch durumunu güncelle
        bool xMinTriggered = response.contains("X-");
        bool xMaxTriggered = response.contains("X+");
        bool yMinTriggered = response.contains("Y-");
        bool yMaxTriggered = response.contains("Y+");
        bool zMinTriggered = response.contains("Z-");
        bool zMaxTriggered = response.contains("Z+");
        
        // Durumu güncelle
        limitSwitchStatus.xMin = xMinTriggered ? LimitSwitchState::Triggered : LimitSwitchState::NotTriggered;
        limitSwitchStatus.xMax = xMaxTriggered ? LimitSwitchState::Triggered : LimitSwitchState::NotTriggered;
        limitSwitchStatus.yMin = yMinTriggered ? LimitSwitchState::Triggered : LimitSwitchState::NotTriggered;
        limitSwitchStatus.yMax = yMaxTriggered ? LimitSwitchState::Triggered : LimitSwitchState::NotTriggered;
        limitSwitchStatus.zMin = zMinTriggered ? LimitSwitchState::Triggered : LimitSwitchState::NotTriggered;
        limitSwitchStatus.zMax = zMaxTriggered ? LimitSwitchState::Triggered : LimitSwitchState::NotTriggered;
        
        limitSwitchStatus.anyTriggered = xMinTriggered || xMaxTriggered || yMinTriggered || yMaxTriggered || zMinTriggered || zMaxTriggered;
        
        emit limitSwitchStatusChanged(limitSwitchStatus);
        
        // Limit switch tetiklendiyse uyarı ver
        if (limitSwitchStatus.anyTriggered) {
            emit errorOccurred("Hardware limit switch tetiklendi!");
        }
    }
}

void SerialCommunication::parseSpindleResponse(const QString &response)
{
    // Spindle durumu parsing
    if (response.contains("S")) {
        // Spindle hızı güncelleme
        QRegularExpression speedRegex(R"(S(\d+))");
        QRegularExpressionMatch match = speedRegex.match(response);
        if (match.hasMatch()) {
            double speed = match.captured(1).toDouble();
            spindleStatus.speed = speed;
            emit spindleSpeedChanged(speed);
        }
    }
}

void SerialCommunication::parseHomingResponse(const QString &response)
{
    if (response.contains("ok") && homingInProgress) {
        homingInProgress = false;
        emit homingCompleted();
    } else if (response.contains("error") && homingInProgress) {
        homingInProgress = false;
        emit homingFailed("Homing hatası: " + response);
    }
}

void SerialCommunication::handleSafetyTimeout()
{
    emit safetyTimeoutOccurred();
    emit errorOccurred("Güvenlik timeout - sistem durduruldu");
    sendEmergencyStop();
}

// Mevcut fonksiyonlar devam ediyor...
bool SerialCommunication::sendCommand(const QString &command)
{
    if (!isConnected()) {
        emit errorOccurred("Seri port bağlı değil");
        return false;
    }
    
    // Güvenlik kontrolü
    if (safetyChecksEnabled && !validateSafetyCommand(command)) {
        return false;
    }
    
    QString formattedCommand = command.trimmed() + "\n";
    commandQueue.enqueue(formattedCommand);
    
    if (!isProcessingCommand) {
        sendNextCommand();
    }
    
    return true;
}

bool SerialCommunication::sendGCodeCommand(const QString &gcode)
{
    return sendCommand(formatGCodeCommand(gcode));
}

bool SerialCommunication::sendJogCommand(char axis, double distance, double speed)
{
    return sendCommand(formatJogCommand(axis, distance, speed));
}

bool SerialCommunication::sendEmergencyStop()
{
    // Emergency stop komutu
    return sendCommand("!");
}

bool SerialCommunication::sendReset()
{
    // Reset komutu
    return sendCommand("$X");
}

void SerialCommunication::requestStatus()
{
    sendCommand("?");
}

void SerialCommunication::requestPosition()
{
    sendCommand("$G");
}

void SerialCommunication::requestSettings()
{
    sendCommand("$$");
}

void SerialCommunication::setBaudRate(int baudRate)
{
    if (isConnected()) {
        serialPort->setBaudRate(baudRate);
    }
}

void SerialCommunication::setDataBits(int dataBits)
{
    if (isConnected()) {
        serialPort->setDataBits(static_cast<QSerialPort::DataBits>(dataBits));
    }
}

void SerialCommunication::setParity(int parity)
{
    if (isConnected()) {
        serialPort->setParity(static_cast<QSerialPort::Parity>(parity));
    }
}

void SerialCommunication::setStopBits(int stopBits)
{
    if (isConnected()) {
        serialPort->setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
    }
}

void SerialCommunication::setFlowControl(int flowControl)
{
    if (isConnected()) {
        serialPort->setFlowControl(static_cast<QSerialPort::FlowControl>(flowControl));
    }
}

QStringList SerialCommunication::getAvailablePorts()
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ports << info.portName();
    }
    return ports;
}

void SerialCommunication::handleReadyRead()
{
    while (serialPort->canReadLine()) {
        QString line = serialPort->readLine().trimmed();
        if (!line.isEmpty()) {
            emit dataReceived(line);
            
            // Response parsing
            parseStatusResponse(line);
            parsePositionResponse(line);
            parseLimitSwitchResponse(line);
            parseSpindleResponse(line);
            parseHomingResponse(line);
            
            processReceivedData();
        }
    }
}

void SerialCommunication::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        QString errorString = "Seri port hatası: " + serialPort->errorString();
        emit errorOccurred(errorString);
        
        if (error == QSerialPort::ResourceError) {
            disconnectFromDevice();
        }
    }
}

void SerialCommunication::handleTimeout()
{
    emit errorOccurred("Komut timeout");
    isProcessingCommand = false;
    sendNextCommand();
}

void SerialCommunication::processReceivedData()
{
    // Basit implementasyon: her satır bir yanıt olarak kabul edilir
    if (isProcessingCommand && !commandQueue.isEmpty()) {
        QString sentCommand = commandQueue.dequeue();
        emit commandCompleted(sentCommand.trimmed());
        isProcessingCommand = false;
        timeoutTimer->stop();
        sendNextCommand();
    }
}

void SerialCommunication::sendNextCommand()
{
    if (commandQueue.isEmpty() || isProcessingCommand) {
        return;
    }
    
    QString command = commandQueue.head();
    qint64 bytesWritten = serialPort->write(command.toUtf8());
    
    if (bytesWritten == command.length()) {
        emit commandSent(command.trimmed());
        isProcessingCommand = true;
        timeoutTimer->start();
        
        // Güvenlik kontrolü
        if (safetyChecksEnabled) {
            safetyTimer->start();
        }
    } else {
        emit errorOccurred("Komut gönderilemedi");
        commandQueue.dequeue(); // Hatalı komutu kaldır
        sendNextCommand();
    }
}

void SerialCommunication::parseStatusResponse(const QString &response)
{
    // GRBL status response parsing
    if (response.startsWith("<")) {
        // Status response format: <Idle|Run|Hold|Jog|Alarm|Door|Check|Home|Sleep|Cycle>,MPos:X,Y,Z,FS:F,S
        QStringList parts = response.mid(1, response.length() - 2).split(',');
        if (parts.size() >= 1) {
            QString status = parts[0];
            emit statusUpdated(status);
            
            // Homing durumu kontrolü
            if (status == "Home" && homingInProgress) {
                homingInProgress = false;
                emit homingCompleted();
            }
        }
    }
}

void SerialCommunication::parsePositionResponse(const QString &response)
{
    // GRBL position response parsing
    if (response.startsWith("[") && response.contains("MPos:")) {
        // Position response format: [GC:G0 G54 G17 G21 G90 G94 M5 M9 T0 F0 S0,MPos:0.000,0.000,0.000]
        int mposIndex = response.indexOf("MPos:");
        if (mposIndex != -1) {
            QString mposPart = response.mid(mposIndex + 5);
            int endBracket = mposPart.indexOf(']');
            if (endBracket != -1) {
                mposPart = mposPart.left(endBracket);
            }
            
            QStringList coords = mposPart.split(',');
            if (coords.size() >= 3) {
                double x = coords[0].toDouble();
                double y = coords[1].toDouble();
                double z = coords[2].toDouble();
                emit positionUpdated(x, y, z);
            }
        }
    }
}

QString SerialCommunication::formatGCodeCommand(const QString &gcode)
{
    return gcode.trimmed();
}

QString SerialCommunication::formatJogCommand(char axis, double distance, double speed)
{
    // GRBL jog komutu formatı: $J=G91 X<distance> F<speed>
    return QString("$J=G91 %1%2 F%3").arg(axis).arg(distance).arg(speed);
}

QString SerialCommunication::formatSpindleCommand(double speed, bool clockwise)
{
    QString command = QString("S%1").arg(static_cast<int>(speed));
    command += clockwise ? " M3" : " M4";
    return command;
}

QString SerialCommunication::formatCoolantCommand(bool on)
{
    return on ? "M8" : "M9";
}

QString SerialCommunication::formatHomingCommand(char axis)
{
    if (axis == ' ') {
        return "$H"; // Tüm eksenler
    } else {
        return QString("$H%1").arg(axis); // Belirli eksen
    }
} 