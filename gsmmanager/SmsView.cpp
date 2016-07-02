#include "SmsView.h"
#include "ui_SmsView.h"

#include "Core.h"

#include "ModemSms.h"

#include <QItemSelection>
#include <QMenu>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QTableView>


bool smsConcatLessThen(const Sms& first, const Sms& second)
{
  return first.concatPartNumber < second.concatPartNumber;
}

// return:
// negative if first is less
// zero if equal
// positive otherwise
int smsGroupCompare(const Sms& first, const Sms& second)
{
  int result = 0;

  result = first.storage - second.storage;
  if (result) return result;

  // Modem Ericsson F5521GW can return status READ for NEW sms
  // when executing AT+CMGR after CMTI event
  //result = first.status - second.status;
  //if (result) return result;

  result = first.sender.compare(second.sender);
  if (result) return result;

  // some operators uses distributed SMSC (like MTS Russia)
  // and SMS parts can be delivered through different SMSC
  //result = first.smsc - second.smsc;
  //if (result) return result;

  result = first.concatReference - second.concatReference;
  if (result) return result;

  result = first.concatTotalCount - second.concatTotalCount;
  if (result) return result;

  return 0;
}

bool smsListLessThen(const SmsList& first, const SmsList& second)
{
  Q_ASSERT(!first.isEmpty());
  Q_ASSERT(!second.isEmpty());

  return smsGroupCompare(first.first(), second.first()) < 0;
}

SmsModel::SmsModel(const QUuid& connectionId, QObject* parent) :
  QStandardItemModel(parent),
  m_connectionId(connectionId),
  m_archivedCount(0)
{
  setColumnCount(ColumnLast);

  // read SMS stored in DB
  SmsDatabaseEntity smsDb;
  if (smsDb.init())
  {
    DatabaseKey key;
    key.insert("a_connection", m_connectionId.toString());
    QList<SmsBase> smsBaseList = smsDb.select(key);

    foreach(const SmsBase& smsBase, smsBaseList)
    {
      Sms sms(smsBase.storage, -1, smsBase.status, smsBase.rawData);
      if (!sms.valid)
      {
        Q_LOGEX(LOG_VERBOSE_ERROR, sms.parseError);
      }
      else
      {
        addSms(sms, false);
      }
    }
  }
}

void SmsModel::addSms(const Sms& sms, bool archive)
{
  int msgRow = -1;

  // FIXME: optimize it!
  // find uniqueness
  for (QList<SmsList>::iterator iter = m_smsList.begin(); iter < m_smsList.end(); ++iter)
  {
    SmsList& smsList = *iter;
    Q_ASSERT(smsList.size());
    if (!smsGroupCompare(sms, smsList.first()))
    {
      for (SmsList::iterator smsIter = smsList.begin(); smsIter < smsList.end(); ++smsIter)
      {
        Sms& iSms = *smsIter;
        if (sms.rawData == iSms.rawData)
        {
          // restore SMS index
          iSms.index = sms.index;
          msgRow = iter - m_smsList.begin();
          updateRowData(msgRow);
          return;
        }
      }
    }
  }

  // SMS concatenation
  if ((sms.concatPartNumber >= 1) &&
      (sms.concatPartNumber <= sms.concatTotalCount))
  {
    for(QList<SmsList>::iterator iter = m_smsList.begin(); iter < m_smsList.end(); ++iter)
    {
      SmsList& smsList = *iter;
      Q_ASSERT(smsList.size());

      const Sms& iSms = smsList.first();

      if ((!smsGroupCompare(iSms, sms)) &&
          (iSms.concatPartNumber >= 1) &&
          (iSms.concatPartNumber <= iSms.concatTotalCount))
      {
        bool collision = false;
        foreach(const Sms& iSms, smsList)
        {
          if (iSms.concatPartNumber == sms.concatPartNumber)
          {
            collision = true;
            break;
          }
        }

        if(!collision)
        {
          msgRow = iter - m_smsList.begin();
          smsList.append(sms);
          // reorder smsList according to part number
          std::sort(smsList.begin(), smsList.end(), smsConcatLessThen);
          break;
        }
      }
    }
  }

  ++m_usedStorage[sms.storage];

  // if need to append new message into the model
  if (msgRow == -1)
  {
    m_smsList.append(SmsList() << sms);

    QList<QStandardItem*> appItems;
    for (int i=0; i< ColumnLast; ++i)
    {
      appItems.append(new QStandardItem());
    }

    msgRow = rowCount();
    ++m_statusCount[sms.status];
    appendRow(appItems);

    if (archive || isArchived(sms))
    {
      ++m_archivedCount;
    }
  }

  Q_ASSERT(msgRow != -1);

  if (archive)
  {
    archiveSms(msgRow);
  }

  updateRowData(msgRow);
}

