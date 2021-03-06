﻿/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2017 by Aleksei Ilin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "ModemSms.h"

#define CMD_SMS_STORAGE_CAPACITY        "AT+CPMS?"
#define CMD_SMS_SET_PREFERRED_STORAGE   "AT+CPMS=%1"
#define CMD_SMS_READ_BY_STATUS          "AT+CMGL=%1"
#define CMD_SMS_READ_BY_INDEX           "AT+CMGR=%1"
#define CMD_SMS_DELETE_BY_INDEX         "AT+CMGD=%1,0"

void SmsConversationHandler::processConversation(ModemRequest *request,
                                                 const Conversation &c,
                                                 ModemRequest::Status& status,
                                                 AnswerData*& answerData) const
{
  status = ModemRequest::Failure;

  if (c.status == Conversation::OK)
  {
    if ((request->type() == SMS_REQUEST_CAPACITY) && (c.data.size() > 0))
    {
      QStringList storageLine = parseAnswerLine(c.data.first(), "+CPMS:");
      const int blockSize = 3;
      bool answerDecoded = false;
      int simUsed = 0;
      int simTotal = 0;
      int phoneUsed = 0;
      int phoneTotal = 0;

      if ((storageLine.size()) && (!(storageLine.size() % blockSize)))
      {
        answerDecoded = true;
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
        SmsAnswerCapacity * answer = new SmsAnswerCapacity();
        answer->simUsed = simUsed;
        answer->simTotal = simTotal;
        answer->phoneUsed = phoneUsed;
        answer->phoneTotal = phoneTotal;
        answerData = answer;
        status = ModemRequest::SuccessCompleted;
      }
      else
      {
        Q_LOGEX(LOG_VERBOSE_ERROR, "Cannot decode modem answer for storage capacity request!");
      }
    }
    else if ((request->type() == SMS_REQUEST_READ) && (c.data.size() > 0))
    {
      const SmsArgsRead* smsArgs = static_cast<SmsArgsRead*> (request->args());

      // set preferred storage
      if (request->stage() == 0)
      {
        request->setStage(1);
        status = ModemRequest::SuccessNeedMoreData;
      }
      // read SMSes
      else if (request->stage() == 1)
      {
        QList<Sms> smsList;
        int answerDecoded = true;
        int smsLineCount = c.data.size();
        if ((smsLineCount > 0) && (!(smsLineCount % 2)))  // each SMS has 2 lines: header and PDU
        {
          for (int i=0; i< c.data.size(); i+=2)
          {
            QStringList headerLine = parseAnswerLine(c.data.at(i), "+CMGL:");

            // header has at least 4 fields:
            // 0    - is index
            // 1    - is status
            // 2    - is 'alpha' and not used
            // last - is octet count and not used
            if (headerLine.size() >= 4)
            {
              SAFE_CONVERT(int, toInt, msgIndex,  headerLine.at(0), answerDecoded=false;break;);
              SAFE_CONVERT(int, toInt, msgStatus, headerLine.at(1), answerDecoded=false;break;);

              QByteArray pdu = c.data.at(i+1);

              Sms sms(smsArgs->smsStorage, msgIndex, (SMS_STATUS)msgStatus, pdu);

              if (sms.valid)
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

        if (answerDecoded)
        {
          SmsAnswerRead * answer = new SmsAnswerRead();
          answer->smsList = smsList;
          answerData = answer;
          status = ModemRequest::SuccessCompleted;
        }
        else
        {
          // reset stage to initial
          request->setStage(0);
          Q_LOGEX(LOG_VERBOSE_ERROR, "Error processing SMS answer!");
        }
      }
    }
    else if ((request->type() == SMS_REQUEST_READ_BY_INDEX) && (c.data.size() > 0))
    {
      const SmsArgsReadByIndex* smsArgs = static_cast<SmsArgsReadByIndex*> (request->args());

      // set preferred storage
      if (request->stage() == 0)
      {
        request->setStage(1);
        status = ModemRequest::SuccessNeedMoreData;
      }
      // read SMS
      else if (request->stage() == 1)
      {
        Sms smsResult;
        int answerDecoded = true;
        if (c.data.size() == 2)
        {
          QStringList headerLine = parseAnswerLine(c.data.at(0), "+CMGR:");

          // header has at least 3 fields:
          // 0    - is status
          // 1    - is 'alpha' and not used
          // last - is octet count and not used
          if (headerLine.size() >= 3)
          {
            SAFE_CONVERT(int, toInt, msgStatus, headerLine.at(0), answerDecoded=false;);

            QByteArray pdu = c.data.at(1);

            Sms sms(smsArgs->smsStorage, smsArgs->smsIndex, (SMS_STATUS)msgStatus, pdu);

            if (sms.valid)
            {
              smsResult = sms;
            }
            else
            {
              answerDecoded = false;
            }
          }
        }
        else
        {
          answerDecoded = false;
        }

        if (answerDecoded)
        {
          SmsAnswerReadByIndex * answer = new SmsAnswerReadByIndex();
          answer->sms = smsResult;
          answerData = answer;
          status = ModemRequest::SuccessCompleted;
        }
        else
        {
          // reset stage to initial
          request->setStage(0);
          Q_LOGEX(LOG_VERBOSE_ERROR, "Error processing SMS answer!");
        }
      }
    }
    else if (request->type() == SMS_REQUEST_DELETE)
    {
      const SmsArgsDelete* smsArgs = static_cast<SmsArgsDelete*> (request->args());

      // set preferred storage
      if (request->stage() == 0)
      {
        request->setStage(1);
        status = ModemRequest::SuccessNeedMoreData;
      }
      // read SMSes
      else if (request->stage() == 1)
      {
        SmsAnswerDeleted * answer = new SmsAnswerDeleted();
        answer->smsStorage = smsArgs->smsStorage;
        answer->smsIndex = smsArgs->smsIndex;
        answerData = answer;
        status = ModemRequest::SuccessCompleted;
      }
    }
  }
}

QByteArray SmsConversationHandler::requestData(const ModemRequest *request) const
{
  const int requestType = request->type();
  const int requestStage = request->stage();

  QByteArray data;

  if (requestType == SMS_REQUEST_CAPACITY)
  {
    data = CMD_SMS_STORAGE_CAPACITY;
  }
  else if (requestType == SMS_REQUEST_READ)
  {
    const SmsArgsRead *smsArgs = static_cast<const SmsArgsRead*> (request->args());

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
  else if (requestType == SMS_REQUEST_READ_BY_INDEX)
  {
    const SmsArgsReadByIndex *smsArgs = static_cast<const SmsArgsReadByIndex*> (request->args());

    if (requestStage == 0)
    {
      data = QString(CMD_SMS_SET_PREFERRED_STORAGE)
             .arg(smsStorageStr(smsArgs->smsStorage)).toLatin1();
    }
    else if (requestStage == 1)
    {
      data = QString(CMD_SMS_READ_BY_INDEX)
             .arg(smsArgs->smsIndex).toLatin1();
    }
    else
    {
      Q_LOGEX(LOG_VERBOSE_CRITICAL, "Wrong stage!")
    }
  }
  else if (requestType == SMS_REQUEST_DELETE)
  {
    const SmsArgsDelete *smsArgs = static_cast<const SmsArgsDelete*> (request->args());

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

RequestArgs* SmsConversationHandler::requestArgs(int type) const
{
  switch(type)
  {
  case SMS_REQUEST_READ:
    return new SmsArgsRead();
  case SMS_REQUEST_READ_BY_INDEX:
    return new SmsArgsReadByIndex();
  case SMS_REQUEST_DELETE:
    return new SmsArgsDelete();
  default:
    return NULL;
  }
}

bool SmsConversationHandler::processUnexpectedData(const QByteArray& data,
                                                    int &replyType,
                                                    AnswerData* &answerData) const
{
  // check for new SMS indication
  QStringList headerLine = parseAnswerLine(QString(data), "+CMTI:");

  if(headerLine.size() == 2)
  {
    bool decoded = false;
    int index;
    SMS_STORAGE storage;

    do
    {
      storage = strToSmsStorage(headerLine.at(0));
      if (storage == -1)
      {
        break;
      }

      SAFE_CONVERT(int, toInt, i2, headerLine.at(1), break;);
      index = i2;

      decoded = true;
    }
    while(0);

    if (decoded)
    {
      SmsAnswerReadUnexpected * answer = new SmsAnswerReadUnexpected();
      answer->smsStorage = storage;
      answer->smsIndex = index;

      replyType = SMS_REQUEST_READ_UNEXPECTED;
      answerData = answer;

      return true;
    }
    else
    {
      QString err = QString("Wrong message indication received: %1").arg(QString(data));
      Q_LOGEX(LOG_VERBOSE_ERROR, err);
    }
  }

  return false;
}


