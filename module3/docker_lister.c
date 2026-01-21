#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/pid_namespace.h>
#include <linux/nsproxy.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <net/net_namespace.h>

#define MODULE_NAME "docker_lister"
#define MAX_CONTAINER_ID_LEN 64
#define MAX_PATH_LEN 256
#define MAX_CONTAINERS 50

struct container_info {
    unsigned int ns_inum; 
    pid_t init_pid;
    char container_id[MAX_CONTAINER_ID_LEN];
    char comm[TASK_COMM_LEN];
    pid_t ppid;
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cloud Society");
MODULE_DESCRIPTION("Simple Docker Container Lister Kernel Module");
MODULE_VERSION("1.0");

static bool is_docker_container_process(struct task_struct *task)
{
    if (!task || !task->nsproxy)
        return false;
    
    if (task->nsproxy->pid_ns_for_children == &init_pid_ns)
        return false;
    
    if (task->nsproxy->net_ns == &init_net)
        return false;
        
    return true;
}


static int extract_container_id(struct task_struct *task, char *container_id)
{
    if (task && task->nsproxy && task->nsproxy->pid_ns_for_children) {
        unsigned int ns_inum = task->nsproxy->pid_ns_for_children->ns.inum;
        snprintf(container_id, MAX_CONTAINER_ID_LEN, "%.8x%.4x", 
                 ns_inum, task->pid & 0xFFFF);
        return 0;
    }
    
    return -1;
}

static void list_docker_containers(void)
{
    struct task_struct *task;
    struct container_info containers[MAX_CONTAINERS];
    int container_count = 0;
    char container_id[MAX_CONTAINER_ID_LEN];
    int i;
    
    printk(KERN_INFO "%s: Iniciando escaneo...\n", MODULE_NAME);
    printk(KERN_INFO "%s: %-12s %-8s %-20s %-8s\n", 
           MODULE_NAME, "C_ID", "PID", "COMM", "PPID");
    printk(KERN_INFO "%s: %s\n", MODULE_NAME, 
           "----------------------------------------------------------");
    
    memset(containers, 0, sizeof(containers));
    
    rcu_read_lock();
    
    for_each_process(task) {
        if (is_docker_container_process(task)) {
            unsigned int ns_inum = task->nsproxy->pid_ns_for_children->ns.inum;
            bool found = false;
            
            for (i = 0; i < container_count; i++) {
                if (containers[i].ns_inum == ns_inum) {
                    found = true;
                    if (task->pid < containers[i].init_pid) {
                        containers[i].init_pid = task->pid;
                        strncpy(containers[i].comm, task->comm, TASK_COMM_LEN);
                        containers[i].ppid = task->parent ? task->parent->pid : 0;
                        extract_container_id(task, containers[i].container_id);
                    }
                    break;
                }
            }
            
            if (!found && container_count < MAX_CONTAINERS) {
                containers[container_count].ns_inum = ns_inum;
                containers[container_count].init_pid = task->pid;
                strncpy(containers[container_count].comm, task->comm, TASK_COMM_LEN);
                containers[container_count].ppid = task->parent ? task->parent->pid : 0;
                extract_container_id(task, containers[container_count].container_id);
                container_count++;
            }
        }
    }
    
    rcu_read_unlock();
    
    for (i = 0; i < container_count; i++) {
        printk(KERN_INFO "%s: %-12s %-8d %-20s %-8d\n",
               MODULE_NAME,
               containers[i].container_id,
               containers[i].init_pid,
               containers[i].comm,
               containers[i].ppid);
    }
    
    printk(KERN_INFO "%s: %s\n", MODULE_NAME,
           "----------------------------------------------------------");
    printk(KERN_INFO "%s: Total encontrados: %d\n", MODULE_NAME, container_count);
    
    if (container_count == 0) {
        printk(KERN_INFO "%s: No hay nada corriendo\n", MODULE_NAME);
    }
}

static int __init docker_lister_init(void)
{
    printk(KERN_INFO "%s: Módulo cargado\n", MODULE_NAME);
    printk(KERN_INFO "%s: ========================================\n", MODULE_NAME);
    
    list_docker_containers();
    
    printk(KERN_INFO "%s: ========================================\n", MODULE_NAME);
    printk(KERN_INFO "%s: Módulo inicializado correctamente\n", MODULE_NAME);
    
    return 0;
}

static void __exit docker_lister_exit(void)
{
    printk(KERN_INFO "%s: Módulo descargado\n", MODULE_NAME);
}

module_init(docker_lister_init);
module_exit(docker_lister_exit);
