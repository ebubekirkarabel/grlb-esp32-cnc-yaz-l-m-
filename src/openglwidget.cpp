#include "openglwidget.h"
#include <QOpenGLShader>
#include <QOpenGLContext>
#include <QDebug>

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_shaderProgram(nullptr),
      m_cameraDistance(200.0f),
      m_cameraXAngle(30.0f),
      m_cameraYAngle(-45.0f),
      m_mousePressed(false),
      m_showGrid(true),
      m_showWorkpiece(true),
      m_showMachineBounds(true),
      m_workpieceSize(100, 100, 10),
      m_machineMin(-50, -50, 0),
      m_machineMax(50, 50, 50)
{
    setMinimumSize(400, 300);
    setFocusPolicy(Qt::StrongFocus);
}

OpenGLWidget::~OpenGLWidget() {
    makeCurrent();
    if (m_shaderProgram) delete m_shaderProgram;
    m_vbo.destroy();
    m_vao.destroy();
    doneCurrent();
}

void OpenGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glClearColor(0.97f, 0.97f, 0.97f, 1.0f);

    // Basit shader
    m_shaderProgram = new QOpenGLShaderProgram(this);
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,
        "attribute vec3 vertex;\n"
        "uniform mat4 mvpMatrix;\n"
        "void main() {\n"
        "    gl_Position = mvpMatrix * vec4(vertex, 1.0);\n"
        "}"
    );
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
        "uniform vec3 color;\n"
        "void main() {\n"
        "    gl_FragColor = vec4(color, 1.0);\n"
        "}"
    );
    m_shaderProgram->link();
    m_vertexLoc = m_shaderProgram->attributeLocation("vertex");
    m_mvpMatrixLoc = m_shaderProgram->uniformLocation("mvpMatrix");
    m_colorLoc = m_shaderProgram->uniformLocation("color");
}

void OpenGLWidget::resizeGL(int w, int h) {
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(45.0f, float(w) / float(h), 1.0f, 1000.0f);
}

void OpenGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_viewMatrix.setToIdentity();
    m_viewMatrix.translate(0, 0, -m_cameraDistance);
    m_viewMatrix.rotate(m_cameraXAngle, 1, 0, 0);
    m_viewMatrix.rotate(m_cameraYAngle, 0, 1, 0);
    m_modelMatrix.setToIdentity();
    QMatrix4x4 mvp = m_projectionMatrix * m_viewMatrix * m_modelMatrix;

    m_shaderProgram->bind();
    m_shaderProgram->setUniformValue(m_mvpMatrixLoc, mvp);

    // Grid çiz
    if (m_showGrid) drawGrid();
    // Workpiece çiz
    if (m_showWorkpiece) drawWorkpiece();
    // Makine sınırları çiz
    if (m_showMachineBounds) drawMachineBounds();
    // Toolpath çiz
    drawToolpath();
    // Mevcut pozisyon çiz
    drawCurrentPosition();

    m_shaderProgram->release();
}

void OpenGLWidget::drawGrid() {
    QVector<QVector3D> lines;
    float gridSize = 100.0f;
    int numLines = 21;
    float step = gridSize / (numLines - 1);
    float start = -gridSize / 2;
    for (int i = 0; i < numLines; ++i) {
        float pos = start + i * step;
        lines.append(QVector3D(pos, start, 0));
        lines.append(QVector3D(pos, -start, 0));
        lines.append(QVector3D(start, pos, 0));
        lines.append(QVector3D(-start, pos, 0));
    }
    m_shaderProgram->setUniformValue(m_colorLoc, QVector3D(0.8f, 0.8f, 0.8f));
    glLineWidth(1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, lines.constData());
    glDrawArrays(GL_LINES, 0, lines.size());
    glDisableClientState(GL_VERTEX_ARRAY);
}

void OpenGLWidget::drawWorkpiece() {
    // Basit dikdörtgen prizma
    float w = m_workpieceSize.x();
    float h = m_workpieceSize.y();
    float d = m_workpieceSize.z();
    QVector<QVector3D> corners = {
        { -w/2, -h/2, 0 }, { w/2, -h/2, 0 },
        { w/2, h/2, 0 }, { -w/2, h/2, 0 },
        { -w/2, -h/2, d }, { w/2, -h/2, d },
        { w/2, h/2, d }, { -w/2, h/2, d }
    };
    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, // alt
        {4,5},{5,6},{6,7},{7,4}, // üst
        {0,4},{1,5},{2,6},{3,7} // dikey
    };
    m_shaderProgram->setUniformValue(m_colorLoc, QVector3D(0.6f, 0.6f, 0.9f));
    glLineWidth(2.0f);
    for (int i = 0; i < 12; ++i) {
        QVector<QVector3D> line = { corners[edges[i][0]], corners[edges[i][1]] };
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, line.constData());
        glDrawArrays(GL_LINES, 0, 2);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
}

