#include <iostream>
#include <fstream>
#include <unordered_map>
#include "CustomMdSpi.h"
#include <string.h>
#include <unistd.h>

// ---- 全局参数声明 ---- //
extern CThostFtdcMdApi *g_pMdUserApi;            // 行情指针
extern char gMdFrontAddr[];                      // 模拟行情前置地址
extern TThostFtdcBrokerIDType gBrokerID;         // 模拟经纪商代码
extern TThostFtdcInvestorIDType gInvesterID;     // 投资者账户名
extern TThostFtdcPasswordType gInvesterPassword; // 投资者密码
extern char *g_pInstrumentID[];                  // 行情合约代码列表
extern int instrumentNum;                        // 行情合约订阅数量
extern TSCNS tscns;								 // 高精度时钟

// ---- ctp_api回调函数 ---- //
// 连接成功应答
void CustomMdSpi::OnFrontConnected()
{
	std::cout << "=====建立网络连接成功=====" << std::endl;
	// 开始登录
	CThostFtdcReqUserLoginField loginReq;
	memset(&loginReq, 0, sizeof(loginReq));
	strcpy(loginReq.BrokerID, gBrokerID);
	strcpy(loginReq.UserID, gInvesterID);
	strcpy(loginReq.Password, gInvesterPassword);
	static int requestID = 0; // 请求编号
	int rt = g_pMdUserApi->ReqUserLogin(&loginReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送登录请求成功" << std::endl;
	else
		std::cerr << "--->>>发送登录请求失败" << std::endl;
}

// 断开连接通知
void CustomMdSpi::OnFrontDisconnected(int nReason)
{
	std::cerr << "=====网络连接断开,正在尝试重连=====" << std::endl;
	// API自动执行 CustomMdSpi::OnFrontConnected(), 以下为重新登录部分
	int sleep_sec = 1;
	for(int i = 0; i < 10; i++){
		CustomMdSpi::OnFrontConnected();
		sleep(sleep_sec);
		sleep_sec *= 2;
		if(i == 9){
			std::cout << ">>>>>>发送登录请求失败" << std::endl;
			exit(0);
		}
	}
}

// 登录应答
void CustomMdSpi::OnRspUserLogin(
	CThostFtdcRspUserLoginField *pRspUserLogin, 
	CThostFtdcRspInfoField *pRspInfo, 
	int nRequestID, 
	bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (!bResult)
	{
		std::cout << "=====账户登录成功=====" << std::endl;
		std::cout << "交易日： " << pRspUserLogin->TradingDay << std::endl;
		// 开始订阅行情
		int rt = g_pMdUserApi->SubscribeMarketData(g_pInstrumentID, instrumentNum);
		if (!rt)
			std::cout << ">>>>>>发送订阅行情请求成功" << std::endl;
		else
			std::cerr << "--->>>发送订阅行情请求失败" << std::endl;
	}
	else
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// 错误通知
void CustomMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (bResult)
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// 订阅行情应答
void CustomMdSpi::OnRspSubMarketData(
	CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
	CThostFtdcRspInfoField *pRspInfo, 
	int nRequestID, 
	bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (!bResult)
	{
		std::cout << "=====订阅行情成功=====" << std::endl;
		std::cout << "合约代码： " << pSpecificInstrument->InstrumentID << std::endl;
		// 如果需要存入文件或者数据库，在这里创建表头,不同的合约单独存储
		char filePath[100] = {'\0'};
		sprintf(filePath, "./stats/%s_market_data.csv", pSpecificInstrument->InstrumentID);
		std::ofstream outFile;
		outFile.open(filePath, std::ios::out); // 新开文件

		outFile << "TradingDay" << "," 
			<< "InstrumentID" << "," 
			<< "ExchangeID" << "," 
			<< "ExChangeInstID" << "," 
			<< "LastPrice" << "," 
			<< "PreSettlementPrice" << "," 
			<< "PreClosePrice" << "," 
			<< "PreOpenInterest" << "," 
			<< "OpenPrice" << "," 
			<< "HighestPrice" << "," 
			<< "LowestPrice" << "," 
			<< "Volume" << "," 
			<< "OpenInterest" << "," 
			<< "ClosePrice" << "," 
			<< "UpperLimitPrice" << "," 
			<< "LowerLimitPrice" << "," 
			<< "UpdateTime" << "," 
			<< "UpdateMillisec" << "," 
			<< "BidPrice1" << "," 
			<< "BidVolume1" << "," 
			<< "AskPrice1" << "," 
			<< "AskVolume1" << "," 
			<< "UpdateTimeUnix" << "," 
			<< "LocalTimeUnix" << std::endl;
		outFile.close();
	}
	else
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

time_t convertTimeStr2TimeStamp(std::string time1, std::string time2){
    struct tm timeinfo;
	std::string timeStr = time1 + time2;
    strptime(timeStr.c_str(), "%Y%m%d%H:%M:%S",  &timeinfo);
    return mktime(&timeinfo);
}

// 行情详情通知
void CustomMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	int64_t tsc = tscns.rdtsc();
	int64_t ns = tscns.tsc2ns(tsc);
	std::string t_date =  pDepthMarketData->TradingDay;
	std::string t_time = pDepthMarketData->UpdateTime;
	time_t fetchtime = convertTimeStr2TimeStamp(t_date, t_time);
	int msec = pDepthMarketData->UpdateMillisec;
	fetchtime *= 1000000000;
	if(msec == 500){
		fetchtime += 500000000;
	}
	std::cout << fetchtime << std::endl;
	char filePath[100] = {'\0'};
	sprintf(filePath, "./stats/%s_market_data.csv", pDepthMarketData->InstrumentID);
	std::ofstream outFile;
	outFile.open(filePath, std::ios::app); // 文件追加写入 
	outFile << pDepthMarketData->TradingDay << "," 
		<< pDepthMarketData->InstrumentID << "," 
		<< pDepthMarketData->ExchangeID << "," 
		<< pDepthMarketData->ExchangeInstID << "," 
		<< pDepthMarketData->LastPrice << "," 
		<< pDepthMarketData->PreSettlementPrice << "," 
		<< pDepthMarketData->PreClosePrice << "," 
		<< pDepthMarketData->PreOpenInterest << "," 
		<< pDepthMarketData->OpenPrice << "," 
		<< pDepthMarketData->HighestPrice << "," 
		<< pDepthMarketData->LowestPrice << "," 
		<< pDepthMarketData->Volume << "," 
		<< pDepthMarketData->OpenInterest << "," 
		<< pDepthMarketData->ClosePrice << "," 
		<< pDepthMarketData->UpperLimitPrice << "," 
		<< pDepthMarketData->LowerLimitPrice << "," 
		<< pDepthMarketData->UpdateTime << "," 
		<< pDepthMarketData->UpdateMillisec << "," 
		<< pDepthMarketData->BidPrice1 << "," 
		<< pDepthMarketData->BidVolume1 << "," 
		<< pDepthMarketData->AskPrice1 << "," 
		<< pDepthMarketData->AskVolume1 << ","
		<< fetchtime << ","
		<< ns << std::endl;
	outFile.close();
}

// Skipped functions
void CustomMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {}
void CustomMdSpi::OnRspUnSubForQuoteRsp(
	CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
	CThostFtdcRspInfoField *pRspInfo, 
	int nRequestID, bool bIsLast){}
void CustomMdSpi::OnRspSubForQuoteRsp(
	CThostFtdcSpecificInstrumentField *pSpecificInstrument,
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID,
	bool bIsLast){}
void CustomMdSpi::OnRspUnSubMarketData(
	CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID, 
	bool bIsLast){}
void CustomMdSpi::OnRspUserLogout(
	CThostFtdcUserLogoutField *pUserLogout,
	CThostFtdcRspInfoField *pRspInfo, 
	int nRequestID, 
	bool bIsLast){}
void CustomMdSpi::OnHeartBeatWarning(int nTimeLapse){}