#ifndef SERIALCOMMUNICATION_H
#define SERIALCOMMUNICATION_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QQueue>

class SerialCommunication : public QObject
{
    Q_OBJECT

public:
    explicit SerialCommunication(QObject *parent = nullptr);
    ~SerialCommunication();
    
    // Bağlantı yönetimi
    bool connectToDevice(const QString &portName, int baudRate = 115200);
    void disconnectFromDevice();
    bool isConnected() const;
    
    // Komut gönderme
    bool sendCommand(const QString &command);
    bool sendGCodeCommand(const QString &gcode);
    bool sendJogCommand(char axis, double distance, double speed);
    bool sendEmergencyStop();
    bool sendReset();
    
    // Durum sorgulama
    void requestStatus();
    void requestPosition();
    void requestSettings();
    
    // Ayarlar
    void setBaudRate(int baudRate);
    void setDataBits(int dataBits);
    void setParity(int parity);
    void setStopBits(int stopBits);
    void setFlowControl(int flowControl);
    
    // Port listesi
    static QStringList getAvailablePorts();
    
signals:
    void connected();
    void disconnected();
    void dataReceived(const QString &data);
    void errorOccurred(const QString &error);
    void statusUpdated(const QString &status);
    void positionUpdated(double x, double y, double z);
    void commandSent(const QString &command);
    void commandCompleted(const QString &command);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void handleTimeout();

private:
    QSerialPort *serialPort;
    QTimer *timeoutTimer;
    QQueue<QString> commandQueue;
    bool isProcessingCommand;
    
    void processReceivedData();
    void sendNextCommand();
    void parseStatusResponse(const QString &response);
    void parsePositionResponse(const QString &response);
    QString formatGCodeCommand(const QString &gcode);
    QString formatJogCommand(char axis, double distance, double speed);
};

#endif // SERIALCOMMUNICATION_H 