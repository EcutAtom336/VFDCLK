 # VFD 时钟 
 该项目是一个基于 ESP32 C3 微控制器的 VFD 时钟，可通过 Android 端软件进行配置
 ## 快速链接
 - [Github](https://github.com/EcutAtom336/VFDCLK/tree/master) 

 - [立创开源硬件平台](https://oshwhub.com/dter/vdf-clk)
 ## 特性
 - 使用 ESP32 C3 微控制器
 - 可通过 Android 端软件进行配置
 - 使用 VFD 显示屏，最高亮度 1000cd/m^2^
 - 内置一个蜂鸣器，可发出提示音
 - 内置光敏电阻，可实现自动亮度
 - 双 Type-C 设计，包含一个侧面 16Pin Type-C （下载和供电）和背面 6Pin Type-C （仅供电）
 - 右上螺丝可触摸设计，可进行简单交互

 ## 开始
 ### 所需工具
 - 镊子
 - 电烙铁
 - 热风枪 或 加热台
 - 万用表
 - 支持 USB 2.0 的 Type-C 数据线
 - 支持 BLE 的 Android 手机

 ### 调试建议
  1. 最后焊接 VFD 屏幕，以防加热损坏 VFD 屏幕。
  2. 烧录前先测试电源是否短路，以免损坏电脑 USB 接口。

 ### 烧录
 烧录时，请使用侧面 Type-C 接口。

 使用附件提供的 .bin 文件进行烧录，烧录方法如下。
 
 1. IDE 烧录
 [IDE 烧录方法](https://docs.espressif.com/projects/esp-techpedia/zh_CN/latest/esp-friends/get-started/try-firmware/try-firmware-platform.html#ide)
 2. Flash Download Tool 烧录
 [Flash Download Tool 烧录](https://docs.espressif.com/projects/esp-techpedia/zh_CN/latest/esp-friends/get-started/try-firmware/try-firmware-platform.html#flash-download-tool)

 对于非开发人员，建议使用 Flash Download Tool 烧录。

 ## 使用方法
 1. 初次上电，长按右上螺丝 3s 进入配置模式，此时 VFD 时钟显示一次性设备码。
 2. 安装附件提供的 Android 软件进行配置。
 3. 当时间可能不准确时，时间分隔符会闪烁。

 ## 注意事项
 - 3D 外壳有两个，分为有脚垫和无脚垫（文件名分别为 shell_foot_pad.STL 和 shell_not_foor_pad.STL）。无脚垫的已验证。有脚垫的仅在无脚垫的基础上加上脚垫，未验证。
 - 面板打印选 1.5 mm 厚，只切割不打印。备注写“两款数量一样”。
 - 由于立创开源硬件平台审核机制，附件所提供的 apk 和 bin 可能不是最新，可前往 Github 查看是否有更新。
 - 为保证 ESP32 C3 天线校准成功以及系统稳定，请使用电压为 5V 且电流不低于 800mA 的电源进行供电。

 ## 安全提示
 - 尽管具有防回流设计，但也不要同时使用两个 Type-C 接口进行供电，以免发生意外。
 - 由于技术原因，配置时所输入的一次性设备码并不是蓝牙的配对码，因此时钟与 Android 手机并没有建立绑定关系，所以配置信息全部以明文发送。

 ## 未来可能更新的功能（画饼）
 - BLE OTA
 - 闹钟功能
 - 配置信息加密
 - 校园网认证

 ## 许可证
 本项目使用 GPL 3.0 许可证。