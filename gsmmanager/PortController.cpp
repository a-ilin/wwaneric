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

#include "PortController.h"

#include "common.h"

#include <QTimer>

const QString statusOk("OK");
const QString statusError("ERROR");

QList<Conversation> parse(const QByteArray &answer, int &unparsedRestSize)
{
  // split by \r\n
  QList<QByteArray> splitted = splitByteArray(answer, phraseSep, KeepSeparators);

  QList<Conversation> conversations;

  unparsedRestSize = 0;

  // split by OK or ERROR
  if(splitted.size() > 0)
  {
    QList<QByteArray> data;

    foreach(const QByteArray & array, splitted)
    {
      unparsedRestSize += array.size();

      const QString arrayStr(array);

      if ( (arrayStr == statusOk) || (arrayStr == statusError) )
      {
        data.append(array);

        // detect echo reply (enabled by default or ATE1)
        {
          QByteArray & first = data.first();

          int posCr = first.indexOf('\r');
          if (posCr != -1)
          {
            QByteArray echoReply(first.constData(), posCr);

            if (first.size() > posCr + 1)
            {
              first.remove(0, posCr + 1);
            }
            else // first.size() == posCr + 1
            {
              data.takeFirst();
            }

            data.prepend(echoReply);
          }
        }

        Conversation conv;
        if (data.size() >= 2)
        {
          conv.statusData = data.takeLast();
          conv.status = QString(conv.statusData) == statusOk ? Conversation::OK : Conversation::ERROR;
          conv.request = data.takeFirst();
        }
        else
        {
          conv.status = Conversation::UNKNOWN;
        }

        conv.data = data;

        conversations.append(conv);
        data.clear();
        unparsedRestSize = 0;
      }
      else if (array != phraseSep)
      {
        data.append(array);
      }
    }

    // eat last separator
    if ((!data.size()) && (unparsedRestSize == phraseSep.size()))
    {
      unparsedRestSize = 0;
    }
  }

  return conversations;
}


PortController::PortController() :
  QObject(),
  m_portControllerStatus(PORT_CONTROLLER_STATUS_READY),
  m_modemDetected(false)
{
  m_timerTimeout = new QTimer(this);
  m_timerTimeout->setSingleShot(true);
  m_timerTimeout->setInterval(10000);  // device timeout
  connect(m_timerTimeout, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));

  m_serialPort = new QSerialPort(this);
  connect(m_serialPort, SIGNAL(readyRead()), SLOT(onReadyRead()));
  connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)),
          SLOT(onError(QSerialPort::SerialPortError)));

  connect(m_serialPort, SIGNAL(readChannelFinished()), SLOT(onReadChannelFinished()));
}


void PortController::setPortName(const QString portName)
{
  m_serialPort->setPortName(portName);
}

void PortController::setPortOptions(const PortOptions options)
{
  m_options = options;
}

