#include "ModemStatusView.h"
#include "ui_ModemStatusView.h"

#include "common.h"

#include "Core.h"
#include "MainWindow.h"
#include "ModemStatus.h"

#include <QClipboard>
#include <QTimer>

ModemStatusView::ModemStatusView(const QString& connectionId, QWidget *parent) :
  QWidget(parent),
  IView(connectionId),
  ui(new Ui::ModemStatusView)
{
  ui->setupUi(this);

  ui->statusWidget->setColumnCount(ColumnLast);

  // context menu actions
  QAction * copyToClipboard = new QAction(tr("Copy to clipboard"), this);
  connect(copyToClipboard, SIGNAL(triggered()), SLOT(onCopyToClipboard()));
  ui->statusWidget->addAction(copyToClipboard);

  m_timer = new QTimer(this);
  m_timer->setInterval(5000);
  connect(m_timer, SIGNAL(timeout()), SLOT(onTimerTimeout()));
}

ModemStatusView::~ModemStatusView()
{
  delete ui;
}

void ModemStatusView::changeEvent(QEvent *e)
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

void ModemStatusView::setStatusInfo(const QString& statusType, const QString& statusInfo)
{
  QList<QTableWidgetItem*> items = ui->statusWidget->findItems(statusType,
                                                                    Qt::MatchExactly);

  QTableWidgetItem * itemInfo = NULL;

  foreach(QTableWidgetItem * item, items)
  {
    if (item->column() == ColumnType)
    {
      int row = item->row();
      itemInfo = ui->statusWidget->item(row, ColumnInfo);
    }
    else
    {
      Q_LOGEX(LOG_VERBOSE_NOTIFICATION, "Type and info mismatch found!");
    }
  }

  if (!itemInfo)
  {
    // add new row
    int row = ui->statusWidget->rowCount();
    ui->statusWidget->setRowCount(row + 1);

    QTableWidgetItem * itemType = new QTableWidgetItem(statusType);
    ui->statusWidget->setItem(row, ColumnType, itemType);

    itemInfo = new QTableWidgetItem();
    ui->statusWidget->setItem(row, ColumnInfo, itemInfo);
  }

  Q_ASSERT(itemInfo);

  itemInfo->setText(statusInfo);

  ui->statusWidget->resizeColumnsToContents();
}

void ModemStatusView::updateSignalQuality(bool noError,
                                          bool signalDetected, double signalDbm, double signalPercent,
                                          bool berDetected, const QString& berPercentRange)
{
  QString unknown = tr("Unknown");
  const QString errStr = tr("<ERROR>");

  setStatusInfo(tr("Signal strength (DBM):"), noError ?
                  signalDetected ? QString::number(signalDbm) :
                                            unknown : errStr);

  setStatusInfo(tr("Signal strength (%):"), noError ?
                  signalDetected ? QString::number(signalPercent) :
                                            unknown : errStr);

  setStatusInfo(tr("Bit error rate (%):"), noError ?
                  berDetected ? berPercentRange :
                                         unknown : errStr);

  Core::instance()->mainWindow()->updateSignalStrength(connectionId(),
                   signalDetected ? signalPercent : 0.0);
}

void ModemStatusView::onCopyToClipboard()
{
  // copy full contents to clipboard
  QString text;

  for(int row=0; row < ui->statusWidget->rowCount(); ++row)
  {
    QString type = ui->statusWidget->item(row, ColumnType)->text();
    QString info = ui->statusWidget->item(row, ColumnInfo)->text();

    text += type + info + QString(ENDL);
  }

  QClipboard * clipboard = QApplication::clipboard();
  clipboard->setText(text);
}

void ModemStatusView::onTimerTimeout()
{
  Core * c = Core::instance();

  c->pushRequest(c->modemRequest(connectionId(), STATUS_HANDLER_NAME, STATUS_REQUEST_SIGNAL_QUALITY, 1));
}

void ModemStatusView::init()
{

}

void ModemStatusView::tini()
{

}

void ModemStatusView::restore(Settings &set)
{
  Q_UNUSED(set);
}

void ModemStatusView::store(Settings &set)
{
  Q_UNUSED(set);
}

QString ModemStatusView::name() const
{
  return tr("Modem status");
}

bool ModemStatusView::event(QEvent *e)
{
  if (e->type() == ModemEventType)
  {

    e->accept();
    return true;
  }
  else
  {
    return QWidget::event(e);
  }
}

void ModemStatusView::processConnectionEvent(Core::ConnectionEvent event, const QVariant& data)
{
  if (event == Core::ConnectionEventCustom)
  {
    ModemReply* reply = data.value<ModemReply*>();
    Q_ASSERT(reply);

    if ((reply->handlerName() == STATUS_HANDLER_NAME))
    {
      const QString errStr = tr("<ERROR>");

      switch(reply->type())
      {
      case STATUS_REQUEST_MANUFACTURER_INFO:
      {
        StatusAnswer * statusAnswer = static_cast<StatusAnswer*> (reply->data());
        QString answer = statusAnswer->data;
        setStatusInfo(tr("Manufacturer:"), reply->status() ? answer : errStr);
      }
        break;
      case STATUS_REQUEST_REVISION_INFO:
      {
        StatusAnswer * statusAnswer = static_cast<StatusAnswer*> (reply->data());
        QString answer = statusAnswer->data;
        setStatusInfo(tr("Revision:"), reply->status() ? answer : errStr);
      }
        break;
      case STATUS_REQUEST_MODEL_INFO:
      {
        StatusAnswer * statusAnswer = static_cast<StatusAnswer*> (reply->data());
        QString answer = statusAnswer->data;
        setStatusInfo(tr("Model:"), reply->status() ? answer : errStr);
      }
        break;
      case STATUS_REQUEST_SERIAL_NUMBER:
      {
        StatusAnswer * statusAnswer = static_cast<StatusAnswer*> (reply->data());
        QString answer = statusAnswer->data;
        setStatusInfo(tr("Serial number:"), reply->status() ? answer : errStr);
      }
        break;
      case STATUS_REQUEST_SIGNAL_QUALITY:
      {
        StatusSignalQualityAnswer * answer =
            static_cast<StatusSignalQualityAnswer*> (reply->data());

        updateSignalQuality(reply->status(), answer->signal_detected, answer->signal_dbm,
                            answer->signal_percent,
                            answer->ber_detected, answer->ber_percent_range);
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

      c->pushRequest(c->modemRequest(connectionId(), STATUS_HANDLER_NAME, STATUS_REQUEST_MANUFACTURER_INFO, 1));
      c->pushRequest(c->modemRequest(connectionId(), STATUS_HANDLER_NAME, STATUS_REQUEST_REVISION_INFO, 1));
      c->pushRequest(c->modemRequest(connectionId(), STATUS_HANDLER_NAME, STATUS_REQUEST_MODEL_INFO, 1));
      c->pushRequest(c->modemRequest(connectionId(), STATUS_HANDLER_NAME, STATUS_REQUEST_SERIAL_NUMBER, 1));

      // push requests immediately and start timer
      onTimerTimeout();
      m_timer->start();
    }
    else
    {
      m_timer->stop();

      updateSignalQuality(true, false, 0.0, 0.0, false, QString());
    }

    Core::instance()->mainWindow()->updateConnectionStatus(connectionId(), opened);
  }
}