SMS_STATUS SmsModel::smsStatus(int row) const
{
  const SmsList& smsList = m_smsList.at(row);
  Q_ASSERT(smsList.size());

  // check for at least one new in read
#ifdef QT_DEBUG
  bool income = false;
#endif

  bool neu = false;
  foreach (const Sms& sms, smsList)
  {
    switch(sms.status)
    {
    case SMS_STATUS_INCOME:
#ifdef QT_DEBUG
      income = true;
#endif
      break;
    case SMS_STATUS_NEW:
      neu = true;
      break;
    default:
      Q_ASSERT(!neu);
      Q_ASSERT(!income);
      break;
    }
  }

  if (neu)
  {
    return SMS_STATUS_NEW;
  }

  return smsList.first().status;
}

SMS_STORAGE SmsModel::smsStorage(int row) const
{
  const SmsList& smsList = m_smsList.at(row);
  Q_ASSERT(smsList.size());
  SMS_STORAGE storage = smsList.first().storage;
  return storage;
}

QList<int> SmsModel::smsIndices(int row) const
{
  const SmsList& smsList = m_smsList.at(row);
  Q_ASSERT(smsList.size());
  QList<int> indices;
  foreach (const Sms& sms, smsList)
  {
    if (sms.index != -1)
    {
      indices.append(sms.index);
    }
  }

  return indices;
}

QString SmsModel::smsText(int row) const
{
  const SmsList& smsList = m_smsList.at(row);
  Q_ASSERT(smsList.size());

  QString msg;
  foreach(const Sms& sms, smsList)
  {
    msg += sms.userText;
  }

  return msg;
}

QDateTime SmsModel::smsDate(int row) const
{
  const SmsList& smsList = m_smsList.at(row);
  Q_ASSERT(smsList.size());
  return smsList.last().dateTime;
}

QString SmsModel::smsSender(int row) const
{
  const SmsList& smsList = m_smsList.at(row);
  Q_ASSERT(smsList.size());
  return smsList.last().sender;
}

QString SmsModel::smsSC(int row) const
{
  const SmsList& smsList = m_smsList.at(row);
  Q_ASSERT(smsList.size());
  return smsList.last().smsc;
}

int SmsModel::smsSizeCount(int row) const
{
  const SmsList& smsList = m_smsList.at(row);
  Q_ASSERT(smsList.size());
  return smsList.size();
}

bool SmsModel::isArchived(int row) const
{
  const SmsList& smsList = m_smsList.at(row);
  Q_ASSERT(smsList.size());
  bool inArchive = true;
  foreach(const Sms& sms, smsList)
  {
    if (!isArchived(sms))
    {
      inArchive = false;
      break;
    }
  }

  return inArchive;
}

bool SmsModel::isArchived(const Sms& sms) const
{
  SmsDatabaseEntity smsDb;
  if (smsDb.init())
  {
    DatabaseKey key;
    key.insert("a_connection", m_connectionId.toString());
    key.insert("i_storage", sms.storage);
    key.insert("i_status", sms.status);
    key.insert("a_raw", QString(sms.rawData));
    QList<SmsBase> list = smsDb.select(key);
    Q_ASSERT(list.size() <= 1);
    if(!list.isEmpty())
    {
      return true;
    }
  }

  return false;
}

int SmsModel::statusCount(SMS_STATUS status) const
{
  return m_statusCount.value(status);
}

int SmsModel::storageCount(SMS_STORAGE storage) const
{
  return m_usedStorage.value(storage);
}

int SmsModel::archivedCount() const
{
  return m_archivedCount;
}

