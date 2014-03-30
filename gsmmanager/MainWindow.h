#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

protected:
  void changeEvent(QEvent *e);

  QList<IView *> createViews();
  void removeViews(QTabWidget* container);

protected slots:
  void containerDestroyed(QObject * obj);

protected:
  Ui::MainWindow *ui;

  typedef QHash<QString, QTabWidget*> ContainerHash;
  ContainerHash m_boxes;

  QMultiMap<QTabWidget*, IView*> m_views;

};

#endif // MAINWINDOW_H
