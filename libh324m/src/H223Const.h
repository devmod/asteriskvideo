#ifndef _H223Const_H_
#define _H223Const_h_
#include <ptlib.h>

#define MUX_TABLE_SIZE  16

#define DEF_MUX_HEADER 1
#define DEF_MUX_SYNC_FLAG 32
#define DEF_MUX_SYNC_THRESH 28
#define DEF_MUX_LEVEL 1

#define DEF_PAYLOAD_MIN 1
#define DEF_PAYLOAD_MAX 128

#define DEF_HEADER_CODE BCH15_5
#define DEF_USE_MPL 0
#define DEF_MPL_CODE BCH15_11

#define DEF_INFO_FIELD_LENGTH  128    

#define NO_INTERLEAVING   0 
#define BIT_INTERLEAVING  1
#define BYTE_INTERLEAVING 2

#define DEF_HEADER_MC_LENGTH    4
#define DEF_HEADER_CRC_LENGTH   3

#define MAX_AL_SENDERS 10
#define MAX_AL_RECEIVERS 10

#define MAX_AL2_SDU_SIZE 1120
#define MAX_AL3_SDU_SIZE 1120

#define Debug printf

#ifndef WIN32
#define BYTE unsigned char
#define WORD unsigned short
#define DWORD unsigned int
#endif


#endif
