## 实际影响内核编译大小和速度的选项

在尝试裁减linux内核的过程中，除了自主思考权衡，主要参考了以下材料及提示：

- <https://blog.csdn.net/liao20081228/article/details/81389813>
- make xconfig 中GUI界面的自带配置指导提示，如
    ![](pics/cut_according1.png "Say x if unsure")
    ![](pics/cut_according2.png "Additional Support")
- 其余各种组件名词的搜索查询

下面大致罗列裁减内核尝试的过程，从qemu测试过程中的部分进程截图可以看出，其中**标注红色**的一些选项是**必须"enable"**的。
|上手裁剪过程|尺寸变化|
|:--:|:--:|
|defconfig中的初始默认配置|10.9MiB|
|<font color=Red>Initial RAM filesystem and RAM disk (initramfs/initrd) support:多种方式转为一种方式Gzip<br>Compiler optimization level:optimize for performance转为optimize for size<br>profiling support/MPS table/Kprobes/Block devices:N<br>Virtualization:N<br>Kernel hacking:N<br>Support for extended (non-PC) x86 platforms:N<br>NUMA Memory Allocation and Scheduler Support:N<br>EFI runtime service support:N<br>Parallel SCSI (SPI) Transport Attributes:N<br>Maintain a devtmpfs filesystem to mount at /dev:N<br></font>Select only drivers that don't need compile-time external firmware:Y<br>Disable drivers features which enable custom firmware building:Y<br>IOPERM and IOPL Emulation:N<br>Multiple devices driver support (RAID and LVM):N<br>CPU microcode loading support:N<br>IA32 Emulation:N<br>|8.4  MiB|
|<font color=Red>/dev/cpu/\*/msr - Model-specific register support:N<br>/dev/cpu/\*/cpuid - CPU information support:N<br>KVM Guest support:N<br>Networking support:N<br></font> POSIX Message Queues:N<br>High Resolution Timer Support:N<br>Export task/process statistics through netlink:N<br>Perf controller\RDMA controller:N<br>Build a relocatable kernel:N<br>Performance monitoring:N<br>Mitigations for speculative execution vulnerabilities:N<br>PCCard (PCMCIA/CardBus) support:N<br>Network File Systems\Miscellaneous filesystems\CD-ROM/DVD Filesystems\DOS/FAT/EXFAT/NT Filesystems:N<br>|5.3  MiB|
|<font color=Red>ACPI Processor P-States driver:N<br>SCSI device support:N<br></font>Enable loadable module support:N<br>Block layer debugging information in debugfs:N<br>PNP debugging messages:M<br>Serial ATA and Parallel ATA drivers:N<br>Sound card support:N<br>Support Autodetect IRQ on standard ports\Support RSA serial ports\Support for Pericom and Acces I/O serial ports:N<br>Input device support:N<br>Graphics support AMD Opteron/Athlon64 on-CPU GART\Legacy cpb sysfs knob support for AMD CPUs:N<br>|4.8 MiB|
|kernel crash dumps:N<br>Cryptographic API:N<br>nable seccomp to safely execute untrusted bytecode:N<br>HugeTLB file system support:N<br>IO Schedulers:N<br>|4.29 MiB|
|<font color=Red>BSD Process Accounting:N<br>Kernel Compress manner:GZIP转为XZ<br></font>kexec system call:N<br>|3.32 MiB|
|<font color=Red>BSD Process Accounting:Y<br>Initial RAM filesystem and RAM disk (initramfs/initrd) support:Y<br></font>Enable process_vm_readv/writev syscalls:Y<br>CPU microcode loading support:Y<br>Enable loadable module support:Y<br>|3.46 MiB|
|<font color=Red>SCSI Transports:Y<br>/dev/cpu/\*/cpuid - CPU information support:Y<br>/dev/cpu/\*/msr - Model-specific register support:Y<br></font>|3.59 MiB|
|Networking support:Y<br>|3.82 MiB|

