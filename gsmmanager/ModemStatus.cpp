#include "ModemStatus.h"

#include "common.h"

#define CMD_MANUFACTURER_INFO  "AT+CGMI"
#define CMD_MODEL_INFO         "AT+CGMM"
#define CMD_SERIAL_NUMBER      "AT+CGSN"
#define CMD_REVISION_INFO      "AT+CGMR"
#define CMD_SIGNAL_QUALITY     "AT+CSQ"



void StatusConversationHandler::processConversation(ModemRequest *request,
                                                    const Conversation &c,
                                                    ModemRequest::Status& status,
                                                    AnswerData*& answerData) const
{
  const int requestType = request->type();
  //const RequestArgs &requestArgs = request->requestArgs;
  //int &requestStage = request->requestStage;

  status = ModemRequest::Failure;

  if ((c.data.size() > 0) && (c.status == Conversation::OK))
  {
    switch(requestType)
    {
    case STATUS_REQUEST_MANUFACTURER_INFO:
    case STATUS_REQUEST_MODEL_INFO:
    case STATUS_REQUEST_SERIAL_NUMBER:
    case STATUS_REQUEST_REVISION_INFO:
    {
      StatusAnswer * answer = new StatusAnswer();
      answer->data = c.data.first();
      answerData = answer;
      status = ModemRequest::SuccessCompleted;
    }
      break;
    case STATUS_REQUEST_SIGNAL_QUALITY:
    {
      StatusSignalQualityAnswer * answer = new StatusSignalQualityAnswer();
      answerData = answer;
      status = ModemRequest::Failure;

      if (c.data.size())
      {
        QStringList headerLine = parseAnswerLine(c.data.first(), "+CSQ:");
        if (headerLine.size() == 2)
        {
          bool converted = true;
          SAFE_CONVERT(int, toInt, rssi, headerLine.at(0), converted = false;);
          SAFE_CONVERT(int, toInt, ber,  headerLine.at(1), converted = false;);

          if (converted)
          {
            if (rssi == 99)
            {
              answer->signal_detected = false;
            }
            else
            {
              answer->signal_detected = true;

              if ((rssi >= 0) && (rssi <= 31))
              {
                answer->signal_dbm = -113.0 + rssi * 2.0;
                answer->signal_percent = rssi / 31.0 * 100.0;
              }
              else
              {
                QString errStr = QString("Unknown RSSI value: %1").arg(rssi);
                Q_LOGEX(LOG_VERBOSE_ERROR, errStr);
              }
            }

            if (ber == 99)
            {
              answer->ber_detected = false;
            }
            else
            {
              answer->ber_detected = true;
              switch(ber)
              {
              case 0:
                answer->ber_percent_range = "< 0.2%";
                break;
              case 1:
                answer->ber_percent_range = "0.2%...0.4%";
                break;
              case 2:
                answer->ber_percent_range = "0.4%...0.8%";
                break;
              case 3:
                answer->ber_percent_range = "0.8%...1.6%";
                break;
              case 4:
                answer->ber_percent_range = "1.6%...3.2%";
                break;
              case 5:
                answer->ber_percent_range = "3.2%...6.4%";
                break;
              case 6:
                answer->ber_percent_range = "6.4%...12.8%";
                break;
              case 7:
                answer->ber_percent_range = "12.8% <";
                break;
              default:
              {
                QString errStr = QString("Unknown BER value: %1").arg(ber);
                Q_LOGEX(LOG_VERBOSE_ERROR, errStr);
              }
              }
            }

            status = ModemRequest::SuccessCompleted;
          }
        }
      }
    }
      break;
    default:
      Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unknown request!");

    }
  }
}

QByteArray StatusConversationHandler::requestData(const ModemRequest *request) const
{
  const int requestType = request->type();
  //const RequestArgs &requestArgs = request->requestArgs;
  //const int &requestStage = request->requestStage;

  QByteArray data;

  if (requestType == STATUS_REQUEST_MANUFACTURER_INFO)
  {
    data = CMD_MANUFACTURER_INFO;
  }
  else if (requestType == STATUS_REQUEST_MODEL_INFO)
  {
    data = CMD_MODEL_INFO;
  }
  else if (requestType == STATUS_REQUEST_SERIAL_NUMBER)
  {
    data = CMD_SERIAL_NUMBER;
  }
  else if (requestType == STATUS_REQUEST_REVISION_INFO)
  {
    data = CMD_REVISION_INFO;
  }
  else if (requestType == STATUS_REQUEST_SIGNAL_QUALITY)
  {
    data = CMD_SIGNAL_QUALITY;
  }

  return data;
}

int StatusConversationHandler::requestTypesCount() const
{
  return STATUS_REQUEST_LAST;
}

QString StatusConversationHandler::name() const
{
  return QString(STATUS_HANDLER_NAME);
}

