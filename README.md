# STC8-BOOTLOADER-TINY

#### 介绍
STC系列单片机一直以“无需下载器、烧录器即可下载烧录”闻名。实现该功能的技巧就是芯片内部固化了一段BootLoader代码。这段代码会在单片机上电的时候运行，因此STC一直都靠“冷启动”来下载。
然而到STC8时代，“冷启动”下载的成功率变低了。这并不是出了什么BUG，而是单片机工作电压范围变宽了。STC以前的5V单片机，只要电压降到3V再恢复5V就能触发BootLoader。现在STC8的工作电压低至1.9V，哪怕是串口IO的倒灌电流都能维持单片机工作，导致其无法“冷启动”。
那为什么不自制一个BootLoader呢？这个项目将会助您实现自己写一个BootLoader的愿望，只要能理解原理就能轻易的应用在产品上！

#### 优点
软件架构说明

#### 缺点
软件架构说明


#### 安装教程

1.  xxxx
2.  xxxx
3.  xxxx

#### 使用说明

1.  xxxx
2.  xxxx
3.  xxxx

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
