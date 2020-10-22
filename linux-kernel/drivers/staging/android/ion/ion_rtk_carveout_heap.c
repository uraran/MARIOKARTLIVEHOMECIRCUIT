/*
 * drivers/gpu/ion/ion_rtk_carveout_heap.c
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/bitops.h>
#include <linux/sched.h>
#include "ion.h"
#include "ion_priv.h"

#include "../uapi/rtk_phoenix_ion.h"
#include "../../../../arch/arm/mach-rtk119x/include/mach/memory.h"

struct ion_rtk_carveout_heap {
	struct ion_heap heap;
    struct mutex poollock;
    struct mutex pagelock;
    struct list_head pool_handlers;
    struct list_head page_handlers;
};

struct ION_RTK_CARVEOUT_POOL_INFO {
	struct gen_pool *gpool;
    ion_phys_addr_t base;
    size_t size;
    unsigned long flags;
    struct list_head list;
};

struct ION_RTK_CARVEOUT_PAGE_INFO {
    struct ION_RTK_CARVEOUT_POOL_INFO * pool_info;
    ion_phys_addr_t base;
    size_t size;
    unsigned long flags;
    char task_comm[TASK_COMM_LEN];
    pid_t pid;
    pid_t tgid;
    struct list_head list;
};

inline unsigned long poolFlag2ION (unsigned long flags)
{
    unsigned long out = 0;
    out |= (flags & RTK_FLAG_NONCACHED) ? ION_FLAG_NONCACHED    : 0;
    out |= (flags & RTK_FLAG_SCPUACC)   ? ION_FLAG_SCPUACC      : 0;
    out |= (flags & RTK_FLAG_ACPUACC)   ? ION_FLAG_ACPUACC      : 0;
    out |= (flags & RTK_FLAG_HWIPACC)   ? ION_FLAG_HWIPACC      : 0;
    return out;
}

void ion_rtk_carveout_show_list(struct ion_heap *heap)
{
	struct ion_rtk_carveout_heap *rtk_carveout_heap =
		container_of(heap, struct ion_rtk_carveout_heap, heap);

    struct ION_RTK_CARVEOUT_PAGE_INFO * page_info;
    struct ION_RTK_CARVEOUT_POOL_INFO * phandler;

    mutex_lock(&rtk_carveout_heap->poollock);
    list_for_each_entry(phandler, &rtk_carveout_heap->pool_handlers, list) {
        pr_info("[POOL] base=0x%08X size=0x%08X flags=0x%x\n",
                (unsigned int) phandler->base,
                (unsigned int) phandler->size,
                (unsigned int) phandler->flags);
        mutex_lock(&rtk_carveout_heap->pagelock);
        list_for_each_entry(page_info, &rtk_carveout_heap->page_handlers, list) {
            if (page_info->pool_info == phandler) {
                pr_info("  +--- [PAGE] base=0x%08X size=0x%08X flags=0x%x [%16.s] pids:%-5d tgid:%-5d\n",
                        (unsigned int) page_info->base,
                        (unsigned int) page_info->size,
                        (unsigned int) page_info->flags,
                        page_info->task_comm,
                        (unsigned int) page_info->pid,
                        (unsigned int) page_info->tgid);
            }
        }
        mutex_unlock(&rtk_carveout_heap->pagelock);
    }
	mutex_unlock(&rtk_carveout_heap->poollock);
}

__maybe_unused static int ion_carveout_heap_debug_show_list(struct ion_heap *heap, struct seq_file *s,
        void *unused)
{
	struct ion_rtk_carveout_heap *rtk_carveout_heap =
		container_of(heap, struct ion_rtk_carveout_heap, heap);

    struct ION_RTK_CARVEOUT_PAGE_INFO * page_info;
    struct ION_RTK_CARVEOUT_POOL_INFO * phandler;

    seq_printf(s,"[%s]\n", __func__);

    mutex_lock(&rtk_carveout_heap->poollock);
    list_for_each_entry(phandler, &rtk_carveout_heap->pool_handlers, list) {
        seq_printf(s,"\n[POOL] base=0x%08X size=0x%08X flags=0x%x\n",
                (unsigned int) phandler->base,
                (unsigned int) phandler->size,
                (unsigned int) phandler->flags);
        mutex_lock(&rtk_carveout_heap->pagelock);
        list_for_each_entry(page_info, &rtk_carveout_heap->page_handlers, list) {
            if (page_info->pool_info == phandler) {
                seq_printf(s,"  +--- [PAGE] base=0x%08X size=0x%08X flags=0x%x [%16.s] pids:%-5d tgid:%-5d\n",
                        (unsigned int) page_info->base,
                        (unsigned int) page_info->size,
                        (unsigned int) page_info->flags,
                        page_info->task_comm,
                        (unsigned int) page_info->pid,
                        (unsigned int) page_info->tgid);
            }
        }
        mutex_unlock(&rtk_carveout_heap->pagelock);
    }
	mutex_unlock(&rtk_carveout_heap->poollock);
    seq_printf(s,"\n");
    return 0;
}

#define POOL_FLAG_MISMATCH(flags,poolflag) ((flags & poolflag) != flags)

struct ION_RTK_CARVEOUT_PAGE_INFO * ion_rtk_carveout_allocate(struct ion_heap *heap,
				      unsigned long size,
				      unsigned long align, unsigned long flags)
{
	struct ion_rtk_carveout_heap *rtk_carveout_heap =
		container_of(heap, struct ion_rtk_carveout_heap, heap);

    struct ION_RTK_CARVEOUT_PAGE_INFO * page_info;
    struct ION_RTK_CARVEOUT_POOL_INFO * phandler;
    unsigned int matchCmp = 0;
    unsigned int compare;
    unsigned long tmp_flag = flags & RTK_ION_FLAG_MASK;
    unsigned long pool_flag;

    ion_phys_addr_t offset = 0;

    struct task_struct *task;
    pid_t pid, tgid;
    char task_comm[TASK_COMM_LEN];

    get_task_struct(current->group_leader);
    task_lock(current->group_leader);
    pid = task_pid_nr(current->group_leader);
    tgid = task_tgid_nr(current->group_leader);
    if (current->group_leader->flags & PF_KTHREAD)
        task = NULL;
    else
        task = current->group_leader;
    task_unlock(current->group_leader);

    if (task)
        get_task_comm(task_comm, task);
    else
        strncpy(task_comm, "KTHREAD", sizeof(task_comm));

    /* decrease usage count */
    put_task_struct(current->group_leader);

