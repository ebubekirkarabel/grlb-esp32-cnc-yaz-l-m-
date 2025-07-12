#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QMessageBox>
#include <QMenu>
#include <QRegularExpression>
#include <Qt>
#include "openglwidget.h"
#include "gcodeparser.h"
#include "serialcommunication.h"
#include "axiscontroller.h"
#include "settings.h"
#include "logger.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , gcodeParser(new GCodeParser(this))
    , serialComm(new SerialCommunication(this))
    , axisController(new AxisController(this))
    , settings(new Settings(this))
    , logger(Logger::instance())
    , currentX(0.0)
    , currentY(0.0)
    , currentZ(0.0)
    , jogSpeed(1000)
    , feedRate(1000)
    , jogStep(1.0)
    , emergencyStopActive(false)
    , xMinLimit(-50.0)
    , xMaxLimit(50.0)
    , yMinLimit(-50.0)
    , yMaxLimit(50.0)
    , zMinLimit(-15.0)
    , zMaxLimit(15.0)
{
    setWindowTitle("CNC Controller v1.0");
    resize(1200, 800);
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    move((screenGeometry.width() - width()) / 2, (screenGeometry.height() - height()) / 2);

    createMenus();
    createToolBar();
    createStatusBar();
    createCentralWidget();
    createEmergencyStopPanel();
    setupConnections();
    initializeModules();
    connectModules();
    updateStatusBar("Hazır");
    logMessage("CNC Controller başlatıldı");
}

MainWindow::~MainWindow() {}

void MainWindow::createMenus() {
    // Dosya menüsü
    QMenu *fileMenu = menuBar()->addMenu("&Dosya");
    
    QAction *openAction = new QAction("&Aç", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openGCodeFile);
    fileMenu->addAction(openAction);
    
    QAction *saveAction = new QAction("&Kaydet", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveGCodeFile);
    fileMenu->addAction(saveAction);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = new QAction("&Çıkış", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);
    
    // Kontrol menüsü
    QMenu *controlMenu = menuBar()->addMenu("&Kontrol");
    
    QAction *startAction = new QAction("&Başlat", this);
    connect(startAction, &QAction::triggered, this, &MainWindow::startCNC);
    controlMenu->addAction(startAction);
    
    QAction *stopAction = new QAction("&Durdur", this);
    connect(stopAction, &QAction::triggered, this, &MainWindow::stopCNC);
    controlMenu->addAction(stopAction);
    
    QAction *pauseAction = new QAction("&Duraklat", this);
    connect(pauseAction, &QAction::triggered, this, &MainWindow::pauseCNC);
    controlMenu->addAction(pauseAction);
    
    QAction *resetAction = new QAction("&Sıfırla", this);
    connect(resetAction, &QAction::triggered, this, &MainWindow::resetCNC);
    controlMenu->addAction(resetAction);
}

void MainWindow::createToolBar()
{
    toolBar = addToolBar("Ana Araç Çubuğu");
    
    // Dosya işlemleri
    QAction *openToolAction = toolBar->addAction("Aç");
    connect(openToolAction, &QAction::triggered, this, &MainWindow::openGCodeFile);
    
    QAction *saveToolAction = toolBar->addAction("Kaydet");
    connect(saveToolAction, &QAction::triggered, this, &MainWindow::saveGCodeFile);
    
    toolBar->addSeparator();
    
    // Kontrol işlemleri
    QAction *startToolAction = toolBar->addAction("Başlat");
    connect(startToolAction, &QAction::triggered, this, &MainWindow::startCNC);
    
    QAction *stopToolAction = toolBar->addAction("Durdur");
    connect(stopToolAction, &QAction::triggered, this, &MainWindow::stopCNC);
}

void MainWindow::createStatusBar()
{
    statusBar = statusBar();
    statusBar->showMessage("Hazır");
}

void MainWindow::createCentralWidget()
{
    centralWidget = new QWidget;
    setCentralWidget(centralWidget);
    
    // Ana layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // YENİ: Güvenlik durumu paneli
    createSafetyStatusPanel();
    mainLayout->addWidget(safetyStatusGroup);
    
    // Alt panel layout
    QHBoxLayout *contentLayout = new QHBoxLayout;
    
    // Sol panel (G-code)
    createGCodePanel();
    contentLayout->addWidget(gcodeGroup, 1);
    
    // Orta panel (3D Simülasyon)
    createSimulationPanel();
    contentLayout->addWidget(simulationGroup, 2);
    
    // Sağ panel (Eksen kontrolü)
    createAxisControlPanel();
    contentLayout->addWidget(axisControlGroup, 1);
    
    mainLayout->addLayout(contentLayout);
}

