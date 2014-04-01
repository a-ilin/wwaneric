#include "SettingsView.h"
#include "ui_SettingsView.h"

#include "../Core.h"

#include <QtSerialPort/QSerialPortInfo>

SettingsView::SettingsView(QWidget *parent) :
  QWidget(parent),
  IView(),
  ui(new Ui::SettingsView)
{
  ui->setupUi(this);
  ui->closeButton->setEnabled(false);

  connect(ui->openButton, SIGNAL(clicked()), this, SLOT(openPortClicked()));
  connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(closePortClicked()));
}

SettingsView::~SettingsView()
{
  delete ui;
}

void SettingsView::changeEvent(QEvent *e)
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

void SettingsView::init()
{
  Modem * modem = Core::instance()->modem();
  connect(modem, SIGNAL(updatedPortStatus(bool)),
          this, SLOT(updatePortStatus(bool)));
}

void SettingsView::tini()
{

}

void SettingsView::restore(Settings &set)
{
  QString serialPortName = set.value("SerialPort").toString();
  int currentIndex = 0;

  // enumerate serial ports
  foreach(const QSerialPortInfo & info, QSerialPortInfo::availablePorts())
  {
    if (info.portName() == serialPortName)
    {
      currentIndex = ui->cbComPort->count();
    }

    ui->cbComPort->addItem(QString("%1 : %2").arg(info.portName()).arg(info.description()), info.portName());
  }

  if (ui->cbComPort->count() > 0)
  {
    ui->cbComPort->setCurrentIndex(currentIndex);
  }
}

void SettingsView::store(Settings &set)
{
  Modem * modem = Core::instance()->modem();
  set.setValue("SerialPort", modem->portName());
}

QString SettingsView::name()
{
  return tr("Settings");
}

void SettingsView::openPortClicked()
{
  Modem * modem = Core::instance()->modem();
  QString portName = ui->cbComPort->currentData().toString();

  modem->setPortName(portName);
  modem->openPort();
}

void SettingsView::closePortClicked()
{
  Core::instance()->modem()->closePort();
}

void SettingsView::updatePortStatus(bool opened)
{
  Modem * modem = Core::instance()->modem();

  if (opened)
  {
    ui->labelComPortStatus->setText(tr("Port %1 opened. Modem status OK.").arg(modem->portName()));
  }
  else
  {
    ui->labelComPortStatus->setText(tr("Port %1 closed.").arg(modem->portName()));
  }

  ui->openButton->setEnabled(!opened);
  ui->closeButton->setEnabled(opened);
}
