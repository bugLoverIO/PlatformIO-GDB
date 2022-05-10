# gdbPipe
GDB connector between GDB (8266 / xtensa lx106 CPU) from Espressif and Visual Studio code with PlatformIO

# Context
A few documentation exists to describe how to use GDB with 8266<p>
This [link](https://arduino-esp8266.readthedocs.io/en/latest/gdb.html) descibes the commands and how to use gdbstub to connect GDB to a 8266. By doing so, a step by step execution is possible. Nevertheless GDB syntax (command line) is quite strict and any mistakes hangs GDB process right away<p>

Goal of this project is to use 'Visual Studio Code' GDB plug'in with Espessif GDB instance, and use the VSC simple graphic interface to drive GDB. It allows to visualy
- add/remove break point
- display variables
- move step by step
  
Nevertheless, the VSC GDB plugin assmues 'main' function exists and inserts a add a “break-insert -f main" at the GDB initialisation. 'main' does not really exit on arduinon ELF program, so GDB exists right away (eg before applying any comamnds)

The proposed solution consists of making a "proxy" in between VSC and GDB (by using pipes), and replaces/insert on the fly 'faulty' VSC commands
  
gdbpipe is written in standard C (POSIX), and it works like a charm on osX and most likely Unix. I didn't test it on Windows. 
  
# How to
Prerequisit PlatformIO should be functional on your system, eg flashing, serial output OK

1. Compile gdbpipe project with your best C compiler (tested with xCode and GCC)
```
gcc gdbpipe.c -o gdbpipe
```
  
2. Edit both platformio.ini & launch.json file, they should be stored on the root fodler of your platformIO project.  

## PlatformIO.ini
```
; PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html

[env:d1_mini]
platform = espressif8266
board = d1_mini
framework = arduino
upload_port = /dev/cu.wchusbserial1450
upload_speed = 460800 ; 921600, 460800, 230400, 115200

monitor_port = /dev/cu.wchusbserial1450
monitor_speed = 115200

build_unflags = -Os 
build_flags = -Og -ggdb -DGDBSTUB_BREAK_ON_INIT
```
  
Two last lines are mandatory to disable optimisation, and to force GDBstub to break on init, it makes attaching processing between 8266 and GDB reliable.<p>
 
## launch.json
```
// AUTOMATICALLY GENERATED FILE. PLEASE DO NOT MODIFY IT MANUALLY
//
// PIO Unified Debugger
//
// Documentation: https://docs.platformio.org/page/plus/debugging.html
// Configuration: https://docs.platformio.org/page/projectconf/section_env_debug.html

{
    "version": "0.2.0",

    "configurations": [
        {
            "logging": {
//                "trace": true,
//                "engineLogging": true
             },
            "name": "(gdb) Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/.pio/build/d1_mini/firmware.elf",
            "args": [],
            "stopAtEntry": true,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": true,
            "MIMode": "gdb",
            "miDebuggerPath": "<GDBpipe PATH>",
            "miDebuggerServerAddress": <USB serial port>",
            "miDebuggerArgs": "--gdb=lx106 xtensa GDB path>",
            "setupCommands": [
                {
                    "text": "set remote hardware-breakpoint-limit 1"
                },
                {
                    "text": "set remote hardware-watchpoint-limit 1"
                },
                {
                    "text": "set remote interrupt-on-connect on"
                },
                {
                    "text": "set remote kill-packet off"
                },
                {
                    "text": "set remote symbol-lookup-packet off"
                },
                {
                    "text": "set remote verbose-resume-packet off"
                },
                {
                    "text": "mem 0x20000000 0x3fefffff ro cache"
                },
                {
                    "text": "mem 0x3ff00000 0x3fffffff rw"
                },
                {
                    "text": "mem 0x40000000 0x400fffff ro cache"
                },
                {
                    "text": "mem 0x40100000 0x4013ffff rw cache"
                },
                {
                    "text": "mem 0x40140000 0x5fffffff ro cache"
                },
                {
                    "text": "mem 0x60000000 0x60001fff rw"
                },
                {
                    "text": "set serial baud 115200"
                }
            ],
            "preLaunchTask": {
                "type": "PlatformIO",
                "task": "Pre-Debug"
            }
        },
    ]
}
```  
Make sure to repalce the following entries with your executable paths
- "miDebuggerPath": "<GDBpipe PATH>",
- "miDebuggerServerAddress": "<USB serial port>",
- "miDebuggerArgs": "--gdb=<lx106 xtensa GDB path>",
NOTE : Don't forget double quotes to set GBBpipe path, USB serial path 
  
# Limitations
- 8266 features **ONE Hardware break point** only, thus setting two or more break points will lead GDB to exit.
- launch.json is an automaticaly generated file, thus changing platformio.ini configuration will erase the changes, make sure to store it somewhere else.
  
# Enjoy

  
  



  