void MainWindow::createSafetyStatusPanel()
{
    safetyStatusGroup = new QGroupBox("Güvenlik Durumu");
    QHBoxLayout *layout = new QHBoxLayout(safetyStatusGroup);
    
    // Limit switch durumu
    QGroupBox *limitGroup = new QGroupBox("Limit Switch Durumu");
    QGridLayout *limitLayout = new QGridLayout(limitGroup);
    
    xMinLimitLabel = new QLabel("X-: OK");
    xMaxLimitLabel = new QLabel("X+: OK");
    yMinLimitLabel = new QLabel("Y-: OK");
    yMaxLimitLabel = new QLabel("Y+: OK");
    zMinLimitLabel = new QLabel("Z-: OK");
    zMaxLimitLabel = new QLabel("Z+: OK");
    
    // Limit switch etiketlerini stilize et
    QList<QLabel*> limitLabels = {xMinLimitLabel, xMaxLimitLabel, yMinLimitLabel, yMaxLimitLabel, zMinLimitLabel, zMaxLimitLabel};
    for (QLabel* label : limitLabels) {
        label->setStyleSheet(
            "QLabel { "
            "color: #00aa00; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #00aa00; "
            "border-radius: 3px; "
            "background-color: #e8f5e8; "
            "}"
        );
    }
    
    limitLayout->addWidget(xMinLimitLabel, 0, 0);
    limitLayout->addWidget(xMaxLimitLabel, 0, 1);
    limitLayout->addWidget(yMinLimitLabel, 1, 0);
    limitLayout->addWidget(yMaxLimitLabel, 1, 1);
    limitLayout->addWidget(zMinLimitLabel, 2, 0);
    limitLayout->addWidget(zMaxLimitLabel, 2, 1);
    
    layout->addWidget(limitGroup);
    
    // Spindle durumu
    QGroupBox *spindleGroup = new QGroupBox("Spindle Durumu");
    QVBoxLayout *spindleLayout = new QVBoxLayout(spindleGroup);
    
    spindleStateLabel = new QLabel("Durum: KAPALI");
    spindleSpeedLabel = new QLabel("Hız: 0 RPM");
    coolantStateLabel = new QLabel("Soğutucu: KAPALI");
    
    // Spindle etiketlerini stilize et
    spindleStateLabel->setStyleSheet(
        "QLabel { "
        "color: #666666; "
        "font-weight: bold; "
        "padding: 2px; "
        "border: 1px solid #666666; "
        "border-radius: 3px; "
        "background-color: #f0f0f0; "
        "}"
    );
    
    spindleSpeedLabel->setStyleSheet(
        "QLabel { "
        "color: #666666; "
        "font-weight: bold; "
        "padding: 2px; "
        "border: 1px solid #666666; "
        "border-radius: 3px; "
        "background-color: #f0f0f0; "
        "}"
    );
    
    coolantStateLabel->setStyleSheet(
        "QLabel { "
        "color: #666666; "
        "font-weight: bold; "
        "padding: 2px; "
        "border: 1px solid #666666; "
        "border-radius: 3px; "
        "background-color: #f0f0f0; "
        "}"
    );
    
    spindleLayout->addWidget(spindleStateLabel);
    spindleLayout->addWidget(spindleSpeedLabel);
    spindleLayout->addWidget(coolantStateLabel);
    
    layout->addWidget(spindleGroup);
    
    // Homing durumu
    QGroupBox *homingGroup = new QGroupBox("Homing Durumu");
    QVBoxLayout *homingLayout = new QVBoxLayout(homingGroup);
    
    homingStatusLabel = new QLabel("Durum: Hazır");
    homingBtn = new QPushButton("Homing Başlat");
    
    homingStatusLabel->setStyleSheet(
        "QLabel { "
        "color: #00aa00; "
        "font-weight: bold; "
        "padding: 2px; "
        "border: 1px solid #00aa00; "
        "border-radius: 3px; "
        "background-color: #e8f5e8; "
        "}"
    );
    
    homingLayout->addWidget(homingStatusLabel);
    homingLayout->addWidget(homingBtn);
    
    layout->addWidget(homingGroup);
    
    // Homing butonu bağlantısı
    connect(homingBtn, &QPushButton::clicked, this, [this]() {
        if (serialComm && serialComm->isConnected()) {
            if (serialComm->startHoming()) {
                homingStatusLabel->setText("Durum: Homing...");
                homingStatusLabel->setStyleSheet(
                    "QLabel { "
                    "color: #ff8800; "
                    "font-weight: bold; "
                    "padding: 2px; "
                    "border: 1px solid #ff8800; "
                    "border-radius: 3px; "
                    "background-color: #fff8e8; "
                    "}"
                );
                homingBtn->setEnabled(false);
            }
        }
    });
}

void MainWindow::createAxisControlPanel()
{
    axisControlGroup = new QGroupBox("Eksen Kontrolü");
    QVBoxLayout *layout = new QVBoxLayout(axisControlGroup);
    
    // X ekseni
    QGroupBox *xGroup = new QGroupBox("X Ekseni");
    QGridLayout *xLayout = new QGridLayout(xGroup);
    
    xPosEdit = new QLineEdit("0.000");
    xPosEdit->setReadOnly(true);
    xTargetEdit = new QLineEdit("0.000");
    
    xPosBtn = new QPushButton("X+");
    xNegBtn = new QPushButton("X-");
    
    xLayout->addWidget(new QLabel("Mevcut:"), 0, 0);
    xLayout->addWidget(xPosEdit, 0, 1);
    xLayout->addWidget(new QLabel("Hedef:"), 1, 0);
    xLayout->addWidget(xTargetEdit, 1, 1);
    xLayout->addWidget(xNegBtn, 2, 0);
    xLayout->addWidget(xPosBtn, 2, 1);
    
    layout->addWidget(xGroup);
    
    // Y ekseni
    QGroupBox *yGroup = new QGroupBox("Y Ekseni");
    QGridLayout *yLayout = new QGridLayout(yGroup);
    
    yPosEdit = new QLineEdit("0.000");
    yPosEdit->setReadOnly(true);
    yTargetEdit = new QLineEdit("0.000");
    
    yPosBtn = new QPushButton("Y+");
    yNegBtn = new QPushButton("Y-");
    
    yLayout->addWidget(new QLabel("Mevcut:"), 0, 0);
    yLayout->addWidget(yPosEdit, 0, 1);
    yLayout->addWidget(new QLabel("Hedef:"), 1, 0);
    yLayout->addWidget(yTargetEdit, 1, 1);
    yLayout->addWidget(yNegBtn, 2, 0);
    yLayout->addWidget(yPosBtn, 2, 1);
    
    layout->addWidget(yGroup);
    
    // Z ekseni
    QGroupBox *zGroup = new QGroupBox("Z Ekseni");
    QGridLayout *zLayout = new QGridLayout(zGroup);
    
    zPosEdit = new QLineEdit("0.000");
    zPosEdit->setReadOnly(true);
    zTargetEdit = new QLineEdit("0.000");
    
    zPosBtn = new QPushButton("Z+");
    zNegBtn = new QPushButton("Z-");
    
    zLayout->addWidget(new QLabel("Mevcut:"), 0, 0);
    zLayout->addWidget(zPosEdit, 0, 1);
    zLayout->addWidget(new QLabel("Hedef:"), 1, 0);
    zLayout->addWidget(zTargetEdit, 1, 1);
    zLayout->addWidget(zNegBtn, 2, 0);
    zLayout->addWidget(zPosBtn, 2, 1);
    
    layout->addWidget(zGroup);
    
    // Soft limit bilgileri
    QGroupBox *limitGroup = new QGroupBox("Soft Limitler");
    QVBoxLayout *limitLayout = new QVBoxLayout(limitGroup);
    
    QLabel *xLimitLabel = new QLabel(QString("X: %1 - %2 mm").arg(xMinLimit).arg(xMaxLimit));
    QLabel *yLimitLabel = new QLabel(QString("Y: %1 - %2 mm").arg(yMinLimit).arg(yMaxLimit));
    QLabel *zLimitLabel = new QLabel(QString("Z: %1 - %2 mm").arg(zMinLimit).arg(zMaxLimit));
    
    limitLayout->addWidget(xLimitLabel);
    limitLayout->addWidget(yLimitLabel);
    limitLayout->addWidget(zLimitLabel);
    
    layout->addWidget(limitGroup);
    
    // Hız ve adım kontrolü
    QGroupBox *speedGroup = new QGroupBox("Hız ve Adım Kontrolü");
    QGridLayout *speedLayout = new QGridLayout(speedGroup);
    
    // Jog adım mesafesi
    jogStepCombo = new QComboBox;
    jogStepCombo->addItems({"0.1", "0.5", "1.0", "5.0", "10.0"});
    jogStepCombo->setCurrentText("1.0");
    
    // Jog hızı - Geliştirilmiş
    jogSpeedCombo = new QComboBox;
    jogSpeedCombo->addItems({"50", "100", "250", "500", "1000", "2000", "5000", "10000"});
    jogSpeedCombo->setCurrentText("1000");
    
    // Jog hızı slider (daha hassas kontrol için)
    jogSpeedSlider = new QSlider(Qt::Horizontal);
    jogSpeedSlider->setRange(50, 10000);
    jogSpeedSlider->setValue(1000);
    jogSpeedSlider->setTickPosition(QSlider::TicksBelow);
    jogSpeedSlider->setTickInterval(1000);
    
    // Jog hızı gösterge etiketi
    jogSpeedLabel = new QLabel("1000 mm/min");
    
    // Feedrate
    feedRateCombo = new QComboBox;
    feedRateCombo->addItems({"100", "500", "1000", "2000", "5000"});
    feedRateCombo->setCurrentText("1000");
    
    speedLayout->addWidget(new QLabel("Jog Adımı (mm):"), 0, 0);
    speedLayout->addWidget(jogStepCombo, 0, 1);
    speedLayout->addWidget(new QLabel("Jog Hızı (mm/min):"), 1, 0);
    speedLayout->addWidget(jogSpeedCombo, 1, 1);
    speedLayout->addWidget(jogSpeedSlider, 2, 0, 1, 2);
    speedLayout->addWidget(jogSpeedLabel, 3, 0, 1, 2);
    speedLayout->addWidget(new QLabel("Feedrate (mm/min):"), 4, 0);
    speedLayout->addWidget(feedRateCombo, 4, 1);
    
    layout->addWidget(speedGroup);
    
    // Bağlantılar
    connect(jogStepCombo, &QComboBox::currentTextChanged,
            [this](const QString &text) { jogStep = text.toDouble(); });
    connect(jogSpeedCombo, &QComboBox::currentTextChanged,
            [this](const QString &text) { 
                jogSpeed = text.toInt(); 
                jogSpeedSlider->setValue(jogSpeed);
                updateJogSpeed();
            });
    connect(jogSpeedSlider, &QSlider::valueChanged, this, &MainWindow::updateJogSpeed);
    connect(feedRateCombo, &QComboBox::currentTextChanged,
            [this](const QString &text) { feedRate = text.toInt(); });
}

