/** @file l502_cdefs.h
    ���� ���������� ����� �������� ��� ���������� ��������, ��������� ��
    ����������� (VisualDSP ��� GCC) */

#ifndef L502_CDEFS_H
#define L502_CDEFS_H

/** ������ ��� ��������� ���������� ��� ������� � ������������ ������ */
#ifdef __GNUC__
#define SECTION(sect, member)  member __attribute__((section(sect)))
#else
#define SECTION(sect, member)  section(sect) member
#endif

/** ����� ��� ��������� ���������� � ������������������ ������� SDRAM */
#ifdef __GNUC__
#define MEM_SDRAM_NOINIT(variable) SECTION(".sdram_noinit", variable)
#else
#define MEM_SDRAM_NOINIT(variable) section("sdram_noinit", NO_INIT) variable
#endif

/** ������ ��� �������� ����������� ���������� */
#ifdef __GNUC__
#define ISR(handler) __attribute__((interrupt_handler,nesting)) void handler(void)
#else
#define ISR(handler) EX_INTERRUPT_HANDLER(handler)
#endif

/** ������ ��� ����������� ����������� ���������� */
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
