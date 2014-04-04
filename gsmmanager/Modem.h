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
  void updatedSms(SMS_STORAGE storage, SMS_STATUS status, const QList<Sms> &smsList);
  void updatedSmsCapacity(int simUsed, int simTotal, int phoneUsed, int phoneTotal);

  /*
   * USSD
   *
  */
  void updatedUssd(const QString &ussd, USSD_STATUS status);

protected:
  // return true if conversation was recognized and successfully processed
  bool processConversation(const Conversation & c);

private:


  static QStringList parseModemAnswer(const QString &inputText,
                                      const QString &splitter,
                                      const QString &command);




};

#endif // MODEM_H