#define SET_OPTION(option, method) \
  if (!m_serialPort->method(m_options.option)) \
  { \
    Q_LOGEX(LOG_VERBOSE_ERROR, "Error on setting serial port option: " # option ); \
    onError(m_serialPort->error()); \
    closePort(); \
  }

void PortController::openPort()
{
  // if port opened but modem not found
  if ((!m_modemDetected) && m_serialPort->isOpen())
  {
    m_serialPort->close();
  }

  if (!m_serialPort->isOpen())
  {
    // clear buffers, etc...
    closePort();

    m_serialPort->open(QIODevice::ReadWrite);

    QSerialPort::SerialPortError errorCode = m_serialPort->error();

    if (errorCode != QSerialPort::NoError)
    {
      emit updatedPortError(errorCode);
      emit updatedPortError(m_serialPort->errorString());
      m_serialPort->clearError();
    }

    if (m_serialPort->isOpen())
    {
      SET_OPTION(baudRate,    setBaudRate);
      SET_OPTION(dataBits,    setDataBits);
      SET_OPTION(flowControl, setFlowControl);
      SET_OPTION(parity,      setParity);
      SET_OPTION(stopBits,    setStopBits);

      sendRequest("AT");
    }
    else
    {
      closePort();
    }
  }
}

void PortController::closePort()
{
  notifyError(m_serialPort->error());

  if (m_serialPort->isOpen())
  {
    m_serialPort->clear();
    m_serialPort->close();
  }

  m_timerTimeout->stop();

  m_bufferReceived.clear();

  m_portControllerStatus = PORT_CONTROLLER_STATUS_READY;
  m_modemDetected = false;

  modemDetected(false);
}

void PortController::modemDetected(bool status)
{
  emit updatedPortStatus(status);
}

void PortController::onReadyRead()
{
  m_timerTimeout->stop();

  QByteArray data = m_serialPort->readAll();
  m_bufferReceived.append(data);

  // debug logging
  {
    QString debugString(QString("Read:" ENDL "HEX: ") + hexString(data) + QString(ENDL) + QString(data));
    Q_LOGEX(LOG_VERBOSE_DEBUG, debugString);
  }

  if ( ! parseBuffer())
  {
    m_timerTimeout->start();
  }
  else
  {
    if (m_portControllerStatus == PORT_CONTROLLER_STATUS_READY)
    {
      if (m_bufferReceived.size() > 0)
      {
        QString str = QString("Answer processed but unneeded data found. Clearing buffer." ENDL " HEX: ") +
                      hexString(m_bufferReceived) + QString(ENDL) + QString(m_bufferReceived);
        Q_LOGEX(LOG_VERBOSE_WARNING, str);

        m_bufferReceived.clear();
      }

      sendRequest();
    }
  }
}

void PortController::onError(QSerialPort::SerialPortError errorCode)
{
  if (errorCode != QSerialPort::NoError)
  {
    notifyError(errorCode);

    if (!m_serialPort->isOpen())
    {
      closePort();
    }
  }
}

void PortController::notifyError(QSerialPort::SerialPortError errorCode)
{
  if (errorCode != QSerialPort::NoError)
  {
    QString errMsg(QString("Serial port error: %1. Description: %2")
                   .arg(errorCode)
                   .arg(m_serialPort->errorString()));

    Q_LOGEX(LOG_VERBOSE_ERROR, errMsg);

    emit updatedPortError(errorCode);
    emit updatedPortError(m_serialPort->errorString());
    m_serialPort->clearError();
  }
}

void PortController::onReadChannelFinished()
{
  Q_LOGEX(LOG_VERBOSE_DEBUG, "Port closed by device.");

  closePort();
}

void PortController::onTimerTimeout()
{
  Q_LOGEX(LOG_VERBOSE_ERROR, "Timeout. Clearing received buffer.");

  m_bufferReceived.clear();
  m_portControllerStatus = PORT_CONTROLLER_STATUS_READY;

  if ((!m_serialPort->isOpen()) || (!m_modemDetected))
  {
    // clearing buffer, etc...
    closePort();
  }
  else
  {
    sendRequest();
  }
}

void PortController::sendToPort(const QByteArray &data)
{
  // debug logging
  {
    QString debugString(QString("Write:" ENDL "HEX: ") + hexString(data) + QString(ENDL) + QString(data));
    Q_LOGEX(LOG_VERBOSE_DEBUG, debugString);
  }

  qint64 bytesWritten = m_serialPort->write(data);

  if (bytesWritten != data.size())
  {
    Q_LOGEX(LOG_VERBOSE_ERROR, QString("Written %1 bytes of %2.").arg(bytesWritten).arg(data.size()));
  }

  Q_ASSERT(!m_timerTimeout->isActive());
  m_timerTimeout->start();
}

void PortController::sendRequest(const QByteArray &request)
{
  if ((m_portControllerStatus == PORT_CONTROLLER_STATUS_READY) && m_serialPort->isOpen())
  {
    QByteArray data = request;
    if (data.size() > 0)
    {
      data.append("\r\n");
      sendToPort(data);
      m_portControllerStatus = PORT_CONTROLLER_STATUS_PROCESS_REQUEST;
    }
  }
}

void PortController::sendRequest()
{
  if (m_portControllerStatus == PORT_CONTROLLER_STATUS_READY)
  {
    sendRequest(requestData());
  }
}

bool PortController::parseBuffer()
{
  if ((m_portControllerStatus == PORT_CONTROLLER_STATUS_READY) ||
      (m_portControllerStatus == PORT_CONTROLLER_STATUS_PROCESS_UNEXPECTED_DATA))
  {
    if (m_modemDetected)
    {
      if (processUnexpectedData(m_bufferReceived))
      {
        m_bufferReceived.clear();
      }
      else
      {
        m_portControllerStatus = PORT_CONTROLLER_STATUS_PROCESS_UNEXPECTED_DATA;
      }
    }
    else
    {
      Q_LOGEX(LOG_VERBOSE_CRITICAL, QString("Received unexpected data %1 but modem was not detected! Clearing buffer.")
              .arg(QString(m_bufferReceived)));
      m_bufferReceived.clear();
    }
  }
  else // PORT_CONTROLLER_STATUS_PROCESS_REQUEST
  {
    int restSize = 0;
    const QList<Conversation> conversations = parse(m_bufferReceived, restSize);
    if (restSize)
    {
      m_bufferReceived.remove(0, m_bufferReceived.size() - restSize);
    }
    else
    {
      m_bufferReceived.clear();
    }

    foreach(const Conversation &c, conversations)
    {
      if (c.status != Conversation::OK)
      {
        if (c.status == Conversation::ERROR)
        {
          QString str = QString("Received ERROR status for request %1")
                        .arg(QString(c.request));
          Q_LOGEX(LOG_VERBOSE_ERROR, str);
        }
        else
        {
          Q_ASSERT(c.status == Conversation::UNKNOWN);
          QString str = QString("Received unknown status %1 for request %2")
                        .arg(QString(c.statusData)).arg(QString(c.request));
          Q_LOGEX(LOG_VERBOSE_ERROR, str);
        }
      }

      if (m_modemDetected)
      {
        if (processConversation(c))
        {
          m_portControllerStatus = PORT_CONTROLLER_STATUS_READY;
        }
      }
      else
      {
        if ((c.request == "*EMRDY: 1") || (c.request == "AT"))
        {
          if (c.status == Conversation::OK)
          {
            m_portControllerStatus = PORT_CONTROLLER_STATUS_READY;

            m_modemDetected = true;
            modemDetected(true);
          }
        }
        else
        {
          QString err = QString("Received request %1 but modem was not detected!")
                        .arg(QString(c.request));
          Q_LOGEX(LOG_VERBOSE_ERROR, err);
        }
      }
    }
  }

  return m_modemDetected && (m_portControllerStatus == PORT_CONTROLLER_STATUS_READY);
}
