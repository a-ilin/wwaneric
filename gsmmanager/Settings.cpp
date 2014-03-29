#include "Settings.h"

#include "Core.h"

#include <QApplication>
#include <QDir>

Settings::Settings() :
  QSettings(Core::instance()->appUserDirectory() + QDir::separator() + QString("gsmmanager.conf"),
            QSettings::IniFormat)
{
}
