#include "ModemSms.h"

#define CMD_SMS_STORAGE_CAPACITY        "AT+CPMS?"
#define CMD_SMS_SET_PREFERRED_STORAGE   "AT+CPMS=%1"
#define CMD_SMS_READ_BY_STATUS          "AT+CMGL=%1"
#define CMD_SMS_DELETE_BY_INDEX         "AT+CMGD=%1,0"

bool SmsConversationHandler::processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished)
{
  const int& requestType = request->requestType;
  int& requestStage = request->requestStage;
  const SmsArgs* smsArgs = static_cast<SmsArgs*> (request->requestArgs);

  bool success = false;

  if (requestType == SMS_REQUEST_CAPACITY)
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
      requestFinished = true;
    }
    else
    {
      Q_LOGEX(LOG_VERBOSE_ERROR, "Cannot decode modem answer for storage capacity request!");
    }
  }
  else if (requestType == SMS_REQUEST_READ)
  {
    // set preferred storage
    if (requestStage == 0)
    {
      requestStage = 1;
      success = true;
      requestFinished = false;
    }
    // read SMSes
    else if (requestStage == 1)
    {
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

            Sms sms(smsArgs->smsStorage, (SMS_STATUS)msgStatus, msgIndex, pdu);

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
        requestFinished = true;
      }
    }
  }
  else if (requestType == SMS_REQUEST_DELETE)
  {
    // set preferred storage
    if (requestStage == 0)
    {
      requestStage = 1;
      success = true;
      requestFinished = false;
    }
    // read SMSes
    else if (requestStage == 1)
    {
      emit deletedSms(smsArgs->smsStorage, smsArgs->smsIndex);
      success = true;
      requestFinished = true;
    }
  }

  return success;
}

QByteArray SmsConversationHandler::requestData(const ModemRequest *request) const
{
  const int &requestType = request->requestType;
  const SmsArgs *smsArgs = static_cast<const SmsArgs*> (request->requestArgs);
  const int &requestStage = request->requestStage;

  QByteArray data;

  if (requestType == SMS_REQUEST_CAPACITY)
  {
    data = CMD_SMS_STORAGE_CAPACITY;
  }
  else if (requestType == SMS_REQUEST_READ)
  {
    if (requestStage == 0)
    {
      data = QString(CMD_SMS_SET_PREFERRED_STORAGE)
             .arg(smsStorageStr(smsArgs->smsStorage)).toLatin1();
    }
    else if (requestStage == 1)
    {
      data = QString(CMD_SMS_READ_BY_STATUS)
             .arg(smsArgs->smsStatus).toLatin1();
    }
    else
    {
      Q_LOGEX(LOG_VERBOSE_CRITICAL, "Wrong stage!")
    }
  }
  else if (requestType == SMS_REQUEST_DELETE)
  {
    if (requestStage == 0)
    {
      data = QString(CMD_SMS_SET_PREFERRED_STORAGE)
             .arg(smsStorageStr(smsArgs->smsStorage)).toLatin1();
    }
    else if (requestStage == 1)
    {
      data = QString(CMD_SMS_DELETE_BY_INDEX)
             .arg(smsArgs->smsIndex).toLatin1();
    }
    else
    {
      Q_LOGEX(LOG_VERBOSE_CRITICAL, "Wrong stage!")
    }
  }

  return data;
}

int SmsConversationHandler::requestTypesCount() const
{
  return SMS_REQUEST_LAST;
}

QString SmsConversationHandler::name() const
{
  return QString(SMS_HANDLER_NAME);
}

RequestArgs* SmsConversationHandler::requestArgs() const
{
  return new SmsArgs();
}

void SmsConversationHandler::updateSms(SMS_STORAGE storage, SMS_STATUS status)
{
  SmsArgs * smsArgs = static_cast<SmsArgs*> (requestArgs());
  smsArgs->smsStorage = storage;
  smsArgs->smsStatus = status;

  modem()->appendRequest(createEmptyRequest(SMS_REQUEST_READ, smsArgs));
}

void SmsConversationHandler::updateSmsCapacity()
{
  modem()->appendRequest(createEmptyRequest(SMS_REQUEST_CAPACITY));
}

void SmsConversationHandler::sendSms(const Sms &sms)
{

}

void SmsConversationHandler::deleteSms(SMS_STORAGE storage, int index)
{
  SmsArgs * smsArgs = static_cast<SmsArgs*> (requestArgs());
  smsArgs->smsStorage = storage;
  smsArgs->smsIndex = index;

  modem()->appendRequest(createEmptyRequest(SMS_REQUEST_DELETE, smsArgs));
}
