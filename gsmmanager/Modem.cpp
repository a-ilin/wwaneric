#include "Modem.h"



QEvent::Type ModemEventType = (QEvent::Type)QEvent::registerEventType();





Modem::Modem() :
  PortController()
{

}

Modem::~Modem()
{

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



bool Modem::processConversation(const Conversation &c)
{
  if (c.size() >= 2)
  {
    QByteArray request(c.first());

    // echo reply has \r at the end
    if (request.at(request.size() - 1) == '\r')
    {
      request.remove(request.size() - 2, 1);
    }

    /* QByteArray status(c.last()); */

    if (request == "AT+CGMI")
    {
      emit updatedManufacturerInfo(c.at(1));
      return true;
    }
    else if (request == "AT+CGMM")
    {
      emit updatedModelInfo(c.at(1));
      return true;
    }
    else if (request == "AT+CGSN")
    {
      emit updatedSerialNumberInfo(c.at(1));
      return true;
    }
    else if (request == "AT+CGMR")
    {
      emit updatedRevisionInfo(c.at(1));
      return true;
    }
    else
    {
      QString str = QString("Unknown request received: %1").arg(QString(request));
      Q_LOGEX(LOG_VERBOSE_WARNING, str);
    }
  }

  return false;
}



/*
 * Some static information
 *
*/
void Modem::updateManufacturerInfo()
{
  sendRequest("AT+CGMI");
}

void Modem::updateModelInfo()
{
  sendRequest("AT+CGMM");
}

void Modem::updateSerialNumberInfo()
{
  sendRequest("AT+CGSN");
}

void Modem::updateRevisionInfo()
{
  sendRequest("AT+CGMR");
}

/*
 * SMS
 *
*/

void Modem::updateSms(SMS_STORAGE storage, SMS_STATUS status)
{

}

void Modem::updateSmsCapacity()
{

}

void Modem::sendSms(const Sms &sms)
{

}

void Modem::deleteSms(SMS_STORAGE storage, int index)
{

}

/*
 * USSD
 *
*/

void Modem::sendUssd(const QString &ussd, USSD_SEND_STATUS status)
{

}




