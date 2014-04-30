#ifndef MODEMSTATUSVIEW_H
#define MODEMSTATUSVIEW_H

#include "IView.h"

#include <QWidget>

namespace Ui
{
  class ModemStatusView;
}

class QTimer;

class ModemStatusView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit ModemStatusView(const QString &connectionId, QWidget *parent = 0);
  ~ModemStatusView();

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

  bool event(QEvent * e);

  void processConnectionEvent(Core::ConnectionEvent event, const QVariant &data);

  enum Columns
  {
    ColumnType,
    ColumnInfo,
    ColumnLast
  };

protected:
  void changeEvent(QEvent *e);

  void setStatusInfo(const QString& statusType, const QString& statusInfo);

  void updateSignalQuality(bool noError,
                           bool signalDetected, double signalDbm, double signalPercent,
                           bool berDetected, const QString& berPercentRange);

protected slots:
  void onCopyToClipboard();
  void onTimerTimeout();

private:
  Ui::ModemStatusView *ui;

  // timer for requesting for frequently modificable data
  QTimer * m_timer;

};

#endif // MODEMSTATUSVIEW_H
