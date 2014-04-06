#ifndef MODEM_H
#define MODEM_H

#include "PortController.h"

#include <QEvent>

#include "Sms.h"
#include "Ussd.h"


// an ID for QEvent object constructing
extern QEvent::Type ModemEventType;

class Modem : public PortController
{
  Q_OBJECT

public:
  Modem();
  ~Modem();


public slots:
  /*
   * Some static information
   *
  */
  void updateManufacturerInfo();
  void updateModelInfo();
  void updateSerialNumberInfo();
  void updateRevisionInfo();


  /*
   * SMS
   *
  */
  void updateSms(SMS_STORAGE storage, SMS_STATUS status);
  void updateSmsCapacity();

  void sendSms(const Sms& sms);
  void deleteSms(SMS_STORAGE storage, int index);

  /*
   * USSD
   *
  */

  void sendUssd(const QString &ussd, USSD_SEND_STATUS status);


signals:

  /*
   * Some static information
   *
  */
  void updatedManufacturerInfo(const QString &info);
  void updatedModelInfo(const QString &info);
  void updatedSerialNumberInfo(const QString &info);
  void updatedRevisionInfo(const QString &info);

  /*
   * SMS
   *
  */
  void updatedSms(const QList<Sms> &smsList);
  void updatedSmsCapacity(int simUsed, int simTotal, int phoneUsed, int phoneTotal);
  void deletedSms(SMS_STORAGE storage, int index);

  /*
   * USSD
   *
  */
  void updatedUssd(const QString &ussd, USSD_STATUS status);

protected:
  // return true if conversation was recognized and successfully processed
  bool processConversation(const Conversation & c);

  // return raw data for request from queue
  QByteArray requestData() const;

private:

  // internal types

  struct SmsArgs
  {
    SMS_STATUS smsStatus;
    SMS_STORAGE smsStorage;
    int smsIndex;
  };

  enum MODEM_REQUEST_TYPE
  {
    MODEM_REQUEST_MANUFACTURER_INFO,
    MODEM_REQUEST_MODEL_INFO,
    MODEM_REQUEST_SERIAL_NUMBER,
    MODEM_REQUEST_REVISION_INFO,
    MODEM_REQUEST_SMS_CAPACITY,
    MODEM_REQUEST_SMS_READ,
    MODEM_REQUEST_SMS_DELETE
  };

  union RequestArgs
  {
    SmsArgs smsArgs;
  };

  struct ModemRequest
  {
    ModemRequest(MODEM_REQUEST_TYPE type) :
      requestType(type),
      requestStage(0)
    {}

    ModemRequest(MODEM_REQUEST_TYPE type, const RequestArgs &args) :
      requestType(type),
      requestArgs(args),
      requestStage(0)
    {}

    MODEM_REQUEST_TYPE requestType;
    RequestArgs requestArgs;
    int requestStage;
  };

private:

  QLinkedList<ModemRequest> m_requests;

};

#endif // MODEM_H
