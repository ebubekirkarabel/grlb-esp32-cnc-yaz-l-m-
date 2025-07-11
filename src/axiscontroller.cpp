#include "axiscontroller.h"
#include <QDebug>

AxisController::AxisController(QObject *parent)
    : QObject(parent)
    , jogTimer(new QTimer(this))
    , continuousTimer(new QTimer(this))
    , currentJogAxis(' ')
    , currentJogPositive(false)
    , jogStep(1.0)
    , jogSpeed(1000.0)
    , acceleration(1.0)
    , maxSpeed(5000.0)
    , emergencyStopActive(false)
{
    // Timer ayarları
    jogTimer->setInterval(50); // 50ms = 20Hz
    continuousTimer->setInterval(50);
    
    // Eksenleri başlat
    initializeAxis('X');
    initializeAxis('Y');
    initializeAxis('Z');
    
    // Timer bağlantıları
    connect(jogTimer, &QTimer::timeout, this, &AxisController::updateJogMovement);
    connect(continuousTimer, &QTimer::timeout, this, &AxisController::updateContinuousMovement);
}

AxisController::~AxisController()
{
}

void AxisController::setPosition(char axis, double position)
{
    if (axisPositions.contains(axis)) {
        updateAxisPosition(axis, position);
    }
}

double AxisController::getPosition(char axis) const
{
    return axisPositions.value(axis).current;
}

void AxisController::setTargetPosition(char axis, double position)
{
    if (axisPositions.contains(axis)) {
        axisPositions[axis].target = position;
    }
}

double AxisController::getTargetPosition(char axis) const
{
    return axisPositions.value(axis).target;
}

void AxisController::startJog(char axis, bool positive, double step, double speed)
{
    if (emergencyStopActive) {
        return;
    }
    
    if (!axisPositions.contains(axis)) {
        return;
    }
    
    // Tek seferlik jog
    double currentPos = axisPositions[axis].current;
    double newPos = currentPos + (positive ? step : -step);
    
    if (validateMovement(axis, newPos)) {
        updateAxisPosition(axis, newPos);
    }
}

void AxisController::stopJog()
{
    // Tek seferlik jog için durdurma gerekmez
}

void AxisController::setJogStep(double step)
{
    jogStep = step;
}

void AxisController::setJogSpeed(double speed)
{
    jogSpeed = speed;
}

void AxisController::startContinuousJog(char axis, bool positive)
{
    if (emergencyStopActive) {
        return;
    }
    
    if (!axisPositions.contains(axis)) {
        return;
    }
    
    currentJogAxis = axis;
    currentJogPositive = positive;
    acceleration = 1.0;
    
    if (!continuousTimer->isActive()) {
        continuousTimer->start();
    }
}

void AxisController::stopContinuousJog()
{
    if (continuousTimer->isActive()) {
        continuousTimer->stop();
    }
    currentJogAxis = ' ';
    acceleration = 1.0;
}

bool AxisController::isJogging() const
{
    return continuousTimer->isActive();
}

void AxisController::setAxisLimits(char axis, double minLimit, double maxLimit)
{
    if (!axisLimits.contains(axis)) {
        axisLimits[axis] = AxisLimits{minLimit, maxLimit, true};
    } else {
        axisLimits[axis].minLimit = minLimit;
        axisLimits[axis].maxLimit = maxLimit;
    }
}

void AxisController::enableAxisLimits(char axis, bool enabled)
{
    if (axisLimits.contains(axis)) {
        axisLimits[axis].enabled = enabled;
    }
}

bool AxisController::checkLimits(char axis, double position) const
{
    if (!axisLimits.contains(axis) || !axisLimits[axis].enabled) {
        return true;
    }
    
    const AxisLimits &limits = axisLimits[axis];
    return position >= limits.minLimit && position <= limits.maxLimit;
}

AxisLimits AxisController::getAxisLimits(char axis) const
{
    return axisLimits.value(axis, AxisLimits{-50.0, 50.0, true});
}

void AxisController::moveToPosition(char axis, double target, double speed)
{
    if (emergencyStopActive) {
        return;
    }
    
    if (!axisPositions.contains(axis)) {
        return;
    }
    
    if (validateMovement(axis, target)) {
        axisPositions[axis].target = target;
        axisPositions[axis].isMoving = true;
        emit movementStarted(axis, target);
        
        // Basit implementasyon: hemen hedefe git
        updateAxisPosition(axis, target);
        axisPositions[axis].isMoving = false;
        emit targetReached(axis, target);
        emit movementStopped(axis);
    }
}

void AxisController::stopMovement(char axis)
{
    if (axisPositions.contains(axis)) {
        axisPositions[axis].isMoving = false;
        emit movementStopped(axis);
    }
}

void AxisController::stopAllMovement()
{
    for (auto it = axisPositions.begin(); it != axisPositions.end(); ++it) {
        it->isMoving = false;
        emit movementStopped(it.key());
    }
}

bool AxisController::isMoving(char axis) const
{
    return axisPositions.value(axis).isMoving;
}

void AxisController::emergencyStop()
{
    if (!emergencyStopActive) {
        emergencyStopActive = true;
        stopAllMovement();
        stopContinuousJog();
        emit emergencyStopActivated();
    }
}

void AxisController::resetEmergencyStop()
{
    if (emergencyStopActive) {
        emergencyStopActive = false;
        emit emergencyStopReset();
    }
}

bool AxisController::isEmergencyStopActive() const
{
    return emergencyStopActive;
}

void AxisController::setAcceleration(double accel)
{
    acceleration = accel;
}

void AxisController::setMaxSpeed(double speed)
{
    maxSpeed = speed;
}

void AxisController::updateJogMovement()
{
    // Bu fonksiyon şu an kullanılmıyor, sürekli jog için updateContinuousMovement kullanılıyor
}

void AxisController::updateContinuousMovement()
{
    if (currentJogAxis == ' ' || emergencyStopActive) {
        return;
    }
    
    if (!axisPositions.contains(currentJogAxis)) {
        return;
    }
    
    double step = calculateJogDistance();
    double currentPos = axisPositions[currentJogAxis].current;
    double newPos = currentPos + (currentJogPositive ? step : -step);
    
    if (validateMovement(currentJogAxis, newPos)) {
        updateAxisPosition(currentJogAxis, newPos);
        applyAcceleration();
    } else {
        stopContinuousJog();
    }
}

void AxisController::initializeAxis(char axis)
{
    axisPositions[axis] = AxisPosition{0.0, 0.0, false};
    axisLimits[axis] = AxisLimits{-50.0, 50.0, true};
}

bool AxisController::validateMovement(char axis, double newPosition) const
{
    if (!checkLimits(axis, newPosition)) {
        emit limitReached(axis, newPosition);
        return false;
    }
    
    return true;
}

void AxisController::updateAxisPosition(char axis, double newPosition)
{
    if (axisPositions.contains(axis)) {
        axisPositions[axis].current = newPosition;
        emit positionChanged(axis, newPosition);
    }
}

double AxisController::calculateJogDistance() const
{
    // Hızı mm/s'ye çevir ve timer aralığıyla çarp
    double speedMMPerSec = jogSpeed / 60.0;
    double timerIntervalSec = continuousTimer->interval() / 1000.0;
    return speedMMPerSec * timerIntervalSec * acceleration;
}

void AxisController::applyAcceleration()
{
    // Kademeli hızlanma
    acceleration = qMin(acceleration + 0.1, 3.0); // Maksimum 3x hızlanma
} 