#include "SmsView.h"
#include "ui_SmsView.h"

#include "Core.h"

#include "ModemSms.h"

#include <QItemSelection>
#include <QMenu>
#include <QStandardItemModel>
#include <QTableView>

#define SMS_STATUS_PROPERTY "smsStatus"
#define SMS_STORAGE_PROPERTY "smsStorage"

SmsView::SmsView(const QString& connectionId, QWidget *parent) :
  QWidget(parent),
  IView(connectionId),
  ui(new Ui::SmsView)
{
  ui->setupUi(this);

  // source model
  m_sourceModel = new QStandardItemModel(0, ColumnLast, this);
  m_sourceModel->setColumnCount(ColumnLast);
  QStringList headerLabels;
  headerLabels
      << ""   // status
      << ""   // storage
      << "Date"
      << "Number"
      << "Message";
  m_sourceModel->setHorizontalHeaderLabels(headerLabels);

  // proxy model
  m_proxyModel = new SmsProxyModel(this);
  m_proxyModel->setSourceModel(m_sourceModel);

  ui->smsView->setModel(m_proxyModel);

  ui->smsView->sortByColumn(ColumnDate, Qt::DescendingOrder);

  connect(ui->smsView->model(), SIGNAL(modelReset()), SLOT(changeViewActionStatus()));
  connect(ui->smsView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
          SLOT(changeViewActionStatus()));
  connect(ui->smsView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          SLOT(changeViewActionStatus()));

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

    changeViewActionStatus();
  }
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
    break;
  default:
    break;
  }
}

