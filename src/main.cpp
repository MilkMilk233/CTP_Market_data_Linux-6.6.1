#include <iostream>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include "CustomMdSpi.h"

using namespace std;

/* 请在此处编辑信息 */
TThostFtdcBrokerIDType gBrokerID = "9999";                         // 模拟经纪商代码
TThostFtdcInvestorIDType gInvesterID = "123456";                   // 投资者账户名
TThostFtdcPasswordType gInvesterPassword = "123456";               // 投资者密码

// Testing set: IM2306, IH2306, TS2309, TF2309, T2309, TL2309

CThostFtdcMdApi *g_pMdUserApi = nullptr;                           // 行情指针
char gMdFrontAddr[] = "tcp://180.168.146.187:10211";               // 模拟行情前置地址
// char *g_pInstrumentID[] = {"ma309", "sn2306","IF2306"}; 		   // 行情合约代码列表
const char *g_pInstrumentID[] = {"sn2306", "IF2306", "IC2306", "IM2306", "IH2306", "TS2309", "TF2309", "TL2309"}; 		   // 行情合约代码列表
int instrumentNum = 8;                                             // 行情合约订阅数量

TSCNS tscns;													   // 高精度时钟
std::deque<pair<CThostFtdcDepthMarketDataField, int64_t>> wd_list; // 用于存放深度行情信息的队列

int main()
{
	// 高精度时钟初始化
	tscns.init();

	// 初始化行情线程
	g_pMdUserApi = CThostFtdcMdApi::CreateFtdcMdApi("./flow/", false, false);   // 创建行情实例
	CThostFtdcMdSpi *pMdUserSpi = new CustomMdSpi;       // 创建行情回调实例
	g_pMdUserApi->RegisterSpi(pMdUserSpi);               // 注册事件类
	g_pMdUserApi->RegisterFront(gMdFrontAddr);           // 设置行情前置地址
	g_pMdUserApi->Init();                                // 连接运行

	// 等到线程退出
	g_pMdUserApi->Join();
	delete pMdUserSpi;
	g_pMdUserApi->Release();
	
	getchar();
	return 0;
}