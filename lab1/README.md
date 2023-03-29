Initial RAM filesystem and RAM disk (initramfs/initrd) support (BLK_DEV_INITRD) 多转一
Compiler optimization level    optimize for performance 转 optimize for size
Select only drivers that don't need compile-time external firmware (STANDALONE)  able
Disable drivers features which enable custom firmware building        able
profiling support        disable
MPS table                disable
Support for extended (non-PC) x86 platforms (X86_EXTENDED_PLATFORM) disable
IOPERM and IOPL Emulation (X86_IOPL_IOPERM) disable
Machine Check / overheating reporting (X86_MCE) & CPU microcode loading support (MICROCODE) &  
NUMA Memory Allocation and Scheduler Support (NUMA)  & AMD Uncore performance events        only Intel
EFI runtime service support (EFI)  &   IA32 Emulation    &  Provide system calls for 32-bit time_t   only 64
Kprobes (KPROBES)                         disable
device drivers  Block devices     disable
Managed device resources verbose debug messages  disable
Maintain a devtmpfs filesystem to mount at /dev  disable
Parallel SCSI (SPI) Transport Attributes         disable
Multiple devices driver support (RAID and LVM)   disable
Macintosh device drivers                         disable
Kernel hacking ->Kernel debugging->Filter access to /dev/mem-> Remote debugging over FireWire early on boot     disable
Virtualization  ->Kernel-based Virtual Machine (KVM) support     disable

10.9MiB ->  8.4  MiB

POSIX Message Queues        disable
High Resolution Timer Support     disable
Export task/process statistics through netlink    disable
Perf controller RDMA controller  Cpuset controller  disable
UTS namespace  IPC namespace  PID Namespaces   disable
Enable 5-level page tables support   disable
/dev/cpu/*/msr - Model-specific register support   disable
/dev/cpu/*/cpuid - CPU information support   disable
EFI runtime service support   disable
Build a relocatable kernel     disable
KVM Guest support (including kvmclock)    disable
Performance monitoring      disable
Mitigations for speculative execution vulnerabilities          disable
Network File Systems  Miscellaneous filesystems           disable
CD-ROM/DVD Filesystems     DOS/FAT/EXFAT/NT Filesystems   disable
PCCard (PCMCIA/CardBus) support     disable
Networking support        disable

8.4 MiB ->  5.3  MiB

Automatically append version information to the version string
Old Idle dynticks config
Legacy cpb sysfs knob support for AMD CPUs     ACPI Processor P-States driver
'ondemand' cpufreq policy governor
Enable loadable module support
Block layer debugging information in debugfs
Support for paging of anonymous memory (swap)
PNP debugging messages
SCSI device support
Serial ATA and Parallel ATA drivers (libata)
Export input device LEDs in sysfs
Input device support
Graphics support AMD Opteron/Athlon64 on-CPU GART support
Autodetect IRQ on standard ports (unsafe)     Support RSA serial ports
Support for Pericom and Acces I/O serial ports
Sound card support

5.3  MiB  ->  4.8 MiB

kernel crash dumps
nable seccomp to safely execute untrusted bytecode
IO Schedulers
File System HugeTLB file system support
Cryptographic API    Kernel hacking

4.8 MiB -> 4.29 MiB

CPU microcode loading support
BSD Process Accounting
kexec system call
Machine Check / overheating reporting  Intel MCE features

**GZIP ->  XZ**
4.29 MiB -> 3.32 MiB

BSD Process Accounting
Initial RAM filesystem and RAM disk (initramfs/initrd) support
Enable process_vm_readv/writev syscalls
Memory controller
Namespaces support
CPU microcode loading support
Enable loadable module support

3.32 MiB  -> 3.46 MiB

SCSI Transports
/dev/cpu/*/cpuid - CPU information support
/dev/cpu/*/msr - Model-specific register support

3.46 MiB  -> 3.59 MiB

Networking support
3.59 MiB   -> 3.82 MiB
