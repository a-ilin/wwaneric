#include "ModemUssd.h"




bool UssdConversationHandler::processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished)
{
  return true;
}

QByteArray UssdConversationHandler::requestData(const ModemRequest *request) const
{
  return QByteArray();
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

}

