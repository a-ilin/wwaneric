#include "Modem.h"

#include "common.h"

#include <QtSerialPort/QSerialPort>


#define RS232_IO_DELAY_MS 250
#define RS232_IO_RETRIES 1



Modem::Modem() :
  QObject(),
  m_batchQueryStarted(false),
  m_portTested(false)
{
  m_serialPort = new QSerialPort();
  unsetLogFunction();
}

Modem::~Modem()
{
  endBatchQuery();
  delete m_serialPort;
}

void Modem::setLogFunction(void(*logFunction)(const QString &))
{
  this->logFunction = logFunction;
}

void Modem::unsetLogFunction()
{
  this->logFunction = &dummyLogFunction;
}

void Modem::dummyLogFunction(const QString &text)
{
  Q_LOGEX(LOG_VERBOSE_RAW, text);
}

void Modem::setSerialPortName(const QString &serialPortName)
{
  m_serialPort->setPortName(serialPortName);

  if(testModem())
  {
    emit modemNotification(MODEM_NOTIFICATION_PORT);
  }
}

bool Modem::openSerialPort()
{
  if (m_batchQueryStarted)
  {
    return true;
  }

  if (m_serialPort->open(QIODevice::ReadWrite))
  {
    m_serialPort->setBaudRate(QSerialPort::Baud115200);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);

    bool result = true;
    sendToSerialPort(QString("\r\n"));
    QString answer = readLineFromSerialPort();

    // if wireless device is ready
    if (answer.contains(QString("*EMRDY: 1")))
    {
      // switch off echo
      sendToSerialPort(QString("ATE0\r\n"));

      // clear output buffer
      readAllRawFromSerialPort();

      result = true;
    }
    else
    {
      m_serialPort->close();
      result = false;
    }

    return result;
  }

  return false;
}

void Modem::closeSerialPort()
{
  if (m_batchQueryStarted)
  {
    return;
  }

  m_serialPort->close();
}

void Modem::sendToSerialPort(const QString str)
{
  m_serialPort->write(str.toLatin1().constData());
  while(m_serialPort->waitForBytesWritten(RS232_IO_DELAY_MS));
  logFunction(QString("Written:\n"+str));
}

QString Modem::readLineFromSerialPort()
{
  int retries = RS232_IO_RETRIES;
  QString result("");

  while( retries > 0 )
  {
    if(!m_serialPort->waitForReadyRead(RS232_IO_DELAY_MS))
    {
      retries--;
      continue;
    }

    result = m_serialPort->readAll();
    if (result.isEmpty())
    {
      continue;
    }

    break;
  }

  result = result.remove(QChar('\r'));
  result = result.remove(QChar('\n'));

  logFunction(QString("Read:\n"+result));
  return result;
}

QStringList Modem::readAllFromSerialPort()
{
  QStringList result;
  QString currentLine = readLineFromSerialPort();

  while(!currentLine.isEmpty())
  {
    result.append(currentLine);
    currentLine = readLineFromSerialPort();
  }

  return result;
}

QString Modem::readAllRawFromSerialPort()
{
  while(m_serialPort->waitForReadyRead(RS232_IO_DELAY_MS));

  QString str(m_serialPort->readAll());
  logFunction(QString("Read:\n"+str));
  return str;
}

inline bool Modem::decodeCommandExecStatus(QString statusString)
{
  static const QString ok = "OK";
  if (statusString != ok)
  {
    return false;
  }

  return true;
}

bool Modem::testModem()
{
  if (m_batchQueryStarted)
  {
    return true;
  }

  if (openSerialPort())
  {
    m_portTested = true;
    m_serialPort->close();
    return true;
  }

  m_portTested = false;
  return false;
}

bool Modem::storageCapacityUsed(int *simUsed, int *simTotal, int *phoneUsed, int *phoneTotal)
{
  if (openSerialPort())
  {
    sendToSerialPort(QString("AT+CPMS?\r\n"));
    QString answer = readLineFromSerialPort();

    if (!decodeCommandExecStatus(readLineFromSerialPort()))
    {
      closeSerialPort();
      return false;
    }

    QStringList answerParts = parseModemAnswer(answer, QString(","), QString("+CPMS:"));

    bool success = true;

    if (answerParts.at(0) == QString("\"ME\""))
    {
      *phoneUsed = answerParts.at(1).toInt();
      *phoneTotal = answerParts.at(2).toInt();
    }
    else if (answerParts.at(0) == QString("\"SM\""))
    {
      *simUsed = answerParts.at(1).toInt();
      *simTotal = answerParts.at(2).toInt();
    }
    else
    {
      logFunction("Cannot parse answer part 1 of command \"AT+CPMS?\"!");
      success = false;
    }

    if (answerParts.at(3) == QString("\"ME\""))
    {
      *phoneUsed = answerParts.at(4).toInt();
      *phoneTotal = answerParts.at(5).toInt();
    }
    else if (answerParts.at(3) == QString("\"SM\""))
    {
      *simUsed = answerParts.at(4).toInt();
      *simTotal = answerParts.at(5).toInt();
    }
    else
    {
      logFunction("Cannot parse answer part 2 of command \"AT+CPMS?\"!");
      success = false;
    }

    if (answerParts.at(6) == QString("\"ME\""))
    {
      *phoneUsed = answerParts.at(7).toInt();
      *phoneTotal = answerParts.at(8).toInt();
    }
    else if (answerParts.at(6) == QString("\"SM\""))
    {
      *simUsed = answerParts.at(7).toInt();
      *simTotal = answerParts.at(8).toInt();
    }
    else
    {
      logFunction("Cannot parse answer part 3 of command \"AT+CPMS?\"!");
      success = false;
    }

    closeSerialPort();
    return success;

  }

  return false;
}