void MainWindow::createGCodePanel()
{
    gcodeGroup = new QGroupBox("G-Code İşleme");
    QVBoxLayout *layout = new QVBoxLayout(gcodeGroup);
    
    // Dosya işlemleri
    QHBoxLayout *fileLayout = new QHBoxLayout;
    openFileBtn = new QPushButton("Dosya Aç");
    saveFileBtn = new QPushButton("Kaydet");
    fileLayout->addWidget(openFileBtn);
    fileLayout->addWidget(saveFileBtn);
    fileLayout->addStretch();
    layout->addLayout(fileLayout);
    
    // G-code editörü
    gcodeEditor = new QTextEdit;
    gcodeEditor->setPlaceholderText("G-code komutlarını buraya yazın veya dosya açın...");
    layout->addWidget(gcodeEditor);
    
    // Komut gönderme
    QHBoxLayout *commandLayout = new QHBoxLayout;
    commandEdit = new QLineEdit;
    commandEdit->setPlaceholderText("G-code komutu girin...");
    sendCommandBtn = new QPushButton("Gönder");
    commandLayout->addWidget(commandEdit);
    commandLayout->addWidget(sendCommandBtn);
    layout->addLayout(commandLayout);
    
    // İlerleme çubuğu
    progressBar = new QProgressBar;
    progressBar->setVisible(false);
    layout->addWidget(progressBar);
    
    // Run from line paneli
    QHBoxLayout *runLayout = new QHBoxLayout;
    runFromLineSpinBox = new QSpinBox;
    runFromLineSpinBox->setMinimum(1);
    runFromLineSpinBox->setMaximum(1);
    runFromLineBtn = new QPushButton("Satırdan Başlat");
    totalLinesLabel = new QLabel("/ 0");
    runLayout->addWidget(new QLabel("Satır:"));
    runLayout->addWidget(runFromLineSpinBox);
    runLayout->addWidget(totalLinesLabel);
    runLayout->addWidget(runFromLineBtn);
    layout->addLayout(runLayout);
    
    connect(runFromLineBtn, &QPushButton::clicked, this, &MainWindow::runFromLine);
}

void MainWindow::createSimulationPanel()
{
    simulationGroup = new QGroupBox("Simülasyon");
    QVBoxLayout *layout = new QVBoxLayout(simulationGroup);

    // OpenGL 3D simülasyon alanı
    openGLWidget = new OpenGLWidget;
    layout->addWidget(openGLWidget, 1);

    // Kontrol butonları
    QGridLayout *controlLayout = new QGridLayout;
    startBtn = new QPushButton("Başlat");
    stopBtn = new QPushButton("Durdur");
    pauseBtn = new QPushButton("Duraklat");
    resetBtn = new QPushButton("Sıfırla");
    controlLayout->addWidget(startBtn, 0, 0);
    controlLayout->addWidget(stopBtn, 0, 1);
    controlLayout->addWidget(pauseBtn, 1, 0);
    controlLayout->addWidget(resetBtn, 1, 1);
    layout->addLayout(controlLayout);

    // Durum bilgileri
    QGroupBox *statusGroup = new QGroupBox("Durum");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    QLabel *statusLabel = new QLabel("Durum: Hazır");
    QLabel *lineLabel = new QLabel("Satır: 0/0");
    QLabel *timeLabel = new QLabel("Süre: 00:00");
    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(lineLabel);
    statusLayout->addWidget(timeLabel);
    layout->addWidget(statusGroup);
}