void SmsModel::archiveSms(int row)
{
  SmsDatabaseEntity smsDb;
  if (smsDb.init())
  {
    QList<SmsBase> smsBaseList;

    foreach(const Sms& sms, m_smsList.at(row))
    {
        SmsBase smsBase;
        smsBase.connectionId = m_connectionId.toString();
        smsBase.storage = sms.storage;
        smsBase.status = sms.status;
        smsBase.rawData = sms.rawData;
        smsBaseList.append(smsBase);
    }

    if (smsDb.insert(smsBaseList))
    {
      ++m_archivedCount;
    }
  }
}

void SmsModel::deleteSmsDevice(SMS_STORAGE storage, int smsIndex)
{
  Q_ASSERT(smsIndex != -1);

  // find corresponding row
  // FIXME: optimize it!
  int msgRow = -1;
  for (QList<SmsList>::iterator iter = m_smsList.begin(); iter < m_smsList.end(); ++iter)
  {
    SmsList& smsList = *iter;
    Q_ASSERT(smsList.size());
    if (smsList.first().storage == storage)
    {
      for (SmsList::iterator smsIter = smsList.begin(); smsIter < smsList.end(); ++smsIter)
      {
        msgRow = iter - m_smsList.begin();
        SMS_STATUS status = smsStatus(msgRow);
        Sms& sms = *smsIter;
        if (sms.index == smsIndex)
        {
          if (isArchived(sms))
          {
            // change index only
            sms.index = -1;
          }
          else
          {
            // delete sms from model
            smsIter = smsList.erase(smsIter);
            --m_usedStorage[storage];

            if (!smsList.size())
            {
              // if there is no other sms in the row delete the row
              --m_statusCount[status];
              iter = m_smsList.erase(iter);
              removeRows(msgRow, 1);
              msgRow = -1;
            }
            else
            {
              // check for status change
              SMS_STATUS neuStatus = smsStatus(msgRow);
              if (status != neuStatus)
              {
                --m_statusCount[status];
                ++m_statusCount[neuStatus];
              }
            }
          }

          break;
        }
      }
    }
  }

  if (msgRow != -1)
  {
    updateRowData(msgRow);
  }
}

void SmsModel::deleteSmsDevice()
{
  QList<SmsList>::iterator iter = m_smsList.begin();

  while (iter < m_smsList.end())
  {
    int msgRow = iter - m_smsList.begin();
    SmsList& smsList = *iter;
    Q_ASSERT(smsList.size());

    if (isArchived(msgRow))
    {
      // change index only to invalid
      for (SmsList::iterator smsIter = smsList.begin(); smsIter < smsList.end(); ++smsIter)
      {
        Sms& sms = *smsIter;
        sms.index = -1;
      }

      updateRowData(msgRow);
      ++iter;
    }
    else
    {
      // remove entire row
      --m_statusCount[smsStatus(msgRow)];
      iter = m_smsList.erase(iter);
      removeRows(msgRow, 1);
    }
  }

  m_usedStorage.clear();
}

void SmsModel::deleteSmsArchive(int row)
{
  bool rmRow = false;
  SMS_STATUS status = smsStatus(row);

  SmsDatabaseEntity smsDb;
  if (smsDb.init())
  {
    SmsList& smsList = m_smsList[row];
    for (SmsList::iterator smsIter = smsList.begin(); smsIter < smsList.end(); ++smsIter)
    {
      Sms& sms = *smsIter;

      DatabaseKey key;
      key.insert("a_connection", m_connectionId.toString());
      key.insert("i_storage", sms.storage);
      key.insert("i_status", sms.status);
      key.insert("a_raw", sms.rawData);

      if (smsDb.delet(key))
      {
        --m_archivedCount;

        // if that SMS existed in archive only
        if (sms.index == -1)
        {
          rmRow = true;
        }
      }
    }
  }

  if (rmRow)
  {
    // remove entire row
    --m_statusCount[status];
    removeRows(row, 1);
  }
}

QVariant SmsModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::UserRole)
  {
    // UserRole used as sort role
    switch(index.column())
    {
    case ColumnArchive:
      return isArchived(index.row()) ? 1 : 0;
    case ColumnStorage:
      return smsStorage(index.row());
    case ColumnStatus:
      return smsStatus(index.row());
    case ColumnDate:
      return smsDate(index.row());
    case ColumnNumber:
      return smsSender(index.row());
    case ColumnMessage:
      return smsText(index.row());
    default:
      Q_ASSERT(false);
    }

    return QStandardItemModel::data(index, Qt::DisplayRole);
  }

  return QStandardItemModel::data(index, role);
}