void SmsView::processConnectionEvent(Core::ConnectionEvent event, const QVariant& data)
{
  if (event == Core::ConnectionEventCustom)
  {
    ModemReply* reply = data.value<ModemReply*>();
    Q_ASSERT(reply);

    if ((reply->handlerName() == SMS_HANDLER_NAME))
    {
      const QString errStr = tr("<ERROR>");

      switch(reply->type())
      {
      case SMS_REQUEST_CAPACITY:
      {
        QString simUsed, simTotal, phoneUsed, phoneTotal;

        if (reply->status())
        {
          SmsAnswerCapacity * answer = static_cast<SmsAnswerCapacity*> (reply->data());

          simUsed  = QString::number(answer->simUsed);
          simTotal = QString::number(answer->simTotal);
          phoneUsed  = QString::number(answer->phoneUsed);
          phoneTotal = QString::number(answer->phoneTotal);
        }
        else
        {
          simUsed    = errStr;
          simTotal   = errStr;
          phoneUsed  = errStr;
          phoneTotal = errStr;
        }

        ui->labelSmsStorageCapacitySIMValue->setText(simUsed + QChar('/') +simTotal);
        ui->labelSmsStorageCapacityPhoneValue->setText(phoneUsed + QChar('/') + phoneTotal);
      }
        break;
      case SMS_REQUEST_READ:
      {
        if (reply->status())
        {
          SmsAnswerRead * answer = static_cast<SmsAnswerRead*> (reply->data());
          readSms(answer->smsList);
        }
        else
        {
          // TODO: show an error message
        }
      }
        break;
      case SMS_REQUEST_DELETE:
      {

      }
        break;
      case SMS_REQUEST_SEND:
      {

      }
        break;
      default:
        Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unknown reply type received!");
      }
    }
  }
  else if (event == Core::ConnectionEventStatus)
  {
    bool opened = data.toBool();

    if (opened)
    {
      Core * c = Core::instance();

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
  }
}

void SmsView::init()
{
}

void SmsView::tini()
{

}

void SmsView::restore(Settings &set)
{
  ui->smsView->horizontalHeader()->restoreState(set.value("smsViewHeader").toByteArray());

  SmsDatabaseEntity smsDb;
  if (smsDb.init())
  {
    //appendSms(smsDb.select());
  }
}

void SmsView::store(Settings &set)
{
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

void SmsView::changeViewActionStatus()
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
}

void SmsView::onSmsDeleteAction()
{
  QModelIndexList selected = ui->smsView->selectionModel()->selectedRows();
  QModelIndex current = ui->smsView->selectionModel()->currentIndex();

  QList<QPair<SMS_STORAGE, QList<int> > > forDeletion;

  if (selected.size() > 1)
  {
    qSort(selected.begin(), selected.end());

    for (int i = selected.size() - 1; i>= 0; --i)
    {
      const QModelIndex &index = selected.at(i);

      QModelIndex storageIndex = index.sibling(index.row(), ColumnStorage);
      Q_ASSERT(storageIndex.isValid());

      forDeletion.append(qMakePair((SMS_STORAGE)storageIndex.data(Qt::UserRole).toInt(),
                                   storageIndex.data(Qt::UserRole+1).value<QList<int> >()));

      QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
      Q_ASSERT(sourceIndex.isValid());
      m_sourceModel->removeRow(sourceIndex.row());
    }
  }
  else if (current.isValid())
  {
    QModelIndex storageIndex = current.sibling(current.row(), ColumnStorage);
    Q_ASSERT(storageIndex.isValid());

    forDeletion.append(qMakePair((SMS_STORAGE)storageIndex.data(Qt::UserRole).toInt(),
                                 storageIndex.data(Qt::UserRole+1).value<QList<int> >()));

    QModelIndex sourceIndex = m_proxyModel->mapToSource(current);
    Q_ASSERT(sourceIndex.isValid());
    m_sourceModel->removeRow(sourceIndex.row());
  }
  else
  {
    Q_ASSERT(false);
  }

  deleteSms(forDeletion);
}

void SmsView::onSmsReplyAction()
{
  QModelIndexList selected = ui->smsView->selectionModel()->selectedRows();
  QModelIndex current = ui->smsView->selectionModel()->currentIndex();

  if (current.isValid() &&
      ((!selected.size()) || ((selected.size() == 1) && (selected.at(0).row() == current.row()))))
  {
    ui->newSmsShowButton->setChecked(true);
    ui->smsReceiver->setText(current.sibling(current.row(), ColumnNumber).data().toString());
    ui->smsText->setFocus();
  }
  else
  {
    Q_ASSERT(false);
  }
}

void SmsView::readSms(const QList<Sms>& smsList)
{
  // TODO: Provide real SMS concatenation
  QList<SmsMeta> smsMetaList;
  foreach(const Sms& sms, smsList)
  {
    smsMetaList.append(SmsMeta(QList<Sms>() << sms));
  }

  // TODO: ???
  //m_sourceModel->removeRows(0, m_sourceModel->rowCount());

  int smsCountNew = 0;
  int smsCountIncome = 0;
  int smsCountDraft = 0;
  int smsCountSent = 0;

  int row = m_sourceModel->rowCount();
  m_sourceModel->setRowCount(m_sourceModel->rowCount() + smsMetaList.size());
  foreach(const SmsMeta& smsMeta, smsMetaList)
  {
    SMS_STATUS status = smsMeta.status();
    QIcon statusIcon;
    switch (status)
    {
    case SMS_STATUS_NEW:
      ++smsCountNew;
      statusIcon = QIcon("icons/sms_unread.png");
      break;
    case SMS_STATUS_INCOME:
      ++smsCountIncome;
      statusIcon = QIcon("icons/sms_income.png");
      break;
    case SMS_STATUS_DRAFT:
      ++smsCountDraft;
      statusIcon = QIcon("icons/sms_draft.png");
      break;
    case SMS_STATUS_SENT:
      ++smsCountSent;
      statusIcon = QIcon("icons/sms_sent.png");
      break;
    default:
      Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unknown status!");
    }

    QIcon storageIcon;
    SMS_STORAGE storage = smsMeta.storage();
    switch(storage)
    {
    case SMS_STORAGE_PHONE:
      storageIcon = QIcon("icons/sms_modem.png");
      break;
    case SMS_STORAGE_SIM:
      storageIcon = QIcon("icons/sms_sim.png");
      break;
    default:
      Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unknown storage!");
    }

    QString smsTooltip;
    {
      QStringList indexes;
      foreach(int index, smsMeta.indexes())
      {
        indexes.append(QString::number(index));
      }

      smsTooltip =
          tr("SMS service center: %1").arg(smsMeta.smsc()) + QString(ENDL) +
          tr("Message contains %1 SMS(es)").arg(indexes.size());
    }

    QStandardItem * itemStorage = new QStandardItem(storageIcon, QString());
    itemStorage->setData(storage, Qt::UserRole);
    itemStorage->setData(QVariant::fromValue<QList<int> >(smsMeta.indexes()), Qt::UserRole+1);
    itemStorage->setToolTip(smsTooltip);
    m_sourceModel->setItem(row, ColumnStorage, itemStorage);

    QStandardItem * itemStatus = new QStandardItem(statusIcon, QString());
    itemStatus->setData(status, Qt::UserRole);
    itemStatus->setToolTip(smsTooltip);
    m_sourceModel->setItem(row, ColumnStatus, itemStatus);

    QStandardItem * itemDate = new QStandardItem(smsMeta.dateTime().toString(Qt::RFC2822Date));
    itemDate->setToolTip(smsTooltip);
    m_sourceModel->setItem(row, ColumnDate, itemDate);

    QStandardItem * itemNumber = new QStandardItem(smsMeta.sender());
    itemNumber->setToolTip(smsTooltip);
    m_sourceModel->setItem(row, ColumnNumber, itemNumber);

    QStandardItem * itemMessage = new QStandardItem(smsMeta.userText());
    itemMessage->setToolTip(smsTooltip);
    m_sourceModel->setItem(row, ColumnMessage, itemMessage);

    ++row;
  }

  m_proxyModel->invalidate();

  ui->smsView->resizeColumnsToContents();

  // set count labels
  ui->labelSmsNewCountValue->setText(QString::number(smsCountNew));
  ui->labelSmsIncomeCountValue->setText(QString::number(smsCountIncome));
  ui->labelSmsDraftsCountValue->setText(QString::number(smsCountDraft));
  ui->labelSmsSentCountValue->setText(QString::number(smsCountSent));
}

void SmsView::deleteSms(const QList<QPair<SMS_STORAGE,QList<int> > >& msgList)
{
  Core * c = Core::instance();

  typedef QPair<SMS_STORAGE,QList<int> > Msg;

  foreach (const Msg& msg, msgList)
  {
    foreach(int index, msg.second)
    {
      ModemRequest * request = c->modemRequest(connectionId(), SMS_HANDLER_NAME, SMS_REQUEST_DELETE, 1);
      SmsArgsDelete * args = static_cast<SmsArgsDelete*> (request->args());
      args->smsStorage = msg.first;
      args->smsIndex = index;
      c->pushRequest(request);
    }
  }
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
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Using SMS_STATUS_ALL is not allowed here!");
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
  if (source_parent.isValid())
  {
    return false;
  }

  QStandardItemModel * model = static_cast<QStandardItemModel*> (sourceModel());

  // check status
  QModelIndex statusIndex =  model->index(source_row, SmsView::ColumnStatus);

  if (!statusIndex.isValid())
  {
    return false;
  }

  SMS_STATUS status = (SMS_STATUS)statusIndex.data(Qt::UserRole).toInt();

  if (m_statusMap.value(status, false) == false)
  {
    return false;
  }

  // check storage
  QModelIndex storageIndex = model->index(source_row, SmsView::ColumnStorage);

  if (!storageIndex.isValid())
  {
    return false;
  }

  SMS_STORAGE storage = (SMS_STORAGE)storageIndex.data(Qt::UserRole).toInt();

  if (m_storageMap.value(storage, false) == false)
  {
    return false;
  }

  return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
