#ifndef BEEP_H_
#define BEEP_H_

/** Beep buzzer for a defined delay in second. Process will wait_clock */
void beep(int freq, float delay);

/** Beep buzzer for a defined delay in second. Process will not wait (async) */
void async_beep(int freq, float delay);
/** Stop async beep with clock interrupt */
void check_buzzer(void);

#endif /* BEEP_H_*/
