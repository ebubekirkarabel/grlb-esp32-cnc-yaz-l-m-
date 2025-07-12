#ifndef AXISCONTROLLER_H
#define AXISCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QVector>

enum class AccelerationProfile {
    Linear,
    SCurve,
    Trapezoidal
};

enum class MicrostepMode {
    Full = 1,
    Half = 2,
    Quarter = 4,
    Eighth = 8,
    Sixteenth = 16,
    ThirtySecond = 32
};

struct MotionProfile {
    double startPosition;
    double endPosition;
    double maxSpeed;
    double acceleration;
    double deceleration;
    double jerk;
    AccelerationProfile profile;
    double totalTime;
    QVector<double> timePoints;
    QVector<double> positionPoints;
    QVector<double> velocityPoints;
};

struct AxisLimits {
    double minLimit;
    double maxLimit;
    bool enabled;
};

struct AxisPosition {
    double current;
    double target;
    bool isMoving;
    // Yeni alanlar
    double currentVelocity;
    double currentAcceleration;
    MicrostepMode microstepMode;
    int stepsPerRevolution;
    double mmPerStep;
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
    
    // Hareket kontrolü - Geliştirilmiş
    void moveToPosition(char axis, double target, double speed);
    void moveToPositionWithProfile(char axis, double target, const MotionProfile &profile);
    void stopMovement(char axis);
    void stopAllMovement();
    bool isMoving(char axis) const;
    
    // Yeni: Gelişmiş hareket planlayıcı
    MotionProfile createMotionProfile(char axis, double startPos, double endPos, double maxSpeed, double acceleration);
    MotionProfile createSCurveProfile(char axis, double startPos, double endPos, double maxSpeed, double acceleration, double jerk);
    MotionProfile createTrapezoidalProfile(char axis, double startPos, double endPos, double maxSpeed, double acceleration);
    void executeMotionProfile(char axis, const MotionProfile &profile);
    
    // Yeni: Mikro-adım kontrolü
    void setMicrostepMode(char axis, MicrostepMode mode);
    MicrostepMode getMicrostepMode(char axis) const;
    void setStepsPerRevolution(char axis, int steps);
    int getStepsPerRevolution(char axis) const;
    void setMmPerStep(char axis, double mmPerStep);
    double getMmPerStep(char axis) const;
    
    // Yeni: Hız profili ayarları
    void setAccelerationProfile(AccelerationProfile profile);
    AccelerationProfile getAccelerationProfile() const;
    void setJerk(double jerk);
    double getJerk() const;
    
    // Acil durum
    void emergencyStop();
    void resetEmergencyStop();
    bool isEmergencyStopActive() const;
    
    // Ayarlar
    void setAcceleration(double acceleration);
    void setMaxSpeed(double maxSpeed);
    
    // Yeni: İstatistikler
    double getCurrentVelocity(char axis) const;
    double getCurrentAcceleration(char axis) const;
    double getTotalDistance(char axis) const;
    double getTotalTime(char axis) const;

signals:
    void positionChanged(char axis, double position);
    void targetReached(char axis, double position);
    void limitReached(char axis, double position);
    void movementStarted(char axis, double target);
    void movementStopped(char axis);
    void emergencyStopActivated();
    void emergencyStopReset();
    void velocityChanged(char axis, double velocity);
    void accelerationChanged(char axis, double acceleration);
    void profileCompleted(char axis, const MotionProfile &profile);

private slots:
    void updateJogMovement();
    void updateContinuousMovement();
    void updateMotionProfile();

private:
    QMap<char, AxisPosition> axisPositions;
    QMap<char, AxisLimits> axisLimits;
    QMap<char, MotionProfile> activeProfiles;
    QTimer *jogTimer;
    QTimer *continuousTimer;
    QTimer *motionTimer;
    
    char currentJogAxis;
    bool currentJogPositive;
    double jogStep;
    double jogSpeed;
    double acceleration;
    double maxSpeed;
    double jerk;
    bool emergencyStopActive;
    AccelerationProfile accelerationProfile;
    
    void initializeAxis(char axis);
    bool validateMovement(char axis, double newPosition) const;
    void updateAxisPosition(char axis, double newPosition);
    double calculateJogDistance() const;
    void applyAcceleration();
    
    // Yeni yardımcı fonksiyonlar
    double calculateSCurveVelocity(double time, double maxSpeed, double acceleration, double jerk);
    double calculateTrapezoidalVelocity(double time, double maxSpeed, double acceleration);
    double calculatePositionFromVelocity(double velocity, double time);
    void updateProfileExecution(char axis);
    void generateProfilePoints(MotionProfile &profile);
};

#endif // AXISCONTROLLER_H 