#if 1
    if (tmp_flag == 0) {
        pr_err(" Warning: flags is zero!! The default value is set RTK_ION_FLAG_MASK [%16.s] pids:%-5d tgid:%-5d\n",
                task_comm, pid, tgid);
        tmp_flag = RTK_ION_FLAG_MASK;
    }
#endif

REALLOC:
    mutex_lock(&rtk_carveout_heap->poollock);
    list_for_each_entry(phandler, &rtk_carveout_heap->pool_handlers, list) {
        if (phandler->size == 0)
            continue;

        pool_flag = poolFlag2ION(phandler->flags);

#if 0
        pr_info("tmp_flag=0x%x pool_flag=0x%x compare=0x%x matchCmp=0x%x\n",
                tmp_flag, pool_flag, (hweight32(pool_flag) - hweight32(tmp_flag)), matchCmp);
#endif

        if (POOL_FLAG_MISMATCH(tmp_flag, pool_flag))
            continue;

        compare = hweight32(pool_flag) - hweight32(tmp_flag);

        if (compare != matchCmp)
            continue;

        offset = gen_pool_alloc(phandler->gpool, size);

        if (offset != 0) {
            page_info = kzalloc(sizeof(struct ION_RTK_CARVEOUT_PAGE_INFO), GFP_KERNEL);
            if (!page_info) {
                gen_pool_free(phandler->gpool, offset, size);
                mutex_unlock(&rtk_carveout_heap->poollock);
                goto ERR;
            }
            page_info->pool_info = phandler;
            page_info->base = offset;
            page_info->size = size;
            page_info->flags = flags;
            page_info->pid = pid;
            page_info->tgid = tgid;
            strncpy(page_info->task_comm, task_comm, sizeof(page_info->task_comm));
            mutex_unlock(&rtk_carveout_heap->poollock);
            return page_info;
        }
    }
	mutex_unlock(&rtk_carveout_heap->poollock);

    if (matchCmp < (hweight32(RTK_ION_FLAG_MASK) - hweight32(tmp_flag))) {
        matchCmp++;
        goto REALLOC;
    }

ERR:
    pr_err("[%s] alloc failed! request size=0x%x align=0x%x flags=0x%x [%16.s] pids:%-5d tgid:%-5d\n",__func__,
            (unsigned int) size,
            (unsigned int) align,
            (unsigned int) flags,
            task_comm,
            (unsigned int) pid,
            (unsigned int) tgid);
    ion_rtk_carveout_show_list(heap);
    return NULL;
}