void SmsModel::updateRowData(int row)
{
  const SmsList& smsList = m_smsList.at(row);
  Q_ASSERT(smsList.size());

  /*
   * Archive
   */
  QStandardItem * itemArchive = item(row, ColumnArchive);
  Q_ASSERT(itemArchive);
  QIcon archiveIcon = isArchived(row) ? QIcon("icons/sms_archive.png") : QIcon();
  itemArchive->setIcon(archiveIcon);

  /*
   * Storage
   */
  bool inDevice = false;
  foreach (const Sms& sms, smsList)
  {
    if (sms.index != -1)
    {
      inDevice = true;
    }
  }

  QIcon storageIcon;
  switch(smsStorage(row))
  {
  case SMS_STORAGE_PHONE:
    storageIcon = (!inDevice) ? QIcon("icons/sms_modem_gray.png") :
                                QIcon("icons/sms_modem.png");
    break;
  case SMS_STORAGE_SIM:
    storageIcon = (!inDevice) ? QIcon("icons/sms_sim_gray.png") :
                                QIcon("icons/sms_sim.png");
    break;
  default:
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unknown storage!");
  }

  QStandardItem * itemStorage = item(row, ColumnStorage);
  Q_ASSERT(itemStorage);
  itemStorage->setIcon(storageIcon);

  /*
   * Status
   */
  QIcon statusIcon;
  switch (smsStatus(row))
  {
  case SMS_STATUS_NEW:
    statusIcon = QIcon("icons/sms_unread.png");
    break;
  case SMS_STATUS_INCOME:
    statusIcon = QIcon("icons/sms_income.png");
    break;
  case SMS_STATUS_DRAFT:
    statusIcon = QIcon("icons/sms_draft.png");
    break;
  case SMS_STATUS_SENT:
    statusIcon = QIcon("icons/sms_sent.png");
    break;
  default:
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unknown status!");
  }

  QStandardItem * itemStatus = item(row, ColumnStatus);
  Q_ASSERT(itemStatus);
  itemStatus->setIcon(statusIcon);

  /*
   * Sender
   */
  QStandardItem * itemSender = item(row, ColumnNumber);
  Q_ASSERT(itemSender);
  itemSender->setText(smsSender(row));

  /*
   * Date
   */
  QStandardItem * itemDate = item(row, ColumnDate);
  Q_ASSERT(itemDate);
  itemDate->setText(smsDate(row).toString(Qt::DefaultLocaleShortDate));

  /*
   * Text
   */
  QStandardItem * itemText = item(row, ColumnMessage);
  Q_ASSERT(itemText);
  itemText->setText(smsText(row));

  /*
   * ToolTip
   */
  QString smsTooltip =
      tr("SMS service center: %1").arg(smsList.first().smsc) + QString(ENDL) +
      tr("Message contains %1 SMS(es)").arg(smsList.size());

  for(int c=0; c< ColumnLast; ++c)
  {
    QStandardItem * i = item(row, c);
    Q_ASSERT(i);
    i->setToolTip(smsTooltip);
  }

}

// property names for UI buttons
#define SMS_STATUS_PROPERTY "smsStatus"
#define SMS_STORAGE_PROPERTY "smsStorage"


SmsView::SmsView(const QUuid& connectionId, QWidget *parent) :
  QWidget(parent),
  IView(connectionId),
  ui(new Ui::SmsView),
  m_autoArchiveOnRead(false),
  m_totalSim(0),
  m_totalPhone(0)
{
  ui->setupUi(this);
}

SmsView::~SmsView()
{
  delete ui;
}

void SmsView::changeEvent(QEvent *e)
{
  QWidget::changeEvent(e);
  switch (e->type())
  {
  case QEvent::LanguageChange:
    ui->retranslateUi(this);
    retranslateUi();
    break;
  default:
    break;
  }
}

