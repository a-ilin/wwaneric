#ifndef MODEMSTATUS_H
#define MODEMSTATUS_H

#include "Modem.h"

#define STATUS_HANDLER_NAME "Status"

enum STATUS_REQUEST_TYPE
{
  STATUS_REQUEST_MANUFACTURER_INFO,
  STATUS_REQUEST_MODEL_INFO,
  STATUS_REQUEST_SERIAL_NUMBER,
  STATUS_REQUEST_REVISION_INFO,
  STATUS_REQUEST_SIGNAL_QUALITY,
  STATUS_REQUEST_LAST  // a value for type counting. not a real request type.
};

struct StatusAnswer : public AnswerData
{
  QString data;
};

struct StatusSignalQualityAnswer : public AnswerData
{
  double signal_dbm;
  double signal_percent;
  bool   signal_detected;
  QString ber_percent_range;
  bool    ber_detected;
};

struct StatusArgs : public RequestArgs
{
};

class StatusConversationHandler : public ConversationHandler
{
public:
  void processConversation(ModemRequest *request,
                           const Conversation &c,
                           ModemRequest::Status &status,
                           AnswerData* &answerData) const;

  QByteArray requestData(const ModemRequest *request) const;
  int requestTypesCount() const;
  QString name() const;
};

#endif // MODEMSTATUS_H
