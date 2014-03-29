#include "ModemStatusView.h"
#include "ui_ModemStatusView.h"

#include "../Core.h"

ModemStatusView::ModemStatusView(QWidget *parent) :
  QWidget(parent),
  IView(),
  ui(new Ui::ModemStatusView)
{
  ui->setupUi(this);
}

void ModemStatusView::init()
{
  Modem * modem = Core::instance()->modem();
  connect(modem, SIGNAL(modemNotification(MODEM_NOTIFICATION_TYPE)),
          this, SLOT(updateStatus()));
}

void ModemStatusView::tini()
{

}

void ModemStatusView::restore(Settings &set)
{

}

void ModemStatusView::store(Settings &set)
{

}

QString ModemStatusView::name()
{
  return tr("Modem Status");
}

void ModemStatusView::updateStatus()
{
  Modem * modem = Core::instance()->modem();

  ui->labelManufacturer->setText(modem->manufacturerInfo());
  ui->labelModel->setText(modem->modelInfo());
  ui->labelFirmware->setText(modem->revisionInfo());
  ui->labelSerialNumber->setText(modem->serialNumberInfo());
}