void SmsView::retranslateUi()
{
  QStringList headerLabels;
  headerLabels
      << tr("Archived")
      << tr("Storage")
      << tr("Status")
      << tr("Date")
      << tr("Number")
      << tr("Message");
  m_sourceModel->setHorizontalHeaderLabels(headerLabels);

  int align = Qt::AlignLeft | Qt::AlignVCenter;
  for (int c = 0; c < headerLabels.size(); ++c)
  {
    m_sourceModel->setHeaderData(c, Qt::Horizontal, align, Qt::TextAlignmentRole);
  }
}

void SmsView::processConnectionEvent(Core::ConnectionEvent event, const QVariant& data)
{
  Core * c = Core::instance();

  if (event == Core::ConnectionEventCustom)
  {
    ModemReply* reply = data.value<ModemReply*>();
    Q_ASSERT(reply);

    if ((reply->handlerName() == SMS_HANDLER_NAME))
    {
      switch(reply->type())
      {
      case SMS_REQUEST_CAPACITY:
      {
        if (reply->status())
        {
          SmsAnswerCapacity * answer = static_cast<SmsAnswerCapacity*> (reply->data());

          m_totalPhone = answer->phoneTotal;
          m_totalSim   = answer->simTotal;
        }
        else
        {
          m_totalSim = -1;
          m_totalPhone = -1;
        }
      }
        break;
      case SMS_REQUEST_READ:
      {
        if (reply->status())
        {
          SmsAnswerRead * answer = static_cast<SmsAnswerRead*> (reply->data());

          foreach(const Sms& sms, answer->smsList)
          {
            m_sourceModel->addSms(sms, m_autoArchiveOnRead);
          }

          updateCounters();
        }
        else
        {
          // TODO: show an error message
        }
      }
        break;
      case SMS_REQUEST_READ_BY_INDEX:
      {
        if (reply->status())
        {
          SmsAnswerReadByIndex * answer = static_cast<SmsAnswerReadByIndex*> (reply->data());

          m_sourceModel->addSms(answer->sms, m_autoArchiveOnRead);
          updateCounters();
        }
        else
        {
          // TODO: show an error message
        }
      }
        break;
      case SMS_REQUEST_READ_UNEXPECTED:
      {
        Q_ASSERT(reply->status());
        SmsAnswerReadUnexpected * answer = static_cast<SmsAnswerReadUnexpected*> (reply->data());

        // sms reading by index and storage
        {
          ModemRequest * request = c->modemRequest(connectionId(), SMS_HANDLER_NAME, SMS_REQUEST_READ_BY_INDEX, 1);
          SmsArgsReadByIndex * args = static_cast<SmsArgsReadByIndex*> (request->args());
          args->smsIndex = answer->smsIndex;
          args->smsStorage = answer->smsStorage;
          c->pushRequest(request);
        }
      }
        break;
      case SMS_REQUEST_DELETE:
      {
        if (reply->status())
        {
          SmsAnswerDeleted * answer = static_cast<SmsAnswerDeleted*> (reply->data());
          m_sourceModel->deleteSmsDevice(answer->smsStorage, answer->smsIndex);
        }
        else
        {
          // TODO: show an error message
        }
      }
        break;
      case SMS_REQUEST_SEND:
      {
        if (reply->status())
        {

        }
        else
        {
          // TODO: show an error message
        }
      }
        break;
      default:
        Q_ASSERT(false);
        Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unknown reply type received!");
      }
    }
  }
  else if (event == Core::ConnectionEventStatus)
  {
    bool opened = data.toBool();

    if (opened)
    {
      // capacity request
      c->pushRequest(c->modemRequest(connectionId(), SMS_HANDLER_NAME, SMS_REQUEST_CAPACITY, 1));

      // sms reading from phone
      {
        ModemRequest * request = c->modemRequest(connectionId(), SMS_HANDLER_NAME, SMS_REQUEST_READ, 1);
        SmsArgsRead * args = static_cast<SmsArgsRead*> (request->args());
        args->smsStatus = SMS_STATUS_ALL;
        args->smsStorage = SMS_STORAGE_PHONE;
        c->pushRequest(request);
      }

      // sms reading from SIM
      {
        ModemRequest * request = c->modemRequest(connectionId(), SMS_HANDLER_NAME, SMS_REQUEST_READ, 1);
        SmsArgsRead * args = static_cast<SmsArgsRead*> (request->args());
        args->smsStatus = SMS_STATUS_ALL;
        args->smsStorage = SMS_STORAGE_SIM;
        c->pushRequest(request);
      }
    }
    else
    {
      // remove SMS that exist in device only
      m_totalSim = 0;
      m_totalPhone = 0;
      m_sourceModel->deleteSmsDevice();
      updateCounters();
    }
  }
}

