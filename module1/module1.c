#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sergioarmgpl");
MODULE_DESCRIPTION("Un módulo de kernel simple");
MODULE_VERSION("0.1");

static int __init simple_module_init(void) {
    printk(KERN_INFO "Hola, este es un módulo de kernel simple.\n");
    return 0;
}

static void __exit simple_module_exit(void) {
    printk(KERN_INFO "Adiós, el módulo de kernel simple se está descargando.\n");
}

module_init(simple_module_init);
module_exit(simple_module_exit);
