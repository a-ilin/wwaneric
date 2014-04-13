#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

  void restore(Settings &set);
  void store(Settings &set);

  QSystemTrayIcon* trayIcon() const { return m_trayIcon; }

protected:
  void changeEvent(QEvent *e);
  void closeEvent(QCloseEvent *event);

  QList<IView *> createViews();
  void removeViews(QTabWidget* container);

protected slots:
  void containerDestroyed(QObject * obj);
  void updateActionTriggered();
  void updatePortStatus(bool opened);
  void addConnection();
  void showPreferences();
  void visitWebsite();
  void showAbout();
  void showAboutQt();
  void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void onShowAction();
  void onExitAction();

protected:
  Ui::MainWindow *ui;

  typedef QHash<QString, QTabWidget*> ContainerHash;
  ContainerHash m_boxes;

  typedef QMultiMap<QTabWidget*, IView*> MapViews;
  MapViews m_views;

  QSystemTrayIcon * m_trayIcon;
  QMenu * m_trayMenu;

  bool m_exit;
  bool m_minimizeOnClose;

};

#endif // MAINWINDOW_H