void SmsView::init()
{
  // source model
  m_sourceModel = new SmsModel(connectionId(), this);

  // proxy model
  m_proxyModel = new SmsProxyModel(this);
  m_proxyModel->setSourceModel(m_sourceModel);
  m_proxyModel->setSortRole(Qt::UserRole);

  ui->smsView->setModel(m_proxyModel);

  ui->smsView->sortByColumn(SmsModel::ColumnDate, Qt::DescendingOrder);

  connect(m_proxyModel, SIGNAL(modelReset()), SLOT(onModelReset()), Qt::QueuedConnection);
  connect(m_proxyModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),
          SLOT(onDataChanged(QModelIndex,QModelIndex,QVector<int>)), Qt::QueuedConnection);
  connect(m_proxyModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
          SLOT(onRowsInserted(QModelIndex,int,int)), Qt::QueuedConnection);
  connect(m_proxyModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
          SLOT(onRowsRemoved(QModelIndex,int,int)), Qt::QueuedConnection);

  connect(ui->smsView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
          SLOT(onSelectionChanged()), Qt::QueuedConnection);
  connect(ui->smsView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          SLOT(onSelectionChanged()), Qt::QueuedConnection);

  // new sms button
  connect(ui->newSmsShowButton, SIGNAL(toggled(bool)), ui->newSmsGroupBox, SLOT(setVisible(bool)));
  ui->newSmsShowButton->setChecked(false);

  // connect status tool buttons
  {
    ui->showUnread->setProperty(SMS_STATUS_PROPERTY, SMS_STATUS_NEW);
    connect(ui->showUnread, SIGNAL(toggled(bool)), SLOT(smsStatusShowButtonToggle(bool)));
    ui->showUnread->setChecked(true);

    ui->showIncome->setProperty(SMS_STATUS_PROPERTY, SMS_STATUS_INCOME);
    connect(ui->showIncome, SIGNAL(toggled(bool)), SLOT(smsStatusShowButtonToggle(bool)));
    ui->showIncome->setChecked(true);

    ui->showDrafts->setProperty(SMS_STATUS_PROPERTY, SMS_STATUS_DRAFT);
    connect(ui->showDrafts, SIGNAL(toggled(bool)), SLOT(smsStatusShowButtonToggle(bool)));
    ui->showDrafts->setChecked(false);

    ui->showSent->setProperty(SMS_STATUS_PROPERTY, SMS_STATUS_SENT);
    connect(ui->showSent  , SIGNAL(toggled(bool)), SLOT(smsStatusShowButtonToggle(bool)));
    ui->showSent->setChecked(false);
  }

  // connect storage tool buttons
  {
    ui->showFromSim->setProperty(SMS_STORAGE_PROPERTY, SMS_STORAGE_SIM);
    connect(ui->showFromSim, SIGNAL(toggled(bool)), SLOT(smsStorageShowButtonToggle(bool)));
    ui->showFromSim->setChecked(true);

    ui->showFromModem->setProperty(SMS_STORAGE_PROPERTY, SMS_STORAGE_PHONE);
    connect(ui->showFromModem, SIGNAL(toggled(bool)), SLOT(smsStorageShowButtonToggle(bool)));
    ui->showFromModem->setChecked(true);
  }

  // SMS view tool buttons actions
  {
    m_smsDelete = new QAction(QIcon("icons/sms_delete.png"), tr("Delete"), this);
    m_smsDelete->setToolTip(tr("Delete message(s)"));
    connect(m_smsDelete, SIGNAL(triggered()), SLOT(onSmsDeleteAction()));
    ui->deleteBtn->setDefaultAction(m_smsDelete);
    ui->smsView->addAction(m_smsDelete);

    m_smsReply = new QAction(QIcon("icons/sms_reply.png"), tr("Reply"), this);
    m_smsReply->setToolTip(tr("Reply to message"));
    connect(m_smsReply, SIGNAL(triggered()), SLOT(onSmsReplyAction()));
    ui->replyBtn->setDefaultAction(m_smsReply);
    ui->smsView->addAction(m_smsReply);
  }

  // header elide
  ui->smsView->horizontalHeader()->setTextElideMode(Qt::ElideRight);

  // reset view
  retranslateUi();
  onModelReset();
}

