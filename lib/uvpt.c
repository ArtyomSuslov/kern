/* User virtual page table helpers */

#include <inc/lib.h>
#include <inc/mmu.h>

extern volatile pte_t uvpt[];     /* VA of "virtual page table" */
extern volatile pde_t uvpd[];     /* VA of current page directory */
extern volatile pdpe_t uvpdp[];   /* VA of current page directory pointer */
extern volatile pml4e_t uvpml4[]; /* VA of current page map level 4 */

pte_t
get_uvpt_entry(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P)) return uvpml4[VPML4(va)];
    if (!(uvpdp[VPDP(va)] & PTE_P) || (uvpdp[VPDP(va)] & PTE_PS)) return uvpdp[VPDP(va)];
    if (!(uvpd[VPD(va)] & PTE_P) || (uvpd[VPD(va)] & PTE_PS)) return uvpd[VPD(va)];
    return uvpt[VPT(va)];
}

uintptr_t
get_phys_addr(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P))
        return -1;
    if (!(uvpdp[VPDP(va)] & PTE_P))
        return -1;
    if (uvpdp[VPDP(va)] & PTE_PS)
        return PTE_ADDR(uvpdp[VPDP(va)]) + ((uintptr_t)va & ((1ULL << PDP_SHIFT) - 1));
    if (!(uvpd[VPD(va)] & PTE_P))
        return -1;
    if ((uvpd[VPD(va)] & PTE_PS))
        return PTE_ADDR(uvpd[VPD(va)]) + ((uintptr_t)va & ((1ULL << PD_SHIFT) - 1));
    if (!(uvpt[VPT(va)] & PTE_P))
        return -1;
    return PTE_ADDR(uvpt[VPT(va)]) + PAGE_OFFSET(va);
}

int
get_prot(void *va) {
    pte_t pte = get_uvpt_entry(va);
    int prot = pte & PTE_AVAIL & ~PTE_SHARE;
    if (pte & PTE_P) prot |= PROT_R;
    if (pte & PTE_W) prot |= PROT_W;
    if (!(pte & PTE_NX)) prot |= PROT_X;
    if (pte & PTE_SHARE) prot |= PROT_SHARE;
    return prot;
}

bool
is_page_dirty(void *va) {
    pte_t pte = get_uvpt_entry(va);
    return pte & PTE_D;
}

bool
is_page_present(void *va) {
    return get_uvpt_entry(va) & PTE_P;
}

int
foreach_shared_region(int (*fun)(void *start, void *end, void *arg), void *arg) {
    /* Calls fun() for every shared region.
     * NOTE: Skip over larger pages/page directories for efficiency */
    // LAB 11: Your code here:

    int res = 0;

    // Обрабатываем каждый уровень таблиц
    for (uintptr_t pml4_addr = 0; pml4_addr < MAX_USER_ADDRESS; pml4_addr += 1ULL << PML4_SHIFT) {
        if (!(uvpml4[VPML4(pml4_addr)] & PTE_P)) {
            continue; // Пропускаем, если PTE не установлен
        }

        for (uintptr_t pdp_addr = 0; pdp_addr < 1ULL << PML4_SHIFT; pdp_addr += 1ULL << PDP_SHIFT) {
            if (!(uvpdp[VPDP(pml4_addr + pdp_addr)] & PTE_P)) {
                continue; // Пропускаем, если PTE не установлен
            }

            for (uintptr_t pd_addr = 0; pd_addr < 1ULL << PDP_SHIFT; pd_addr += 1ULL << PD_SHIFT) {
                if (!(uvpd[VPD(pml4_addr + pdp_addr + pd_addr)] & PTE_P)) {
                    continue; // Пропускаем, если PTE не установлен
                }

                for (uintptr_t pt_addr = 0; pt_addr < 1ULL << PD_SHIFT; pt_addr += 1ULL << PT_SHIFT) {
                    uintptr_t addr = pml4_addr + pdp_addr + pd_addr + pt_addr;

                    // Проверяем, является ли запись страницы действительной и разделяемой
                    if (uvpt[VPT(addr)] & PTE_P && uvpt[VPT(addr)] & PTE_SHARE) {
                        // Вызываем функцию для обработки региона памяти
                        res = fun((void *)addr, (void *)(addr + PAGE_SIZE), arg);
                    }
                }
            }
        }
    }

    return res;
}