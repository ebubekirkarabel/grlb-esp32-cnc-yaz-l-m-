#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector>
#include <QMouseEvent>
#include <QWheelEvent>

// Toolpath noktası
struct ToolpathPoint {
    QVector3D position;
    bool isRapid;  // G0 = rapid, G1 = cutting
    double feedRate;
    int lineNumber;
};

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget();

    // Toolpath yönetimi
    void clearToolpath();
    void addToolpathPoint(const QVector3D &position, bool isRapid = false);
    void setToolpath(const QVector<ToolpathPoint> &toolpath);
    void updateCurrentPosition(const QVector3D &position);
    
    // Görselleştirme ayarları
    void setShowGrid(bool show);
    void setShowWorkpiece(bool show);
    void setShowMachineBounds(bool show);
    void setWorkpieceSize(float width, float height, float depth);
    void setMachineBounds(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
    
    // Kamera kontrolü
    void resetCamera();
    void setCameraDistance(float distance);
    void setCameraRotation(float xAngle, float yAngle);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    
    // Mouse kontrolü
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    // OpenGL nesneleri
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLShaderProgram *m_shaderProgram;
    
    // Toolpath verileri
    QVector<ToolpathPoint> m_toolpath;
    QVector<QVector3D> m_rapidLines;
    QVector<QVector3D> m_cuttingLines;
    QVector3D m_currentPosition;
    
    // Kamera
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_modelMatrix;
    float m_cameraDistance;
    float m_cameraXAngle;
    float m_cameraYAngle;
    
    // Mouse kontrolü
    QPoint m_lastMousePos;
    bool m_mousePressed;
    
    // Görselleştirme ayarları
    bool m_showGrid;
    bool m_showWorkpiece;
    bool m_showMachineBounds;
    QVector3D m_workpieceSize;
    QVector3D m_machineMin;
    QVector3D m_machineMax;
    
    // Shader kaynak kodları
    const char* vertexShaderSource;
    const char* fragmentShaderSource;
    
    // Yardımcı fonksiyonlar
    void initializeShaders();
    void setupVertexAttribs();
    void drawGrid();
    void drawWorkpiece();
    void drawMachineBounds();
    void drawToolpath();
    void drawCurrentPosition();
    void updateMatrices();
    void processToolpath();
    
    // Shader uniform değişkenleri
    int m_mvpMatrixLoc;
    int m_colorLoc;
    int m_vertexLoc;

signals:
    void cameraChanged();
    void toolpathUpdated();
};

#endif // OPENGLWIDGET_H 