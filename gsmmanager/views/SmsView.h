#ifndef SMSVIEW_H
#define SMSVIEW_H

#include "../IView.h"

#include <QWidget>

namespace Ui
{
  class SmsView;
}

class Sms;

class SmsView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit SmsView(QWidget *parent = 0);
  ~SmsView();

  void init();
  void tini();

  void restore(Settings &set);
  void store(Settings &set);

  QWidget * widget() {return this;}

  QString name();

  bool event(QEvent * e);

public slots:


signals:

protected:
  void changeEvent(QEvent *e);

private:
  void fillSmsTextAndTooltip(Sms *sms, QString * smsText, QString * smsTooltip) const;

private slots:
  void updateStatus();
  void appendSms(const QList<Sms> &smsList);
  void smsContextMenuRequested(const QPoint &pos);

  void updateSmsCapacity(int simUsed, int simTotal, int phoneUsed, int phoneTotal);

private:
  Ui::SmsView *ui;

  int m_newSmsCount;
  int m_readSmsCount;
  int m_draftSmsCount;
  int m_sentSmsCount;

};

#endif // SMSVIEW_H
