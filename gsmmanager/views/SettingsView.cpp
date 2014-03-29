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

  connect(ui->cbComPort, SIGNAL(currentIndexChanged(int)), this, SLOT(serialPortChanged(int)));
}

void SettingsView::init()
{
  Modem * modem = Core::instance()->modem();
  connect(modem, SIGNAL(modemNotification(MODEM_NOTIFICATION_TYPE)),
          this, SLOT(updatePortStatus()));
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
  set.setValue("SerialPort", m_serialPortName);
}

QString SettingsView::name()
{
  return tr("Settings");
}

void SettingsView::serialPortChanged(int newIndex)
{
  m_serialPortName = ui->cbComPort->itemData(newIndex).toString();
  Core::instance()->modem()->setSerialPortName(m_serialPortName);
}

void SettingsView::updatePortStatus()
{
  if (Core::instance()->modem()->portTested())
  {
    ui->labelComPortStatus->setText(tr("Modem status OK."));
  }
  else
  {
    ui->labelComPortStatus->setText(tr("Error accessing serial port!"));
  }
}