void ion_rtk_carveout_free(struct ion_heap *heap, ion_phys_addr_t addr,
		       unsigned long size)
{
	struct ion_rtk_carveout_heap *rtk_carveout_heap =
		container_of(heap, struct ion_rtk_carveout_heap, heap);

    struct ION_RTK_CARVEOUT_PAGE_INFO * page_info, * tmp_page_info;

	if (addr == ION_CARVEOUT_ALLOCATE_FAIL)
		return;

    mutex_lock(&rtk_carveout_heap->pagelock);
    list_for_each_entry_safe(page_info,  tmp_page_info, &rtk_carveout_heap->page_handlers, list) {
        if (page_info->base == addr) {
            if (page_info->size != size)
                pr_err("[%s] free size is mismatch! (alloc:%d free:%lu)\n", __func__, page_info->size, size);
            gen_pool_free(page_info->pool_info->gpool, page_info->base, page_info->size);
            list_del(&page_info->list);
            kfree(page_info);
            mutex_unlock(&rtk_carveout_heap->pagelock);
            return;
        }
    }
	mutex_unlock(&rtk_carveout_heap->pagelock);

    pr_err("[%s] free faile!\n",__func__);
}

static int ion_rtk_carveout_heap_phys(struct ion_heap *heap,
				  struct ion_buffer *buffer,
				  ion_phys_addr_t *addr, size_t *len)
{
	struct sg_table *table = buffer->priv_virt;
	struct page *page = sg_page(table->sgl);
	ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

	*addr = paddr;
	*len = buffer->size;
	return 0;
}

static int ion_rtk_carveout_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long size, unsigned long align,
				      unsigned long flags)
{
	struct ion_rtk_carveout_heap *rtk_carveout_heap =
		container_of(heap, struct ion_rtk_carveout_heap, heap);
	struct sg_table *table;
    struct ION_RTK_CARVEOUT_PAGE_INFO * page_info;
	int ret;

	if (align > PAGE_SIZE)
		return -EINVAL;

	table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto err_free;

	page_info = ion_rtk_carveout_allocate(heap, size, align, flags);

	if (page_info == NULL) {
		ret = -ENOMEM;
		goto err_free_table;
	}

    buffer->flags |= poolFlag2ION(page_info->pool_info->flags);

    buffer->flags |= (flags & ION_USAGE_MASK);

    if (buffer->flags & ION_USAGE_MMAP_NONCACHED) {
        buffer->flags |= ION_FLAG_CACHED;
        buffer->flags |= ION_FLAG_NONCACHED;
        buffer->flags &= ~ION_FLAG_CACHED_NEEDS_SYNC;
    } else if (buffer->flags & ION_USAGE_MMAP_WRITECOMBINE) {
        buffer->flags &= ~ION_FLAG_CACHED;
        buffer->flags &= ~ION_FLAG_NONCACHED;
        buffer->flags &= ~ION_FLAG_CACHED_NEEDS_SYNC;
    } else if (buffer->flags & ION_USAGE_MMAP_CACHED) {
        buffer->flags |= ION_FLAG_CACHED;
        buffer->flags &= ~ION_FLAG_NONCACHED;
        buffer->flags |= ION_FLAG_CACHED_NEEDS_SYNC;
    }

    if (    !(buffer->flags & ION_FLAG_SCPUACC) ||
            (buffer->flags & ION_USAGE_PROTECTED)) {
        buffer->flags |= ION_FLAG_CACHED;
        buffer->flags |= ION_FLAG_NONCACHED;
        buffer->flags &= ~ION_FLAG_CACHED_NEEDS_SYNC;
    }

    mutex_lock(&rtk_carveout_heap->pagelock);
    list_add(&page_info->list, &rtk_carveout_heap->page_handlers);
    mutex_unlock(&rtk_carveout_heap->pagelock);

	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(page_info->base)), page_info->size, 0);

	buffer->priv_virt = table;

	return 0;

err_free_table:
	sg_free_table(table);
err_free:
	kfree(table);
    ion_rtk_carveout_show_list(heap);
	return ret;
}

static void ion_rtk_carveout_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;
	struct sg_table *table = buffer->priv_virt;
	struct page *page = sg_page(table->sgl);
	ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

    if (    (buffer->flags & ION_FLAG_SCPUACC) &&
            !(buffer->flags & ION_USAGE_PROTECTED)) {
        ion_heap_buffer_zero(buffer);

        if (ion_buffer_cached(buffer))
            dma_sync_sg_for_device(NULL, table->sgl, table->nents,
                    DMA_BIDIRECTIONAL);
    }

	ion_rtk_carveout_free(heap, paddr, buffer->size);
	sg_free_table(table);
	kfree(table);
}

