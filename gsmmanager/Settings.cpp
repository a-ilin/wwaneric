#include "Settings.h"

#include "Core.h"

#include <QApplication>
#include <QDir>

Settings::Settings() :
  QSettings(configFile(),
            QSettings::IniFormat)
{
}

QString Settings::configFile()
{
  return Core::instance()->appUserDirectory() + QDir::separator() + QString("gsmmanager.conf");
}
