#include <QApplication>
#include <QStyleFactory>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // Qt uygulaması oluşturma
    QApplication app(argc, argv);
    
    // Uygulama bilgileri
    app.setApplicationName("CNC Controller");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("CNC Software");
    
    // Modern tema uygulama (Windows'ta daha iyi görünüm)
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Ana pencere oluşturma
    MainWindow window;
    window.show();
    
    // Uygulama döngüsünü başlatma
    return app.exec();
} 