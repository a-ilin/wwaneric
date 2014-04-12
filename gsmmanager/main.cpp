#include "common.h"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QSettings>

#include "Core.h"


int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  QApplication::setOrganizationName("Aleksei Ilin");
  QApplication::setApplicationName("wwanEric");

  QApplication::setWindowIcon(QIcon(":/icons/modem.png"));

  // this needs to remove unnecessary QT_INSTALL path
  QStringList newLibraryPaths;
  newLibraryPaths << a.applicationDirPath();
  a.setLibraryPaths(newLibraryPaths);

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

  if (!core->tini())
  {
    --returnValue;
  }

  if (returnValue)
  {
    Q_LOGEX(LOG_VERBOSE_ERROR, QString("Application return value: %1").arg(returnValue));
  }

  delete core;

  return returnValue;
}
