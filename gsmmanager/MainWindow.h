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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Core.h"
#include "Settings.h"

#include <QMainWindow>
#include <QMultiMap>
#include <QSystemTrayIcon>

namespace Ui
{
  class MainWindow;
}

class IView;

class QSystemTrayIcon;
class QTabWidget;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

  bool eventFilter(QObject* watched, QEvent* event);

  void addViewGroup(const QUuid& uuid);
  void removeViewGroup(const QUuid& uuid, bool storeSettings);
  void setViewGroupName(const QUuid& uuid, const QString& userName);

  void init();
  void tini();

  QSystemTrayIcon* trayIcon() const { return m_trayIcon; }

  void updateConnectionStatus(const QUuid& connectionId, bool status);
  void updateSignalStrength(const QUuid& connectionId, double strengthPercent);

  enum ConnectionColumns
  {
    ColumnName,
    ColumnStatus,
    ColumnSignalStrength,
    ColumnLast
  };

protected:
  void changeEvent(QEvent *e);
  void closeEvent(QCloseEvent *event);

  struct Box
  {
    Box() :
      container(NULL),
      view(NULL) {}

    QDockWidget * container;
    IView * view;
  };

  struct ConnBox
  {
    QString name;
    QList<Box> boxes;
  };

  QList<IView *> createViews(const QUuid& uuid);

  void restore();
  void store();

  int connectionWidgetRow(const QUuid& uuid) const;

  QString askUserConnectionName(const QUuid& uuid);

protected slots:
  void addConnection();
  void showPreferences();
  void visitWebsite();
  void showAbout();
  void showAboutQt();
  void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void onShowAction();
  void onExitAction();
  void onRemoveGroupAction();
  void onRenameGroupAction();

  void onConnectionContextMenu(const QPoint& pos);

  void onConnectionEvent(const QUuid& connectionId,
                         Core::ConnectionEvent event,
                         const QVariant &data);

protected:
  Ui::MainWindow *ui;


  typedef QMap<QUuid, ConnBox> ConnectMap;
  ConnectMap m_connBoxes;

  QSystemTrayIcon * m_trayIcon;
  QMenu * m_trayMenu;

  bool m_exit;
  bool m_minimizeOnClose;

};

#endif // MAINWINDOW_H
