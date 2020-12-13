/* 
 * File:   vscp4hass.h
 * Author: maarten
 *
 * Created on December 6, 2020, 4:15 PM
 */

#ifndef VSCP4HASS_H
#define	VSCP4HASS_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdint.h>
    
typedef struct {
    uint8_t vscp_class;
    uint8_t vscp_event_on;
    uint8_t vscp_event_off;
}vscp4hass_bs_class_t;
    
#define VSCP4HASS_BS_MAX_CLASS_ID 0x17
    
extern const vscp4hass_bs_class_t vscp4hass_bs_class_map[];


#ifdef	__cplusplus
}
#endif

#endif	/* VSCP4HASS_H */

