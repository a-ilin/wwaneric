#ifndef SERIALPORTSETTINGSVIEW_H
#define SERIALPORTSETTINGSVIEW_H

#include "IView.h"

#include <QWidget>

namespace Ui
{
  class SerialPortSettingsView;
}

class SerialPortSettingsView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit SerialPortSettingsView(const QString &connectionId, QWidget *parent = 0);
  ~SerialPortSettingsView();

  void init();
  void tini();

  void restore(Settings &set);
  void store(Settings &set);

  QWidget * widget() {return this;}

  QString name() const;

  QString id() const
  {
    return "SerialPortSettings";
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
  Ui::SerialPortSettingsView *ui;

};

#endif // SERIALPORTSETTINGSVIEW_H
