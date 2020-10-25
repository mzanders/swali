/* 
 * File:   can.h
 * Author: maarten
 *
 * Created on September 18, 2017, 9:21 PM
 */

#ifndef CAN_H
#define	CAN_H
#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

    typedef uint32_t can_id_t;

    void can_init(void);

    uint8_t can_send_extended(can_id_t id, uint8_t data[], uint8_t data_len);

    uint8_t can_receive_extended(can_id_t *id, uint8_t data[], uint8_t *data_len);

    void can_add_rx_filter(can_id_t mask, can_id_t filter);


#ifdef	__cplusplus
}
#endif

#endif	/* CAN_H */

