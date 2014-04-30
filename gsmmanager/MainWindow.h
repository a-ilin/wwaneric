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

  void addViewGroup(const QString &groupName);
  void removeViewGroup(const QString &groupName);

  void init();
  void tini();

  QSystemTrayIcon* trayIcon() const { return m_trayIcon; }

  void updateConnectionStatus(const QString& connectionId, bool status);
  void updateSignalStrength(const QString& connectionId, double strengthPercent);

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

  QList<IView *> createViews(const QString& groupName);
  void removeViews(const QList<Box>& boxes, const QString &groupName);

  void restore();
  void store();

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
  void onConnectionEvent(const QString &connectionId,
                         Core::ConnectionEvent event,
                         const QVariant &data);

protected:
  Ui::MainWindow *ui;


  typedef QHash<QString, Box> ContainerHash;
  ContainerHash m_boxes;

  QSystemTrayIcon * m_trayIcon;
  QMenu * m_trayMenu;

  bool m_exit;
  bool m_minimizeOnClose;

};

#endif // MAINWINDOW_H
