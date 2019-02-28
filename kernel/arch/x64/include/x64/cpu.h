#ifndef __X64_CPU_H__
#define __X64_CPU_H__

#include "apic.h"
#include "gdt.h"
#include "idt.h"
#include "tss.h"

struct thread;
struct cpuvar {
    struct thread *idle_thread;
    struct gdtr gdtr;
    struct gdtr idtr;
    struct tss tss;
    uint64_t ioapic;
    uint64_t gdt[GDT_DESC_NUM];
    struct idt_entry idt[IDT_DESC_NUM];
};

#endif
