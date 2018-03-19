#define _CRT_SECURE_NO_WARNINGS
#undef UNICODE
#undef _UNICODE
#include <windows.h>
#include <stdio.h>
#include <direct.h>
#include <stdlib.h>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "NxCoreAPI.h"
#include "NxCoreAPI_class.h"

using namespace std;

static NxCoreClass nxCoreClass;

struct OUTPUT { // Declare OUTPUT struct type
				// Trades
				// Trades
	char tradedir[_MAX_DIR];
	char tradefname[_MAX_FNAME];
	// Exchange Quotes
	char exgquotedir[_MAX_DIR];
	char exgquotefname[_MAX_FNAME];
	// MarketMaker Quotes
	//char mmquotedir[_MAX_DIR];
	//char mmquotefname[_MAX_FNAME];
	// Symbol Change
	//char symboldir[_MAX_DIR];
	//char symbolfname[_MAX_FNAME];
};

struct OUTPUTFILES {
	ofstream tradeStream;
	ofstream qExgStream;
	//ofstream qMmStream;
	//ofstream symStream;
};
OUTPUTFILES outFiles = {};

void getSymbol(const NxCoreMessage* pNxCoreMsg, char *Symbol)
{
	// Is this a valid option?
	if ((pNxCoreMsg->coreHeader.pnxStringSymbol->String[0] == 'o') && (pNxCoreMsg->coreHeader.pnxOptionHdr))
	{
		// If pnxsDateAndStrike->String[1] == ' ', then this symbol is in new OSI format.
		if (pNxCoreMsg->coreHeader.pnxOptionHdr->pnxsDateAndStrike->String[1] == ' ')
		{
			sprintf(Symbol, "%s,(opt_new),%04d-%02d-%02d,%c,%08d",
				pNxCoreMsg->coreHeader.pnxStringSymbol->String,
				pNxCoreMsg->coreHeader.pnxOptionHdr->nxExpirationDate.Year,
				pNxCoreMsg->coreHeader.pnxOptionHdr->nxExpirationDate.Month,
				pNxCoreMsg->coreHeader.pnxOptionHdr->nxExpirationDate.Day,
				(pNxCoreMsg->coreHeader.pnxOptionHdr->PutCall == 0) ? 'C' : 'P',
				pNxCoreMsg->coreHeader.pnxOptionHdr->strikePrice);
		}
		// Otherwise the symbol is in old OPRA format.
		else
		{
			sprintf(Symbol, "%s,(opt_old),%c,%c",
				pNxCoreMsg->coreHeader.pnxStringSymbol->String,
				pNxCoreMsg->coreHeader.pnxOptionHdr->pnxsDateAndStrike->String[0],
				pNxCoreMsg->coreHeader.pnxOptionHdr->pnxsDateAndStrike->String[1]);
		}
	}
	// Not an option, just copy the symbol
	else
	{
		sprintf(Symbol, "%s,(non_opt)", pNxCoreMsg->coreHeader.pnxStringSymbol->String);
	}
}
inline const char* symbol(char* sym, NxString* nx, NxOptionHdr* oh)
{
	// If an option
	if (oh)
	{
		// If pnxsDateAndStrike->String[1] == ' ', then this symbol is in new OSI format. 	
		if (oh->pnxsDateAndStrike->String[1] == ' ')
		{
			sprintf(sym, "%s,(opt_new),%04d-%02d-%02d,%c,%08d",
				nx->String,
				oh->nxExpirationDate.Year,
				oh->nxExpirationDate.Month,
				oh->nxExpirationDate.Day,
				(oh->PutCall == 0) ? 'C' : 'P',
				oh->strikePrice);
		}
		// Otherwise the symbol is in old OPRA format.
		else
		{
			sprintf(sym, "%s,(opt_old),%c,%c",
				nx->String,
				oh->pnxsDateAndStrike->String[0],
				oh->pnxsDateAndStrike->String[1]);
		}
	}
	// otherwise a non-option
	else
	{
		sprintf(sym, "%s,(non_opt)", nx->String);
	}

	return sym;
}
int processNxCoreTrade(const NxCoreSystem* pNxCoreSys, const NxCoreMessage* pNxCoreMsg)
{
	const NxCoreTrade& nt = pNxCoreMsg->coreData.Trade;
	const NxCoreHeader& ch = pNxCoreMsg->coreHeader;
	const NxTime&       t = pNxCoreSys->nxTime;
	const NxDate& d = pNxCoreSys->nxDate;
	char symbol[128];
	getSymbol(pNxCoreMsg, symbol);

	//Output to file
	//outFiles.tradeStream << "trade" << "\t";
	outFiles.tradeStream << (int)d.Year << "-";									// System Date (yyyy-mm-dd)
	outFiles.tradeStream << setfill('0') << setw(2) << (int)d.Month << "-";
	outFiles.tradeStream << setfill('0') << setw(2) << (int)d.Day << "|";
	outFiles.tradeStream << setfill('0') << setw(2) << (int)t.Hour << ":";		// System Time (HH:MM:SS.NNN)
	outFiles.tradeStream << setfill('0') << setw(2) << (int)t.Minute << ":";
	outFiles.tradeStream << setfill('0') << setw(2) << (int)t.Second << ".";
	outFiles.tradeStream << setfill('0') << setw(3) << (int)t.Millisecond << "|";
	outFiles.tradeStream << symbol << "|";
	outFiles.tradeStream << nt.Size << "|";									// Trade Size
	outFiles.tradeStream << fixed << setprecision(3) << nxCoreClass.PriceToDouble(nt.Price, nt.PriceType) << "|";	// Trade Price
	outFiles.tradeStream << nt.TotalVolume << "\n";								// Trade Total Volume

	return NxCALLBACKRETURN_CONTINUE;
}
int processNxCoreExgQuote(const NxCoreSystem* pNxCoreSys, const NxCoreMessage* pNxCoreMsg)
{
	const NxCoreHeader&   ch = pNxCoreMsg->coreHeader;
	const NxCoreExgQuote& eq = pNxCoreMsg->coreData.ExgQuote;
	const NxCoreQuote&    cq = eq.coreQuote;
	const NxTime& t = pNxCoreSys->nxTime;
	const NxDate& d = pNxCoreSys->nxDate;
	char symbol[128];
	getSymbol(pNxCoreMsg, symbol);

	// Output
	//outFiles.qExgStream << "NBBO quote" << "\t";
	outFiles.qExgStream << (int)d.Year << "-";									// System Date (yyyy-mm-dd)
	outFiles.qExgStream << setfill('0') << setw(2) << (int)d.Month << "-";
	outFiles.qExgStream << setfill('0') << setw(2) << (int)d.Day << "|";
	outFiles.qExgStream << setfill('0') << setw(2) << (int)t.Hour << ":";		// System Time (HH:MM:SS.NNN)
	outFiles.qExgStream << setfill('0') << setw(2) << (int)t.Minute << ":";
	outFiles.qExgStream << setfill('0') << setw(2) << (int)t.Second << ".";
	outFiles.qExgStream << setfill('0') << setw(3) << (int)t.Millisecond << "|";
	outFiles.qExgStream << symbol << "|";										// Symbol
	outFiles.qExgStream << (int)eq.BestAskSize << "|";	
	outFiles.qExgStream << fixed << setprecision(3) << nxCoreClass.PriceToDouble(eq.BestAskPrice, cq.PriceType) << "|";
	outFiles.qExgStream << (int)eq.BestBidSize << "|";
	outFiles.qExgStream << fixed << setprecision(3) << nxCoreClass.PriceToDouble(eq.BestBidPrice, cq.PriceType) << "\n";

	return NxCALLBACKRETURN_CONTINUE;
}
/*int processNxCoreMMQuote(const NxCoreSystem* pNxCoreSys, const NxCoreMessage* pNxCoreMsg)
{
	const NxCoreHeader& ch = pNxCoreMsg->coreHeader;
	const NxCoreMMQuote& mm = pNxCoreMsg->coreData.MMQuote;
	const NxCoreQuote& cq = mm.coreQuote;
	const NxTime& t = pNxCoreSys->nxTime;
	const NxDate& d = pNxCoreSys->nxDate;
	char symbol[128];
	getSymbol(pNxCoreMsg, symbol);
	char buf[1024] = { 0 };
	char* p = buf;

	// Output
	outFiles.qMmStream << "mmquote" << "\t";
	outFiles.qMmStream << (int)d.Year << "-";									// System Date (yyyy-mm-dd)
	outFiles.qMmStream << setfill('0') << setw(2) << (int)d.Month << "-";
	outFiles.qMmStream << setfill('0') << setw(2) << (int)d.Day << "\t";
	outFiles.qMmStream << setfill('0') << setw(2) << (int)t.Hour << ":";		// System Time (HH:MM:SS.NNN)
	outFiles.qMmStream << setfill('0') << setw(2) << (int)t.Minute << ":";
	outFiles.qMmStream << setfill('0') << setw(2) << (int)t.Second << ".";
	outFiles.qMmStream << setfill('0') << setw(3) << (int)t.Millisecond << "\t";
	outFiles.qMmStream << (int)t.TimeZone << "\t";							// System Time Zone
	outFiles.qMmStream << (int)d.DSTIndicator << "\t";						// DST Indicator
	outFiles.qMmStream << (int)d.NDays << "\t";								// Number of Days since 1883-01-01
	outFiles.qMmStream << (int)d.DayOfWeek << "\t";							// Day of Week
	outFiles.qMmStream << (int)d.DayOfYear << "\t";							// Day of Year
	outFiles.qMmStream << (int)ch.nxSessionDate.Year << "-";					// nxSessionDate
	outFiles.qMmStream << setfill('0') << setw(2) << (int)ch.nxSessionDate.Month;
	outFiles.qMmStream << "-" << setfill('0') << setw(2) << (int)ch.nxSessionDate.Day << "\t";
	outFiles.qMmStream << (int)ch.nxSessionDate.DSTIndicator << "\t";			// Session DST Indicator
	outFiles.qMmStream << (int)ch.nxSessionDate.NDays << "\t";				// Session Number of Days since 1883-01-01
	outFiles.qMmStream << (int)ch.nxSessionDate.DayOfWeek << "\t";			// Session Day of Week
	outFiles.qMmStream << (int)ch.nxSessionDate.DayOfYear << "\t";			// Session Day of Year
	outFiles.qMmStream << setfill('0') << setw(2) << (int)ch.nxExgTimestamp.Hour << ":"; // nxExgTimestamp
	outFiles.qMmStream << setfill('0') << setw(2) << (int)ch.nxExgTimestamp.Minute << ":";
	outFiles.qMmStream << setfill('0') << setw(2) << (int)ch.nxExgTimestamp.Second << ".";
	outFiles.qMmStream << setfill('0') << setw(3) << (int)ch.nxExgTimestamp.Millisecond << "\t";
	outFiles.qMmStream << (int)ch.nxExgTimestamp.TimeZone << "\t";			// Exchange Timestamp Time Zone
	outFiles.qMmStream << symbol << "\t";										// Symbol
	outFiles.qMmStream << (int)ch.ListedExg << "\t";							// Listed Exchange Index
																				//outFiles.qMmStream << nxCoreClass.GetDefinedString(NxST_EXCHANGE, ch.ListedExg) << "\t";		// Listed Exchange
	outFiles.qMmStream << (int)ch.ReportingExg << "\t";						// Reporting Exchange Index
																			//outFiles.qMmStream << nxCoreClass.GetDefinedString(NxST_EXCHANGE, ch.ReportingExg) << "\t";	// Reporting Exchange
	outFiles.qMmStream << (int)ch.SessionID << "\t";							// Session ID
																				//outFiles.qMmStream << (int)cq.AskPrice << "\t";
	outFiles.qMmStream << fixed << setprecision(3) << nxCoreClass.PriceToDouble((int)cq.AskPrice, cq.PriceType) << "\t";
	//outFiles.qMmStream << (int)cq.AskPriceChange << "\t";
	outFiles.qMmStream << fixed << setprecision(3) << nxCoreClass.PriceToDouble((int)cq.AskPriceChange, cq.PriceType) << "\t";
	outFiles.qMmStream << (int)cq.AskSize << "\t";
	outFiles.qMmStream << (int)cq.AskSizeChange << "\t";
	//outFiles.qMmStream << (int)cq.BidPrice << "\t";
	outFiles.qMmStream << fixed << setprecision(3) << nxCoreClass.PriceToDouble(cq.BidPrice, cq.PriceType) << "\t";
	//outFiles.qMmStream << (int)cq.BidPriceChange << "\t";
	outFiles.qMmStream << fixed << setprecision(3) << nxCoreClass.PriceToDouble(cq.BidPriceChange, cq.PriceType) << "\t";
	outFiles.qMmStream << (int)cq.BidSize << "\t";
	outFiles.qMmStream << (int)cq.BidSizeChange << "\t";
	outFiles.qMmStream << (int)cq.NasdaqBidTick << "\t";
	outFiles.qMmStream << (int)cq.PriceType << "\t";
	outFiles.qMmStream << (int)cq.QuoteCondition << "\t";
	//outFiles.qMmStream << nxCoreClass.GetDefinedString(NxST_QUOTECONDITION, cq.QuoteCondition) << "\t";
	outFiles.qMmStream << (int)cq.Refresh << "\t";
	outFiles.qMmStream << (int)mm.MarketMakerType << "\t";
	outFiles.qMmStream << mm.pnxStringMarketMaker->String << "\t";
	outFiles.qMmStream << (int)mm.QuoteType << "\n";

	return NxCALLBACKRETURN_CONTINUE;
}
int processNxCoreSymbolChange(const NxCoreSystem* pNxCoreSys, const NxCoreMessage* pNxCoreMsg)
{
	const NxCoreSymbolChange& sc = pNxCoreMsg->coreData.SymbolChange;
	const NxCoreHeader&       ch = pNxCoreMsg->coreHeader;
	const NxDate& d = pNxCoreSys->nxDate;
	char symN[128];
	char symO[128];
	char outString[250];
	switch (sc.Status)
	{
	case NxSS_ADD:
	{
		sprintf(outString, "%.2d:%.2d:%.2d.%.3d\tADD\t%s\t%ld",
			(int)pNxCoreSys->nxTime.Hour,
			(int)pNxCoreSys->nxTime.Minute,
			(int)pNxCoreSys->nxTime.Second,
			(int)pNxCoreSys->nxTime.Millisecond,
			symbol(symN, ch.pnxStringSymbol, ch.pnxOptionHdr), ch.ListedExg);

		break;
	}
	case NxSS_DEL:
	{
		sprintf(outString, "%.2d:%.2d:%.2d.%.3d\tDEL\t%s\t%ld\n",
			(int)pNxCoreSys->nxTime.Hour,
			(int)pNxCoreSys->nxTime.Minute,
			(int)pNxCoreSys->nxTime.Second,
			(int)pNxCoreSys->nxTime.Millisecond,
			symbol(symO, ch.pnxStringSymbol, ch.pnxOptionHdr), ch.ListedExg);
		break;
	}
	case NxSS_MOD:
	{
		sprintf(outString, "%.2d:%.2d:%.2d.%.3d\tMOD\t%s\t%ld\t%s\t%ld\n",
			(int)pNxCoreSys->nxTime.Hour,
			(int)pNxCoreSys->nxTime.Minute,
			(int)pNxCoreSys->nxTime.Second,
			(int)pNxCoreSys->nxTime.Millisecond,
			symbol(symN, sc.pnxsSymbolOld, sc.pnxOptionHdrOld), sc.ListedExgOld,
			symbol(symO, ch.pnxStringSymbol, ch.pnxOptionHdr), ch.ListedExg);
		break;
	}
	}

	outFiles.symStream << "symbolchange" << "\t";
	outFiles.symStream << (int)d.Year << "-";									// System Date (yyyy-mm-dd)
	outFiles.symStream << setfill('0') << setw(2) << (int)d.Month << "-";
	outFiles.symStream << setfill('0') << setw(2) << (int)d.Day << "\t";
	outFiles.symStream << outString << "\n";
	return NxCALLBACKRETURN_CONTINUE;
} */
int __stdcall OnNxCoreCallback(const NxCoreSystem* pNxCoreSys, const NxCoreMessage* pNxCoreMsg)
{
	switch (pNxCoreMsg->MessageType)
	{
	case NxMSG_STATUS:       break;
	case NxMSG_EXGQUOTE:     return processNxCoreExgQuote(pNxCoreSys, pNxCoreMsg);
	case NxMSG_MMQUOTE:      break; //return processNxCoreMMQuote(pNxCoreSys, pNxCoreMsg);
	case NxMSG_TRADE:        return processNxCoreTrade(pNxCoreSys, pNxCoreMsg);
	case NxMSG_CATEGORY:     break;
	case NxMSG_SYMBOLCHANGE: break; // return processNxCoreSymbolChange(pNxCoreSys, pNxCoreMsg);
	case NxMSG_SYMBOLSPIN:   break;
	}
	return NxCALLBACKRETURN_CONTINUE;
}

