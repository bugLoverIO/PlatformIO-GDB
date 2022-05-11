# gdbPipe
GDB connector between GDB (8266 / xtensa lx106 CPU) from Espressif and Visual Studio code with PlatformIO

# Context
A few documentation exists to describe how to use GDB with Espressif 8266<p>
This [link](https://arduino-esp8266.readthedocs.io/en/latest/gdb.html) descibes the commands and how to use gdbstub to connect GDB to a 8266. By doing so, a step by step execution is possible. Nevertheless GDB syntax (command line) is quite strict and any mistake hangs GDB process right away<p>

Goal of this project is to use 'Visual Studio Code' GDB plug'in (Native Debug plug'in) with Espessif GDB release, and use the VSC simple graphic interface to drive GDB. It allows to visualy
- add/remove break point
- display variables
- move step by step
  
Nevertheless, the VSC GDB plugin assmues 'main' function exists and inserts a “break-insert -f main" at the GDB initialisation. 'main' does not really exit on ELF Arduino program, so GDB exits right away (eg before applying any comamnds) :angry:

The proposed solution consists of making a "proxy" between VSC and GDB (by using pipes), and replaces/inserts on the fly 'faulty' VSC commands<p>
Two actions are made by this program
- replace break-insert -f main by break-insert -f <any function>. (default being loop)
- once connected, to force GDB to continue the program to run up to the 'loop' break point
  
  
# How to
Prerequisit PlatformIO should be functional on your system, eg flashing, serial output OK. <p>
Tested with following versions
- Visual Studio Code - osX 1.67.1
- Platform IO - Core 5.2.5 / Home 3.4.1
- PlatformIO Espressif 8266 - 3.2.0
- GDB Native Debug - 0.26.0
- Xtensa GDB - 3.0.4

1. gdbpipe is written in standard C (POSIX), and it works on osX and most likely any Linux. Compile gdbpipe.c with your C compiler (tested with xCode and GCC), and may work on windows 10 with cygwin or with Windows Subsystem for Linux
```
gcc gdbpipe.c -o gdbpipe
```
Its usage is trivial, the mandatory parameter (--gdb) describes Espressif GDB path, in order to fork the process.
It performs a simple pipe connection eg, any gdbpipe STDIN is forwarded to GDB STDIN, any GDB STDOUT is forwarded to gdbpipe STDOUT, with adapations in between. 
```
usage: gdbpipe [--help] --gdb path [--func name] [--init true|false] [extra GDB parametes] 
options
  -h, --help               : This message
  -g, --gdb  <path>        : GDB path
  -f, --func <string>      : Function name (in C code) used as 'main' subtitute (default is 'loop')
  -i, --init true|false    : Enable/disable 'continue' to cope with GDBSTUB_BREAK_ON_INIT pragma (default is true)
  [extra GDB parameters]   : Can be anything up to 19 parameters 
```     
  
2. Edit both platformio.ini & launch.json file, they are placed/created on the root folder of your platformIO project.  

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
  
Two last lines are mandatory for debuging. First one disable optimisation, and second one forces GDBstub to break on init, it makes attaching processing between 8266 and GDB reliable.<p>
 
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
            "miDebuggerPath": "/user/john/gdbpipe",
            "miDebuggerServerAddress": "/dev/cu.wchusbserialxxxx",
            "miDebuggerArgs": "--gdb=/usr/john/Arduino/xtensa/xtensa-lx106-elf-gdb",
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
Make sure to repalce the following entries with your executable paths & environment.
- "miDebuggerPath"  \<GDBpipe PATH\> (between quotes)
- "miDebuggerServerAddress" : \<USB serial port\> (between qutoes)
- "miDebuggerArgs" : --gdb=\<lx106 xtensa GDB path\> (full option should be between quotes)
  
# Limitations
- 8266 features **ONE Hardware break point** only, thus setting two or more break points will lead GDB to exit.
- launch.json is an automaticaly generated file, thus updating platformio.ini configuration will erase the changes, make sure to store it somewhere before any change.
- func parameter must be a 'C' function name. It can't be <file name>:<line> syntax.
- init parameter set to false should be used with disabling -DGDBSTUB_BREAK_ON_INIT, but then attaching GDB to the 8266 is not reliable 
- last but not the least, xtensa GDB remains fragile, and time to time VSC "continue" button does not work as expected. One solution seems to be 'reflash the SW and try again'.   

# Enjoy

  
  



  


