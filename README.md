# CTP-Market-data-Linux-6.6.1  

[toc]

## 简介  

本代码基于CTP API @ Linux (v6.6.1)，在Linux (Ubuntu 18.04LTS) 下实现了对期货行情信息的按需获取。

- [x] 将接收到的行情写入本地文件： CSV文档储存位置为`stats/`，`.con`文件储存位置为`flow/`
- [x] 减少回调函数的延迟、提升解析效率： 采用`pthread`的双线程结构，`api_handler` 线程负责将回调函数的结果，连同本地精确时间一同写入内存（`queue`），`md_parser` 线程负责从内存(`queue`)中读取结果并写入本地CSV文件。
- [x] 保留接收行情的本地时间： 执行回调函数时，第一时间记录本地系统时间。(使用了开源方案减少时间误差 - 注[1])
- [x] 自动重连和登录：由于simnow平台的行情接口无需登录验证，且据接口文档的记载，自动重连功能是包含在API底层的; 所以我参照接口文档的要求在`OnFrontDisconnected` 函数的实现上做了自动登录，按递增时间间隔尝试连续登录10次。

## 项目结构

```apl
.
|-- docs
|   `-- 6.6.9_API接口说明.chm		 
|-- flow
|-- makefile
|-- marketdata.Dockerfile
|-- README.md
|-- src		# 源代码文件夹
|   |-- CTPAPI	# 官方API存放位置
|   |   |-- error.dtd
|   |   |-- error.xml
|   |   |-- ThostFtdcMdApi.h
|   |   |-- ThostFtdcUserApiDataType.h
|   |   |-- ThostFtdcUserApiStruct.h
|   |   `-- thostmduserapi_se.so
|   |-- CustomMdSpi.cpp	  # 对Spi&Api的重写实现
|   |-- CustomMdSpi.h	  # 头文件
|   `-- main.cpp		  # 项目运行入口，在此指明登录信息
`-- stats				  # 存放CSV数据，不同合约的解析结果写入不同的 csv 文件
```



## 如何编译 & 运行本项目

1. 在`src/main.cpp`中，指定要查询的期货代码，同时更新`BrokerID`, `InvesterID`，`InvesterPassword` 等信息。以下为代码示例：

```c++
// 公共参数
// ...
TThostFtdcBrokerIDType gBrokerID = "9999";                        // 模拟经纪商代码
TThostFtdcInvestorIDType gInvesterID = "12345";                   // 投资者账户名
TThostFtdcPasswordType gInvesterPassword = "abcdefg";             // 投资者密码

// 行情参数
// ...
char gMdFrontAddr[] = "tcp://180.168.146.187:10131";               // 模拟行情前置地址
char *g_pInstrumentID[] = {"TF1706", "zn1705", "cs1801", "CF705"}; // 行情合约代码列表
int instrumentNum = 4;                                             // 行情合约订阅数量
```
2. 构建Docker容器。

```bash
(base) −> docker build -f marketdata.Dockerfile -t marketdata .
(base) −> docker run -it --privileged=true --mount type=bind,source=.,target=/opt/marketdata marketdata bash
```
3. g++编译。

```bash
−> cd /opt/marketdata/
-> make clean
−> make
```
4. 运行。

```bash
−> ./test
```




## 备注

1. 本项目所有编码格式默认为`UTF-8` (与官方API文件使用的 `GBK` 不同)
2. [SimNow提供的第一套系统](https://www.simnow.com.cn/product.action)开放时间和交易时间同步；非交易时间可选用第二套系统。

## 未来改进方向

- 效率进一步优化：可支持更多线程(3+)并行
  - 其中1+线程用于`api_handler`，实现多号多开，提升行情数据落地速度。
  - 另外1+线程用于`md_parser`，采用多进程 + 多锁模式，加速数据落地后的解析与写入。
- 将所有需要手动录入的信息(如账号密码前置信息等)集中到一个文件里统一管理

## 参考

1. 高精度低延时时钟在user space下的实现：[MengRao/tscns: A low overhead nanosecond clock based on x86 TSC (github.com)](https://github.com/MengRao/tscns)

2. [CTP API v6.6.9 接口手册](./docs/6.6.9_API接口说明.chm)

   
