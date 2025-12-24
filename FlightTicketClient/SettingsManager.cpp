#include "SettingsManager.h"
#include <QApplication>
#include <QSettings>
#include <QPalette>
#include <QColor>
#include <QStyleHints>
#include <QStyle>
#include <QFont>

double SettingsManager::minScale = 0.5;
double SettingsManager::maxScale = 2.0;

SettingsManager& SettingsManager::instance()
{
    static SettingsManager inst;
    return inst;
}

SettingsManager::SettingsManager(QObject *parent) : QObject(parent) {}

void SettingsManager::load()
{
    QSettings s;
    m_theme = static_cast<ThemeMode>(s.value("ui/themeMode", 0).toInt());
    m_scale = s.value("ui/scaleFactor", 1.0).toDouble();

    if (m_scale < minScale) m_scale = minScale;
    if (m_scale > maxScale) m_scale = maxScale;
}

void SettingsManager::save() const
{
    QSettings s;
    s.setValue("ui/themeMode", static_cast<int>(m_theme));
    s.setValue("ui/scaleFactor", m_scale);
}

void SettingsManager::applyAll()
{
    applyScale();
    applyTheme();

    QObject::connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]{
        if (m_theme == ThemeMode::System) applyTheme();
    });
}

void SettingsManager::applyTheme()
{
    auto *h = QGuiApplication::styleHints();

    switch (m_theme) {
    case ThemeMode::Light:
        h->setColorScheme(Qt::ColorScheme::Light);
        break;
    case ThemeMode::Dark:
        h->setColorScheme(Qt::ColorScheme::Dark);
        break;
    case ThemeMode::System:
    default:
        h->unsetColorScheme();
        break;
    }
}

void SettingsManager::applyScale()
{
    if (m_baseFontPt < 0.0) {
        m_baseFontPt = qApp->font().pointSizeF();
        if (m_baseFontPt <= 0) m_baseFontPt = 10.0;
    }

    QFont f = qApp->font();
    f.setPointSizeF(m_baseFontPt * m_scale);
    qApp->setFont(f);
}

void SettingsManager::setThemeMode(ThemeMode mode)
{
    if (m_theme == mode) return;
    m_theme = mode;
    save();
    applyTheme();
    emit themeModeChanged(m_theme);
}

void SettingsManager::setScaleFactor(double factor)
{
    if (factor < minScale) factor = minScale;
    if (factor > maxScale) factor = maxScale;
    if (qFuzzyCompare(m_scale, factor)) return;

    m_scale = factor;
    save();
    applyScale();
    emit scaleFactorChanged(m_scale);
}
