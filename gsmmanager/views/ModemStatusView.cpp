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

void ModemStatusView::init()
{
  Modem * modem = Core::instance()->modem();

  connect(modem, SIGNAL(updatedManufacturerInfo(QString)), SLOT(updateManufacturerInfo(QString)));
  connect(modem, SIGNAL(updatedModelInfo(QString)), SLOT(updateModelInfo(QString)));
  connect(modem, SIGNAL(updatedRevisionInfo(QString)), SLOT(updateRevisionInfo(QString)));
  connect(modem, SIGNAL(updatedSerialNumberInfo(QString)), SLOT(updateSerialNumberInfo(QString)));
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

bool ModemStatusView::event(QEvent *e)
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

void ModemStatusView::updateStatus()
{
  Modem * modem = Core::instance()->modem();
  modem->updateManufacturerInfo();
  modem->updateModelInfo();
  modem->updateRevisionInfo();
  modem->updateSerialNumberInfo();
}

void ModemStatusView::updateManufacturerInfo(const QString &info)
{
  ui->labelManufacturer->setText(info);
}

void ModemStatusView::updateRevisionInfo(const QString &info)
{
  ui->labelFirmware->setText(info);
}

void ModemStatusView::updateModelInfo(const QString &info)
{
  ui->labelModel->setText(info);
}

void ModemStatusView::updateSerialNumberInfo(const QString &info)
{
  ui->labelSerialNumber->setText(info);
}
