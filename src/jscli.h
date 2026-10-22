#ifndef JSCLI_H
#define JSCLI_H

#include <stdint.h>
#include "hostfs.h"

/*SWI used by the JSCLI helper modules (src/jscli, in the extension ROM) to
  run * commands for the frontend. See jscli.c for the protocol*/
#define ARCEM_SWI_JSCLI     (ARCEM_SWI_CHUNK + 5)

void jscli_swi(uint32_t *regs);
void jscli_reset(void);

#endif
