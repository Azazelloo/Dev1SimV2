/** @file l502_cdefs.h
    Файл определяет набор макросов для выполнения операций, зависящий от
    компилятора (VisualDSP или GCC) */

#ifndef L502_CDEFS_H
#define L502_CDEFS_H

/** Макрос для помещения переменной или функции в определенную секцию */
#ifdef __GNUC__
#define SECTION(sect, member)  member __attribute__((section(sect)))
#else
#define SECTION(sect, member)  section(sect) member
#endif

/** Марос для помещения переменной в неинициализируемую область SDRAM */
#ifdef __GNUC__
#define MEM_SDRAM_NOINIT(variable) SECTION(".sdram_noinit", variable)
#else
#define MEM_SDRAM_NOINIT(variable) section("sdram_noinit", NO_INIT) variable
#endif

/** Макрос для описания обработчика прерываний */
#ifdef __GNUC__
#define ISR(handler) __attribute__((interrupt_handler,nesting)) void handler(void)
#else
#define ISR(handler) EX_INTERRUPT_HANDLER(handler)
#endif

/** Макрос для регистрации обработчика прерываний */
#ifdef __GNUC__
#define REGISTER_ISR(ivg, isr) do { \
    int i=0; \
    ssync(); \
    *pEVT##ivg = isr; \
    ssync(); \
    asm volatile ("cli %0; bitset (%0, %1); sti %0; csync;": "+d"(i) : "i"(ivg)); \
    } while(0)
#else
#define REGISTER_ISR(ivg, isr) register_handler(ik_ivg##ivg, isr)
#endif



#endif // L502_CDEFS_H
