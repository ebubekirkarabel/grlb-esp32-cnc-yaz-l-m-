#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QFile>
#include <QTextStream>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , currentCommandIndex(0)
    , jogSpeed(1000)
    , feedRate(1000)
    , speedMultiplier(1.0)
    , jogStep(1.0)
    , currentJogAxis(' ')
    , currentJogPositive(false)
    , jogAcceleration(1.0)
    , currentX(0.0)
    , currentY(0.0)
    , currentZ(0.0)
    , emergencyStopActive(false)
{
    // Pencere ayarları
    setWindowTitle("CNC Controller v1.0");
    resize(1200, 800);
    
    // Pencereyi ekranın ortasına yerleştirme
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    // Soft limits ayarlama (%15-%50 arası)
    xMinLimit = -50.0; xMaxLimit = 50.0;
    yMinLimit = -50.0; yMaxLimit = 50.0;
    zMinLimit = -15.0; zMaxLimit = 15.0;
    
    // Jog timer'ını oluşturma
    jogTimer = new QTimer(this);
    jogTimer->setInterval(50); // 50ms = 20Hz
    
    // UI bileşenlerini oluşturma
    createMenus();
    createToolBar();
    createStatusBar();
    createCentralWidget();
    createEmergencyStopPanel();
    setupConnections();
    
    // Başlangıç durumu
    updateStatusBar("Hazır");
    logMessage("CNC Controller başlatıldı");
}

MainWindow::~MainWindow()
{
}

void MainWindow::createMenus()
{
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
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // Sol panel (Eksen kontrolü)
    createAxisControlPanel();
    mainLayout->addWidget(axisControlGroup, 1);
    
    // Orta panel (G-code)
    createGCodePanel();
    mainLayout->addWidget(gcodeGroup, 2);
    
    // Sağ panel (Simülasyon)
    createSimulationPanel();
    mainLayout->addWidget(simulationGroup, 1);
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
    connect(jogStepCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            [this](const QString &text) { jogStep = text.toDouble(); });
    connect(jogSpeedCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            [this](const QString &text) { jogSpeed = text.toInt(); });
    connect(feedRateCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
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
}

void MainWindow::createSimulationPanel()
{
    simulationGroup = new QGroupBox("Simülasyon");
    QVBoxLayout *layout = new QVBoxLayout(simulationGroup);
    
    // Simülasyon alanı (şimdilik basit bir etiket)
    QLabel *simLabel = new QLabel("3D Simülasyon Alanı\n(Bu alan daha sonra geliştirilecek)");
    simLabel->setAlignment(Qt::AlignCenter);
    simLabel->setStyleSheet("QLabel { background-color: #f0f0f0; border: 1px solid #ccc; padding: 20px; }");
    layout->addWidget(simLabel);
    
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
    
    // Sürekli jog timer bağlantısı
    connect(jogTimer, &QTimer::timeout, [this]() {
        if (currentJogAxis == 'X') continuousJogX();
        else if (currentJogAxis == 'Y') continuousJogY();
        else if (currentJogAxis == 'Z') continuousJogZ();
    });
    
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
    
    double newX = currentX + jogStep;
    if (checkSoftLimits('X', newX)) {
        updateAxisPosition('X', newX);
        logMessage(QString("X+ Jog: %1 mm (adım: %2 mm)").arg(newX).arg(jogStep));
    }
}

void MainWindow::jogXNegative()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    double newX = currentX - jogStep;
    if (checkSoftLimits('X', newX)) {
        updateAxisPosition('X', newX);
        logMessage(QString("X- Jog: %1 mm (adım: %2 mm)").arg(newX).arg(jogStep));
    }
}

void MainWindow::jogYPositive()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    double newY = currentY + jogStep;
    if (checkSoftLimits('Y', newY)) {
        updateAxisPosition('Y', newY);
        logMessage(QString("Y+ Jog: %1 mm (adım: %2 mm)").arg(newY).arg(jogStep));
    }
}

void MainWindow::jogYNegative()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    double newY = currentY - jogStep;
    if (checkSoftLimits('Y', newY)) {
        updateAxisPosition('Y', newY);
        logMessage(QString("Y- Jog: %1 mm (adım: %2 mm)").arg(newY).arg(jogStep));
    }
}

void MainWindow::jogZPositive()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    double newZ = currentZ + jogStep;
    if (checkSoftLimits('Z', newZ)) {
        updateAxisPosition('Z', newZ);
        logMessage(QString("Z+ Jog: %1 mm (adım: %2 mm)").arg(newZ).arg(jogStep));
    }
}

void MainWindow::jogZNegative()
{
    if (emergencyStopActive) {
        logMessage("Emergency Stop aktif! Hareket engellendi.");
        return;
    }
    
    double newZ = currentZ - jogStep;
    if (checkSoftLimits('Z', newZ)) {
        updateAxisPosition('Z', newZ);
        logMessage(QString("Z- Jog: %1 mm (adım: %2 mm)").arg(newZ).arg(jogStep));
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
    if (!command.isEmpty()) {
        logMessage("G-code komutu gönderildi: " + command);
        commandEdit->clear();
        // Burada gerçek G-code işleme kodu gelecek
    }
}

void MainWindow::processGCodeFile()
{
    QString gcode = gcodeEditor->toPlainText();
    if (!gcode.isEmpty()) {
        gcodeCommands = gcode.split('\n', QString::SkipEmptyParts);
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
    // Bu fonksiyon daha sonra log dosyasına yazma için genişletilecek
    qDebug() << message;
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
    
    currentJogAxis = axis;
    currentJogPositive = positive;
    jogAcceleration = 1.0;
    
    if (!jogTimer->isActive()) {
        jogTimer->start();
    }
    
    logMessage(QString("Sürekli %1%2 jog başlatıldı").arg(axis).arg(positive ? "+" : "-"));
}

void MainWindow::stopContinuousJog()
{
    if (jogTimer->isActive()) {
        jogTimer->stop();
        currentJogAxis = ' ';
        jogAcceleration = 1.0;
        logMessage("Sürekli jog durduruldu");
    }
}

void MainWindow::continuousJogX()
{
    double step = (jogSpeed / 60.0) * (jogTimer->interval() / 1000.0) * jogAcceleration;
    double newX = currentX + (currentJogPositive ? step : -step);
    
    if (checkSoftLimits('X', newX)) {
        updateAxisPosition('X', newX);
        jogAcceleration = qMin(jogAcceleration + 0.1, 3.0); // Maksimum 3x hızlanma
    } else {
        stopContinuousJog();
    }
}

void MainWindow::continuousJogY()
{
    double step = (jogSpeed / 60.0) * (jogTimer->interval() / 1000.0) * jogAcceleration;
    double newY = currentY + (currentJogPositive ? step : -step);
    
    if (checkSoftLimits('Y', newY)) {
        updateAxisPosition('Y', newY);
        jogAcceleration = qMin(jogAcceleration + 0.1, 3.0); // Maksimum 3x hızlanma
    } else {
        stopContinuousJog();
    }
}

void MainWindow::continuousJogZ()
{
    double step = (jogSpeed / 60.0) * (jogTimer->interval() / 1000.0) * jogAcceleration;
    double newZ = currentZ + (currentJogPositive ? step : -step);
    
    if (checkSoftLimits('Z', newZ)) {
        updateAxisPosition('Z', newZ);
        jogAcceleration = qMin(jogAcceleration + 0.1, 3.0); // Maksimum 3x hızlanma
    } else {
        stopContinuousJog();
    }
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