void OpenGLWidget::drawMachineBounds() {
    // Makine sınırları için kutu
    float xMin = m_machineMin.x(), xMax = m_machineMax.x();
    float yMin = m_machineMin.y(), yMax = m_machineMax.y();
    float zMin = m_machineMin.z(), zMax = m_machineMax.z();
    QVector<QVector3D> corners = {
        { xMin, yMin, zMin }, { xMax, yMin, zMin },
        { xMax, yMax, zMin }, { xMin, yMax, zMin },
        { xMin, yMin, zMax }, { xMax, yMin, zMax },
        { xMax, yMax, zMax }, { xMin, yMax, zMax }
    };
    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, // alt
        {4,5},{5,6},{6,7},{7,4}, // üst
        {0,4},{1,5},{2,6},{3,7} // dikey
    };
    m_shaderProgram->setUniformValue(m_colorLoc, QVector3D(0.2f, 0.2f, 0.2f));
    glLineWidth(1.0f);
    for (int i = 0; i < 12; ++i) {
        QVector<QVector3D> line = { corners[edges[i][0]], corners[edges[i][1]] };
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, line.constData());
        glDrawArrays(GL_LINES, 0, 2);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
}

void OpenGLWidget::drawToolpath() {
    if (m_toolpath.isEmpty()) return;
    m_shaderProgram->setUniformValue(m_colorLoc, QVector3D(0.0f, 0.7f, 0.0f));
    glLineWidth(2.0f);
    QVector<QVector3D> points;
    for (const auto &pt : m_toolpath) points.append(pt.position);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, points.constData());
    glDrawArrays(GL_LINE_STRIP, 0, points.size());
    glDisableClientState(GL_VERTEX_ARRAY);
}

void OpenGLWidget::drawCurrentPosition() {
    // Küçük bir nokta
    m_shaderProgram->setUniformValue(m_colorLoc, QVector3D(1.0f, 0.0f, 0.0f));
    glPointSize(8.0f);
    QVector<QVector3D> pt = { m_currentPosition };
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, pt.constData());
    glDrawArrays(GL_POINTS, 0, 1);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void OpenGLWidget::clearToolpath() {
    m_toolpath.clear();
    update();
}

void OpenGLWidget::addToolpathPoint(const QVector3D &position, bool isRapid) {
    ToolpathPoint pt;
    pt.position = position;
    pt.isRapid = isRapid;
    pt.feedRate = 1000.0;
    pt.lineNumber = m_toolpath.size();
    m_toolpath.append(pt);
    update();
}

void OpenGLWidget::setToolpath(const QVector<ToolpathPoint> &toolpath) {
    m_toolpath = toolpath;
    update();
}

void OpenGLWidget::updateCurrentPosition(const QVector3D &position) {
    m_currentPosition = position;
    update();
}

void OpenGLWidget::setShowGrid(bool show) { m_showGrid = show; update(); }
void OpenGLWidget::setShowWorkpiece(bool show) { m_showWorkpiece = show; update(); }
void OpenGLWidget::setShowMachineBounds(bool show) { m_showMachineBounds = show; update(); }
void OpenGLWidget::setWorkpieceSize(float w, float h, float d) { m_workpieceSize = QVector3D(w, h, d); update(); }
void OpenGLWidget::setMachineBounds(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax) { m_machineMin = QVector3D(xMin, yMin, zMin); m_machineMax = QVector3D(xMax, yMax, zMax); update(); }
void OpenGLWidget::resetCamera() { m_cameraDistance = 200.0f; m_cameraXAngle = 30.0f; m_cameraYAngle = -45.0f; update(); }
void OpenGLWidget::setCameraDistance(float d) { m_cameraDistance = d; update(); }
void OpenGLWidget::setCameraRotation(float x, float y) { m_cameraXAngle = x; m_cameraYAngle = y; update(); }

void OpenGLWidget::mousePressEvent(QMouseEvent *event) {
    m_mousePressed = true;
    m_lastMousePos = event->pos();
}
void OpenGLWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_mousePressed) {
        int dx = event->x() - m_lastMousePos.x();
        int dy = event->y() - m_lastMousePos.y();
        m_cameraYAngle += dx * 0.5f;
        m_cameraXAngle += dy * 0.5f;
        m_lastMousePos = event->pos();
        update();
    }
}
void OpenGLWidget::wheelEvent(QWheelEvent *event) {
    m_cameraDistance -= event->angleDelta().y() * 0.1f;
    if (m_cameraDistance < 10.0f) m_cameraDistance = 10.0f;
    if (m_cameraDistance > 1000.0f) m_cameraDistance = 1000.0f;
    update();
} 