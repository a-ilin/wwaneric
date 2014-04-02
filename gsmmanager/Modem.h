#ifndef MODEM_H
#define MODEM_H

#include <QEvent>
#include <QObject>
#include <QLinkedList>
#include <QList>
#include <QQueue>
#include <QSet>
#include <QtSerialPort/QSerialPort>
#include <QStringList>

#include "Sms.h"
#include "Ussd.h"


// an ID for QEvent object constructing
extern QEvent::Type ModemEventType;

class QSerialPort;
class QTimer;

typedef QList<QByteArray> Conversation;

class Modem : public QObject
{
  Q_OBJECT

public:
  Modem();
  ~Modem();

  QString portName() const { return m_serialPort->portName(); }

public slots:

  /*
   * Connection information
   *
  */
  void setPortName(const QString &portName);
  void openPort();
  void closePort();

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
   * Connection information
   *
  */
  void updatedPortStatus(bool opened);
  void updatedPortError(QSerialPort::SerialPortError errorCode);
  void updatedPortError(const QString& errorString);

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

private:

  // writes raw data to port
  void sendToPort(const QByteArray &data);
  // wrapper for sendToPort, adds request to set before calling sendToPort
  void sendRequest(const QByteArray &request);
  void sendRequestFromQueue();

  static QStringList parseModemAnswer(const QString &inputText,
                                      const QString &splitter,
                                      const QString &command);

  // return true if all requested data received and false otherwise
  bool parseBuffer();

  // return true if conversation was recognized and successfully processed
  bool processConversation(const Conversation & c);

  void notifyError(QSerialPort::SerialPortError errorCode);

private slots:
  // reads data from port to buffer
  void onReadyRead();
  // error processing
  void onError(QSerialPort::SerialPortError errorCode);
  void onReadChannelFinished();
  // connection timeout
  void onTimerTimeout();

private:

  enum MODEM_STATUS
  {
    MODEM_STATUS_READY,
    MODEM_STATUS_BUSY
  } m_modemStatus;

  // modem detected on specified port
  bool m_modemDetected;

  // interface to serial port communication
  QSerialPort * m_serialPort;

  // received data buffer
  QByteArray m_bufferReceived;

  // set of requests to send to modem
  typedef QLinkedList<QByteArray> Requests;
  Requests m_requestsToSend;

  // timer timeout
  QTimer * m_timerTimeout;
};

#endif // MODEM_H
