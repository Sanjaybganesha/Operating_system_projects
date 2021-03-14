#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include<linux/sched/signal.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/mm_types.h>
MODULE_LICENSE("GPL");

static int pid_mem = 1;//to get pid int value.
module_param(pid_mem, int, S_IRUGO);

static int print_mem(struct task_struct *task)
{
        unsigned long v =0;//virtual address
        unsigned long p=0;//physical address
        int count=0;
        //creating referencing
        struct pid *pid;
        struct mm_struct *mm;
        struct vm_area_struct *vma;
        struct task_struct *pid_struct;
        //five level page directories

        pgd_t *pgd;
        p4d_t *p4d;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *ptep,pte;

        pid = find_get_pid (pid_mem);
        pid_struct = pid_task(pid,PIDTYPE_PID );

        mm = task->mm;
        printk("\nThis mm_struct has %d vmas.\n", mm->map_count);

        for (vma = mm->mmap; vma; vma = vma->vm_next)
        {
        for(v = vma->vm_start; v < (vma->vm_end); v++){
                 pgd = pgd_offset(mm, v);
         

         
	 if (pgd_none(*pgd))
                   return 0;
         p4d=p4d_offset(pgd,v);
        
	 if (p4d_none(*p4d))
                return 0;
          pud=pud_offset(p4d,v);  
	 
	if (pud_none(*pud))
                 return 0;
         pmd=pmd_offset(pud,v);                                                                                                                                                       
//to check errors in the page tables.	
 	if (pgd_bad(*pgd)) {
                  pgd_ERROR(*pgd);
                  pgd_clear(pgd);
                  return 0;
         }

	if (p4d_bad(*p4d)) {
                p4d_ERROR(*p4d);
                p4d_clear(p4d);
                return 0;
         }

         if (pud_bad(*pud)) {
                  pud_ERROR(*pud);
                  pud_clear(pud);
                  return 0;

          }
	 


         ptep = pte_offset_map(pmd, v);
         if (!ptep)
                 return 0;
         pte = *ptep;
         if (pte_present(pte)) {
                p=(unsigned long)pte_pfn(pte);
         }
        }
        printk ("\nVma number %d is \n", ++count);
        printk("  from 0x%lx till 0x%lx\n", vma->vm_start, vma->vm_end);
        printk("Physical Address is 0x%lx",p);;
        }


return 0;
}


//initialization
int __init init_module(){
        struct task_struct *task;
        //checking process id
        printk("\n the pid is %d.\n", pid_mem);
        for_each_process(task) {
                if ( task->pid == pid_mem) {
                        printk("%s[%d]\n", task->comm, task->pid);
                        print_mem(task);
                }
        }
        return 0;
}
//exiting the module
void __exit cleanup_module()
{
                printk(KERN_INFO "job is done");
}

MODULE_DESCRIPTION("VIRTUAL ADDRESS ---> PHYSICAL ADDRESS\n");

