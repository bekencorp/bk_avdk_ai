DVP常见问题
=================================


:link_to_translation:`en:[English]`

1. 简介
---------------------------------

本节主要描述调试和使用DVP摄像头过程中常遇到的问题和解决方案。

    Q：识别不到摄像头

    A：使用 `dvp <../../projects/peripheral/dvp/index.html>`_ 示例工程，测试已经适配好的示例sensor，gc2145，
    如果不能识别表明，表明新适配的sensor，dvp_xxx.c中参数配置有错，最根本的是I2C读写地址，新sensor的CHIP_ID。
    如果连gc2145都不能识别，则检查sensor电源DVDD，IOVDD是否符合协议要求，硬件上控电的GPIO是否也是GPIO28，I2C也是对应的GPIO0和GPIO1。
    另外还有注意接触问题，也可能因为物理上连接不够紧密，导致不能正常工作。

    Q：出图异常，且打印：”sensor's yuyv data resoltion is not right”。

    A：表示主控采集的dvp数据与配置的分辨率不一致，可以通过逻辑分析仪，抓取的vsync/hsync/pclk的信号，
    必须遵守以下条件：如果配置给主控的分辨率为640X480，那么一个vsync内部必须包含480个hsync个脉冲，一个hsync内部必须包含640*2=1280个pclk脉冲。
    如果不符合，那么必然出图异常。
    可能因为物理上接触不良，导致这个问题，需要重新拔插安装。
    可能因为线序与BK7258默认不一致，导致数据采集异常。
    可能因为pclk受板子的电磁干扰导致主控采样不准确，可以在plk上接一个上拉滤波电容8-22pf。

    Q：出图异常，且打印：”sensor fifo is full”。

    A：表示主控接收dvp数据太慢了，导致sensor fifo溢出，解决方案可以尝试：
    降低帧率/降低分辨率/增大YUV_BUF硬件模块时钟（当前默认JPEG:120MHz，YUV_BUF:120MHz，H264:120MHz）。

    Q：出图异常，且打印：”jpeg code rate is slow than sensor's data rate”。

    A：表示编码速度太慢，解决方案可以尝试：降低帧率/降低分辨率/增大编码硬件模块时钟（当前默认JPEG:120MHz H264:120MHz）。

    Q：出图异常，且打印：”h264 encode erro”。

    A：表示h264编码错误，可能出现的原因是sensor的帧间隙时间太低了，导致主控异常；
    也可能是新一帧h264编码开始之前，上一帧还未编码完成。这种情况，软件代码已经cover了，直接复位相关的硬件模块。

    Q：开关后I2C异常，摄像头无法正常通信。

    A：出现这种情况一般是，其他外设与DVP共用一组I2C，该I2C被其他外设切过去使用；
    建议使用软件I2C，防止出现复用后，不能工作的问题，打开软件I2C功能宏控：

    +--------------------+---------------+-------------------------------------+
    |     marco          |     value     |           implication               |
    +--------------------+---------------+-------------------------------------+
    | CONFIG_SIM_I2C     |       Y       |        使能软件模拟I2C功能          |
    +--------------------+---------------+-------------------------------------+

    Q：配置成h264/JPEG编码模式，出现异常打印："··· size no match···".

    A：一般情况是dma搬运的数据长度与实际编码长度不一致，这样会在软件上默认丢弃这样的错帧，防止出现花屏等问题，并且将对应的模块重置。

    Q：出图时视角不在正中间。

    A：这样的问题是因为配置dvp的参数（寄存器）导致的，建议找sensor原厂的工程师重新配置，目前SDK提供的配置，只保证出图正常。
