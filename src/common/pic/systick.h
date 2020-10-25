/* 
 * File:   systick.h
 * Author: maazan34255
 *
 * Created on 28 oktober 2016, 22:20
 */

#ifndef SYSTICK_H
#define	SYSTICK_H

void systick_initialize(void);
void systick_register(void (*Callback)(void));
void systick_service(void);


#endif	/* SYSTICK_H */

