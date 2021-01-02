# STC8-BOOTLOADER-TINY

## 介绍
STC系列单片机一直以“无需下载器、烧录器即可下载烧录”闻名。实现该功能的技巧就是芯片内部固化了一段BootLoader代码。这段代码会在单片机上电的时候运行，因此STC一直都靠“冷启动”来下载。

然而到STC8时代，“冷启动”下载的成功率变低了。这并不是出了什么BUG，而是单片机工作电压范围变宽了。STC以前的5V单片机，只要电压降到3V再恢复5V就能触发BootLoader。现在STC8的工作电压低至1.9V，哪怕是串口IO的倒灌电流都能维持单片机工作，导致其无法“冷启动”。

那为什么不自制一个BootLoader呢？这个项目将会助您实现自己写一个BootLoader的愿望，只要能理解原理就能轻易的应用在产品上！

## 优点
* 不限定通信协议；UART、IIC、SPI都可以，甚至可以自定义协议。
* BOOT独立；即使APP下载中断或者安装中断也不会损坏BOOT固件。
* APP独立；不需要对APP工程做任何设置，不用对APP的BIN文件做任何改动既可以直接下载。只要APP里没有擦除BOOT区的操作就行。
* 多种触发模式；既可以使用一个IO用作BOOT启动脚（经典应用），也可以在APP中通过通信握手判断后跳转到BOOT（自由度更大）。

## 缺点
* 多多少少会占用flash空间，导致可用flash空间减少。
* 开启BOOT引脚时，会占用一个IO。
* 未对传输进行加密，有被窃听的可能。

#### 但这终究是Tiny版，只实现了基本的BootLoader功能。在此基础上可以自由的添加所需的功能组件。

### 参考环境

| KEIL版本  | V9.59.0.0  |
| :------------: | :------------: |
| C Compiler  | V9.59.0.0  |
| Assembler  | V8.2.7.0  |
| Linker  | V6.22.2.0  |
| Librarian  | V4.30.1.0  |
| Hex Converter  | V2.7.0.0  |
| CPU DLL  | V3.122.0.0  |
| Dialog DLL  | V2.66.0.0  |

### 示例硬件

STC8A8K64S4A12，占用P0.0作为BOOT脚。

### 消耗资源

| 优化等级  | 8级  |
| :------------: | :------------: |
| DATA区  | 28字节  |
| XDATA区 | 512字节  |
| FLASH区  | 1375字节  |

## 使用教程

### BOOT端准备工作
1. 打开STC-ISP工具，选择单片机的型号。*本例中用到的STC8A8K64S4A12*。
2. 串口选择实际连接的COM号，接下来点击“打开程序文件”，选择BootLoader工程生成的hex。
3. 实例代码中的定时器和波特率都是按照24MHz来设置的，建议选择24MHz。如果选择了其他的频率，需要修改代码。
4. **务必记得将eeprom空间设置到该型号最大值（*比如STC8A8K64S4A12最大值是64K*）。**然后点击下载再冷启动即可。
5. 下载成功之后，BOOT端就准备完成。当前的板子就已经支持iap下载了。
![使用STC-ISP下载BootLoader](https://gitee.com/jackchio/stc8-bootloader-tiny/raw/master/%E5%9B%BE%E7%89%87/stc-isp%E8%AE%BE%E7%BD%AE.gif "使用STC-ISP下载BootLoader")

### APP端准备工作
1. APP端不用做太多的设置，只是当前BootLoader只支持bin文件，所以得把keil生成的hex文件转换成bin文件。
2. 用stc-isp工具打开APP程序文件后，点击程序文件窗口右下角的“保存数据”，然后保存成bin格式就行。
![hex转bin](https://gitee.com/jackchio/stc8-bootloader-tiny/raw/master/%E5%9B%BE%E7%89%87/hex%E8%BD%ACbin.gif "hex转bin")

### 下载APP步骤
BootLoader运行的原理就是“接收数据->写到FLASH”。因此可以自己改造成SPI下载或者IIC下载的版本。目前本例用的是串口，因此需要用到串口助手。推荐使用SSCOM，因为它在发送数据会有个延时，BootLoader程序会在这个延时里将接收到数据写入到Flash中。
1. 打开SSCOM，点击“发送”->“发送文件延时设置”->“每次发送256字节延时50ms”。
2. 点击“打开文件”，选择要下载的bin文件。
3. 打开串口，波特率选择115200，如果需要其他波特率就需要改代码。
4. 将BOOT脚拉高（*在本例中，BOOT脚被定义为P0.0脚，且拉低运行APP、拉高运行BootLoader*）。重启单片机，串口上会发送“[BOOT]”说明已经成功进入BootLoader程序了。
5. 发送英文字符‘!’解锁，请留意输入法不要输入中文字符‘！’。虽然长得很像，但是从编码来说，英文字符占一个字节，中文字符占两个字节。
6. 解锁成功后，单片机会在串口上发送“[Erase]”，这表示程序正在擦除Flash，原来的APP在这一步会被擦除掉。不一会便会显示“[Start]”，此时只需点击SSCOM的“发送文件”就可以把bin文件下载到单片机中。
7. 接收之后，会在串口上返回接收的字节数用于对比。然后就会自动运行APP。之后只要BOOT脚拉低就可以一直运行APP了。
![APP下载](https://gitee.com/jackchio/stc8-bootloader-tiny/raw/master/%E5%9B%BE%E7%89%87/bin%E4%B8%8B%E8%BD%BD.gif)

### 异常情况
1. **收到“[Time Out]”**，这是因为BootLoader程序不能无限制的死等，所以单片机在发送“[Start]”之后会等待2秒，2秒内没有收到任何数据的话就进入超时状态。一般超时后会自动重启BootLoader程序，所以此时再发送一次‘!’就行。
2. **收到“[Lock]”**，为了防止一些乱码被误下载到单片机中，在进入BootLoader程序后，不会马上擦除Flash，而是接收到‘!’才开始擦除Flash。如果此时发送的是‘!’以外的字符，会认为是错误的通信，单片机进入锁定状态。进入锁定状态之后不可再接收数据，此时必须重启单片机才行。
3. **返回的接收字节数和发送的字节数不一样**，说明在接收256字节时，上位机（串口助手）没有给足够的时间让单片机保存数据，由于单片机在写入Flash的时候，CPU会停止响应中断，所以会漏掉数据。因此如果使用串口下载的话，推荐使用SSCOM，SSCOM有的发送文件延时功能能很好解决这个问题。


