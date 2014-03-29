#ifndef MODEM_H
#define MODEM_H

#include <QObject>
#include <QList>
#include <QStringList>

#include "Sms.h"

class QSerialPort;

enum MODEM_NOTIFICATION_TYPE
{
  MODEM_NOTIFICATION_PORT,
  MODEM_NOTIFICATION_SMS,
  MODEM_NOTIFICATION_USSD,
  MODEM_NOTIFICATION_LAST
};

class Modem : public QObject
{
  Q_OBJECT

public:
  Modem();
  virtual ~Modem();

  void setSerialPortName(const QString &serialPortName);

  // test if modem works
  bool testModem();

  bool portTested() const { return m_portTested; }

  /*
   * Communication speed up
   *
  */
  // return true on success or false on fail
  bool startBatchQuery();
  void endBatchQuery();


  /*
   * Some static information
   *
  */
  QString manufacturerInfo();
  QString modelInfo();
  QString serialNumberInfo();
  QString revisionInfo();

  /*
   * SMS
   *
  */

  QList<Sms> readSms(SMS_STORAGE storage, SMS_STATUS status);

  // return true on success or false on fail
  bool storageCapacityUsed(int *simUsed, int *simTotal, int *phoneUsed, int *phoneTotal);

  bool deleteSms(SMS_STORAGE storage, int index);

  /*
   * Logging
   *
  */
  void setLogFunction(void(*logFunction)(const QString&));
  void unsetLogFunction();

signals:
  void modemNotification(MODEM_NOTIFICATION_TYPE type);

private:

  // console log function (for communication raw view)
  void (*logFunction)(const QString &text);
  static void dummyLogFunction(const QString &text);

  bool openSerialPort();
  void closeSerialPort();

  void sendToSerialPort(const QString str);
  QString readLineFromSerialPort();
  QStringList readAllFromSerialPort();
  QString readAllRawFromSerialPort();
  inline bool decodeCommandExecStatus(QString statusString);

  // Parse answer from modem (inputText) for arguments.
  // If searched argument (command) is specified then
  // only string contained it will be parsed.
  // Splitter specifies delimiter between parsed elements.
  QStringList parseModemAnswer(const QString &inputText,
                               const QString &splitter,
                               const QString &command=QString());


  bool selectPreferredSmsStorage(SMS_STORAGE storage);

private:
  // interface to serial port communication
  QSerialPort * m_serialPort;

  // when set to true serial port don't being opened and closed at each query
  bool m_batchQueryStarted;

  // true when port test is successful
  bool m_portTested;
};

#endif // MODEM_H
