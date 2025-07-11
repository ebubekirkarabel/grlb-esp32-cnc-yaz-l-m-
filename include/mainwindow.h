#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QMap>
#include <QVector>
#include <QTimer>
#include <QKeyEvent>
#include <QSlider>
#include <QSpinBox>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    // Dosya işlemleri
    void openGCodeFile();
    void saveGCodeFile();
    
    // Eksen kontrolü - Yeni slot'lar
    void jogXPositive();
    void jogXNegative();
    void jogYPositive();
    void jogYNegative();
    void jogZPositive();
    void jogZNegative();
    
    // Sürekli jog için timer slot'ları
    void continuousJogX();
    void continuousJogY();
    void continuousJogZ();
    
    // Buton basılı tutma olayları
    void onJogButtonPressed();
    void onJogButtonReleased();
    
    // Emergency Stop
    void emergencyStop();
    void resetEmergencyStop();
    
    // CNC kontrolü
    void startCNC();
    void stopCNC();
    void pauseCNC();
    void resetCNC();
    
    // G-code işleme
    void sendGCodeCommand();
    void processGCodeFile();
    void runFromLine();
    void updateJogSpeed();
    void updateTotalLines();

private:
    // UI bileşenleri oluşturma
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void createCentralWidget();
    void createAxisControlPanel();
    void createGCodePanel();
    void createSimulationPanel();
    void createEmergencyStopPanel();
    
    // Yardımcı fonksiyonlar
    void setupConnections();
    void updateStatusBar(const QString &message);
    void logMessage(const QString &message);
    
    // Yeni yardımcı fonksiyonlar
    void updateAxisPosition(char axis, double newPosition);
    bool checkSoftLimits(char axis, double newPosition);
    void startContinuousJog(char axis, bool positive);
    void stopContinuousJog();
    double getJogStep();
    
    // Emergency Stop fonksiyonları
    void emergencyStop();
    void resetEmergencyStop();
    bool isEmergencyStopActive() const { return emergencyStopActive; }
    
    // UI bileşenleri
    QWidget *centralWidget;
    
    // Menü ve toolbar
    QMenuBar *menuBar;
    QToolBar *toolBar;
    QStatusBar *statusBar;
    
    // Ana paneller
    QGroupBox *axisControlGroup;
    QGroupBox *gcodeGroup;
    QGroupBox *simulationGroup;
    
    // OpenGL 3D görselleştirme
    OpenGLWidget *openGLWidget;
    
    // Eksen kontrolü - Güncellenmiş
    QPushButton *xPosBtn, *xNegBtn;
    QPushButton *yPosBtn, *yNegBtn;
    QPushButton *zPosBtn, *zNegBtn;
    QLineEdit *xPosEdit, *yPosEdit, *zPosEdit;
    QLineEdit *xTargetEdit, *yTargetEdit, *zTargetEdit;
    
    // Yeni eksen kontrolü bileşenleri
    QComboBox *jogStepCombo;  // Jog adım mesafesi
    QComboBox *jogSpeedCombo; // Jog hızı
    QSlider *jogSpeedSlider;  // Jog hızı slider
    QLabel *jogSpeedLabel;    // Jog hızı gösterge
    QComboBox *feedRateCombo; // Feedrate
    
    // Sürekli jog için timer'lar
    QTimer *jogTimer;
    char currentJogAxis;      // Hangi eksende jog yapılıyor
    bool currentJogPositive;  // Pozitif mi negatif mi
    double jogAcceleration;   // Hızlanma faktörü
    
    // G-code paneli
    QTextEdit *gcodeEditor;
    QPushButton *openFileBtn, *saveFileBtn;
    QPushButton *sendCommandBtn;
    QLineEdit *commandEdit;
    QProgressBar *progressBar;
    
    // Run from line özelliği
    QSpinBox *runFromLineSpinBox;
    QPushButton *runFromLineBtn;
    QLabel *totalLinesLabel;
    
    // Kontrol paneli
    QPushButton *startBtn, *stopBtn, *pauseBtn, *resetBtn;
    
    // G-code dosyası
    QString currentGCodeFile;
    QStringList gcodeCommands;
    int currentCommandIndex;
    
    // Hız kontrolü - Güncellenmiş
    int jogSpeed;           // Jog hızı (mm/min)
    int feedRate;           // Feedrate (mm/min)
    double speedMultiplier; // Hız çarpanı (0.1 - 2.0)
    double jogStep;         // Jog adım mesafesi (mm)
    
    // Soft limits
    double xMinLimit, xMaxLimit;
    double yMinLimit, yMaxLimit;
    double zMinLimit, zMaxLimit;
    
    // Mevcut pozisyonlar
    double currentX, currentY, currentZ;
    
    // Emergency Stop
    bool emergencyStopActive;
    QPushButton *emergencyStopBtn;
    QLabel *emergencyStopLabel;
};

#endif // MAINWINDOW_H 