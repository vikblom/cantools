#ifndef CANTOMAT_H
#define CANTOMAT_H

#include <stdint.h>
#include <time.h>


/* CAN message type */
typedef struct {
    struct {
        time_t tv_sec;
        uint32_t tv_nsec;
    } t; /* time stamp */
    uint8_t   bus;     /* can bus */
    uint32_t  id;      /* numeric CAN-ID */
    uint8_t   dlc;
    uint8_t   byte_arr[8];
} canMessage_t;

/* message received callback function */
typedef void (* msgRxCb_t)(canMessage_t *message, void *cbData);

#endif /* CANTOMAT_H */
