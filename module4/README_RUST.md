# Módulo de Kernel con Rust

Este módulo combina C y Rust para mostrar un mensaje "Hola Mundo" desde código Rust.

## Requisitos

- Rust instalado (`rustc` disponible)
- Headers del kernel de Linux
- Herramientas de compilación del kernel

## Estructura

- `simple_module.c`: Módulo principal en C que llama a la función Rust
- `rust_helper.rs`: Función Rust que imprime "Hola Mundo desde Rust!"
- `Makefile`: Compilación del código Rust y enlazado con el módulo C

## Compilación

```bash
make
```

## Cargar el módulo

```bash
sudo insmod simple_module.ko
```

## Ver los mensajes

```bash
dmesg | tail
```

Deberías ver:
- "Hola, este es un módulo de kernel simple."
- "Hola Mundo desde Rust!"

## Descargar el módulo

```bash
sudo rmmod simple_module
```

## Limpiar

```bash
make clean
```

## Notas

El código Rust usa `#![no_std]` porque se ejecuta en el contexto del kernel (sin biblioteca estándar).
La función `rust_hola_mundo()` es exportada con `extern "C"` para que pueda ser llamada desde C.
