/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2017 by Aleksei Ilin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#ifndef SMSVIEW_H
#define SMSVIEW_H

#include "IView.h"
#include "Sms.h"

#include <QStandardItemModel>
#include <QWidget>

namespace Ui
{
  class SmsView;
}

class SmsProxyModel;

class QStandardItem;

typedef QList<Sms> SmsList;

class SmsModel : public QStandardItemModel
{
public:
  enum Columns
  {
    ColumnArchive,
    ColumnStorage,
    ColumnStatus,
    ColumnDate,
    ColumnNumber,
    ColumnMessage,
    ColumnLast
  };

  explicit SmsModel(const QUuid& connectionId, QObject * parent = NULL);

  // info
  SMS_STATUS smsStatus(int row) const;
  SMS_STORAGE smsStorage(int row) const;
  QList<int> smsIndices(int row) const;
  QString smsText(int row) const;
  QDateTime smsDate(int row) const;
  QString smsSender(int row) const;
  QString smsSC(int row) const;
  int smsSizeCount(int row) const;

  bool isArchived(int row) const;
  bool isArchived(const Sms& sms) const;

  int statusCount(SMS_STATUS status) const;
  int storageCount(SMS_STORAGE storage) const;
  int archivedCount() const;

  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

  // actions
  void addSms(const Sms& sms, bool archive);
  void archiveSms(int row);
  void deleteSmsDevice(SMS_STORAGE storage, int smsIndex);
  void deleteSmsDevice();
  void deleteSmsArchive(int row);

private:
  void updateRowData(int row);

private:

  QUuid m_connectionId;

  // internal data store for the model
  QList<SmsList> m_smsList;

  // SMS storage capacity
  QMap<SMS_STORAGE, int> m_usedStorage;
  // SMS group count by status
  QMap<SMS_STATUS, int> m_statusCount;
  // archived SMS counter
  int m_archivedCount;
};

class SmsView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit SmsView(const QUuid& connectionId, QWidget *parent = 0);
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

  void processConnectionEvent(Core::ConnectionEvent event, const QVariant &data);

protected:
  void changeEvent(QEvent *e);
  void retranslateUi();

  void updateCounters();

private slots:
  void smsStatusShowButtonToggle(bool checked);
  void smsStorageShowButtonToggle(bool checked);

  void onSelectionChanged();
  void onSmsDeleteAction();
  void onSmsReplyAction();

  void onDataChanged(const QModelIndex& topLeft,
                     const QModelIndex& bottomRight,
                     const QVector<int>& roles = QVector<int> ());
  void onModelReset();
  void onRowsInserted(const QModelIndex& parent, int first, int last);
  void onRowsRemoved(const QModelIndex& parent, int first, int last);

private:
  Ui::SmsView *ui;

  SmsModel * m_sourceModel;
  SmsProxyModel * m_proxyModel;

  QAction * m_smsDelete;
  QAction * m_smsReply;

  bool m_autoArchiveOnRead;

  int m_totalSim;
  int m_totalPhone;
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
   // show filters
   QMap<SMS_STATUS, bool> m_statusMap;
   QMap<SMS_STORAGE, bool> m_storageMap;
};


#endif // SMSVIEW_H
