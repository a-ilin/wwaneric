/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2017 by Aleksei Ilin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

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
