#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <tonc.h>


void sio_handle_irq_slave();
void slave_routine(void);

extern volatile int got_new_data;


#endif //INTERRUPT_H