#include "ModemUssd.h"

#define CMD_USSD_SEND "AT+CUSD=%1,\"%2\",15"


bool UssdConversationHandler::processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished)
{
  const int& requestType = request->requestType;
  //const UssdArgs* ussdArgs = static_cast<UssdArgs*> (request->requestArgs);

  bool success = false;

  if (requestType == USSD_REQUEST_SEND)
  {
    bool decoded = true;
    QStringList headerLine = parseAnswerLine(c.at(1), "+CUSD:");
    if (headerLine.size() > 1)
    {
      SAFE_CONVERT(int, toInt, iStatus, headerLine.at(0), decoded=false;);
      if (decoded && checkUssdStatus(iStatus))
      {
        USSD_STATUS status = (USSD_STATUS)iStatus;
        QString answer = headerLine.at(1);
        emit updatedUssd(answer, status);
        success = true;
        requestFinished = true;
      }
      else
      {
        QString err = QString("Wrong USSD status received: %1").arg(headerLine.at(0));
        Q_LOGEX(LOG_VERBOSE_ERROR, err);
      }
    }
    else
    {
      Q_LOGEX(LOG_VERBOSE_ERROR, "Header size is not more then to 1!");
    }
  }

  return success;
}

QByteArray UssdConversationHandler::requestData(const ModemRequest *request) const
{
  const int &requestType = request->requestType;
  const UssdArgs *ussdArgs = static_cast<const UssdArgs*> (request->requestArgs);

  QByteArray data;

  if (requestType == USSD_REQUEST_SEND)
  {
    data = QString(CMD_USSD_SEND).arg(ussdArgs->status).arg(ussdArgs->ussd).toLatin1();
  }

  return data;
}

int UssdConversationHandler::requestTypesCount() const
{
  return USSD_REQUEST_LAST;
}

QString UssdConversationHandler::name() const
{
  return QString(USSD_HANDLER_NAME);
}

void UssdConversationHandler::sendUssd(const QString &ussd, USSD_SEND_STATUS status)
{
  UssdArgs * ussdArgs = static_cast<UssdArgs*> (requestArgs());
  ussdArgs->ussd = ussd;
  ussdArgs->status = status;

  modem()->appendRequest(createEmptyRequest(USSD_REQUEST_SEND, ussdArgs));
}

RequestArgs *UssdConversationHandler::requestArgs() const
{
  return new UssdArgs();
}