QStringList Modem::parseModemAnswer(const QString &inputText,
                                    const QString &splitter,
                                    const QString &command)
{
  QString text = inputText;

  if (!command.isEmpty())
  {
    QStringList splittedInput = text.split(QString("\r\n"), QString::SkipEmptyParts);

    bool foundLine = false;

    foreach(QString str, splittedInput)
    {
      if (str.startsWith(command))
      {
        text = str;
        foundLine = true;
        break;
      }
    }

    if (!foundLine)
    {
      return QStringList();
    }

    text = text.mid(command.length());
    while(text.length() && (text.at(0) == QChar(' ')))
    {
      text = text.mid(1);
    }
  }

  QStringList result = text.split(splitter, QString::KeepEmptyParts);

  return result;
}

bool Modem::selectPreferredSmsStorage(SMS_STORAGE storage)
{
  if (m_serialPort->isOpen())
  {
    // change current storage
    QString selectedStorage;
    if (storage == SMS_STORAGE_PHONE)
    {
      selectedStorage = "\"ME\"";
    }
    else
    {
      selectedStorage = "\"SM\"";
    }
    sendToSerialPort(QString("AT+CPMS=%1\r\n").arg(selectedStorage));
    readLineFromSerialPort(); // status of storage capacity
    if (!decodeCommandExecStatus(readLineFromSerialPort()))
    {
      return false;
    }

    return true;
  }

  return false;
}

QList<Sms> Modem::readSms(SMS_STORAGE storage, SMS_STATUS status)
{
  QList<Sms> result;

  if (openSerialPort())
  {
    // change current storage
    if (!selectPreferredSmsStorage(storage))
    {
      closeSerialPort();
      return result;
    }

    // read messages
    sendToSerialPort(QString("AT+CMGL=%1\r\n").arg(QString::number(status)));

    QStringList answer = readAllFromSerialPort();

    // Every message takes two string.
    // Also the status string placed at the end.
    int messagesCount = (answer.length() - 1) / 2;

    if (!decodeCommandExecStatus(answer.at(answer.length() - 1)))
    {
      closeSerialPort();
      return result;
    }

    for (int i=0; i< messagesCount; i++)
    {
      // decode header
      QStringList answerParts = parseModemAnswer(answer.at(i*2), QString(","), QString("+CMGL:"));

      QByteArray pduData = answer.at((i*2)+1).toLatin1();

      Sms newSms(storage,
                 (SMS_STATUS)answerParts.at(1).toInt(),
                 answerParts.at(0).toInt(),
                 pduData);

      if (!newSms.isValid())
      {
        logFunction(tr("PDU parsing error: %1\n").arg(newSms.parseError()));
      }

      //place into result
      result.append(newSms);
    }
  }

  closeSerialPort();

  return result;
}

bool Modem::deleteSms(SMS_STORAGE storage, int index)
{
  bool result = false;

  if (openSerialPort())
  {
    // change current storage
    if (selectPreferredSmsStorage(storage))
    {
      sendToSerialPort(QString("AT+CMGD=%1,0\r\n").arg(index));
      //readLineFromSerialPort();

      if (decodeCommandExecStatus(readLineFromSerialPort()))
      {
        result = true;
      }
    }

    closeSerialPort();
  }

  return result;
}

bool Modem::startBatchQuery()
{
  if (openSerialPort())
  {
    m_batchQueryStarted = true;
    return true;
  }

  return false;
}

void Modem::endBatchQuery()
{
  m_batchQueryStarted = false;
  closeSerialPort();
}

QString Modem::manufacturerInfo()
{
  QString result;

  if (openSerialPort())
  {
    sendToSerialPort(QString("AT+CGMI\r\n"));
    result = readLineFromSerialPort();

    if (!decodeCommandExecStatus(readLineFromSerialPort()))
    {
      closeSerialPort();
      return QString();
    }
  }

  closeSerialPort();
  return result;
}

QString Modem::modelInfo()
{
  QString result;

  if (openSerialPort())
  {
    sendToSerialPort(QString("AT+CGMM\r\n"));
    result = readLineFromSerialPort();

    if (!decodeCommandExecStatus(readLineFromSerialPort()))
    {
      closeSerialPort();
      return QString();
    }
  }

  closeSerialPort();
  return result;
}

QString Modem::serialNumberInfo()
{
  QString result;

  if (openSerialPort())
  {
    sendToSerialPort(QString("AT+CGSN\r\n"));
    result = readLineFromSerialPort();

    if (!decodeCommandExecStatus(readLineFromSerialPort()))
    {
      closeSerialPort();
      return QString();
    }
  }

  closeSerialPort();
  return result;
}

QString Modem::revisionInfo()
{
  QString result;

  if (openSerialPort())
  {
    sendToSerialPort(QString("AT+CGMR\r\n"));
    result = readLineFromSerialPort();

    if (!decodeCommandExecStatus(readLineFromSerialPort()))
    {
      closeSerialPort();
      return QString();
    }
  }

  closeSerialPort();
  return result;
}
