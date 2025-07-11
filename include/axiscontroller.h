#ifndef AXISCONTROLLER_H
#define AXISCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QMap>

struct AxisLimits {
    double minLimit;
    double maxLimit;
    bool enabled;
};

struct AxisPosition {
    double current;
    double target;
    bool isMoving;
};

class AxisController : public QObject
{
    Q_OBJECT

public:
    explicit AxisController(QObject *parent = nullptr);
    ~AxisController();
    
    // Eksen pozisyon yönetimi
    void setPosition(char axis, double position);
    double getPosition(char axis) const;
    void setTargetPosition(char axis, double position);
    double getTargetPosition(char axis) const;
    
    // Jog kontrolü
    void startJog(char axis, bool positive, double step, double speed);
    void stopJog();
    void setJogStep(double step);
    void setJogSpeed(double speed);
    
    // Sürekli hareket
    void startContinuousJog(char axis, bool positive);
    void stopContinuousJog();
    bool isJogging() const;
    
    // Limit kontrolü
    void setAxisLimits(char axis, double minLimit, double maxLimit);
    void enableAxisLimits(char axis, bool enabled);
    bool checkLimits(char axis, double position) const;
    AxisLimits getAxisLimits(char axis) const;
    
    // Hareket kontrolü
    void moveToPosition(char axis, double target, double speed);
    void stopMovement(char axis);
    void stopAllMovement();
    bool isMoving(char axis) const;
    
    // Acil durum
    void emergencyStop();
    void resetEmergencyStop();
    bool isEmergencyStopActive() const;
    
    // Ayarlar
    void setAcceleration(double acceleration);
    void setMaxSpeed(double maxSpeed);
    
signals:
    void positionChanged(char axis, double position);
    void targetReached(char axis, double position);
    void limitReached(char axis, double position);
    void movementStarted(char axis, double target);
    void movementStopped(char axis);
    void emergencyStopActivated();
    void emergencyStopReset();

private slots:
    void updateJogMovement();
    void updateContinuousMovement();

private:
    QMap<char, AxisPosition> axisPositions;
    QMap<char, AxisLimits> axisLimits;
    QTimer *jogTimer;
    QTimer *continuousTimer;
    
    char currentJogAxis;
    bool currentJogPositive;
    double jogStep;
    double jogSpeed;
    double acceleration;
    double maxSpeed;
    bool emergencyStopActive;
    
    void initializeAxis(char axis);
    bool validateMovement(char axis, double newPosition) const;
    void updateAxisPosition(char axis, double newPosition);
    double calculateJogDistance() const;
    void applyAcceleration();
};

#endif // AXISCONTROLLER_H 