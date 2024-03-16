#include <stdio.h>
#include "mcu.h"
#include "mcu_interrupt.h"

void MCU_Interrupt_Start(int32_t mask)
{
    MCU_PushStack(mcu.pc);
    MCU_PushStack(mcu.cp);
    MCU_PushStack(mcu.sr);
    mcu.sr &= ~STATUS_T;
    if (mask >= 0)
    {
        mcu.sr &= ~STATUS_INT_MASK;
        mcu.sr |= mask << 8;
    }
    mcu.sleep = 0;
}

void MCU_Interrupt_SetRequest(uint32_t interrupt, uint32_t value)
{
    mcu.interrupt_pending[interrupt] = value;
}

void MCU_Interrupt_Exception(uint32_t exception)
{
#if 0
    if (interrupt == INTERRUPT_SOURCE_IRQ0 && (dev_register[DEV_P1CR] & 0x20) == 0)
        return;
    if (interrupt == INTERRUPT_SOURCE_IRQ1 && (dev_register[DEV_P1CR] & 0x40) == 0)
        return;
#endif
    mcu.exception_pending = exception;
}

void MCU_Interrupt_TRAPA(uint32_t vector)
{
    mcu.trapa_pending[vector] = 1;
}

void MCU_Interrupt_StartVector(uint32_t vector, int32_t mask)
{
    uint32_t address = MCU_GetVectorAddress(vector);
    MCU_Interrupt_Start(mask);
    mcu.cp = address >> 16;
    mcu.pc = address;
}

void MCU_Interrupt_Handle(void)
{
#if 0
    if (mcu.cycles % 2000 == 0 && mcu.sleep)
    {
        MCU_Interrupt_StartVector(VECTOR_INTERNAL_INTERRUPT_94);
        return;
    }
    if (mcu.cycles % 2000 == 1000 && mcu.sleep)
    {
        MCU_Interrupt_StartVector(VECTOR_INTERNAL_INTERRUPT_A4);
        return;
    }
    if (mcu.cycles % 2000 == 1500 && mcu.sleep)
    {
        MCU_Interrupt_StartVector(VECTOR_INTERNAL_INTERRUPT_B4);
        return;
    }
#endif
    uint32_t i;
    for (i = 0; i < 16; i++)
    {
        if (mcu.trapa_pending[i])
        {
            mcu.trapa_pending[i] = 0;
            MCU_Interrupt_StartVector(VECTOR_TRAPA_0 + i, -1);
            return;
        }
    }
    if (mcu.exception_pending >= 0)
    {
        switch (mcu.exception_pending)
        {
            case EXCEPTION_SOURCE_ADDRESS_ERROR:
                MCU_Interrupt_StartVector(VECTOR_ADDRESS_ERROR, -1);
                break;
            case EXCEPTION_SOURCE_INVALID_INSTRUCTION:
                MCU_Interrupt_StartVector(VECTOR_INVALID_INSTRUCTION, -1);
                break;
            case EXCEPTION_SOURCE_TRACE:
                MCU_Interrupt_StartVector(VECTOR_TRACE, -1);
                break;

        }
        mcu.exception_pending = -1;
        return;
    }
    if (mcu.interrupt_pending[INTERRUPT_SOURCE_NMI])
    {
        // mcu.interrupt_pending[INTERRUPT_SOURCE_NMI] = 0;
        MCU_Interrupt_StartVector(VECTOR_NMI, 7);
        return;
    }
    uint32_t mask = (mcu.sr >> 8) & 7;
    for (i = INTERRUPT_SOURCE_NMI + 1; i < INTERRUPT_SOURCE_MAX; i++)
    {
        int32_t vector = -1;
        int32_t level = 0;
        if (!mcu.interrupt_pending[i])
            continue;
        switch (i)
        {
            case INTERRUPT_SOURCE_IRQ0:
                if ((dev_register[DEV_P1CR] & 0x20) == 0)
                    continue;
                vector = VECTOR_IRQ0;
                level = (dev_register[DEV_IPRA] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_IRQ1:
                if ((dev_register[DEV_P1CR] & 0x40) == 0)
                    continue;
                vector = VECTOR_IRQ1;
                level = (dev_register[DEV_IPRA] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_FRT0_OCIA:
                vector = VECTOR_INTERNAL_INTERRUPT_94;
                level = (dev_register[DEV_IPRB] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_FRT0_OCIB:
                vector = VECTOR_INTERNAL_INTERRUPT_98;
                level = (dev_register[DEV_IPRB] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_FRT0_FOVI:
                vector = VECTOR_INTERNAL_INTERRUPT_9C;
                level = (dev_register[DEV_IPRB] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_FRT1_OCIA:
                vector = VECTOR_INTERNAL_INTERRUPT_A4;
                level = (dev_register[DEV_IPRB] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_FRT1_OCIB:
                vector = VECTOR_INTERNAL_INTERRUPT_A8;
                level = (dev_register[DEV_IPRB] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_FRT1_FOVI:
                vector = VECTOR_INTERNAL_INTERRUPT_AC;
                level = (dev_register[DEV_IPRB] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_FRT2_OCIA:
                vector = VECTOR_INTERNAL_INTERRUPT_B4;
                level = (dev_register[DEV_IPRC] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_FRT2_OCIB:
                vector = VECTOR_INTERNAL_INTERRUPT_B8;
                level = (dev_register[DEV_IPRC] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_FRT2_FOVI:
                vector = VECTOR_INTERNAL_INTERRUPT_BC;
                level = (dev_register[DEV_IPRC] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_TIMER_CMIA:
                vector = VECTOR_INTERNAL_INTERRUPT_C0;
                level = (dev_register[DEV_IPRC] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_TIMER_CMIB:
                vector = VECTOR_INTERNAL_INTERRUPT_C4;
                level = (dev_register[DEV_IPRC] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_TIMER_OVI:
                vector = VECTOR_INTERNAL_INTERRUPT_C8;
                level = (dev_register[DEV_IPRC] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_ANALOG:
                vector = VECTOR_INTERNAL_INTERRUPT_E0;
                level = (dev_register[DEV_IPRD] >> 0) & 7;
                break;
            default:
                break;
        }
        if (mask < level)
        {
            // mcu.interrupt_pending[INTERRUPT_SOURCE_NMI] = 0;
            MCU_Interrupt_StartVector(vector, level);
            return;
        }
    }
}