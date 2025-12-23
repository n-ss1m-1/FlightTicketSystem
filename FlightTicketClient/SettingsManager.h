#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>

class SettingsManager : public QObject
{
    Q_OBJECT
public:
    enum class ThemeMode { System = 0, Light = 1, Dark = 2 };
    static double minScale;
    static double maxScale;
    Q_ENUM(ThemeMode)

    static SettingsManager& instance();

    void load();
    void save() const;

    void applyAll();
    void applyTheme();
    void applyScale();

    ThemeMode themeMode() const { return m_theme; }
    double scaleFactor() const { return m_scale; } // 1.0 = 100%

public slots:
    void setThemeMode(ThemeMode mode);
    void setScaleFactor(double factor);

signals:
    void themeModeChanged(ThemeMode);
    void scaleFactorChanged(double);

private:
    explicit SettingsManager(QObject *parent=nullptr);

    ThemeMode m_theme = ThemeMode::System;
    double m_scale = 1.0;

    double m_baseFontPt = -1.0; // 记录原始字体大小，避免叠乘
};

#endif // SETTINGSMANAGER_H
