#ifndef MODEMSMS_H
#define MODEMSMS_H

#include "Modem.h"

#include "Sms.h"

#define SMS_HANDLER_NAME "SMS"

enum SMS_REQUEST_TYPE
{
  SMS_REQUEST_CAPACITY,
  SMS_REQUEST_READ,
  SMS_REQUEST_DELETE,
  SMS_REQUEST_LAST  // a value for type counting. not a real request type.
};

struct SmsArgs : public RequestArgs
{
  SMS_STATUS smsStatus;
  SMS_STORAGE smsStorage;
  int smsIndex;
};

class SmsConversationHandler : public ConversationHandler
{
  Q_OBJECT

public:
  bool processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished);
  QByteArray requestData(const ModemRequest *request) const;
  int requestTypesCount() const;
  QString name() const;

public slots:
  void updateSms(SMS_STORAGE storage, SMS_STATUS status);
  void updateSmsCapacity();
  void sendSms(const Sms& sms);
  void deleteSms(SMS_STORAGE storage, int index);

signals:
  void updatedSms(const QList<Sms> &smsList);
  void updatedSmsCapacity(int simUsed, int simTotal, int phoneUsed, int phoneTotal);
  void sentSms();
  void deletedSms(SMS_STORAGE storage, int index);

protected:
  RequestArgs* requestArgs() const;
};

#endif // MODEMSMS_H
