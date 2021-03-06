﻿/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2017 by Aleksei Ilin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "SerialPortSettingsView.h"
#include "ui_SerialPortSettingsView.h"

#include "common.h"

#include "Core.h"
#include "MainWindow.h"

#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>

#define DEFAULT_TO_CB_LINK_PROPERTY "defaultToCheckBoxLink"

SerialPortSettingsView::SerialPortSettingsView(const QUuid& connectionId, QWidget *parent) :
  QWidget(parent),
  IView(connectionId),
  ui(new Ui::SerialPortSettingsView)
{
  ui->setupUi(this);
  ui->closeButton->setEnabled(false);

  connect(ui->openButton, SIGNAL(clicked()), this, SLOT(openPortClicked()));
  connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(closePortClicked()));

  connect(ui->showAdvancedPortOptions, SIGNAL(toggled(bool)), ui->advancedPortOptions, SLOT(setVisible(bool)));
  ui->showAdvancedPortOptions->setChecked(false);
}

SerialPortSettingsView::~SerialPortSettingsView()
{
  delete ui;
}

void SerialPortSettingsView::changeEvent(QEvent *e)
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

void SerialPortSettingsView::init()
{
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

void SerialPortSettingsView::tini()
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

void SerialPortSettingsView::restore(Settings &set)
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
    ui->openPortAtStartup->setChecked(true);
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

void SerialPortSettingsView::store(Settings &set)
{
  set.setValue("SerialPort", ui->cbComPort->currentData().toString());
  set.setValue("SerialPort_openAtStartup", ui->openPortAtStartup->isChecked() ? 1 : 0);

  STORE_ADV_OPTION(baudRate,    BaudRate);
  STORE_ADV_OPTION(dataBits,    DataBits);
  STORE_ADV_OPTION(flowControl, FlowControl);
  STORE_ADV_OPTION(parity,      Parity);
  STORE_ADV_OPTION(stopBits,    StopBits);
}

QString SerialPortSettingsView::name() const
{
  return tr("Serial port settings");
}

void SerialPortSettingsView::processConnectionEvent(Core::ConnectionEvent event, const QVariant& data)
{
  if (event == Core::ConnectionEventStatus)
  {
    bool opened = data.toBool();

    if (opened)
    {
      ui->labelComPortStatus->setText(tr("Port opened. Modem status OK."));
    }
    else
    {
      ui->labelComPortStatus->setText(tr("Port closed."));
    }

    ui->openButton->setEnabled(!opened);
    ui->cbComPort->setEnabled(!opened);
    ui->advancedPortOptions->setEnabled(!opened);
    ui->closeButton->setEnabled(opened);
  }
}

void SerialPortSettingsView::openPortClicked()
{
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

  Core::instance()->openConnection(connectionId(), portName, options);
}

void SerialPortSettingsView::closePortClicked()
{
  Core::instance()->closeConnection(connectionId());
}

void SerialPortSettingsView::defaultCheckBoxStateChanged(int state)
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