void MainWindow::createEmergencyStopPanel()
{
    // Emergency Stop butonu - Ana pencereye ekle
    emergencyStopBtn = new QPushButton("EMERGENCY STOP");
    emergencyStopBtn->setMinimumHeight(80);
    emergencyStopBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #ff0000; "
        "color: white; "
        "font-size: 18px; "
        "font-weight: bold; "
        "border: 3px solid #cc0000; "
        "border-radius: 10px; "
        "padding: 10px; "
        "}"
        "QPushButton:hover { "
        "background-color: #ff3333; "
        "border-color: #ff0000; "
        "}"
        "QPushButton:pressed { "
        "background-color: #cc0000; "
        "border-color: #990000; "
        "}"
    );
    
    // Emergency Stop durumu etiketi
    emergencyStopLabel = new QLabel("Sistem Hazır");
    emergencyStopLabel->setAlignment(Qt::AlignCenter);
    emergencyStopLabel->setStyleSheet(
        "QLabel { "
        "color: #00aa00; "
        "font-size: 14px; "
        "font-weight: bold; "
        "padding: 5px; "
        "border: 2px solid #00aa00; "
        "border-radius: 5px; "
        "background-color: #e8f5e8; "
        "}"
    );
    
    // Emergency Stop panelini ana pencereye ekle
    QVBoxLayout *emergencyLayout = new QVBoxLayout;
    emergencyLayout->addWidget(emergencyStopBtn);
    emergencyLayout->addWidget(emergencyStopLabel);
    
    // Ana pencereye ekle (en üstte)
    QWidget *emergencyWidget = new QWidget;
    emergencyWidget->setLayout(emergencyLayout);
    emergencyWidget->setMaximumHeight(150);
    
    // Ana layout'a ekle
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(centralWidget->layout());
    if (mainLayout) {
        mainLayout->insertWidget(0, emergencyWidget);
    }
}

void MainWindow::setupConnections()
{
    // Eksen kontrolü - Buton basılı tutma olayları
    connect(xPosBtn, &QPushButton::pressed, [this]() { startContinuousJog('X', true); });
    connect(xPosBtn, &QPushButton::released, this, &MainWindow::stopContinuousJog);
    connect(xNegBtn, &QPushButton::pressed, [this]() { startContinuousJog('X', false); });
    connect(xNegBtn, &QPushButton::released, this, &MainWindow::stopContinuousJog);
    
    connect(yPosBtn, &QPushButton::pressed, [this]() { startContinuousJog('Y', true); });
    connect(yPosBtn, &QPushButton::released, this, &MainWindow::stopContinuousJog);
    connect(yNegBtn, &QPushButton::pressed, [this]() { startContinuousJog('Y', false); });
    connect(yNegBtn, &QPushButton::released, this, &MainWindow::stopContinuousJog);
    
    connect(zPosBtn, &QPushButton::pressed, [this]() { startContinuousJog('Z', true); });
    connect(zPosBtn, &QPushButton::released, this, &MainWindow::stopContinuousJog);
    connect(zNegBtn, &QPushButton::pressed, [this]() { startContinuousJog('Z', false); });
    connect(zNegBtn, &QPushButton::released, this, &MainWindow::stopContinuousJog);
    
    // Tek tıklama için (eski fonksiyonlar)
    connect(xPosBtn, &QPushButton::clicked, this, &MainWindow::jogXPositive);
    connect(xNegBtn, &QPushButton::clicked, this, &MainWindow::jogXNegative);
    connect(yPosBtn, &QPushButton::clicked, this, &MainWindow::jogYPositive);
    connect(yNegBtn, &QPushButton::clicked, this, &MainWindow::jogYNegative);
    connect(zPosBtn, &QPushButton::clicked, this, &MainWindow::jogZPositive);
    connect(zNegBtn, &QPushButton::clicked, this, &MainWindow::jogZNegative);
    
    // G-code işlemleri
    connect(openFileBtn, &QPushButton::clicked, this, &MainWindow::openGCodeFile);
    connect(saveFileBtn, &QPushButton::clicked, this, &MainWindow::saveGCodeFile);
    connect(sendCommandBtn, &QPushButton::clicked, this, &MainWindow::sendGCodeCommand);
    connect(commandEdit, &QLineEdit::returnPressed, this, &MainWindow::sendGCodeCommand);
    
    // Emergency Stop
    connect(emergencyStopBtn, &QPushButton::clicked, this, &MainWindow::emergencyStop);
    
    // Emergency Stop butonuna sağ tık ile reset özelliği
    emergencyStopBtn->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(emergencyStopBtn, &QPushButton::customContextMenuRequested, [this](const QPoint &pos) {
        if (emergencyStopActive) {
            QMenu *menu = new QMenu(this);
            QAction *resetAction = menu->addAction("Emergency Stop'u Reset Et");
            connect(resetAction, &QAction::triggered, this, &MainWindow::resetEmergencyStop);
            menu->exec(emergencyStopBtn->mapToGlobal(pos));
        }
    });
    
    // CNC kontrolü
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::startCNC);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::stopCNC);
    connect(pauseBtn, &QPushButton::clicked, this, &MainWindow::pauseCNC);
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::resetCNC);
    
    // YENİ: Hardware limit switch kontrolleri
    if (serialComm) {
        connect(serialComm, &SerialCommunication::limitSwitchTriggered, this, [this](char axis, bool isMin) {
            QString axisName = QString(axis);
            QString direction = isMin ? "Min" : "Max";
            logMessage(QString("HARDWARE LIMIT SWITCH TETİKLENDİ: %1 %2").arg(axisName).arg(direction));
            emergencyStop();
        });
        
        connect(serialComm, &SerialCommunication::limitSwitchReleased, this, [this](char axis, bool isMin) {
            QString axisName = QString(axis);
            QString direction = isMin ? "Min" : "Max";
            logMessage(QString("Hardware limit switch serbest: %1 %2").arg(axisName).arg(direction));
        });
        
        connect(serialComm, &SerialCommunication::limitSwitchStatusChanged, this, [this](const LimitSwitchStatus &status) {
            updateLimitSwitchDisplay(status);
        });
        
        // YENİ: Spindle kontrolleri
        connect(serialComm, &SerialCommunication::spindleStateChanged, this, [this](SpindleState state) {
            updateSpindleDisplay(state);
        });
        
        connect(serialComm, &SerialCommunication::spindleSpeedChanged, this, [this](double speed) {
            updateSpindleSpeedDisplay(speed);
        });
        
        connect(serialComm, &SerialCommunication::coolantStateChanged, this, [this](bool on) {
            updateCoolantDisplay(on);
        });
        
        // YENİ: Homing kontrolleri
        connect(serialComm, &SerialCommunication::homingStarted, this, [this]() {
            logMessage("Homing işlemi başladı");
            updateStatusBar("Homing işlemi başladı");
        });
        
        connect(serialComm, &SerialCommunication::homingCompleted, this, [this]() {
            logMessage("Homing işlemi tamamlandı");
            updateStatusBar("Homing tamamlandı");
            // Homing tamamlandıktan sonra emergency stop reset edilebilir
            if (emergencyStopActive) {
                showHomingCompletedDialog();
            }
        });
        
        connect(serialComm, &SerialCommunication::homingFailed, this, [this](const QString &error) {
            logMessage("Homing hatası: " + error);
            updateStatusBar("Homing hatası: " + error);
        });
        
        // YENİ: Güvenlik timeout
        connect(serialComm, &SerialCommunication::safetyTimeoutOccurred, this, [this]() {
            logMessage("GÜVENLİK TIMEOUT - Sistem acil durduruldu");
            emergencyStop();
        });
    }
}

