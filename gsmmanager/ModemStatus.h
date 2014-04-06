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
  STATUS_REQUEST_LAST  // a value for type counting. not a real request type.
};

struct StatusArgs : public RequestArgs
{
};

class StatusConversationHandler : public ConversationHandler
{
  Q_OBJECT

public:
  bool processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished);
  QByteArray requestData(const ModemRequest *request) const;
  int requestTypesCount() const;
  QString name() const;

public slots:
  void updateManufacturerInfo();
  void updateModelInfo();
  void updateSerialNumberInfo();
  void updateRevisionInfo();

signals:
  void updatedManufacturerInfo(const QString &info);
  void updatedModelInfo(const QString &info);
  void updatedSerialNumberInfo(const QString &info);
  void updatedRevisionInfo(const QString &info);
};

#endif // MODEMSTATUS_H