经过多次裁减尝试后大致总结出如下几个会影响内核编译尺寸的选项

- 内核压缩方式由默认的gzip改为xz（时间换空间）
- 编译优化由默认的优先性能改为优先尺寸
- 与内核进程启动及调度有关的选项如与NVME、SCSI、devtmpfs、EFI、ACPI、BSD、Initamfs/initrd等有关的配置（不可擅动）
- 不必要的额外支持，如对AMD芯片、32bit系统、其他高性能x86平台的支持
- 不必要的外设支持，如声卡、网卡、CD-ROM、DVD等
- 不必要的功能支持，如profiling、Kprobes、kernel hacking、debugging等
- 不必要的数据传输支持，如网络数据传输协议、外设接口数据传输支持等
- 不必要的图处理模块

## initrd程序在何种情况下会造成内核恐慌

> Linux内核在运行其他用户进程前必须先完成一个挂载根文件系统的引导过程，即在自身初始化完成之后需要找到并运行 init 程序。Linux 2.6 kernel 提出了一种新的实现机制，即 initramfs。这是一种 RAM filesystem 而不是 disk。initramfs/initrd实际是一个 cpio 归档（小型根目录），这个目录中包含了启动阶段中必须的驱动模块，可执行文件和启动脚本。当系统启动的时候，bootload会把initrd文件读到内存中，然后把initrd的起始地址告诉内核。内核在运行过程中解压initrd，并把initrd挂载为根目录，然后执行根目录中的init.c程序，让它来自动加载设备驱动程序以及在/dev目录下建立必要的设备节点，这之后就可以mount真正的根目录，并切换到这个根目录中了。

实验过程中发现，即使在init.c程序中加入sleep()延时函数也不会导致内核恐慌，但只要程序末尾不含持续循环语句，如while(1){}，就会导致内核恐慌，原因是：init进程是所有系统进程的父进程，它被内核调用起来并负责调用所有其他进程。 如果任何进程的父进程退出，init进程将成为它的父进程。由此可见init进程将一直存在，不可中断，因此若失去while(1){}这个循环，内核将无法继续调用其他进程，自然会造成内核恐慌。

![](pics/bug1.png "无法打开initramfs")
![](pics/openinitramfs.png "成功解压initramfs")
![](pics/bug2.png "无法打开/dev/root根目录")
![](pics/mainbug.png "内核恐慌kernal panic 报错")
![](pics/initrd_output.png "initrd程序结果输出")

## 实验过程其他附图

![](pics/apt_update_ali.png "阿里云镜像")
![](pics/qt5GUI_forxconfig.png "make xconfig 需要Qt5")
![](pics/qt5.png "Qt version 5.9.5")
![](pics/connect_to_GUI.png "try to establish a GUI for xconfig")
![](pics/Xlaunch_GUI.png "Xlaunch GUI")
![](pics/config_byhelp.png "make xconfig interface")
![](pics/First-level%20catalog1.png "config 中的一级目录")
![](pics/First-level%20catalog2.png "config 中的一级目录")

![](pics/bug3.png "粗心在vscode中gcc编译时只生成了.exe可执行文件，没有生成.o目标文件")

![](pics/Serial_compilation.png "串行编译界面-目录有序")
![](pics/Parallel_compilation.png "并行编译界面-目录乱序")

![](pics/gemu_initrd.png "qemu测试界面")
![](pics/necessities1.png "qemu测试进程截图")
![](pics/necessities2.png "qemu测试进程截图")
![](pics/necessities3.png "qemu测试进程截图")
![](pics/ps_look.png "ps查看进程信息")

![](pics/size_of_default.png)
![](pics/firsttry.png)
![](pics/trysecond.png)
![](pics/trythird.png)
![](pics/tryfourth.png)
![](pics/tryfifth.png)
![](pics/trysixth.png)
