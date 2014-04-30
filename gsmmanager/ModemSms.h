#ifndef MODEMSMS_H
#define MODEMSMS_H

#include "Modem.h"

#include "Sms.h"

#define SMS_HANDLER_NAME "SMS"

enum SMS_REQUEST_TYPE
{
  SMS_REQUEST_CAPACITY,
  SMS_REQUEST_READ,
  SMS_REQUEST_SEND,
  SMS_REQUEST_DELETE,
  SMS_REQUEST_LAST  // a value for type counting. not a real request type.
};

struct SmsArgsRead : public RequestArgs
{
  SMS_STATUS smsStatus;
  SMS_STORAGE smsStorage;
};

struct SmsArgsDelete : public RequestArgs
{
  SMS_STORAGE smsStorage;
  int smsIndex;
};

struct SmsAnswerDeleted : public AnswerData
{
  SMS_STORAGE smsStorage;
  int smsIndex;
};

struct SmsAnswerCapacity : public AnswerData
{
  int simUsed;
  int simTotal;
  int phoneUsed;
  int phoneTotal;
};

struct SmsAnswerRead : public AnswerData
{
  QList<Sms> smsList;
};

class SmsConversationHandler : public ConversationHandler
{
public:
  void processConversation(ModemRequest *request,
                           const Conversation &c,
                           ModemRequest::Status &status,
                           AnswerData* &answerData) const;

  QByteArray requestData(const ModemRequest *request) const;
  int requestTypesCount() const;
  QString name() const;

protected:
  RequestArgs* requestArgs(int type) const;
};

#endif // MODEMSMS_H
