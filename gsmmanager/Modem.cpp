#include "Modem.h"

#include "common.h"

QEvent::Type ModemEventType = (QEvent::Type)QEvent::registerEventType();


#define CMD_MANUFACTURER_INFO "AT+CGMI"
#define CMD_MODEL_INFO "AT+CGMM"
#define CMD_SERIAL_NUMBER "AT+CGSN"
#define CMD_REVISION_INFO "AT+CGMR"


#define CMD_SMS_STORAGE_CAPACITY "AT+CPMS?"
#define CMD_SMS_SET_PREFERRED_STORAGE "AT+CPMS=%1"
#define CMD_SMS_READ_BY_STATUS "AT+CMGL=%1"

QStringList parseAnswerLine(const QString &line, const QString &command)
{
  QStringList result;
  int index = line.indexOf(command);
  if(index == 0)  // line should start with the command
  {
    index = command.size();
    while(line.at(index) == ' ')
    {
      ++index;
    }

    result = line.mid(index).split(',');
  }

  return result;
}

Modem::Modem() :
  PortController()
{

}

Modem::~Modem()
{
}



bool Modem::processConversation(const Conversation &c)
{
  if (!m_requests.size())
  {
    // no repeat for unexpected answers
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "No pending requests exist!");
    return true;
  }

  // conversation successfully processed
  bool success = false;
  // conversation processed and corresponding request should be removed
  bool requestProcessed = false;

  if (c.size() >= 2)
  {
    QByteArray request(c.first());

    // echo reply has \r at the end
    if (request.at(request.size() - 1) == '\r')
    {
      request.remove(request.size() - 2, 1);
    }

    /* QByteArray status(c.last()); */

    MODEM_REQUEST_TYPE &requestType = m_requests.first().requestType;
    RequestArgs &requestArgs = m_requests.first().requestArgs;
    int &requestStage = m_requests.first().requestStage;

    if ((requestType == MODEM_REQUEST_MANUFACTURER_INFO) && (request == CMD_MANUFACTURER_INFO))
    {
      emit updatedManufacturerInfo(c.at(1));
      success = true;
      requestProcessed = true;
    }
    else if ((requestType == MODEM_REQUEST_MODEL_INFO) && (request == CMD_MODEL_INFO))
    {
      emit updatedModelInfo(c.at(1));
      success = true;
      requestProcessed = true;
    }
    else if ((requestType == MODEM_REQUEST_SERIAL_NUMBER) && (request == CMD_SERIAL_NUMBER))
    {
      emit updatedSerialNumberInfo(c.at(1));
      success = true;
      requestProcessed = true;
    }
    else if ((requestType == MODEM_REQUEST_REVISION_INFO) && (request == CMD_REVISION_INFO))
    {
      emit updatedRevisionInfo(c.at(1));
      success = true;
      requestProcessed = true;
    }
    else if ((requestType == MODEM_REQUEST_SMS_CAPACITY) && (request == CMD_SMS_STORAGE_CAPACITY))
    {
      QStringList storageLine = parseAnswerLine(c.at(1), "+CPMS:");
      const int blockSize = 3;
      bool answerDecoded = true;
      int simUsed = 0;
      int simTotal = 0;
      int phoneUsed = 0;
      int phoneTotal = 0;

      if (!(storageLine.size() % blockSize))
      {
        for(int i = 0; i < storageLine.size() / blockSize; ++i)
        {
          int * p_used = 0;
          int * p_total = 0;

          if (storageLine.at(i*blockSize) == SMS_STORAGE_PHONE_STR)
          {
            p_used = &phoneUsed;
            p_total = &phoneTotal;
          }
          else if (storageLine.at(i*blockSize) == SMS_STORAGE_SIM_STR)
          {
            p_used = &simUsed;
            p_total = &simTotal;
          }
          else
          {
            break;
          }

          SAFE_CONVERT(int, toInt, used, storageLine.at(i*blockSize+1), answerDecoded = false;break;);
          *p_used = used;
          SAFE_CONVERT(int, toInt, total, storageLine.at(i*blockSize+2), answerDecoded = false;break;);
          *p_total = total;
        }
      }
      else
      {
        answerDecoded = false;
      }

      if (answerDecoded)
      {
        emit updatedSmsCapacity(simUsed, simTotal, phoneUsed, phoneTotal);
        success = true;
        requestProcessed = true;
      }
      else
      {
        Q_LOGEX(LOG_VERBOSE_ERROR, "Cannot decode modem answer for storage capacity request!");
      }
    }
    else if (requestType == MODEM_REQUEST_SMS_READ)
    {
      const QString cmdSetSmsStorage = QString(CMD_SMS_SET_PREFERRED_STORAGE)
                                       .arg(smsStorageStr(requestArgs.smsArgs.smsStorage));

      const QString cmdReadSmsByStatus = QString(CMD_SMS_READ_BY_STATUS)
                                         .arg(requestArgs.smsArgs.smsStatus);

      // set preferred storage
      if ((requestStage == 0) && (request == cmdSetSmsStorage))
      {
        requestStage = 1;
        success = true;
        requestProcessed = false;
      }
      // read SMSes
      else if ((requestStage == 1) && (request == cmdReadSmsByStatus))
      {
        SmsArgs smsArgs = requestArgs.smsArgs;

        QList <Sms> smsList;
        int answerDecoded = true;
        int smsLineCount = c.size() - 2; // exclude request itself and "OK"
        if ((smsLineCount > 0) && (!(smsLineCount % 2)))  // each SMS has 2 lines: header and PDU
        {
          for (int i=1; i< c.size()-1; i+=2)
          {
            QStringList headerLine = parseAnswerLine(c.at(i), "+CMGL:");

            // header has 4 fields
            if (headerLine.size() == 4)
            {
              SAFE_CONVERT(int, toInt, msgIndex,  headerLine.at(0), answerDecoded=false;break;);
              SAFE_CONVERT(int, toInt, msgStatus, headerLine.at(1), answerDecoded=false;break;);
              // field at index 2 is 'alpha' and not used
              // field at index 3 is octet count and not used

              QByteArray pdu = c.at(i+1);

              Sms sms(smsArgs.smsStorage, (SMS_STATUS)msgStatus, msgIndex, pdu);

              if (sms.isValid())
              {
                smsList.append(sms);
              }
              else
              {
                answerDecoded = false;
                break;
              }
            }
            else
            {
              answerDecoded = false;
              break;
            }
          }
        }

        if (answerDecoded == false)
        {
          // reset stage to initial
          requestStage = 0;
          Q_LOGEX(LOG_VERBOSE_ERROR, "Error processing SMS answer!");
        }
        else
        {
          emit updatedSms(smsList);
          success = true;
          requestProcessed = true;
        }
      }
    }
    else
    {
      QString str = QString("Unprocessed answer received. Request: type %1. Answer: %2")
                    .arg(requestType)
                    .arg(QString(request));
      Q_LOGEX(LOG_VERBOSE_WARNING, str);
    }
  }
  else
  {
    Q_LOGEX(LOG_VERBOSE_WARNING, "Too small conversation received!");
  }

  if (requestProcessed)
  {
    m_requests.takeFirst();
  }

  return success;
}

