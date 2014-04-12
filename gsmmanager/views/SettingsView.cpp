#include "SettingsView.h"
#include "ui_SettingsView.h"

#include "../common.h"

#include "../Core.h"
#include "../MainWindow.h"

#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>

#define DEFAULT_TO_CB_LINK_PROPERTY "defaultToCheckBoxLink"

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

  // baud rate
  ui->baudRate->addItem("1200",   QSerialPort::Baud1200);
  ui->baudRate->addItem("2400",   QSerialPort::Baud2400);
  ui->baudRate->addItem("4800",   QSerialPort::Baud4800);
  ui->baudRate->addItem("9600",   QSerialPort::Baud9600);
  ui->baudRate->addItem("19200",  QSerialPort::Baud19200);
  ui->baudRate->addItem("38400",  QSerialPort::Baud38400);
  ui->baudRate->addItem("57600",  QSerialPort::Baud57600);
  ui->baudRate->addItem("115200", QSerialPort::Baud115200);
  ui->baudRateDefault->setProperty(DEFAULT_TO_CB_LINK_PROPERTY, QVariant::fromValue<QComboBox*>(ui->baudRate));
  connect(ui->baudRateDefault, SIGNAL(stateChanged(int)), SLOT(defaultCheckBoxStateChanged(int)));

  // data bits
  ui->dataBits->addItem("5", QSerialPort::Data5);
  ui->dataBits->addItem("6", QSerialPort::Data6);
  ui->dataBits->addItem("7", QSerialPort::Data7);
  ui->dataBits->addItem("8", QSerialPort::Data8);
  ui->dataBitsDefault->setProperty(DEFAULT_TO_CB_LINK_PROPERTY, QVariant::fromValue<QComboBox*>(ui->dataBits));
  connect(ui->dataBitsDefault, SIGNAL(stateChanged(int)), SLOT(defaultCheckBoxStateChanged(int)));

  // flow control
  ui->flowControl->addItem(tr("No flow control"),             QSerialPort::NoFlowControl);
  ui->flowControl->addItem(tr("Hardware control (RTS/CTS)"),  QSerialPort::HardwareControl);
  ui->flowControl->addItem(tr("Software control (XON/XOFF)"), QSerialPort::SoftwareControl);
  ui->flowControlDefault->setProperty(DEFAULT_TO_CB_LINK_PROPERTY, QVariant::fromValue<QComboBox*>(ui->flowControl));
  connect(ui->flowControlDefault, SIGNAL(stateChanged(int)), SLOT(defaultCheckBoxStateChanged(int)));

  // parity
  ui->parity->addItem(tr("No parity"),    QSerialPort::NoParity);
  ui->parity->addItem(tr("Even parity"),  QSerialPort::EvenParity);
  ui->parity->addItem(tr("Odd parity"),   QSerialPort::OddParity);
  ui->parity->addItem(tr("Space parity"), QSerialPort::SpaceParity);
  ui->parity->addItem(tr("Mark parity"),  QSerialPort::MarkParity);
  ui->parityDefault->setProperty(DEFAULT_TO_CB_LINK_PROPERTY, QVariant::fromValue<QComboBox*>(ui->parity));
  connect(ui->parityDefault, SIGNAL(stateChanged(int)), SLOT(defaultCheckBoxStateChanged(int)));

  // stop bits
  ui->stopBits->addItem("1",   QSerialPort::OneStop);
#ifdef Q_OS_WIN
  ui->stopBits->addItem("1.5", QSerialPort::OneAndHalfStop);
#endif
  ui->stopBits->addItem("2",   QSerialPort::TwoStop);
  ui->stopBitsDefault->setProperty(DEFAULT_TO_CB_LINK_PROPERTY, QVariant::fromValue<QComboBox*>(ui->stopBits));
  connect(ui->stopBitsDefault, SIGNAL(stateChanged(int)), SLOT(defaultCheckBoxStateChanged(int)));
}

void SettingsView::tini()
{

}

