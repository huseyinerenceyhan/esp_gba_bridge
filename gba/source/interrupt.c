#include "interrupt.h"

#define SIO_START  (1 << 7)
#define SIO_SO_HIGH  (1 << 3)
#define SIO_SO_IRQ  (1 << 14)





IWRAM_CODE void sio_handle_irq_slave() {
    REG_SIOCNT |= SIO_SO_HIGH;


    REG_SIOCNT &= ~(SIO_START | SIO_SO_HIGH);

    REG_SIOCNT |= SIO_START | SIO_SO_HIGH;

    REG_SIOCNT &= ~SIO_SO_HIGH;
}


IWRAM_CODE void slave_routine(void) {

        REG_IF = IRQ_SERIAL;
        sio_handle_irq_slave();
		got_new_data = 1;
}