// Slot fonksiyonları
void MainWindow::openGCodeFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "G-Code Dosyası Aç", "", "G-Code Files (*.gcode *.nc *.txt);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            gcodeEditor->setPlainText(content);
            currentGCodeFile = fileName;
            updateStatusBar("Dosya açıldı: " + fileName);
            logMessage("G-code dosyası açıldı: " + fileName);
            updateTotalLines(); // Dosya açıldığında toplam satır sayısını güncelle
        } else {
            QMessageBox::warning(this, "Hata", "Dosya açılamadı!");
        }
    }
}

void MainWindow::saveGCodeFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "G-Code Dosyası Kaydet", "", "G-Code Files (*.gcode *.nc *.txt);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << gcodeEditor->toPlainText();
            currentGCodeFile = fileName;
            updateStatusBar("Dosya kaydedildi: " + fileName);
            logMessage("G-code dosyası kaydedildi: " + fileName);
            updateTotalLines(); // Dosya kaydedildiğinde toplam satır sayısını güncelle
        } else {
            QMessageBox::warning(this, "Hata", "Dosya kaydedilemedi!");
        }
    }
}

void MainWindow::jogXPositive()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    if (axisController) {
        axisController->startJog('X', true, jogStep, jogSpeed);
        logMessage(QString("X+ Jog: %1 mm (adım: %2 mm)").arg(currentX + jogStep).arg(jogStep));
    }
}

void MainWindow::jogXNegative()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    if (axisController) {
        axisController->startJog('X', false, jogStep, jogSpeed);
        logMessage(QString("X- Jog: %1 mm (adım: %2 mm)").arg(currentX - jogStep).arg(jogStep));
    }
}

void MainWindow::jogYPositive()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    if (axisController) {
        axisController->startJog('Y', true, jogStep, jogSpeed);
        logMessage(QString("Y+ Jog: %1 mm (adım: %2 mm)").arg(currentY + jogStep).arg(jogStep));
    }
}

void MainWindow::jogYNegative()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    if (axisController) {
        axisController->startJog('Y', false, jogStep, jogSpeed);
        logMessage(QString("Y- Jog: %1 mm (adım: %2 mm)").arg(currentY - jogStep).arg(jogStep));
    }
}

void MainWindow::jogZPositive()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    if (axisController) {
        axisController->startJog('Z', true, jogStep, jogSpeed);
        logMessage(QString("Z+ Jog: %1 mm (adım: %2 mm)").arg(currentZ + jogStep).arg(jogStep));
    }
}

void MainWindow::jogZNegative()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    if (axisController) {
        axisController->startJog('Z', false, jogStep, jogSpeed);
        logMessage(QString("Z- Jog: %1 mm (adım: %2 mm)").arg(currentZ - jogStep).arg(jogStep));
    }
}

void MainWindow::startCNC()
{
    updateStatusBar("CNC Başlatıldı");
    logMessage("CNC işlemi başlatıldı");
    startBtn->setEnabled(false);
    stopBtn->setEnabled(true);
    pauseBtn->setEnabled(true);
}

void MainWindow::stopCNC()
{
    updateStatusBar("CNC Durduruldu");
    logMessage("CNC işlemi durduruldu");
    startBtn->setEnabled(true);
    stopBtn->setEnabled(false);
    pauseBtn->setEnabled(false);
}

void MainWindow::pauseCNC()
{
    updateStatusBar("CNC Duraklatıldı");
    logMessage("CNC işlemi duraklatıldı");
}

void MainWindow::emergencyStop()
{
    if (!emergencyStopActive) {
        emergencyStopActive = true;
        
        // Tüm hareketleri durdur
        stopContinuousJog();
        
        // CNC işlemini durdur
        stopCNC();
        
        // Hardware emergency stop gönder
        if (serialComm && serialComm->isConnected()) {
            serialComm->sendEmergencyStop();
        }
        
        // Buton görünümünü güncelle
        emergencyStopBtn->setText("EMERGENCY STOP\n(Reset to Continue)");
        emergencyStopBtn->setStyleSheet(
            "QPushButton { "
            "background-color: #cc0000; "
            "color: white; "
            "font-size: 16px; "
            "font-weight: bold; "
            "border: 3px solid #990000; "
            "border-radius: 10px; "
            "padding: 10px; "
            "}"
        );
        
        // Durum etiketini güncelle
        emergencyStopLabel->setText("EMERGENCY STOP AKTİF!");
        emergencyStopLabel->setStyleSheet(
            "QLabel { "
            "color: #ff0000; "
            "font-size: 14px; "
            "font-weight: bold; "
            "padding: 5px; "
            "border: 2px solid #ff0000; "
            "border-radius: 5px; "
            "background-color: #ffe8e8; "
            "}"
        );
        
        // Tüm kontrol butonlarını devre dışı bırak
        startBtn->setEnabled(false);
        stopBtn->setEnabled(false);
        pauseBtn->setEnabled(false);
        xPosBtn->setEnabled(false);
        xNegBtn->setEnabled(false);
        yPosBtn->setEnabled(false);
        yNegBtn->setEnabled(false);
        zPosBtn->setEnabled(false);
        zNegBtn->setEnabled(false);
        
        // Log mesajı
        logMessage("EMERGENCY STOP AKTİF! Tüm hareketler durduruldu.");
        updateStatusBar("EMERGENCY STOP - Sistem Durduruldu");
        
        // Sesli uyarı (Windows için)
        QApplication::beep();
    }
}

