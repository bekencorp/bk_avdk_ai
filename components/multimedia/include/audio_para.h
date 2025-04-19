#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <os/os.h>


///default eq parameter

#define EQ0 1
#define EQ0A0 -1668050
#define EQ0A1 734106
#define EQ0B0 934084
#define EQ0B1 -1715174
#define EQ0B2 801474

#define EQ1 1
#define EQ1A0 -1764715
#define EQ1A1 784980
#define EQ1B0 1034243
#define EQ1B1 -1764715
#define EQ1B2 799312


#define FILTER_PREGAIN_FRA_BITS (14)

#define DEFAULT_EQ_PARA_DL_VOICE()                   \
    {                                        \
    .filters = 2,                                                             \
    .globle_gain = (uint32_t)(1.12f * (1 << FILTER_PREGAIN_FRA_BITS)),          \
    .eq_para[0].a[0] = -EQ0A0,                                                  \
    .eq_para[0].a[1] = -EQ0A1,                                                  \
    .eq_para[0].b[0] = EQ0B0,                                                   \
    .eq_para[0].b[1] = EQ0B1,                                                   \
    .eq_para[0].b[2] = EQ0B2,                                                   \
    .eq_para[1].a[0] = -EQ1A0,                                                  \
    .eq_para[1].a[1] = -EQ1A1,                                                  \
    .eq_para[1].b[0] = EQ1B0,                                                   \
    .eq_para[1].b[1] = EQ1B1,                                                   \
    .eq_para[1].b[2] = EQ1B2,                                                   \
}


#ifdef __cplusplus
}
#endif


