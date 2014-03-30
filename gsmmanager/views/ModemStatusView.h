#ifndef MODEMSTATUSVIEW_H
#define MODEMSTATUSVIEW_H

#include "../IView.h"

#include <QWidget>

namespace Ui
{
  class ModemStatusView;
}

class ModemStatusView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit ModemStatusView(QWidget *parent = 0);
  ~ModemStatusView();

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
  void updateStatus();

private:
  Ui::ModemStatusView *ui;

};

#endif // MODEMSTATUSVIEW_H