int main(int argc, char** argv)
{
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	char outputdir[_MAX_DIR];
	OUTPUT outputInfo;

	if (!nxCoreClass.LoadNxCore("F:\\CU\\18 spring\\model based trading\\hw\\bonus1\\Try1\\x64\\Release\\NxCoreAPI.dll") &&
		!nxCoreClass.LoadNxCore("F:\\CU\\18 spring\\model based trading\\hw\\bonus1\\Try1\\x64\\Release\\NxCoreAPI64.dll"))
	{
		fprintf(stderr, "Can't find NxCoreAPI.dll\n");
		return -1;
	}

	if (NULL == argv[1])
	{
		fprintf(stderr, "There is no input file\n");
		return -1;
	}
	// Create Output Directories
	_splitpath(argv[1], drive, dir, fname, ext);
	sprintf(outputdir, "%s%s\\processed\\", drive, dir);
	sprintf(outputInfo.tradedir, "%s\\trade", outputdir);
	sprintf(outputInfo.exgquotedir, "%s\\exgquote", outputdir);

	_mkdir(outputdir);
	_mkdir(outputInfo.tradedir);
	_mkdir(outputInfo.exgquotedir);

	// Create Output File Names
	sprintf(outputInfo.tradefname, "%s\\%s_TRADES.csv", outputInfo.tradedir, fname);
	sprintf(outputInfo.exgquotefname, "%s\\%s_EXGQUOTE.csv", outputInfo.exgquotedir, fname);

	// Open File Handles
	outFiles.tradeStream.open(outputInfo.tradefname, std::ios::app);
	outFiles.qExgStream.open(outputInfo.exgquotefname, std::ios::app);

	printf("Directory: %s%s\tTape: %s%s\n", drive, dir, fname, ext);

	outFiles.tradeStream << "Date" << "|"<<"Time"<<"|"<<"Ticker"<<"|"<<"Quantity"<<"|"<<"Price"<<"|"<<"Total Volumn"<<"\n";
	outFiles.qExgStream <<  "Date" << "|" << "Time" << "|" << "Ticker" << "|" <<"Ask Size"<<"|" <<"Ask Price" << "|" <<"Bid Size"<<"|" <<"Bid Price" <<"\n";

	nxCoreClass.ProcessTape(argv[1], 0, NxCF_EXCLUDE_CRC_CHECK, 0, OnNxCoreCallback);     // Export all data

	outFiles.tradeStream.close();
	outFiles.qExgStream.close();

	return 0;
}