# CTP-Market-data-Linux-6.6.1  

[toc]

## 简介  

本代码基于CTP API @ Linux (v6.6.1)，在Linux (Ubuntu 18.04LTS) 下实现了对期货行情信息的按需获取。

- [ ] 
- [ ] 

## 项目结构

```apl
.
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
`-- stats
```



## 如何编译 & 运行本项目

1. 在`src/main.cpp`中，指定要查询的期货代码，同时更新`BrokerID`, `InvesterID`，`InvesterPassword` 等信息。以下为代码示例：

```c++
// ---- 全局变量 ---- //
// 公共参数
TThostFtdcBrokerIDType gBrokerID = "9999";                        // 模拟经纪商代码
TThostFtdcInvestorIDType gInvesterID = "12345";                   // 投资者账户名
TThostFtdcPasswordType gInvesterPassword = "abcdefg";             // 投资者密码

// 行情参数
CThostFtdcMdApi *g_pMdUserApi = nullptr;                           // 行情指针
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

## 参考

1. 高精度低延时时钟在user space下的实现：[MengRao/tscns: A low overhead nanosecond clock based on x86 TSC (github.com)](https://github.com/MengRao/tscns)
2. 
