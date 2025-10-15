#ifndef ASR_FN_ASR_H_
#define ASR_FN_ASR_H_

#include <stdbool.h>

int Wanson_ASR_Init();
int Wanson_Model_Init(unsigned int* start_addr, unsigned int* end_addr);
int Wanson_ASR_Reset(int domain);

/*****************************************
* Input:
*      - buf      : Audio data (16k, 16bit, mono) 
*      - buf_len  : Now must be 480 (30ms) 
*
* Output:
*      - text     : The text of ASR 
*      - score    : The confidence of ASR (Now not used)
*
* Return value    :  0 - No result
*                    1 - Has result 
*                   -1 - Error
******************************************/
int  Wanson_ASR_Recog(short *buf, int buf_len, const char **text, float *score);

void Wanson_ASR_Release();

int Wanson_ASR_Get_Activ_Group_Index();
#endif
