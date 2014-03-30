#ifndef SETTINGSVIEW_H
#define SETTINGSVIEW_H

#include "../IView.h"

#include <QWidget>

namespace Ui
{
  class SettingsView;
}

class SettingsView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit SettingsView(QWidget *parent = 0);
  ~SettingsView();

  void init();
  void tini();

  void restore(Settings &set);
  void store(Settings &set);

  QWidget * widget() {return this;}

  QString name();

public slots:


signals:

protected:
  void changeEvent(QEvent *e);

private:


private slots:
  void serialPortChanged(int newIndex);
  void updatePortStatus();

private:
  Ui::SettingsView *ui;

  QString m_serialPortName;

};

#endif // SETTINGSVIEW_H
