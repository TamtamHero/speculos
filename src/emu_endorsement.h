#ifndef _EMU_ENDORSEMENT_H
#define _EMU_ENDORSEMENT_H

unsigned long sys_os_endorsement_get_public_key(uint8_t index, uint8_t *buffer);
unsigned long sys_os_endorsement_key1_sign_data(uint8_t *data, size_t dataLength, uint8_t *signature);

#endif
