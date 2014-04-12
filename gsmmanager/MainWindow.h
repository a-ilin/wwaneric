#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Settings.h"

#include <QMainWindow>
#include <QMultiMap>

namespace Ui
{
  class MainWindow;
}

class IView;

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

protected:
  void changeEvent(QEvent *e);

  QList<IView *> createViews();
  void removeViews(QTabWidget* container);

protected slots:
  void containerDestroyed(QObject * obj);
  void updateActionTriggered();
  void updatedPortStatus(bool opened);

protected:
  Ui::MainWindow *ui;

  typedef QHash<QString, QTabWidget*> ContainerHash;
  ContainerHash m_boxes;

  typedef QMultiMap<QTabWidget*, IView*> MapViews;
  MapViews m_views;

};

#endif // MAINWINDOW_H
