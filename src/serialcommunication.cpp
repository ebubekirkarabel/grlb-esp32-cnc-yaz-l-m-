#include "serialcommunication.h"
#include <QDebug>
#include <QSerialPortInfo>

SerialCommunication::SerialCommunication(QObject *parent)
    : QObject(parent)
    , serialPort(new QSerialPort(this))
    , timeoutTimer(new QTimer(this))
    , isProcessingCommand(false)
{
    // Timer ayarları
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(5000); // 5 saniye timeout
    
    // Seri port bağlantıları
    connect(serialPort, &QSerialPort::readyRead, this, &SerialCommunication::handleReadyRead);
    connect(serialPort, &QSerialPort::errorOccurred, this, &SerialCommunication::handleError);
    connect(timeoutTimer, &QTimer::timeout, this, &SerialCommunication::handleTimeout);
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
        return true;
    } else {
        emit errorOccurred("Bağlantı hatası: " + serialPort->errorString());
        return false;
    }
}

void SerialCommunication::disconnectFromDevice()
{
    if (serialPort->isOpen()) {
        serialPort->close();
        emit disconnected();
    }
    
    // Bekleyen komutları temizle
    commandQueue.clear();
    isProcessingCommand = false;
    timeoutTimer->stop();
}

bool SerialCommunication::isConnected() const
{
    return serialPort->isOpen();
}

bool SerialCommunication::sendCommand(const QString &command)
{
    if (!isConnected()) {
        emit errorOccurred("Seri port bağlı değil");
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