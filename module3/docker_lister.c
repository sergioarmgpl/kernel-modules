/*
 * Docker Container Lister Kernel Module
 * 
 * Este módulo simple lista todos los contenedores Docker corriendo en el sistema
 * cuando es cargado. La información se muestra en los logs del kernel
 * (dmesg).
 */

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

#define MODULE_NAME "docker_lister"
#define MAX_CONTAINER_ID_LEN 64
#define MAX_PATH_LEN 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cloud Society");
MODULE_DESCRIPTION("Simple Docker Container Lister Kernel Module");
MODULE_VERSION("1.0");

/*
 * Función para verificar si un proceso está en un contenedor Docker
 * examina el cgroup del proceso buscando patrones de Docker
 */
static bool is_docker_container_process(struct task_struct *task)
{
    char cgroup_path[MAX_PATH_LEN];
    struct file *file;
    char *buf;
    loff_t pos = 0;
    ssize_t bytes_read;
    bool is_docker = false;
    
    if (!task || !task->nsproxy)
        return false;
    
    /* Verificar si tiene namespace PID diferente al init */
    if (task->nsproxy->pid_ns_for_children == &init_pid_ns)
        return false;
    
    /* Intentar leer el archivo cgroup del proceso */
    snprintf(cgroup_path, sizeof(cgroup_path), "/proc/%d/cgroup", task->pid);
    
    file = filp_open(cgroup_path, O_RDONLY, 0);
    if (IS_ERR(file))
        return false;
    
    buf = kmalloc(1024, GFP_KERNEL);
    if (!buf) {
        filp_close(file, NULL);
        return false;
    }
    
    /* Leer contenido del archivo cgroup */
    bytes_read = kernel_read(file, buf, 1023, &pos);
    if (bytes_read > 0) {
        buf[bytes_read] = '\0';
        /* Buscar patrones típicos de Docker en cgroups */
        if (strstr(buf, "/docker/") || strstr(buf, "docker-") || 
            strstr(buf, ".scope") || strstr(buf, "container")) {
            is_docker = true;
        }
    }
    
    kfree(buf);
    filp_close(file, NULL);
    return is_docker;
}

/*
 * Función para extraer ID del contenedor Docker del cgroup
 */
static int extract_container_id(struct task_struct *task, char *container_id)
{
    char cgroup_path[MAX_PATH_LEN];
    struct file *file;
    char *buf;
    char *docker_pos, *id_start, *id_end;
    loff_t pos = 0;
    ssize_t bytes_read;
    int ret = -1;
    
    snprintf(cgroup_path, sizeof(cgroup_path), "/proc/%d/cgroup", task->pid);
    
    file = filp_open(cgroup_path, O_RDONLY, 0);
    if (IS_ERR(file))
        return -1;
    
    buf = kmalloc(1024, GFP_KERNEL);
    if (!buf) {
        filp_close(file, NULL);
        return -1;
    }
    
    bytes_read = kernel_read(file, buf, 1023, &pos);
    if (bytes_read > 0) {
        buf[bytes_read] = '\0';
        
        /* Buscar patrón /docker/ en el cgroup */
        docker_pos = strstr(buf, "/docker/");
        if (docker_pos) {
            id_start = docker_pos + 8; /* Saltar "/docker/" */
            id_end = strpbrk(id_start, "/\n");
            if (id_end) {
                int id_len = min((int)(id_end - id_start), MAX_CONTAINER_ID_LEN - 1);
                strncpy(container_id, id_start, id_len);
                container_id[id_len] = '\0';
                
                /* Truncar a los primeros 12 caracteres (Docker short ID) */
                if (strlen(container_id) > 12) {
                    container_id[12] = '\0';
                }
                ret = 0;
            }
        }
    }
    
    kfree(buf);
    filp_close(file, NULL);
    return ret;
}

/*
 * Función para listar todos los contenedores Docker
 */
static void list_docker_containers(void)
{
    struct task_struct *task;
    int container_count = 0;
    char container_id[MAX_CONTAINER_ID_LEN];
    
    printk(KERN_INFO "%s: Iniciando escaneo de contenedores Docker...\n", MODULE_NAME);
    printk(KERN_INFO "%s: %-12s %-8s %-20s %-8s\n", 
           MODULE_NAME, "CONTAINER_ID", "PID", "COMM", "PPID");
    printk(KERN_INFO "%s: %s\n", MODULE_NAME, 
           "----------------------------------------------------------");
    
    rcu_read_lock();
    
    for_each_process(task) {
        if (container_count >= 50) {
            printk(KERN_INFO "%s: ... (limitado a 50 contenedores)\n", MODULE_NAME);
            break;
        }
        
        if (is_docker_container_process(task)) {
            /* Intentar extraer ID del contenedor */
            if (extract_container_id(task, container_id) != 0) {
                strncpy(container_id, "unknown", sizeof(container_id));
            }
            
            printk(KERN_INFO "%s: %-12s %-8d %-20s %-8d\n",
                   MODULE_NAME,
                   container_id,                 /* ID del contenedor */
                   task->pid,                    /* PID del proceso */
                   task->comm,                   /* Nombre del comando */
                   task->parent ? task->parent->pid : 0);  /* PID del padre */
            
            container_count++;
        }
    }
    
    rcu_read_unlock();
    
    printk(KERN_INFO "%s: %s\n", MODULE_NAME,
           "----------------------------------------------------------");
    printk(KERN_INFO "%s: Total de contenedores Docker encontrados: %d\n", MODULE_NAME, container_count);
    
    if (container_count == 0) {
        printk(KERN_INFO "%s: No se encontraron contenedores Docker corriendo\n", MODULE_NAME);
        printk(KERN_INFO "%s: Asegúrese de que Docker esté corriendo y tenga contenedores activos\n", MODULE_NAME);
    }
}

/*
 * Función de inicialización del módulo
 */
static int __init docker_lister_init(void)
{
    printk(KERN_INFO "%s: Módulo de listado de contenedores Docker cargado\n", MODULE_NAME);
    printk(KERN_INFO "%s: ========================================\n", MODULE_NAME);
    
    /* Listar todos los contenedores Docker */
    list_docker_containers();
    
    printk(KERN_INFO "%s: ========================================\n", MODULE_NAME);
    printk(KERN_INFO "%s: Módulo inicializado correctamente\n", MODULE_NAME);
    printk(KERN_INFO "%s: Use 'dmesg | grep %s' para ver la salida\n", MODULE_NAME, MODULE_NAME);
    
    return 0;
}

/*
 * Función de limpieza del módulo
 */
static void __exit docker_lister_exit(void)
{
    printk(KERN_INFO "%s: Módulo de listado de contenedores Docker descargado\n", MODULE_NAME);
}

module_init(docker_lister_init);
module_exit(docker_lister_exit);
