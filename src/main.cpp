#include <iostream>
#include <stdio.h>
#include <string>
#include "CustomMdSpi.h"

/* 请在此处编辑信息 -- 开始 */

#define THHREAD_NUM 2											   // 定义线程数量，默认为双线程
TThostFtdcBrokerIDType gBrokerID = "9999";                         // 模拟经纪商代码
TThostFtdcInvestorIDType gInvesterID = "123456";                   // 投资者账户名
TThostFtdcPasswordType gInvesterPassword = "123456";               // 投资者密码
char gMdFrontAddr[] = "tcp://180.168.146.187:10131";               // 模拟行情前置地址
const char *g_pInstrumentID[] = {"sn2306", "IF2306", "IC2306", "IM2306", "IH2306", "TS2309", "TF2309", "TL2309"}; 		   // 行情合约代码列表
int instrumentNum = 8;                                             // 行情合约订阅数量

/* 请在此处编辑信息 -- 结束 */

CThostFtdcMdApi *g_pMdUserApi = nullptr;                           // 行情指针
TSCNS tscns;													   // 高精度时钟
std::queue<std::pair<CThostFtdcDepthMarketDataField, int64_t>> wd_list; // 用于存放深度行情信息的队列
bool stop_signal = false;									       // 进程结束信号
pthread_mutex_t wd_list_lock;								       // 合约信息队列锁
sem_t wd_quota;													   // 队列管理信号量       						   

time_t convertTimeStr2TimeStamp(std::string time1, std::string time2){
    struct tm timeinfo;
	std::string timeStr = time1 + time2;
    strptime(timeStr.c_str(), "%Y%m%d%H:%M:%S",  &timeinfo);
    return mktime(&timeinfo);
}

void* api_handler(void* args) {
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
    pthread_exit(NULL);
}

void* md_parser(void* args){
	while(true){
		sem_wait(&wd_quota);
		if(stop_signal) break;
		pthread_mutex_lock(&wd_list_lock);
		CThostFtdcDepthMarketDataField pDepthMarketData = wd_list.front().first;
		int64_t tsc = wd_list.front().second;
		wd_list.pop();
		pthread_mutex_unlock(&wd_list_lock);

		int64_t ns = tscns.tsc2ns(tsc);
		std::string t_date =  pDepthMarketData.TradingDay;
		std::string t_time = pDepthMarketData.UpdateTime;
		time_t fetchtime = convertTimeStr2TimeStamp(t_date, t_time);
		int msec = pDepthMarketData.UpdateMillisec;
		fetchtime *= 1000000000;
		if(msec == 500){
			fetchtime += 500000000;
		}
		std::cout << "已解析落地行情数据: " << pDepthMarketData.InstrumentID<< ", 时间: "<< fetchtime << std::endl;
		char filePath[100] = {'\0'};
		sprintf(filePath, "./stats/%s_market_data.csv", pDepthMarketData.InstrumentID);
		std::ofstream outFile;
		outFile.open(filePath, std::ios::app); // 文件追加写入 
		outFile << pDepthMarketData.TradingDay << "," 
			<< pDepthMarketData.InstrumentID << "," 
			<< pDepthMarketData.ExchangeID << "," 
			<< pDepthMarketData.ExchangeInstID << "," 
			<< pDepthMarketData.LastPrice << "," 
			<< pDepthMarketData.PreSettlementPrice << "," 
			<< pDepthMarketData.PreClosePrice << "," 
			<< pDepthMarketData.PreOpenInterest << "," 
			<< pDepthMarketData.OpenPrice << "," 
			<< pDepthMarketData.HighestPrice << "," 
			<< pDepthMarketData.LowestPrice << "," 
			<< pDepthMarketData.Volume << "," 
			<< pDepthMarketData.OpenInterest << "," 
			<< pDepthMarketData.ClosePrice << "," 
			<< pDepthMarketData.UpperLimitPrice << "," 
			<< pDepthMarketData.LowerLimitPrice << "," 
			<< pDepthMarketData.UpdateTime << "," 
			<< pDepthMarketData.UpdateMillisec << "," 
			<< pDepthMarketData.BidPrice1 << "," 
			<< pDepthMarketData.BidVolume1 << "," 
			<< pDepthMarketData.AskPrice1 << "," 
			<< pDepthMarketData.AskVolume1 << ","
			<< fetchtime << ","
			<< ns << std::endl;
		outFile.close();

	}
	pthread_exit(NULL);
}

int main()
{
	// 高精度时钟初始化
	tscns.init();
	pthread_t thds[THHREAD_NUM]; // thread pool
	sem_init(&wd_quota,0,0);
	pthread_mutex_init(&wd_list_lock, NULL);

	pthread_create(&thds[0], NULL, api_handler, nullptr);
	for (int thd = 1; thd < THHREAD_NUM; thd++) pthread_create(&thds[thd], NULL, md_parser, nullptr);
	for (int thd = 0; thd < THHREAD_NUM; thd++) pthread_join(thds[thd], NULL);
	getchar();
	return 0;
}