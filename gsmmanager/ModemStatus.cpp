#include "ModemStatus.h"

#define CMD_MANUFACTURER_INFO  "AT+CGMI"
#define CMD_MODEL_INFO         "AT+CGMM"
#define CMD_SERIAL_NUMBER      "AT+CGSN"
#define CMD_REVISION_INFO      "AT+CGMR"


bool StatusConversationHandler::processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished)
{
  const int &requestType = request->requestType;
  //const RequestArgs &requestArgs = request.requestArgs;
  //int &requestStage = request.requestStage;

  bool success = false;

  if (requestType == STATUS_REQUEST_MANUFACTURER_INFO)
  {
    emit updatedManufacturerInfo(c.at(1));
    success = true;
    requestFinished = true;
  }
  else if (requestType == STATUS_REQUEST_MODEL_INFO)
  {
    emit updatedModelInfo(c.at(1));
    success = true;
    requestFinished = true;
  }
  else if (requestType == STATUS_REQUEST_SERIAL_NUMBER)
  {
    emit updatedSerialNumberInfo(c.at(1));
    success = true;
    requestFinished = true;
  }
  else if (requestType == STATUS_REQUEST_REVISION_INFO)
  {
    emit updatedRevisionInfo(c.at(1));
    success = true;
    requestFinished = true;
  }

  return success;
}

QByteArray StatusConversationHandler::requestData(const ModemRequest *request) const
{
  const int &requestType = request->requestType;
  //const RequestArgs &requestArgs = request->requestArgs;
  //const int &requestStage = request->requestStage;

  QByteArray data;

  if (requestType == STATUS_REQUEST_MANUFACTURER_INFO)
  {
    data = CMD_MANUFACTURER_INFO;
  }
  else if (requestType == STATUS_REQUEST_MODEL_INFO)
  {
    data = CMD_MODEL_INFO;
  }
  else if (requestType == STATUS_REQUEST_SERIAL_NUMBER)
  {
    data = CMD_SERIAL_NUMBER;
  }
  else if (requestType == STATUS_REQUEST_REVISION_INFO)
  {
    data = CMD_REVISION_INFO;
  }

  return data;
}

int StatusConversationHandler::requestTypesCount() const
{
  return STATUS_REQUEST_LAST;
}

QString StatusConversationHandler::name() const
{
  return QString(STATUS_HANDLER_NAME);
}

void StatusConversationHandler::updateManufacturerInfo()
{
  modem()->appendRequest(createEmptyRequest(STATUS_REQUEST_MANUFACTURER_INFO));
}

void StatusConversationHandler::updateModelInfo()
{
  modem()->appendRequest(createEmptyRequest(STATUS_REQUEST_MODEL_INFO));
}

void StatusConversationHandler::updateSerialNumberInfo()
{
  modem()->appendRequest(createEmptyRequest(STATUS_REQUEST_SERIAL_NUMBER));
}

void StatusConversationHandler::updateRevisionInfo()
{
  modem()->appendRequest(createEmptyRequest(STATUS_REQUEST_REVISION_INFO));
}
