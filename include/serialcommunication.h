#ifndef SERIALCOMMUNICATION_H
#define SERIALCOMMUNICATION_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QQueue>

enum class LimitSwitchState {
    NotTriggered,
    Triggered,
    Error
};

enum class SpindleState {
    Off,
    Clockwise,
    CounterClockwise,
    Error
};

struct LimitSwitchStatus {
    LimitSwitchState xMin;
    LimitSwitchState xMax;
    LimitSwitchState yMin;
    LimitSwitchState yMax;
    LimitSwitchState zMin;
    LimitSwitchState zMax;
    bool anyTriggered;
};

struct SpindleStatus {
    SpindleState state;
    double speed;
    double targetSpeed;
    bool coolantOn;
    bool airBlastOn;
};

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
    
    // Yeni: Hardware limit switch kontrolü
    void requestLimitSwitchStatus();
    LimitSwitchStatus getLimitSwitchStatus() const;
    bool isAnyLimitSwitchTriggered() const;
    void enableLimitSwitchMonitoring(bool enabled);
    
    // Yeni: Spindle kontrolü
    bool setSpindleSpeed(double speed);
    bool startSpindle(bool clockwise = true);
    bool stopSpindle();
    bool setCoolant(bool on);
    bool setAirBlast(bool on);
    SpindleStatus getSpindleStatus() const;
    
    // Yeni: Homing işlemleri
    bool startHoming();
    bool startHomingAxis(char axis);
    bool isHoming() const;
    void setHomingEnabled(bool enabled);
    
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
    
    // Yeni: Güvenlik ayarları
    void setSafetyTimeout(int milliseconds);
    int getSafetyTimeout() const;
    void enableSafetyChecks(bool enabled);
    bool areSafetyChecksEnabled() const;

signals:
    void connected();
    void disconnected();
    void dataReceived(const QString &data);
    void errorOccurred(const QString &error);
    void statusUpdated(const QString &status);
    void positionUpdated(double x, double y, double z);
    void commandSent(const QString &command);
    void commandCompleted(const QString &command);
    
    // Yeni sinyaller
    void limitSwitchTriggered(char axis, bool isMin);
    void limitSwitchReleased(char axis, bool isMin);
    void limitSwitchStatusChanged(const LimitSwitchStatus &status);
    void spindleStateChanged(SpindleState state);
    void spindleSpeedChanged(double speed);
    void coolantStateChanged(bool on);
    void airBlastStateChanged(bool on);
    void homingStarted();
    void homingCompleted();
    void homingFailed(const QString &error);
    void safetyTimeoutOccurred();

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void handleTimeout();
    void handleSafetyTimeout();

private:
    QSerialPort *serialPort;
    QTimer *timeoutTimer;
    QTimer *safetyTimer;
    QTimer *statusTimer;
    QQueue<QString> commandQueue;
    bool isProcessingCommand;
    
    // Yeni üye değişkenler
    LimitSwitchStatus limitSwitchStatus;
    SpindleStatus spindleStatus;
    bool limitSwitchMonitoringEnabled;
    bool homingInProgress;
    bool homingEnabled;
    bool safetyChecksEnabled;
    int safetyTimeout;
    
    void processReceivedData();
    void sendNextCommand();
    void parseStatusResponse(const QString &response);
    void parsePositionResponse(const QString &response);
    void parseLimitSwitchResponse(const QString &response);
    void parseSpindleResponse(const QString &response);
    void parseHomingResponse(const QString &response);
    
    QString formatGCodeCommand(const QString &gcode);
    QString formatJogCommand(char axis, double distance, double speed);
    QString formatSpindleCommand(double speed, bool clockwise);
    QString formatCoolantCommand(bool on);
    QString formatHomingCommand(char axis = ' ');
    
    // Yeni yardımcı fonksiyonlar
    void startStatusMonitoring();
    void stopStatusMonitoring();
    void checkSafetyConditions();
    bool validateSafetyCommand(const QString &command);
};

#endif // SERIALCOMMUNICATION_H 