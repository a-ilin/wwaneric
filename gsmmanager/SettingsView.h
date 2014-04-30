#ifndef SETTINGSVIEW_H
#define SETTINGSVIEW_H

#include "IView.h"

#include <QWidget>

namespace Ui
{
  class SettingsView;
}

class SettingsView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit SettingsView(const QString &connectionId, QWidget *parent = 0);
  ~SettingsView();

  void init();
  void tini();

  void restore(Settings &set);
  void store(Settings &set);

  QWidget * widget() {return this;}

  QString name() const;

  QString id() const
  {
    return "Settings";
  }

  void processConnectionEvent(Core::ConnectionEvent event, const QVariant &data);

public slots:


signals:

protected:
  void changeEvent(QEvent *e);

private:


private slots:
  void openPortClicked();
  void closePortClicked();

  void defaultCheckBoxStateChanged(int state);

private:
  Ui::SettingsView *ui;

};

#endif // SETTINGSVIEW_H