void SmsView::tini()
{

}

void SmsView::restore(Settings &set)
{
  m_autoArchiveOnRead = set.value("autoArchiveOnRead", false).toBool();
  ui->smsView->horizontalHeader()->restoreState(set.value("smsViewHeader").toByteArray());
}

void SmsView::store(Settings &set)
{
  set.setValue("autoArchiveOnRead", m_autoArchiveOnRead);
  set.setValue("smsViewHeader", ui->smsView->horizontalHeader()->saveState());
}

QString SmsView::name() const
{
  return tr("SMS");
}

void SmsView::smsStatusShowButtonToggle(bool checked)
{
  QToolButton * button = qobject_cast<QToolButton*>(sender());
  Q_ASSERT(button);

  SAFE_CONVERT(SMS_STATUS, toInt, status, button->property(SMS_STATUS_PROPERTY), Q_ASSERT(false));

  m_proxyModel->setSmsStatusShow(status, checked);
}

void SmsView::smsStorageShowButtonToggle(bool checked)
{
  QToolButton * button = qobject_cast<QToolButton*>(sender());
  Q_ASSERT(button);

  SAFE_CONVERT(SMS_STORAGE, toInt, storage, button->property(SMS_STORAGE_PROPERTY), Q_ASSERT(false));

  m_proxyModel->setSmsStorageShow(storage, checked);
}

void SmsView::onSelectionChanged()
{
  QModelIndexList selected = ui->smsView->selectionModel()->selectedRows();
  QModelIndex current = ui->smsView->selectionModel()->currentIndex();

  m_smsDelete->setEnabled((!selected.isEmpty()) || (current.isValid()));

  bool singleSelected = false;
  if (current.isValid() && (selected.size() == 1))
  {
    if (current.row() == selected.at(0).row())
    {
      singleSelected = true;
    }
  }

  m_smsReply->setEnabled(singleSelected);

  // set selected SMS data into field
  ui->currentSmsInfo->clear();
  if (singleSelected)
  {
    int sourceRow = m_proxyModel->mapToSource(current).row();

    const QString row("<b>%1 </b>%2<br>");
    QString smsHeader = row.arg(tr("Sender:")).arg(m_sourceModel->smsSender(sourceRow)) +

                        row.arg(tr("Date and Time:")).arg(m_sourceModel->smsDate(sourceRow)
                                                          .toString(Qt::DefaultLocaleShortDate)) +

                        row.arg(tr("Service center:")).arg(m_sourceModel->smsSC(sourceRow)) +

                        row.arg(tr("SMS size:")).arg(QString::number(m_sourceModel->smsSizeCount(sourceRow))
                                                     + QChar(' ') + tr("SMSes"));

    ui->currentSmsInfo->appendHtml(smsHeader);
    ui->currentSmsInfo->appendPlainText(m_sourceModel->smsText(sourceRow));

    ui->currentSmsInfo->verticalScrollBar()->setValue(0);
  }
}

void SmsView::onSmsDeleteAction()
{
  QModelIndexList selected = ui->smsView->selectionModel()->selectedRows();
  QModelIndex current = ui->smsView->selectionModel()->currentIndex();

  QModelIndexList list;
  if (selected.size() > 1)
  {
    list = selected;
  }
  else if (current.isValid())
  {
    list << current;
  }
  else
  {
    Q_ASSERT(false);
  }

  Core * c = Core::instance();
  for (int i = list.size() - 1; i>= 0; --i)
  {
    QModelIndex index = m_proxyModel->mapToSource(list.at(i));
    QList<int> smsIndices = m_sourceModel->smsIndices(index.row());
    SMS_STORAGE smsStorage = m_sourceModel->smsStorage(index.row());

    foreach(int smsIndex, smsIndices)
    {
      ModemRequest * request = c->modemRequest(connectionId(), SMS_HANDLER_NAME, SMS_REQUEST_DELETE, 1);
      SmsArgsDelete * args = static_cast<SmsArgsDelete*> (request->args());
      args->smsStorage = smsStorage;
      args->smsIndex = smsIndex;
      c->pushRequest(request);
    }
  }
}