void MainWindow::resetEmergencyStop()
{
    if (emergencyStopActive) {
        // YENİ: Güvenli reset prosedürü
        if (serialComm && serialComm->isConnected()) {
            // Önce limit switch durumunu kontrol et
            LimitSwitchStatus limitStatus = serialComm->getLimitSwitchStatus();
            if (limitStatus.anyTriggered) {
                QMessageBox::warning(this, "Güvenlik Uyarısı", 
                    "Hardware limit switch tetikli! Önce limit switch'i serbest bırakın.");
                return;
            }
            
            // Homing işlemi gerekli mi kontrol et
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Güvenlik Kontrolü",
                "Emergency Stop sonrası güvenli sıfırlama için homing işlemi gerekli.\n"
                "Homing işlemini başlatmak istiyor musunuz?",
                QMessageBox::Yes | QMessageBox::No);
            
            if (reply == QMessageBox::Yes) {
                // Homing işlemini başlat
                if (serialComm->startHoming()) {
                    logMessage("Emergency Stop reset için homing işlemi başlatıldı");
                    updateStatusBar("Homing işlemi başlatıldı...");
                    return; // Homing tamamlanana kadar bekle
                } else {
                    QMessageBox::warning(this, "Hata", "Homing işlemi başlatılamadı!");
                    return;
                }
            } else {
                // Kullanıcı homing istemiyor, uyarı ver
                QMessageBox::warning(this, "Güvenlik Uyarısı",
                    "Homing yapmadan devam etmek güvenli değildir!\n"
                    "Pozisyon hatası olabilir.");
            }
        }
        
        // Reset işlemini tamamla
        completeEmergencyStopReset();
    }
}

void MainWindow::completeEmergencyStopReset()
{
    emergencyStopActive = false;
    
    // Buton görünümünü normale döndür
    emergencyStopBtn->setText("EMERGENCY STOP");
    emergencyStopBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #ff0000; "
        "color: white; "
        "font-size: 18px; "
        "font-weight: bold; "
        "border: 3px solid #cc0000; "
        "border-radius: 10px; "
        "padding: 10px; "
        "}"
        "QPushButton:hover { "
        "background-color: #ff3333; "
        "border-color: #ff0000; "
        "}"
        "QPushButton:pressed { "
        "background-color: #cc0000; "
        "border-color: #990000; "
        "}"
    );
    
    // Durum etiketini güncelle
    emergencyStopLabel->setText("Sistem Hazır");
    emergencyStopLabel->setStyleSheet(
        "QLabel { "
        "color: #00aa00; "
        "font-size: 14px; "
        "font-weight: bold; "
        "padding: 5px; "
        "border: 2px solid #00aa00; "
        "border-radius: 5px; "
        "background-color: #e8f5e8; "
        "}"
    );
    
    // Tüm kontrol butonlarını tekrar etkinleştir
    startBtn->setEnabled(true);
    stopBtn->setEnabled(true);
    pauseBtn->setEnabled(true);
    xPosBtn->setEnabled(true);
    xNegBtn->setEnabled(true);
    yPosBtn->setEnabled(true);
    yNegBtn->setEnabled(true);
    zPosBtn->setEnabled(true);
    zNegBtn->setEnabled(true);
    
    // Log mesajı
    logMessage("Emergency Stop reset edildi. Sistem tekrar hazır.");
    updateStatusBar("Sistem Hazır");
}

void MainWindow::showHomingCompletedDialog()
{
    QMessageBox::information(this, "Homing Tamamlandı",
        "Homing işlemi başarıyla tamamlandı.\n"
        "Emergency Stop artık güvenli şekilde reset edilebilir.");
    
    // Homing tamamlandıktan sonra reset işlemini tamamla
    completeEmergencyStopReset();
}

// YENİ: Limit switch display fonksiyonları
void MainWindow::updateLimitSwitchDisplay(const LimitSwitchStatus &status)
{
    // Limit switch durumunu UI'da göster
    QString statusText = "Limit Switch: ";
    
    // X ekseni limit switch'leri
    if (status.xMin == LimitSwitchState::Triggered) {
        statusText += "X- ";
        xMinLimitLabel->setText("X-: TETİKLİ!");
        xMinLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #ff0000; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #ff0000; "
            "border-radius: 3px; "
            "background-color: #ffe8e8; "
            "}"
        );
    } else {
        xMinLimitLabel->setText("X-: OK");
        xMinLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #00aa00; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #00aa00; "
            "border-radius: 3px; "
            "background-color: #e8f5e8; "
            "}"
        );
    }
    
    if (status.xMax == LimitSwitchState::Triggered) {
        statusText += "X+ ";
        xMaxLimitLabel->setText("X+: TETİKLİ!");
        xMaxLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #ff0000; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #ff0000; "
            "border-radius: 3px; "
            "background-color: #ffe8e8; "
            "}"
        );
    } else {
        xMaxLimitLabel->setText("X+: OK");
        xMaxLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #00aa00; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #00aa00; "
            "border-radius: 3px; "
            "background-color: #e8f5e8; "
            "}"
        );
    }
    
    // Y ekseni limit switch'leri
    if (status.yMin == LimitSwitchState::Triggered) {
        statusText += "Y- ";
        yMinLimitLabel->setText("Y-: TETİKLİ!");
        yMinLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #ff0000; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #ff0000; "
            "border-radius: 3px; "
            "background-color: #ffe8e8; "
            "}"
        );
    } else {
        yMinLimitLabel->setText("Y-: OK");
        yMinLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #00aa00; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #00aa00; "
            "border-radius: 3px; "
            "background-color: #e8f5e8; "
            "}"
        );
    }
    
    if (status.yMax == LimitSwitchState::Triggered) {
        statusText += "Y+ ";
        yMaxLimitLabel->setText("Y+: TETİKLİ!");
        yMaxLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #ff0000; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #ff0000; "
            "border-radius: 3px; "
            "background-color: #ffe8e8; "
            "}"
        );
    } else {
        yMaxLimitLabel->setText("Y+: OK");
        yMaxLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #00aa00; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #00aa00; "
            "border-radius: 3px; "
            "background-color: #e8f5e8; "
            "}"
        );
    }
    
    // Z ekseni limit switch'leri
    if (status.zMin == LimitSwitchState::Triggered) {
        statusText += "Z- ";
        zMinLimitLabel->setText("Z-: TETİKLİ!");
        zMinLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #ff0000; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #ff0000; "
            "border-radius: 3px; "
            "background-color: #ffe8e8; "
            "}"
        );
    } else {
        zMinLimitLabel->setText("Z-: OK");
        zMinLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #00aa00; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #00aa00; "
            "border-radius: 3px; "
            "background-color: #e8f5e8; "
            "}"
        );
    }
    
    if (status.zMax == LimitSwitchState::Triggered) {
        statusText += "Z+ ";
        zMaxLimitLabel->setText("Z+: TETİKLİ!");
        zMaxLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #ff0000; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #ff0000; "
            "border-radius: 3px; "
            "background-color: #ffe8e8; "
            "}"
        );
    } else {
        zMaxLimitLabel->setText("Z+: OK");
        zMaxLimitLabel->setStyleSheet(
            "QLabel { "
            "color: #00aa00; "
            "font-weight: bold; "
            "padding: 2px; "
            "border: 1px solid #00aa00; "
            "border-radius: 3px; "
            "background-color: #e8f5e8; "
            "}"
        );
    }
    
    if (!status.anyTriggered) {
        statusText += "Tümü OK";
    }
    
    // Status bar'da göster
    updateStatusBar(statusText);
}

