#ifndef __BK_COMMON_TYPES__
#define __BK_COMMON_TYPES__

#include "types.h"

#define GLOBAL_INT_DECLARATION() 
#define GLOBAL_INT_DISABLE() 
#define GLOBAL_INT_RESTORE()   

typedef void (*FUNCPTR)(void);
typedef void (*FUNC_1PARAM_PTR)(void *ctxt);
typedef void (*FUNC_2PARAM_PTR)(void *arg, unsigned char vif_idx);
typedef void (*FUNC_2PARAM_CB)(uint32_t larg, uint32_t rarg);

#endif
