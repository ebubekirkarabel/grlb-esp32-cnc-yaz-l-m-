#ifndef TOOLPATHVISUALIZER_H
#define TOOLPATHVISUALIZER_H

#include <QWidget>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QMaterial>
#include <Qt3DRender/QMesh>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QSphereMesh>
#include <QVector3D>
#include <QVector>
#include <QString>

// Toolpath noktası
struct ToolpathPoint {
    QVector3D position;
    bool isRapid;  // G0 = rapid, G1 = cutting
    double feedRate;
    int lineNumber;
};

// Görselleştirme ayarları
struct VisualizationSettings {
    bool showToolpath = true;
    bool showWorkpiece = true;
    bool showMachine = true;
    bool showGrid = true;
    float toolpathWidth = 2.0f;
    float rapidColor[3] = {1.0f, 0.0f, 0.0f};    // Kırmızı
    float cuttingColor[3] = {0.0f, 1.0f, 0.0f};  // Yeşil
    float workpieceColor[3] = {0.8f, 0.8f, 0.8f}; // Gri
    float machineColor[3] = {0.2f, 0.2f, 0.2f};  // Koyu gri
};

class ToolpathVisualizer : public QWidget
{
    Q_OBJECT

public:
    explicit ToolpathVisualizer(QWidget *parent = nullptr);
    ~ToolpathVisualizer();

    // Toolpath yönetimi
    void clearToolpath();
    void addToolpathPoint(const QVector3D &position, bool isRapid = false, double feedRate = 1000.0);
    void setToolpath(const QVector<ToolpathPoint> &toolpath);
    void updateCurrentPosition(const QVector3D &position);
    
    // Görselleştirme ayarları
    void setVisualizationSettings(const VisualizationSettings &settings);
    VisualizationSettings getVisualizationSettings() const;
    
    // Kamera kontrolü
    void resetCamera();
    void setCameraPosition(const QVector3D &position);
    void setCameraTarget(const QVector3D &target);
    
    // Workpiece ayarları
    void setWorkpieceSize(float width, float height, float depth);
    void setWorkpiecePosition(const QVector3D &position);
    
    // Makine sınırları
    void setMachineLimits(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
    
    // G-code analizi
    void analyzeGCode(const QStringList &gcodeLines);
    void highlightLine(int lineNumber);
    void showProgress(int currentLine, int totalLines);

signals:
    void toolpathUpdated();
    void cameraChanged();
    void lineSelected(int lineNumber);

private slots:
    void onFrameSwapped();

private:
    // Qt3D bileşenleri
    Qt3DExtras::Qt3DWindow *m_3dWindow;
    Qt3DCore::QEntity *m_rootEntity;
    Qt3DRender::QCamera *m_camera;
    Qt3DExtras::QOrbitCameraController *m_cameraController;
    
    // Toolpath bileşenleri
    Qt3DCore::QEntity *m_toolpathEntity;
    QVector<ToolpathPoint> m_toolpath;
    QVector<Qt3DCore::QEntity*> m_toolpathLines;
    
    // Workpiece
    Qt3DCore::QEntity *m_workpieceEntity;
    Qt3DExtras::QCuboidMesh *m_workpieceMesh;
    Qt3DExtras::QPhongMaterial *m_workpieceMaterial;
    
    // Makine sınırları
    Qt3DCore::QEntity *m_machineEntity;
    Qt3DExtras::QCuboidMesh *m_machineMesh;
    Qt3DExtras::QPhongMaterial *m_machineMaterial;
    
    // Grid
    Qt3DCore::QEntity *m_gridEntity;
    
    // Mevcut pozisyon
    Qt3DCore::QEntity *m_currentPositionEntity;
    Qt3DExtras::QSphereMesh *m_currentPositionMesh;
    Qt3DExtras::QPhongMaterial *m_currentPositionMaterial;
    
    // Ayarlar
    VisualizationSettings m_settings;
    
    // Yardımcı fonksiyonlar
    void setupScene();
    void setupCamera();
    void setupMaterials();
    void createToolpathLine(const QVector3D &start, const QVector3D &end, bool isRapid);
    void createWorkpiece();
    void createMachineBounds();
    void createGrid();
    void updateCurrentPositionVisual();
    void parseGCodeLine(const QString &line, QVector3D &position, bool &isRapid, double &feedRate);
    
    // G-code parser yardımcıları
    double extractParameter(const QString &line, QChar param);
    bool hasParameter(const QString &line, QChar param);
};

#endif // TOOLPATHVISUALIZER_H 