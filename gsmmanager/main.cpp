#include "common.h"

#include <QApplication>
#include <QDir>
#include <QIcon>

#include "Core.h"
#include "Settings.h"

#include "crashdump.h"

int main(int argc, char *argv[])
{
  // dump handler registration
#ifndef QT_DEBUG
  CrashDumpHandler dumpHandler(kInfoLevelSmall);
  dumpHandler.addToArchiveList(Settings::configFile().toStdWString());
#endif

  QApplication a(argc, argv);

  QApplication::setOrganizationName("Aleksei Ilin");
  QApplication::setApplicationName("wwanEric");

  QApplication::setWindowIcon(QIcon(":/icons/modem.png"));

#ifdef QT_DEBUG
  a.addLibraryPath(a.applicationDirPath());
#else
  // this is needs to remove unnecessary QT_INSTALL path
  QStringList newLibraryPaths;
  newLibraryPaths << a.applicationDirPath();
  a.setLibraryPaths(newLibraryPaths);
#endif

  int returnValue = 0;

  // Core
  Core * core = new Core();
  if (core->init())
  {
    returnValue = a.exec();
  }
  else
  {
    --returnValue;
  }

  core->tini();

  if (returnValue)
  {
    Q_LOGEX(LOG_VERBOSE_ERROR, QString("Application return value: %1").arg(returnValue));
  }

  delete core;

  return returnValue;
}