// YENİ: Spindle display fonksiyonları
void MainWindow::updateSpindleDisplay(SpindleState state)
{
    QString stateText;
    QString color;
    QString bgColor;
    
    switch (state) {
        case SpindleState::Off:
            stateText = "Durum: KAPALI";
            color = "#666666";
            bgColor = "#f0f0f0";
            break;
        case SpindleState::Clockwise:
            stateText = "Durum: SAAT YÖNÜNDE";
            color = "#008800";
            bgColor = "#e8f5e8";
            break;
        case SpindleState::CounterClockwise:
            stateText = "Durum: TERS YÖNDE";
            color = "#880000";
            bgColor = "#f5e8e8";
            break;
        case SpindleState::Error:
            stateText = "Durum: HATA";
            color = "#ff0000";
            bgColor = "#ffe8e8";
            break;
    }
    
    spindleStateLabel->setText(stateText);
    spindleStateLabel->setStyleSheet(
        QString("QLabel { "
                "color: %1; "
                "font-weight: bold; "
                "padding: 2px; "
                "border: 1px solid %1; "
                "border-radius: 3px; "
                "background-color: %2; "
                "}").arg(color).arg(bgColor)
    );
    
    logMessage(stateText);
}

void MainWindow::updateSpindleSpeedDisplay(double speed)
{
    QString speedText = QString("Hız: %1 RPM").arg(static_cast<int>(speed));
    spindleSpeedLabel->setText(speedText);
    
    // Hıza göre renk değiştir
    QString color = "#666666";
    if (speed > 0) {
        color = "#008800";
    }
    
    spindleSpeedLabel->setStyleSheet(
        QString("QLabel { "
                "color: %1; "
                "font-weight: bold; "
                "padding: 2px; "
                "border: 1px solid %1; "
                "border-radius: 3px; "
                "background-color: #f0f0f0; "
                "}").arg(color)
    );
    
    logMessage(speedText);
}

void MainWindow::updateCoolantDisplay(bool on)
{
    QString status = on ? "Soğutucu: AÇIK" : "Soğutucu: KAPALI";
    QString color = on ? "#008800" : "#666666";
    QString bgColor = on ? "#e8f5e8" : "#f0f0f0";
    
    coolantStateLabel->setText(status);
    coolantStateLabel->setStyleSheet(
        QString("QLabel { "
                "color: %1; "
                "font-weight: bold; "
                "padding: 2px; "
                "border: 1px solid %1; "
                "border-radius: 3px; "
                "background-color: %2; "
                "}").arg(color).arg(bgColor)
    );
    
    logMessage(status);
}

void MainWindow::resetCNC()
{
    updateStatusBar("CNC Sıfırlandı");
    logMessage("CNC işlemi sıfırlandı");
    
    // Emergency Stop'u da reset et
    resetEmergencyStop();
    
    // Sürekli jog'u durdur
    stopContinuousJog();
    
    // Pozisyonları sıfırla
    updateAxisPosition('X', 0.0);
    updateAxisPosition('Y', 0.0);
    updateAxisPosition('Z', 0.0);
    
    // Hedef pozisyonları sıfırla
    xTargetEdit->setText("0.000");
    yTargetEdit->setText("0.000");
    zTargetEdit->setText("0.000");
}

void MainWindow::sendGCodeCommand()
{
    QString command = commandEdit->text().trimmed();
    if (!command.isEmpty() && gcodeParser) {
        GCodeCommand parsedCommand = gcodeParser->parseLine(command);
        if (parsedCommand.isValid) {
            logMessage("G-code komutu gönderildi: " + command);
            if (serialComm && serialComm->isConnected()) {
                serialComm->sendGCodeCommand(command);
            }
        } else {
            logMessage("Geçersiz G-code komutu: " + parsedCommand.errorMessage);
        }
        commandEdit->clear();
    }
}

void MainWindow::processGCodeFile()
{
    QString gcode = gcodeEditor->toPlainText();
    if (!gcode.isEmpty() && gcodeParser) {
        QVector<GCodeCommand> commands = gcodeParser->parseFile(gcode);
        gcodeCommands.clear();
        for (const GCodeCommand &cmd : commands) {
            gcodeCommands.append(cmd.originalLine);
        }
        currentCommandIndex = 0;
        progressBar->setVisible(true);
        progressBar->setMaximum(gcodeCommands.size());
        logMessage(QString("G-code dosyası işleniyor: %1 komut").arg(gcodeCommands.size()));
    }
}

void MainWindow::updateStatusBar(const QString &message)
{
    statusBar->showMessage(message);
}

void MainWindow::logMessage(const QString &message)
{
    // Logger modülünü kullan
    if (logger) {
        logger->info(message, LogCategories::MAIN);
    } else {
        qDebug() << message;
    }
}

void MainWindow::initializeModules()
{
    // Logger'ı başlat
    logger->setLogFile("cnc_controller.log");
    logger->setLogLevel(LogLevel::Info);
    
    // Ayarları başlat
    settings->loadSettings();
    
    // Ayarlardan değerleri yükle
    jogStep = settings->getJogStep();
    jogSpeed = settings->getJogSpeed();
    feedRate = settings->getDefaultFeedRate();
    
    // Eksen limitlerini ayarla
    axisController->setAxisLimits('X', settings->getAxisMinLimit('X'), settings->getAxisMaxLimit('X'));
    axisController->setAxisLimits('Y', settings->getAxisMinLimit('Y'), settings->getAxisMaxLimit('Y'));
    axisController->setAxisLimits('Z', settings->getAxisMinLimit('Z'), settings->getAxisMaxLimit('Z'));
    
    LOG_INFO("Modüller başlatıldı", LogCategories::MAIN);
}

