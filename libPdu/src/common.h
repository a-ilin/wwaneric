#ifndef COMMON_H
#define COMMON_H

#ifdef PDU_DECODE_EXPORTS
#define PDU_DECODE_API __declspec(dllexport)
#else
#define PDU_DECODE_API __declspec(dllimport)
#endif

#endif // COMMON_H
