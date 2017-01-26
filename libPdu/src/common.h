#ifndef PDU_COMMON_H
#define PDU_COMMON_H

#ifdef PDU_DECODE_EXPORTS
#define PDU_DECODE_API __declspec(dllexport)
#else
#define PDU_DECODE_API __declspec(dllimport)
#endif

#endif // PDU_COMMON_H