void SmsView::onSmsReplyAction()
{
  QModelIndexList selected = ui->smsView->selectionModel()->selectedRows();
  QModelIndex current = ui->smsView->selectionModel()->currentIndex();

  if (current.isValid() &&
      ((!selected.size()) || ((selected.size() == 1) && (selected.at(0).row() == current.row()))))
  {
    ui->newSmsShowButton->setChecked(true);
    ui->smsReceiver->setText(current.sibling(current.row(), SmsModel::ColumnNumber).data().toString());
    ui->smsText->setFocus();
  }
  else
  {
    Q_ASSERT(false);
  }
}

void SmsView::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
  Q_UNUSED(topLeft);
  Q_UNUSED(bottomRight);
  Q_UNUSED(roles);

  updateCounters();
}

void SmsView::onModelReset()
{
  onSelectionChanged();
  updateCounters();
}

void SmsView::onRowsInserted(const QModelIndex& parent, int first, int last)
{
  Q_UNUSED(parent);
  Q_UNUSED(first);
  Q_UNUSED(last);

  onModelReset();
}

void SmsView::onRowsRemoved(const QModelIndex& parent, int first, int last)
{
  Q_UNUSED(parent);
  Q_UNUSED(first);
  Q_UNUSED(last);

  onModelReset();
}

void SmsView::updateCounters()
{
  // status counters
  ui->labelSmsNewCountValue->setText(QString::number(m_sourceModel->statusCount(SMS_STATUS_NEW)));
  ui->labelSmsIncomeCountValue->setText(QString::number(m_sourceModel->statusCount(SMS_STATUS_INCOME)));
  ui->labelSmsDraftsCountValue->setText(QString::number(m_sourceModel->statusCount(SMS_STATUS_DRAFT)));
  ui->labelSmsSentCountValue->setText(QString::number(m_sourceModel->statusCount(SMS_STATUS_SENT)));

  // storage counters
  ui->labelSmsStorageCapacityArchiveValue->setText(QString::number(m_sourceModel->archivedCount()));

  const QString errStr(tr("<ERROR>"));
  const QString redText("<font color=\"red\">%1</font>");

#define SET_COUNTER(label, used, total) \
  { \
    const int criticalPercent = 75; \
    bool critical = false; \
    QString usedStr(QString::number(used)); \
    QString totalStr(errStr); \
    if (total != -1) \
    { \
      totalStr = QString::number(total); \
      if (total && (used * 100 / total >= criticalPercent)) \
      { \
        usedStr = redText.arg(used); \
        critical = true; \
      } \
    } \
    label->setText(usedStr + QString("<b>/</b>") + totalStr); \
    if (critical) \
    { \
      label->setToolTip(tr("Used capacity is above %1 percent of total storage capacity!").arg(criticalPercent)); \
    } \
  }

  SET_COUNTER(ui->labelSmsStorageCapacitySIMValue,   m_sourceModel->storageCount(SMS_STORAGE_SIM),   m_totalSim);
  SET_COUNTER(ui->labelSmsStorageCapacityPhoneValue, m_sourceModel->storageCount(SMS_STORAGE_PHONE), m_totalPhone);
}




SmsProxyModel::SmsProxyModel(QObject* parent) :
  QSortFilterProxyModel(parent)
{

}

void SmsProxyModel::setSmsStatusShow(SMS_STATUS status, bool show)
{
  Q_ASSERT(checkSmsStatus(status));

  if (status != SMS_STATUS_ALL)
  {
    m_statusMap.insert(status, show);
  }
  else
  {
    Q_ASSERT(false);
  }

  invalidate();
}

void SmsProxyModel::setSmsStorageShow(SMS_STORAGE storage, bool show)
{
  Q_ASSERT(checkSmsStorage(storage));

  m_storageMap.insert(storage, show);
  invalidate();
}

bool SmsProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
  SmsModel * model = static_cast<SmsModel*> (sourceModel());

  // check status
  SMS_STATUS status = model->smsStatus(source_row);
  if (m_statusMap.value(status, false) == false)
  {
    return false;
  }

  // check storage
  SMS_STORAGE storage = model->smsStorage(source_row);
  if (m_storageMap.value(storage, false) == false)
  {
    return false;
  }

  return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}