void MainWindow::connectModules()
{
    // Eksen kontrolcüsü sinyallerini bağla
    connect(axisController, &AxisController::positionChanged, this, [this](char axis, double position) {
        updateAxisPosition(axis, position);
    });
    
    connect(axisController, &AxisController::limitReached, this, [this](char axis, double position) {
        logMessage(QString("%1 ekseni limit aşıldı: %2 mm").arg(axis).arg(position));
    });
    
    connect(axisController, &AxisController::emergencyStopActivated, this, [this]() {
        emergencyStop();
    });
    
    // Seri iletişim sinyallerini bağla
    connect(serialComm, &SerialCommunication::connected, this, [this]() {
        updateStatusBar("Seri port bağlandı");
        logMessage("Seri port bağlantısı kuruldu");
    });
    
    connect(serialComm, &SerialCommunication::disconnected, this, [this]() {
        updateStatusBar("Seri port bağlantısı kesildi");
        logMessage("Seri port bağlantısı kesildi");
    });
    
    connect(serialComm, &SerialCommunication::errorOccurred, this, [this](const QString &error) {
        logMessage("Seri port hatası: " + error);
    });
    
    // G-code parser sinyallerini bağla
    connect(gcodeParser, &GCodeParser::parsingCompleted, this, [this](int totalCommands) {
        logMessage(QString("G-code parsing tamamlandı: %1 komut").arg(totalCommands));
    });
    
    connect(gcodeParser, &GCodeParser::parsingError, this, [this](int line, const QString &error) {
        logMessage(QString("G-code parsing hatası (satır %1): %2").arg(line).arg(error));
    });
    
    LOG_INFO("Modül bağlantıları kuruldu", LogCategories::MAIN);
}

// Yeni yardımcı fonksiyonlar
void MainWindow::updateAxisPosition(char axis, double newPosition)
{
    switch (axis) {
        case 'X':
            currentX = newPosition;
            xPosEdit->setText(QString::number(newPosition, 'f', 3));
            break;
        case 'Y':
            currentY = newPosition;
            yPosEdit->setText(QString::number(newPosition, 'f', 3));
            break;
        case 'Z':
            currentZ = newPosition;
            zPosEdit->setText(QString::number(newPosition, 'f', 3));
            break;
    }
}

bool MainWindow::checkSoftLimits(char axis, double newPosition)
{
    double minLimit, maxLimit;
    
    switch (axis) {
        case 'X':
            minLimit = xMinLimit;
            maxLimit = xMaxLimit;
            break;
        case 'Y':
            minLimit = yMinLimit;
            maxLimit = yMaxLimit;
            break;
        case 'Z':
            minLimit = zMinLimit;
            maxLimit = zMaxLimit;
            break;
        default:
            return false;
    }
    
    if (newPosition < minLimit || newPosition > maxLimit) {
        logMessage(QString("%1 ekseni soft limit aşıldı! (%2 mm) Limit: %3 - %4 mm")
                  .arg(axis).arg(newPosition).arg(minLimit).arg(maxLimit));
        return false;
    }
    
    return true;
}

void MainWindow::startContinuousJog(char axis, bool positive)
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Sürekli jog engellendi.");
        return;
    }
    
    if (axisController) {
        axisController->startContinuousJog(axis, positive);
        logMessage(QString("Sürekli %1%2 jog başlatıldı").arg(axis).arg(positive ? "+" : "-"));
    }
}

void MainWindow::stopContinuousJog()
{
    if (axisController) {
        axisController->stopContinuousJog();
        logMessage("Sürekli jog durduruldu");
    }
}

void MainWindow::continuousJogX() {
    if (axisController) {
        axisController->startContinuousJog('X', true);
    }
}

void MainWindow::continuousJogY() {
    if (axisController) {
        axisController->startContinuousJog('Y', true);
    }
}

void MainWindow::continuousJogZ() {
    if (axisController) {
        axisController->startContinuousJog('Z', true);
    }
}

void MainWindow::onJogButtonPressed() {
    // Bu fonksiyon artık kullanılmıyor, modüllere yönlendirme yapılıyor
}

void MainWindow::onJogButtonReleased() {
    // Bu fonksiyon artık kullanılmıyor, modüllere yönlendirme yapılıyor
}


double MainWindow::getJogStep()
{
    return jogStep;
}

// Klavye kısayolları
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_X:
            if (event->modifiers() & Qt::ShiftModifier) {
                startContinuousJog('X', true);  // Shift+X = X+
            } else {
                startContinuousJog('X', false); // X = X-
            }
            break;
        case Qt::Key_Y:
            if (event->modifiers() & Qt::ShiftModifier) {
                startContinuousJog('Y', true);  // Shift+Y = Y+
            } else {
                startContinuousJog('Y', false); // Y = Y-
            }
            break;
        case Qt::Key_Z:
            if (event->modifiers() & Qt::ShiftModifier) {
                startContinuousJog('Z', true);  // Shift+Z = Z+
            } else {
                startContinuousJog('Z', false); // Z = Z-
            }
            break;
        case Qt::Key_Space:
            stopContinuousJog(); // Boşluk = Durdur
            break;
        case Qt::Key_R:
            if (event->modifiers() & Qt::ControlModifier) {
                resetCNC(); // Ctrl+R = Reset
            }
            break;
        case Qt::Key_E:
            emergencyStop(); // E = Emergency Stop
            break;
        default:
            QMainWindow::keyPressEvent(event);
            break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_X:
        case Qt::Key_Y:
        case Qt::Key_Z:
            stopContinuousJog();
            break;
        default:
            QMainWindow::keyReleaseEvent(event);
            break;
    }
} 

void MainWindow::updateJogSpeed()
{
    jogSpeed = jogSpeedSlider->value();
    jogSpeedLabel->setText(QString::number(jogSpeed) + " mm/min");
    jogSpeedCombo->setCurrentText(QString::number(jogSpeed));
    
    // AxisController'a yeni hızı bildir
    if (axisController) {
        axisController->setJogSpeed(jogSpeed);
    }
    
    // Ayarları kaydet
    if (settings) {
        settings->setJogSpeed(jogSpeed);
    }
}

void MainWindow::updateTotalLines()
{
    int total = gcodeEditor->toPlainText().split('\n', Qt::SkipEmptyParts).size();
    runFromLineSpinBox->setMaximum(total > 0 ? total : 1);
    totalLinesLabel->setText("/ " + QString::number(total));
}

void MainWindow::runFromLine()
{
    int line = runFromLineSpinBox->value();
    QStringList lines = gcodeEditor->toPlainText().split('\n', Qt::SkipEmptyParts);
    if (line > 0 && line <= lines.size()) {
        // Sadece seçilen satırdan itibaren komutları işle
        QStringList subLines = lines.mid(line - 1);
        // Burada G-code işleme fonksiyonunu çağırabilirsiniz
        logMessage(QString("%1. satırdan başlatılıyor: %2 komut").arg(line).arg(subLines.size()));
        // Örnek: openGLWidget->setToolpath(...);
    }
} 