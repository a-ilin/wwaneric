/*
Copyright (C) 2010- Alexander Chudnovets <effractor@gmail.com>

Based on SMS Server Tools 3
Copyright (C) 2006- Keijo Kasvi
http://smstools3.kekekasvi.com/

This program is free software unless you got it under another license directly
from the author. You can redistribute it and/or modify it under the terms of
the GNU General Public License as published by the Free Software Foundation.
Either version 2 of the License, or (at your option) any later version.
*/

#ifndef PDU_H
#define PDU_H

#ifdef PDU_DECODE_EXPORTS
#define PDU_DECODE_API __declspec(dllexport)
#else
#define PDU_DECODE_API __declspec(dllimport)
#endif

class PDU_DECODE_API PDU {
public:
    enum NumerFormat {
        NF_UNKNOWN = 129,
        NF_INTERNATIONAL = 145,
        NF_INATIONAL = 161
    };

    enum Alphabet {
        GSM = -1,
        ISO = 0,
        BINARY = 1,
        UCS2 = 2
    };

    // Constructors/Destructors
    PDU();
    PDU(const char *pdu);
    virtual ~PDU();

    // Getters
    inline char* getPDU() const { return m_pdu; }
    inline char* getSMSC() const { return m_smsc; }
    inline char* getNumber() const { return m_number; }
    inline char* getNumberType() const { return m_number_type; }
    inline char* getDate() const { return m_date; }
    inline char* getTime() const { return m_time; }
    inline char* getUDHType() const { return m_udh_type; }
    inline char* getUDHData() const { return m_udh_data; }
    inline char* getMessage() const { return m_message; }
    inline char* getError() const { return m_err; }
    inline int getMessageLen() const { return m_message_len; }

    // Setters
    void setMessage(const char* message, const int message_len = -1);
    void setSMSC(const char* smsc);
    void setNumber(const char* number);
    void setAlphabet(const Alphabet alphabet);

    //
    bool parse();
    void generate();

    // convert codepage
    //int convert(const char *tocode, const char *fromcode);
    int convertFromUcs2ToUtf8();
    int convertFromUtf8ToUcs2();

private:
    char* m_pdu;
    char* m_pdu_ptr;
    char* m_message;
    int m_message_len;
    char* m_smsc;
    char* m_number;
    char* m_number_type;
    NumerFormat m_number_fmt;
    char* m_date;
    char* m_time;
    char* m_udh_type;
    char* m_udh_data;
    char* m_err;
    bool m_with_udh;
    bool m_report;
    bool m_is_statusreport;
    int m_replace;
    int m_alphabet;
    bool m_flash;
    const char* m_mode;
    int m_validity;
    int m_system_msg;
    int m_replace_msg;

    void reset();
    bool parseSMSC();
    bool parseDeliver();
    bool parseStatusReport();
    int explainAddressType(const char *octet_char, int octet_int);
};

#endif