static int ion_carveout_heap_debug_show(struct ion_heap *heap, struct seq_file *s,
        void *unused)
{
    struct ion_rtk_carveout_heap *rtk_carveout_heap =
        container_of(heap, struct ion_rtk_carveout_heap, heap);
    struct ION_RTK_CARVEOUT_PAGE_INFO * page_info;
    struct ION_RTK_CARVEOUT_POOL_INFO * phandler;
    unsigned long offset, allocSize, MemSize;

    enum __mem_state {
        NONE,
        FREE,
        USED,
    };


    seq_printf(s,"\n");
    mutex_lock(&rtk_carveout_heap->poollock);
    list_for_each_entry(phandler, &rtk_carveout_heap->pool_handlers, list) {
        enum __mem_state mem_state = NONE;

        offset = phandler->base;
        allocSize = 0;

        mutex_lock(&rtk_carveout_heap->pagelock);

        list_for_each_entry(page_info, &rtk_carveout_heap->page_handlers, list) {
            if (page_info->pool_info == phandler)
                allocSize += page_info->size;
        }


        seq_printf(s,           "   [POOL] base=0x%08X size=0x%08X flags=0x%-2x Free: %lu kB  Usage:%lu%%\n",
                (unsigned int) phandler->base,
                (unsigned int) phandler->size,
                (unsigned int) phandler->flags,
                (phandler->size - allocSize) / 1024,
                (allocSize * 100)/phandler->size);

        MemSize = 0;

        seq_printf(s,                   "       +-------------------+ (0x%08x)\n", (unsigned int) phandler->base);
        while (offset <= (phandler->base + phandler->size)) {
            enum __mem_state new_mem_state = NONE;
            struct ION_RTK_CARVEOUT_PAGE_INFO * find_page_info = NULL;
            list_for_each_entry(page_info, &rtk_carveout_heap->page_handlers, list) {
                if (page_info->pool_info == phandler && offset <= page_info->base) {
                    if (find_page_info == NULL)
                        find_page_info = page_info;
                    else if (find_page_info->base > page_info->base)
                        find_page_info = page_info;
                }
            }

            if (find_page_info == NULL) {
                if (offset == phandler->base) {
                    MemSize = phandler->size;
                    offset = phandler->base + phandler->size;
                }

                seq_printf(s,           "       | %s : %10lu |\n",
                        (mem_state == USED) ? "used" : "free",
                        MemSize);

                if (offset != (phandler->base + phandler->size))
                    seq_printf(s,       "       | %s : %10lu |\n",
                            "free",
                            (phandler->base + phandler->size) - offset);

                break;
            }

            new_mem_state   = (offset == find_page_info->base) ? USED : FREE;

            if (mem_state == NONE) {
                mem_state   = new_mem_state;
                MemSize     = 0;
            }

            if (new_mem_state != mem_state) {
                seq_printf(s,           "       | %s : %10lu |\n",
                        (mem_state == USED) ? "used" : "free",
                        MemSize);
                mem_state   = new_mem_state;
                MemSize = 0;
            }

            if (new_mem_state == USED) {
                MemSize += find_page_info->size;
                offset = find_page_info->base + find_page_info->size;
            } else {
                MemSize += find_page_info->base - offset;
                offset = find_page_info->base;
            }
        }
        seq_printf(s,                   "       +-------------------+ (0x%08x)\n\n",
                (unsigned int) phandler->base + phandler->size - 1);

        mutex_unlock(&rtk_carveout_heap->pagelock);
    }
	mutex_unlock(&rtk_carveout_heap->poollock);
    seq_printf(s,"\n");
    //return ion_carveout_heap_debug_show_list(heap, s, unused);
    return 0;
}

static struct sg_table *ion_rtk_carveout_heap_map_dma(struct ion_heap *heap,
						  struct ion_buffer *buffer)
{
	return buffer->priv_virt;
}

static void ion_rtk_carveout_heap_unmap_dma(struct ion_heap *heap,
					struct ion_buffer *buffer)
{
	return;
}

static struct ion_heap_ops rtk_carveout_heap_ops = {
	.allocate = ion_rtk_carveout_heap_allocate,
	.free = ion_rtk_carveout_heap_free,
	.phys = ion_rtk_carveout_heap_phys,
	.map_dma = ion_rtk_carveout_heap_map_dma,
	.unmap_dma = ion_rtk_carveout_heap_unmap_dma,
	.map_user = ion_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
};

