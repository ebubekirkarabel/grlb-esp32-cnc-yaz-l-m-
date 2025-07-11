#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QString>
#include <QObject>

// Plugin sistemi için temel interface
class PluginInterface
{
public:
    virtual ~PluginInterface() = default;
    
    // Plugin bilgileri
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QString description() const = 0;
    
    // Plugin yaşam döngüsü
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    
    // Plugin işlevleri
    virtual void execute() = 0;
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
};

// Plugin türleri
enum class PluginType {
    GCodeProcessor,    // G-code işleme
    Visualization,     // 3D görselleştirme
    Communication,     // İletişim protokolleri
    Safety,           // Güvenlik özellikleri
    Optimization,     // Optimizasyon algoritmaları
    Custom            // Özel pluginler
};

// Plugin metadata
struct PluginInfo {
    QString name;
    QString version;
    QString description;
    PluginType type;
    QString author;
    QString license;
};

Q_DECLARE_INTERFACE(PluginInterface, "com.cnc.plugin.interface")

#endif // PLUGININTERFACE_H 