#define RESTORE_ADV_OPTION(option, ClassName) \
  { \
    QVariant option ## Var = set.value("SerialPort_" # ClassName); \
    QVariant option ## Default ## Var = set.value("SerialPort_" # ClassName "_default"); \
    if (!option ## Var.isNull()) \
    { \
      bool converted = true; \
      SAFE_CONVERT(int, toInt, option, option ## Var, converted = false; \
        Q_LOGEX(LOG_VERBOSE_ERROR, "Option value type casting error: " # ClassName)); \
      SAFE_CONVERT(int, toInt, option ## Default, option ## Default ## Var, converted = false; \
        Q_LOGEX(LOG_VERBOSE_ERROR, "Option default value type casting error: " # ClassName)); \
      if (converted) \
      { \
        bool found = false; \
        for (int i=0; i< ui->option->count(); ++i) \
        { \
          if (ui->option->itemData(i).toInt() == option) \
          { \
            found = true; \
            ui->option->setCurrentIndex(i); \
            break; \
          } \
        } \
        if (!found) \
        { \
          Q_LOGEX(LOG_VERBOSE_ERROR, "Cannot find suitable item for option value: " # option); \
        } \
        ui->option ## Default->setCheckState(option ## Default || (!found) ? Qt::Checked : Qt::Unchecked); \
      } \
    } \
    else \
    { \
      ui->option ## Default->setCheckState(Qt::Checked); \
    } \
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

  SAFE_CONVERT(int, toInt, openAtStartup, set.value("SerialPort_openAtStartup"), openAtStartup = 0;);
  if (openAtStartup)
  {
    ui->openPortAtStartup->setChecked(Qt::Checked);
    QTimer::singleShot(1000, this, SLOT(openPortClicked()));
  }

  RESTORE_ADV_OPTION(baudRate,    BaudRate);
  RESTORE_ADV_OPTION(dataBits,    DataBits);
  RESTORE_ADV_OPTION(flowControl, FlowControl);
  RESTORE_ADV_OPTION(parity,      Parity);
  RESTORE_ADV_OPTION(stopBits,    StopBits);
}

#define STORE_ADV_OPTION(option, ClassName) \
  set.setValue("SerialPort_" # ClassName, ui->option->currentData().toInt()); \
  set.setValue("SerialPort_" # ClassName "_default", ui->option ## Default->isChecked() ? 1 : 0);

void SettingsView::store(Settings &set)
{
  set.setValue("SerialPort", ui->cbComPort->currentData().toString());
  set.setValue("SerialPort_openAtStartup", ui->openPortAtStartup->isChecked() ? 1 : 0);

  STORE_ADV_OPTION(baudRate,    BaudRate);
  STORE_ADV_OPTION(dataBits,    DataBits);
  STORE_ADV_OPTION(flowControl, FlowControl);
  STORE_ADV_OPTION(parity,      Parity);
  STORE_ADV_OPTION(stopBits,    StopBits);
}

QString SettingsView::name()
{
  return tr("Settings");
}

void SettingsView::openPortClicked()
{
  Modem * modem = Core::instance()->modem();
  QString portName = ui->cbComPort->currentData().toString();

  PortOptions options;
  options.baudRate           = (QSerialPort::BaudRate)ui->baudRate->currentData().toInt();
  options.baudRateDefault    = ui->baudRateDefault->isChecked();
  options.dataBits           = (QSerialPort::DataBits)ui->dataBits->currentData().toInt();
  options.dataBitsDefault    = ui->dataBitsDefault->isChecked();
  options.flowControl        = (QSerialPort::FlowControl)ui->flowControl->currentData().toInt();
  options.flowControlDefault = ui->flowControlDefault->isChecked();
  options.parity             = (QSerialPort::Parity)ui->parity->currentData().toInt();
  options.parityDefault      = ui->parityDefault->isChecked();
  options.stopBits           = (QSerialPort::StopBits)ui->stopBits->currentData().toInt();
  options.stopBitsDefault    = ui->stopBitsDefault->isChecked();

  modem->setPortOptions(options);
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
  ui->cbComPort->setEnabled(!opened);
  ui->advancedSettings->setEnabled(!opened);
  ui->closeButton->setEnabled(opened);

  if (opened)
  {
    Core::instance()->mainWindow()->updateActionTriggered();
  }
}

void SettingsView::defaultCheckBoxStateChanged(int state)
{
  QObject * s = sender();
  Q_ASSERT(s);

  QComboBox * cb = qobject_cast<QComboBox*> (s->property(DEFAULT_TO_CB_LINK_PROPERTY).value<QComboBox*>());
  Q_ASSERT(cb);

  if (state == Qt::Checked)
  {
    cb->setEnabled(false);
  }
  else
  {
    cb->setEnabled(true);
  }
}