struct ion_heap *ion_rtk_carveout_heap_create(struct ion_platform_heap *heap_data)
{
    struct ion_rtk_carveout_heap *rtk_carveout_heap;
    struct list_head * pool_list = (struct list_head *) heap_data->priv;
    struct ion_rtk_priv_pool * pool;
    struct ION_RTK_CARVEOUT_POOL_INFO * phandler;
    struct gen_pool *gpool;
    struct page * page;
    int ret = 0;

    if (!pool_list) {
        printk("[%s] pool_list is NULL!!!!\n",__func__);
        return ERR_PTR(-ENOMEM);
    }

    rtk_carveout_heap = kzalloc(sizeof(struct ion_rtk_carveout_heap), GFP_KERNEL);

    if (!rtk_carveout_heap)
        return ERR_PTR(-ENOMEM);

    mutex_init(&rtk_carveout_heap->poollock);
    mutex_init(&rtk_carveout_heap->pagelock);

    INIT_LIST_HEAD(&rtk_carveout_heap->pool_handlers);
    INIT_LIST_HEAD(&rtk_carveout_heap->page_handlers);

    list_for_each_entry(pool, pool_list, list) {
        page = pfn_to_page(PFN_DOWN(pool->base));

        if (pool->flags & RTK_FLAG_SCPUACC) {
            ion_pages_sync_for_device(NULL, page, pool->size, DMA_BIDIRECTIONAL);

            ret = ion_heap_pages_zero(page, pool->size, pgprot_writecombine(PAGE_KERNEL));
        }

        if (ret)
            goto ERROR;

        gpool = gen_pool_create(PAGE_SHIFT, -1);

        if (!gpool) {
            printk("[%s] gen_pool_create failed! \n",__func__);
            ret = -ENOMEM;
            goto ERROR;
        }

        gen_pool_add(gpool, pool->base, pool->size, -1);

        phandler = kzalloc(sizeof(struct ION_RTK_CARVEOUT_POOL_INFO), GFP_KERNEL);
        phandler->gpool = gpool;
        phandler->base = pool->base;
        phandler->size = pool->size;
        phandler->flags = pool->flags;
        mutex_lock(&rtk_carveout_heap->poollock);
        list_add(&phandler->list, &rtk_carveout_heap->pool_handlers);
        mutex_unlock(&rtk_carveout_heap->poollock);
    }

    rtk_carveout_heap->heap.ops = &rtk_carveout_heap_ops;
    rtk_carveout_heap->heap.type = ION_HEAP_TYPE_CARVEOUT;
    rtk_carveout_heap->heap.flags = 0;//ION_HEAP_FLAG_DEFER_FREE;
    rtk_carveout_heap->heap.debug_show = ion_carveout_heap_debug_show;

    //ion_rtk_carveout_show_list(&rtk_carveout_heap->heap);
    return &rtk_carveout_heap->heap;

ERROR:
    kfree(rtk_carveout_heap);
    return ERR_PTR(ret);
}

void ion_rtk_carveout_heap_destroy(struct ion_heap *heap)
{
	struct ion_rtk_carveout_heap *rtk_carveout_heap =
	     container_of(heap, struct  ion_rtk_carveout_heap, heap);

    struct ION_RTK_CARVEOUT_PAGE_INFO * page_info, * tmp_page_info;

    struct ION_RTK_CARVEOUT_POOL_INFO * phandler, * tmp_phandler;

    ion_rtk_carveout_show_list(heap);

    mutex_lock(&rtk_carveout_heap->pagelock);
    list_for_each_entry_safe(page_info,  tmp_page_info, &rtk_carveout_heap->page_handlers, list) {
        gen_pool_free(page_info->pool_info->gpool, page_info->base, page_info->size);
        list_del(&page_info->list);
        kfree(page_info);
    }
	mutex_unlock(&rtk_carveout_heap->pagelock);

    mutex_lock(&rtk_carveout_heap->poollock);
    list_for_each_entry_safe(phandler, tmp_phandler, &rtk_carveout_heap->pool_handlers, list) {
        gen_pool_destroy(phandler->gpool);
        list_del(&phandler->list);
        kfree(phandler);
    }
	mutex_unlock(&rtk_carveout_heap->poollock);

	kfree(rtk_carveout_heap);
	rtk_carveout_heap = NULL;
}
