/* 
 * File:   discrete.h
 * Author: maazan34255
 *
 * Created on 12 september 2017, 11:24
 */

#ifndef DISCRETE_H
#define	DISCRETE_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif
uint8_t discrete_read(uint8_t id);
void discrete_write(uint8_t id, uint8_t value);

#define GREEN_LED_ID 255
#define PUSHBUTTON_ID 255

#ifdef	__cplusplus
}
#endif

#endif	/* DISCRETE_H */

