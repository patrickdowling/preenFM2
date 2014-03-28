#ifndef DWT_H_
#define DWT_H_

#define REG_DWT_CONTROL 0xE0001000
#define REG_DWT_CYCCNT 0xE0001004
#define REG_SCB_DEMCR 0xE000EDFC

#define DWT_CONTROL ((volatile unsigned int *)REG_DWT_CONTROL)
#define DWT_CYCCNT ((volatile unsigned int *)REG_DWT_CYCCNT)
#define SCB_DEMCT ((volatile unsigned int *)REG_SCB_DEMCR)

#define RESET_DWT_CYCCNT()			\
  do {						\
  *SCB_DEMCT = *SCB_DEMCT | 0x01000000;		\
  *DWT_CYCCNT = 0;				\
  *DWT_CONTROL = *DWT_CONTROL | 1;		\
  } while ( 0 )

#define READ_DWT_CYCCNT()			\
  *(DWT_CYCCNT)

#endif /* DWT_H_ */
