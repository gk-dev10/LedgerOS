#include "vmm.h"
#include "pmm.h"
#include "../memory.h"
#include "../console/console.h"

static uint64_t *kernel_pml4 = NULL;

void vmm_init(void) {
    kernel_pml4 = (uint64_t *)pmm_alloc_page();
    if (kernel_pml4) {
        console_printf("[VMM] Initialized Virtual Memory Manager (PML4 Base: 0x%x)\n", (uint32_t)(uintptr_t)kernel_pml4);
    }
}

bool vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    if (!kernel_pml4) return false;

    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = NULL;
    if (!(kernel_pml4[pml4_idx] & VMM_FLAG_PRESENT)) {
        pdpt = (uint64_t *)pmm_alloc_page();
        if (!pdpt) return false;
        kernel_pml4[pml4_idx] = (uint64_t)(uintptr_t)pdpt | VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE | flags;
    } else {
        pdpt = (uint64_t *)(uintptr_t)(kernel_pml4[pml4_idx] & ~0xFFFULL);
    }

    uint64_t *pd = NULL;
    if (!(pdpt[pdpt_idx] & VMM_FLAG_PRESENT)) {
        pd = (uint64_t *)pmm_alloc_page();
        if (!pd) return false;
        pdpt[pdpt_idx] = (uint64_t)(uintptr_t)pd | VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE | flags;
    } else {
        pd = (uint64_t *)(uintptr_t)(pdpt[pdpt_idx] & ~0xFFFULL);
    }

    uint64_t *pt = NULL;
    if (!(pd[pd_idx] & VMM_FLAG_PRESENT)) {
        pt = (uint64_t *)pmm_alloc_page();
        if (!pt) return false;
        pd[pd_idx] = (uint64_t)(uintptr_t)pt | VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE | flags;
    } else {
        pt = (uint64_t *)(uintptr_t)(pd[pd_idx] & ~0xFFFULL);
    }

    pt[pt_idx] = (phys & ~0xFFFULL) | VMM_FLAG_PRESENT | flags;
    return true;
}
