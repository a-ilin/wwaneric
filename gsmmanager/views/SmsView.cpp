#include "SmsView.h"
#include "ui_SmsView.h"

#include "../Core.h"
#include "../Sms.h"

#include <QListWidget>
#include <QMenu>

SmsView::SmsView(QWidget *parent) :
  QWidget(parent),
  IView(),
  ui(new Ui::SmsView)
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

void SmsView::init()
{
  Modem * modem = Core::instance()->modem();
  connect(modem, SIGNAL(modemNotification(MODEM_NOTIFICATION_TYPE)),
          this, SLOT(updateSmsFromModem()));
}

void SmsView::tini()
{

}

void SmsView::restore(Settings &set)
{
  SmsDatabaseEntity smsDb;
  if (smsDb.init())
  {
    updateSms(smsDb.select());
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
  QListWidgetItem * item = lw->itemAt(pos);
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
    bool result = Core::instance()->modem()->deleteSms(sms.storage(), sms.index());
    if(result)
    {
      delete lw->takeItem(index.row());
    }
  }
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

void SmsView::updateSmsFromModem()
{
  // SMS storage status
  int simUsed = 0;
  int simTotal = 0;
  int phoneUsed = 0;
  int phoneTotal = 0;

  Modem * modem = Core::instance()->modem();

  modem->storageCapacityUsed(&simUsed, &simTotal, &phoneUsed, &phoneTotal);

  ui->labelSmsStorageCapacitySIMValue->setText(QString::number(simUsed) +
                                               QString("/") +
                                               QString::number(simTotal));

  ui->labelSmsStorageCapacityPhoneValue->setText(QString::number(phoneUsed) +
                                                 QString("/") +
                                                 QString::number(phoneTotal));

  // clear SMS widgets
  ui->lwSmsIncome->clear();
  ui->lwSmsDrafts->clear();
  ui->lwSmsSend->clear();


  QList<Sms> smsList;

  // SMS new read from SIM and phone
  smsList.append(modem->readSms(SMS_STORAGE_SIM, SMS_STATUS_NEW));
  smsList.append(modem->readSms(SMS_STORAGE_PHONE, SMS_STATUS_NEW));

  // SMS old read from SIM and phone
  smsList.append(modem->readSms(SMS_STORAGE_SIM, SMS_STATUS_INCOME));
  smsList.append(modem->readSms(SMS_STORAGE_PHONE, SMS_STATUS_INCOME));

  // SMS drafts read from SIM and phone
  smsList.append(modem->readSms(SMS_STORAGE_SIM, SMS_STATUS_DRAFT));
  smsList.append(modem->readSms(SMS_STORAGE_PHONE, SMS_STATUS_DRAFT));

  // SMS sent read from SIM and phone
  smsList.append(modem->readSms(SMS_STORAGE_SIM, SMS_STATUS_SENT));
  smsList.append(modem->readSms(SMS_STORAGE_PHONE, SMS_STATUS_SENT));

  // write to DB
  SmsDatabaseEntity smsDb;
  if (smsDb.init())
  {
    smsDb.insert(smsList);
  }

  updateSms(smsList);
}

void SmsView::updateSms(const QList<Sms> &smsList)
{
  int newSmsCount = 0;
  int readSmsCount = 0;
  int draftSmsCount = 0;
  int sentSmsCount = 0;

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
      newSmsCount++;
    }

    if (sms.status() == SMS_STATUS_INCOME)
    {
      ui->lwSmsIncome->addItem(smsItem);
      readSmsCount++;
    }

    if (sms.status() == SMS_STATUS_DRAFT)
    {
      ui->lwSmsDrafts->addItem(smsItem);
      draftSmsCount++;
    }

    if (sms.status() == SMS_STATUS_SENT)
    {
      ui->lwSmsSend->addItem(smsItem);
      sentSmsCount++;
    }
  }

  ui->labelSmsNewCountValue->setText(QString::number(newSmsCount));
  ui->labelSmsIncomeCountValue->setText(QString::number(readSmsCount));
  ui->labelSmsDraftsCountValue->setText(QString::number(draftSmsCount));
  ui->labelSmsSentCountValue->setText(QString::number(sentSmsCount));
}
