#include "ModemInit.h"


#define CMD_CHARACTER_SET      "AT+CSCS=\"UTF-8\""

#define LAST_INIT_STAGE 0

bool InitConversationHandler::processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished)
{
  const int &requestType = request->requestType;
  //const RequestArgs &requestArgs = request->requestArgs;
  int &requestStage = request->requestStage;

  bool success = false;

  if (c.status == Conversation::OK)
  {
    if (requestType == INIT_REQUEST_INIT)
    {
      // set preferred storage
      if (requestStage < LAST_INIT_STAGE)
      {
        ++requestStage;
        success = true;
        requestFinished = false;
      }
      else
      {
        success = true;
        requestFinished = true;
        emit modemInited();
      }
    }
  }

  return success;
}

QByteArray InitConversationHandler::requestData(const ModemRequest *request) const
{
  const int &requestType = request->requestType;
  //const RequestArgs &requestArgs = request->requestArgs;
  const int &requestStage = request->requestStage;

  QByteArray data;

  if (requestType == INIT_REQUEST_INIT)
  {
    if (requestStage == 0)
    {
      data = CMD_CHARACTER_SET;
    }
  }

  return data;
}

int InitConversationHandler::requestTypesCount() const
{
  return INIT_REQUEST_LAST;
}

QString InitConversationHandler::name() const
{
  return QString(INIT_HANDLER_NAME);
}

void InitConversationHandler::initModem()
{
  modem()->appendRequest(createEmptyRequest(INIT_REQUEST_INIT));
}
