---
name: interrupts-and-exceptions-baremetal
description: Bare-metal interrupt and exception skill. Use when writing ISRs, configuring interrupt controllers/priorities, handling fault/abort exceptions, or measuring interrupt latency. Activates on queries about ISR, vector table, interrupt controller, data abort, or interrupt priority.
---

# Interrupts and Exceptions (Bare-Metal)

## Purpose

Guide agents through bare-metal interrupt handling: interrupt controller configuration, ISR writing rules, exception handlers, priorities, nesting, and latency considerations. (Reference examples below are Cortex-M NVIC; see Better-Rockpod notes for classic-ARM mapping.)

## When to Use

- Configuring peripheral IRQ priorities
- Writing ISRs that must not block
- Debugging faults/aborts after enabling interrupts
- Sharing data between ISR and main loop
- Optimizing interrupt latency

## Workflow

### 1. NVIC overview (Cortex-M)

```
Exception / IRQ flow
├── NVIC receives IRQ (priority compare with BASEPRI/PRIMask)
├── Stacking: automatic save r0-r3, r12, lr, pc, psr
├── Branch to handler from vector table
├── Handler runs (should be short)
└── Unstack and return — tail-chain if another IRQ pending
```

### 2. Enable and prioritize an IRQ

```c
#include "stm32f4xx.h"  /* CMSIS device header */

void uart_irq_init(void) {
    NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2, 0));
    NVIC_EnableIRQ(USART2_IRQn);
}
```

Priority: lower numeric value = higher urgency (on most Cortex-M implementations). Check vendor docs for grouping bits.

### 3. ISR template

```c
void USART2_IRQHandler(void) {
    if (USART2->SR & USART_SR_RXNE) {
        uint8_t b = (uint8_t)USART2->DR;  /* read clears RXNE */
        ringbuf_push(b);
    }
    if (USART2->SR & USART_SR_ORE) {
        (void)USART2->DR;  /* clear overrun */
    }
}
```

ISR rules:
- No blocking calls (`printf`, `malloc`, long loops)
- Minimize work — defer to main via flag/ring buffer
- Clear interrupt flags per datasheet (read-to-clear vs write-1-clear)

### 4. Critical sections

```c
uint32_t primask = __get_PRIMASK();
__disable_irq();
/* atomic section */
__set_PRIMASK(primask);
```

### 5. Latency

| Factor | Impact |
|--------|--------|
| Higher priority IRQ | Preempts lower |
| Long ISRs | Starves other IRQs and main |
| Long critical sections | Priority inversion / missed deadlines |

## Common Problems

| Symptom | Cause | Fix |
|---------|-------|-----|
| IRQ never fires | Controller not enabled or IRQ masked | Enable at controller AND peripheral |
| Spurious re-entry | Flag not cleared | Clear per datasheet (read-to-clear vs W1C) |
| Fault in ISR | Stack overflow | Check IRQ stack size |
| Lost bytes | ISR too slow | Ring buffer + shorter ISR |
| Priority inversion | Long critical section | Shorten disabled-IRQ window |

## Better-Rockpod notes

- iPods are classic ARM, NOT Cortex-M: no NVIC, no automatic stacking. IRQ
  entry goes through the ARM vector table (`firmware/target/arm/crt0.S` and
  per-SoC `system-target.h` / `system-*.c`). PP5022 uses the PortalPlayer
  interrupt controller (`CPU_INT_EN` etc. in `pp5020.h`); S5L8702 uses a
  VIC (`firmware/target/arm/s5l8702/`).
- Rockbox critical sections: `disable_irq_save()` / `restore_irq()` from
  `firmware/export/system.h` — use those, not CMSIS intrinsics.
- Rockbox threads are cooperative; ISRs typically post to a queue
  (`queue_post`) or set flags consumed by threads.
- The ISR rules, flag-clearing table, and latency reasoning above apply as-is.