QByteArray Modem::requestData() const
{
  QByteArray data;

  foreach(const ModemRequest &request, m_requests)
  {
    const MODEM_REQUEST_TYPE &requestType = request.requestType;
    const RequestArgs &requestArgs = request.requestArgs;
    const int &requestStage = request.requestStage;

    if (requestType == MODEM_REQUEST_MANUFACTURER_INFO)
    {
      data = CMD_MANUFACTURER_INFO;
    }
    else if (requestType == MODEM_REQUEST_MODEL_INFO)
    {
      data = CMD_MODEL_INFO;
    }
    else if (requestType == MODEM_REQUEST_SERIAL_NUMBER)
    {
      data = CMD_SERIAL_NUMBER;
    }
    else if (requestType == MODEM_REQUEST_REVISION_INFO)
    {
      data = CMD_REVISION_INFO;
    }
    else if (requestType == MODEM_REQUEST_SMS_CAPACITY)
    {
      data = CMD_SMS_STORAGE_CAPACITY;
    }
    else if (requestType == MODEM_REQUEST_SMS_READ)
    {
      if (requestStage == 0)
      {
        data = QString(CMD_SMS_SET_PREFERRED_STORAGE)
               .arg(smsStorageStr(requestArgs.smsArgs.smsStorage)).toLatin1();
      }
      else if (requestStage == 1)
      {
        data = QString(CMD_SMS_READ_BY_STATUS)
               .arg(requestArgs.smsArgs.smsStatus).toLatin1();
      }
      else
      {
        Q_LOGEX(LOG_VERBOSE_CRITICAL, "Wrong stage!")
      }
    }
    else
    {
      Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unknown request type!");
    }

    if (data.size())
    {
      break;
    }
  }

  return data;
}



/*
 * Some static information
 *
*/
void Modem::updateManufacturerInfo()
{
  m_requests.append(ModemRequest(MODEM_REQUEST_MANUFACTURER_INFO));
  sendRequest();
}

void Modem::updateModelInfo()
{
  m_requests.append(ModemRequest(MODEM_REQUEST_MODEL_INFO));
  sendRequest();
}

void Modem::updateSerialNumberInfo()
{
  m_requests.append(ModemRequest(MODEM_REQUEST_SERIAL_NUMBER));
  sendRequest();
}

void Modem::updateRevisionInfo()
{
  m_requests.append(ModemRequest(MODEM_REQUEST_REVISION_INFO));
  sendRequest();
}

/*
 * SMS
 *
*/

void Modem::updateSms(SMS_STORAGE storage, SMS_STATUS status)
{
  SmsArgs smsArgs;
  smsArgs.smsStorage = storage;
  smsArgs.smsStatus = status;

  RequestArgs args;
  args.smsArgs = smsArgs;

  m_requests.append(ModemRequest(MODEM_REQUEST_SMS_READ, args));

  sendRequest();
}

void Modem::updateSmsCapacity()
{
  m_requests.append(ModemRequest(MODEM_REQUEST_SMS_CAPACITY));
  sendRequest();
}

void Modem::sendSms(const Sms &sms)
{

}

void Modem::deleteSms(SMS_STORAGE storage, int index)
{

}

/*
 * USSD
 *
*/

void Modem::sendUssd(const QString &ussd, USSD_SEND_STATUS status)
{

}




