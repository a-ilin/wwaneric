#include "SmsView.h"
#include "ui_SmsView.h"

#include "../Core.h"

#include <QListWidget>
#include <QMenu>

SmsView::SmsView(QWidget *parent) :
  QWidget(parent),
  IView(),
  ui(new Ui::SmsView),
  m_newSmsCount(0),
  m_readSmsCount(0),
  m_draftSmsCount(0),
  m_sentSmsCount(0)
{
  ui->setupUi(this);

  connect(ui->lwSmsIncome, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(smsContextMenuRequested(QPoint)));
  connect(ui->lwSmsSend, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(smsContextMenuRequested(QPoint)));
  connect(ui->lwSmsDrafts, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(smsContextMenuRequested(QPoint)));
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

bool SmsView::event(QEvent *e)
{
  if (e->type() == ModemEventType)
  {
    updateStatus();
    e->accept();
    return true;
  }
  else
  {
    return QWidget::event(e);
  }
}

void SmsView::init()
{
  Modem * modem = Core::instance()->modem();

  connect(modem, SIGNAL(updatedSmsCapacity(int,int,int,int)), SLOT(updateSmsCapacity(int,int,int,int)));
  connect(modem, SIGNAL(updatedSms(QList<Sms>)), SLOT(appendSms(QList<Sms>)));
  connect(modem, SIGNAL(deletedSms(SMS_STORAGE,int)), SLOT(deleteSms(SMS_STORAGE,int)));
}

void SmsView::tini()
{

}

void SmsView::restore(Settings &set)
{
  SmsDatabaseEntity smsDb;
  if (smsDb.init())
  {
    appendSms(smsDb.select());
  }
}

void SmsView::store(Settings &set)
{

}

QString SmsView::name()
{
  return tr("SMS");
}

void SmsView::smsContextMenuRequested(const QPoint &pos)
{
  QListWidget * lw = qobject_cast<QListWidget *> (sender());
  Q_ASSERT(lw);

  QModelIndex index = lw->indexAt(pos);
  Sms sms = index.data(Qt::UserRole).value<Sms>();

  // construct menu
  QMenu *menu = new QMenu(this);
  QAction deleteSms("Delete SMS", this);
  if(!index.isValid())
  {
    deleteSms.setEnabled(false);
  }

  menu->addAction(&deleteSms);
  QAction *executed = menu->exec(lw->viewport()->mapToGlobal(pos));

  if (executed == &deleteSms)
  {
    Core::instance()->modem()->deleteSms(sms.storage(), sms.index());
  }
}

void SmsView::updateSmsCapacity(int simUsed, int simTotal, int phoneUsed, int phoneTotal)
{
  ui->labelSmsStorageCapacitySIMValue->setText(QString::number(simUsed) +
                                               QString("/") +
                                               QString::number(simTotal));

  ui->labelSmsStorageCapacityPhoneValue->setText(QString::number(phoneUsed) +
                                                 QString("/") +
                                                 QString::number(phoneTotal));
}

void SmsView::fillSmsTextAndTooltip(Sms * sms, QString * smsText, QString * smsTooltip) const
{
  *smsText =
      tr("Timestamp: ") + sms->dateTime().toString() + QString("\n") +
      tr("From: ") + sms->sender() + QString("\n") +
      sms->userText()
   ;

  *smsTooltip =
      tr("SMSC: ") + sms->smsc() + QString("\n") +
      tr("Storage: ") + (sms->storage() == SMS_STORAGE_SIM ? tr("SIM") : tr("Phone")) + QString("\n") +
      tr("Index: ") + QString::number(sms->index()) + QString("\n") +
      tr("UDH type: ") + sms->udhType() + QString("\n")
      ;
}

void SmsView::updateStatus()
{
  Modem * modem = Core::instance()->modem();
  modem->updateSmsCapacity();

  modem->updateSms(SMS_STORAGE_SIM, SMS_STATUS_ALL);
  modem->updateSms(SMS_STORAGE_PHONE, SMS_STATUS_ALL);

  // clear SMS widgets
  ui->lwSmsIncome->clear();
  ui->lwSmsDrafts->clear();
  ui->lwSmsSend->clear();
  m_newSmsCount = 0;
  m_readSmsCount = 0;
  m_draftSmsCount = 0;
  m_sentSmsCount = 0;
}

void SmsView::appendSms(const QList<Sms> &smsList)
{
  // write to DB
  SmsDatabaseEntity smsDb;
  if (smsDb.init())
  {
    smsDb.insert(smsList);
  }

  // TODO: Make SMS list sorting by date, but unread must be first!

  foreach(const Sms &sms, smsList)
  {
    QString smsText =
        tr("Timestamp: ") + sms.dateTime().toString() + QString("\n") +
        tr("From: ") + sms.sender() + QString("\n") +
        sms.userText();

    QString smsTooltip =
        tr("SMSC: ") + sms.smsc() + QString("\n") +
        tr("Storage: ") + (sms.storage() == SMS_STORAGE_SIM ? tr("SIM") : tr("Phone")) + QString("\n") +
        tr("Index: ") + QString::number(sms.index()) + QString("\n") +
        tr("UDH type: ") + sms.udhType() + QString("\n");

    QListWidgetItem *smsItem = new QListWidgetItem(smsText);
    QVariant smsOverlay;
    smsOverlay.setValue(sms);
    smsItem->setData(Qt::UserRole, smsOverlay);
    smsItem->setToolTip(smsTooltip);

    if (sms.status() == SMS_STATUS_NEW)
    {
      // temporary solution for new items
      smsItem->setBackgroundColor(Qt::GlobalColor::cyan);
      ui->lwSmsIncome->addItem(smsItem);
      m_newSmsCount++;
    }

    if (sms.status() == SMS_STATUS_INCOME)
    {
      ui->lwSmsIncome->addItem(smsItem);
      m_readSmsCount++;
    }

    if (sms.status() == SMS_STATUS_DRAFT)
    {
      ui->lwSmsDrafts->addItem(smsItem);
      m_draftSmsCount++;
    }

    if (sms.status() == SMS_STATUS_SENT)
    {
      ui->lwSmsSend->addItem(smsItem);
      m_sentSmsCount++;
    }
  }

  ui->labelSmsNewCountValue->setText(QString::number(m_newSmsCount));
  ui->labelSmsIncomeCountValue->setText(QString::number(m_readSmsCount));
  ui->labelSmsDraftsCountValue->setText(QString::number(m_draftSmsCount));
  ui->labelSmsSentCountValue->setText(QString::number(m_sentSmsCount));
}

void SmsView::deleteSms(SMS_STORAGE storage, int index)
{
  QList<QListWidget*> lws;
  lws << ui->lwSmsIncome << ui->lwSmsDrafts << ui->lwSmsSend;

  QListWidget * lwItem = NULL;
  int rowItem = -1;

  foreach(QListWidget* lw, lws)
  {
    int rowCount = lw->model()->rowCount();
    for (int i=0; i < rowCount; ++i)
    {
      Sms sms = lw->model()->index(i, 0).data(Qt::UserRole).value<Sms>();
      if ((sms.storage() == storage) && (sms.index() == index))
      {
        lwItem = lw;
        rowItem = i;
        break;
      }
    }
    if (lwItem)
    {
      break;
    }
  }

  if (lwItem)
  {
    delete lwItem->takeItem(rowItem);
  }
  else
  {
    QString err = QString("Specified item not found in list! Storage: %1. Index: %2.")
                  .arg(storage).arg(index);
    Q_LOGEX(LOG_VERBOSE_ERROR, err);
  }
}
