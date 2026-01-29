#![no_std]
#![no_main]

use core::ffi::c_int;

// Función Rust que será llamada desde C
#[no_mangle]
pub extern "C" fn rust_hola_mundo() -> c_int {
    // En el contexto del kernel, usamos printk
    // Esta es una función externa definida en el kernel de Linux
    extern "C" {
        fn printk(fmt: *const u8, ...) -> c_int;
    }
    
    // String con terminador nulo para printk
    let mensaje = b"KERN_INFO Hola Mundo desde Rust!\n\0";
    
    unsafe {
        printk(mensaje.as_ptr());
    }
    
    0
}

// Handler para panic (requerido en no_std)
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
