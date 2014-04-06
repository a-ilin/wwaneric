#include "PortController.h"

#include "common.h"

#include <QTimer>

QList<Conversation> parse(const QByteArray &answer, int &unparsedRestSize)
{
  // split by \r\n
  const QByteArray phraseSep("\r\n");
  QList<QByteArray> splitted = splitByteArray(answer, phraseSep, KeepSeparators);

  QList<Conversation> conversations;

  unparsedRestSize = 0;

  // split by OK or ERROR
  if(splitted.size() > 0)
  {
    Conversation conversation;

    foreach(const QByteArray & array, splitted)
    {
      unparsedRestSize += array.size();

      const QString arrayStr(array);

      if ( (arrayStr == QString("OK")) || (arrayStr == QString("ERROR")) )
      {
        conversation.append(array);

        // detect echo reply (enabled by default or ATE1)
        {
          QByteArray & first = conversation.first();

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
              conversation.takeFirst();
            }

            conversation.prepend(echoReply);
          }
        }

        conversations.append(conversation);
        conversation.clear();
        unparsedRestSize = 0;
      }
      else if (array != phraseSep)
      {
        conversation.append(array);
      }
    }

    // eat last separator
    if ((!conversation.size()) && (unparsedRestSize == phraseSep.size()))
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
  m_timerTimeout->setInterval(1500);  // device timeout
  connect(m_timerTimeout, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));

  m_serialPort = new QSerialPort(this);
  connect(m_serialPort, SIGNAL(readyRead()), SLOT(onReadyRead()));
  connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)),
          SLOT(onError(QSerialPort::SerialPortError)));

  connect(m_serialPort, SIGNAL(readChannelFinished()), SLOT(onReadChannelFinished()));
}


void PortController::setPortName(const QString &portName)
{
  m_serialPort->setPortName(portName);
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
    m_serialPort->close();
    m_serialPort->clear();
  }

  m_timerTimeout->stop();

  m_bufferReceived.clear();

  m_portControllerStatus = PORT_CONTROLLER_STATUS_READY;
  m_modemDetected = false;

  emit updatedPortStatus(false);
}

void PortController::onReadyRead()
{
  m_timerTimeout->stop();

  QByteArray data = m_serialPort->readAll();
  m_bufferReceived.append(data);

  // debug logging
  {
    QString hexString;
    for(int i=0; i< data.size(); ++i)
    {
      hexString += QString::number(data.constData()[i], 16) + QChar(' ');
    }

    QString debugString(QString("Read:\nHEX: ") + hexString + QChar('\n') + QString(data));

    Q_LOGEX(LOG_VERBOSE_DEBUG, debugString);
  }

  if (!parseBuffer())
  {
    m_timerTimeout->start();
  }
  else
  {
    if (m_portControllerStatus == PORT_CONTROLLER_STATUS_READY)
    {
      if (m_bufferReceived.size() > 0)
      {
        QString str = QString("Answer processed but unneeded data found. Clearing buffer. Data: ")
                      .arg(QString(m_bufferReceived));
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
  Q_LOGEX(LOG_VERBOSE_ERROR, "Request timeout. Clearing received buffer.");

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
    QString hexString;
    for(int i=0; i< data.size(); ++i)
    {
      hexString += QString::number(data.constData()[i], 16) + QChar(' ');
    }

    QString debugString(QString("Write:\nHEX: ") + hexString + QChar('\n') + QString(data));

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
  if (m_serialPort->isOpen())
  {
    QByteArray data = request;
    if ((m_portControllerStatus == PORT_CONTROLLER_STATUS_READY) &&
        (data.size() > 0))
    {
      data.append("\r\n");
      sendToPort(data);
      m_portControllerStatus = PORT_CONTROLLER_STATUS_BUSY;
    }
  }
}

void PortController::sendRequest()
{
  sendRequest(requestData());
}

bool PortController::parseBuffer()
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

  if (m_portControllerStatus == PORT_CONTROLLER_STATUS_READY)
  {
    QString str = QString("Modem status is READY when new data arrived. Clearing data buffer.");
    Q_LOGEX(LOG_VERBOSE_WARNING, str);
    m_bufferReceived.clear();
  }
  else
  {
    foreach(const Conversation &c, conversations)
    {
      // the first is the echo reply
      // the last is the status OK/ERROR
      if (c.size() >= 2)
      {
        QByteArray request(c.first());
        QByteArray status(c.last());

        if (status != "OK")
        {
          if (status == "ERROR")
          {
            QString str = QString("Received ERROR status for request %1")
                          .arg(QString(request));
            Q_LOGEX(LOG_VERBOSE_ERROR, str);
          }
          else
          {
            QString str = QString("Received unknown status %1 for request %2")
                          .arg(QString(status)).arg(QString(request));
            Q_LOGEX(LOG_VERBOSE_ERROR, str);
          }

          continue;
        }

        if ((request == "*EMRDY: 1") || (request == "AT"))
        {
          if (m_modemDetected)
          {
            Q_LOGEX(LOG_VERBOSE_WARNING, "Received READY while modem was already detected.");
          }

          m_portControllerStatus = PORT_CONTROLLER_STATUS_READY;

          m_modemDetected = true;
          emit updatedPortStatus(true);
        }
        else if (m_modemDetected)
        {
          if (processConversation(c))
          {
            m_portControllerStatus = PORT_CONTROLLER_STATUS_READY;
          }
        }
        else
        {
          QString err = QString("Received request %1 but modem was not detected!")
                        .arg(QString(request));
          Q_LOGEX(LOG_VERBOSE_ERROR, err);
        }
      }
      else
      {
        QString str = QString("Received request size is too small: %1").arg(c.size());
        Q_LOGEX(LOG_VERBOSE_WARNING, str);
      }
    }
  }

  return m_modemDetected && (m_portControllerStatus == PORT_CONTROLLER_STATUS_READY);
}
