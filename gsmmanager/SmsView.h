#ifndef SMSVIEW_H
#define SMSVIEW_H

#include "IView.h"
#include "Sms.h"

#include <QWidget>

namespace Ui
{
  class SmsView;
}

class Sms;
class SmsProxyModel;

class QStandardItemModel;

class SmsView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit SmsView(const QString &connectionId, QWidget *parent = 0);
  ~SmsView();

  void init();
  void tini();

  void restore(Settings &set);
  void store(Settings &set);

  QWidget * widget() {return this;}

  QString name() const;

  QString id() const
  {
    return "SMS";
  }

  enum Columns
  {
    ColumnStatus,
    ColumnStorage,
    ColumnDate,
    ColumnNumber,
    ColumnMessage,
    ColumnLast
  };

  void processConnectionEvent(Core::ConnectionEvent event, const QVariant &data);

protected:
  void changeEvent(QEvent *e);
  void readSms(const QList<Sms> &smsList);
  void deleteSms(const QList<SmsMeta>& smsList);

private slots:
  void smsStatusShowButtonToggle(bool checked);
  void smsStorageShowButtonToggle(bool checked);

  void changeViewActionStatus();
  void onSmsDeleteAction();
  void onSmsReplyAction();

private:
  Ui::SmsView *ui;

  QStandardItemModel * m_sourceModel;
  SmsProxyModel * m_proxyModel;

  QAction * m_smsDelete;
  QAction * m_smsReply;
};



#include <QSortFilterProxyModel>
class SmsProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  SmsProxyModel(QObject * parent = NULL);

  void setSmsStatusShow(SMS_STATUS status, bool show);
  void setSmsStorageShow(SMS_STORAGE storage, bool show);

protected:
   bool	filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

private:
   QMap<SMS_STATUS, bool> m_statusMap;
   QMap<SMS_STORAGE, bool> m_storageMap;
};


#endif // SMSVIEW_H
