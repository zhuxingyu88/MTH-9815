
#include <iostream>
#include <memory>
#include "tradebookingservice.hpp"
#include "positionservice.hpp"
#include "pricingservice.hpp"
#include "riskservice.hpp"
#include "marketdataservice.hpp"
#include "executionservice.hpp"
#include "streamingservice.hpp"
#include "inquiryservice.hpp"
#include "historicaldataservice.hpp"

using namespace std;

int main() {
    int numOftrades=18, numofprice=36, numofmarket=36, numofiq=36;
    map<string, Bond> m_bond;
    ifstream iFile("./Input/bonds.txt");//construct input file
    string line;//store each read line
    while(getline(iFile, line)){
        stringstream sStream(line);
        string tmp;
        vector<string> data;
        while (getline(sStream, tmp, ',')) {
            data.push_back(tmp);
        }
        date maturity(from_simple_string(data[3]));//get maturity
        Bond bond(data[0],CUSIP,data[2],stof(data[1]),maturity);//construct bond
        m_bond.insert(std::make_pair(data[0],bond));//add this entry to m_bond
    }
    vector<string> bids; //store bond ids
    map<string, double> m_bond_pv01;
    for(auto & it : m_bond){
        bids.push_back(it.first);//push bond ids to bids
    }
    //assign reasonable values as pv01
    m_bond_pv01[bids[0]]=0.295;
    m_bond_pv01[bids[1]]=0.102;
    m_bond_pv01[bids[2]]=0.031;
    m_bond_pv01[bids[3]]=0.017;
    m_bond_pv01[bids[4]]=0.0707;
    m_bond_pv01[bids[5]]=0.0455;
    PV01<Bond> temp(m_bond[bids[0]],0,0);
    //configure services, listeners, etc and link them together
    BondTradeBookService bt_service;//construct trade book service
    BondTradeBookingConnector bt_connector; //construct trade book connector
    BondPositionService bposition; //construct bond position service
    BondRiskService bndrisk(m_bond_pv01, m_bond); //construct bond risk service
    BondRiskHistoricalConnector b_risk_connector; //construct bond risk historical data connector
    BondRiskHistoricalData b_risk_data(b_risk_connector); //construct bond risk historical data service
    //and link with corresponding connector
    BondRiskRecordListener b_risk_record_listen(b_risk_data);//construct risk record listener
    auto b_pv01_listen = make_shared<BondPV01HistoricalListener>(temp);//construct pv01 listener for historical data
    //the temp will be modifed later
    bndrisk.AddListener(b_pv01_listen.get());//add pv01 listener to bond risk service
    //construct bond sectors risk listener and link with pv01 lister and risk record listener for historical data service
    auto b_sector_listen= make_shared<BondSectorsRiskListener>(*b_pv01_listen,b_risk_record_listen) ;
    bndrisk.AddListener(b_sector_listen.get());//add bond sectors listener to risk service
    //construct bond position connector for historical data
    BondPositionHistoricalConnector bp_his_connector;
    //construct bond position historical data service and link with connector
    BondPositionHistoricalData bp_his_data(bp_his_connector);
    //construct bond position listener for historical data service and linke with bond historical position servce
    auto bp_his_listener= make_shared<BondPositionHistoricalListener>(bp_his_data);
    //construct bond position listener and link with risk service
    auto bnd_pos_listener= make_shared<BondPositionServiceListener>(bndrisk) ;
    //add positionlisteners to bond position service
    bposition.AddListener(bnd_pos_listener.get());
    bposition.AddListener(bp_his_listener.get());
    //construct trade listener and link with bond position service
    auto ptr_bt_listen= make_shared<BondTradeListener>(bposition);
    //add trade listener to tradebooking service
    bt_service.AddListener(ptr_bt_listen.get());
    //flow trade data to trade book connector, no more than 60
    for(int i=0;i<numOftrades;++i){
        bt_connector.Subscribe(bt_service, m_bond);
    }
    //construct bond price service
    BondPriceService bp_service;
    //construct price connector
    BondPriceConnector bp_connector;
    //construct bond algo stream service
    BondAlgoStreamingService b_algo_stream;
    //construct bond price listener and link with algo stream service
    auto b_price_listener= make_shared<BondPriceListener>(b_algo_stream);
    //add bond price listener to bond price serivce
    bp_service.AddListener(b_price_listener.get());
    //construct bond stream service
    BondStreamingService b_stream_service;
    //construct bond stream connector for historical data
    BondStreamHistoricalConnector b_stream_connect;
    //construct bond stream historical service and link with connector
    BondStreamHistoricalData b_stream_data(b_stream_connect);
    //construct bond stream listener for historical data service and link with bond stream historical service
    auto b_stream_listen= make_shared<BondStreamHistoricalListener>(b_stream_data);
    //add listener to bond stream service
    b_stream_service.AddListener(b_stream_listen.get());
    //construct bond algo stream listener and link with bond stream service
    auto b_algo_stream_listener = make_shared<BondAlgoStreamListener>(b_stream_service);
    //add bond algo stream listener to bond algo stream service
    b_algo_stream.AddListener(b_algo_stream_listener.get() );
    //flow price data to bond price connector
    for(int i=1;i<=numofprice;++i){
        bp_connector.Subscribe(bp_service,m_bond);
    }
    //test the update pv01 function
    bndrisk.UpdateBondPV01(bids[2],0.03);
    //construct bond execution service
    BondExecutionService b_exe_service;
    //construct bond execution connector for historical data
    BondExecutionHistoricalConnector b_exe_connect;
    //construct bond execution historical data service and link with connector
    BondExecutionHistoricalData b_exe_data(b_exe_connect);
    //construct bond executionorder listener and link with bond execution historical data service
    auto b_exe_listen= make_shared<BondExecutionHistoricalListener>(b_exe_data);
    //construct bond algoexecution listener and link with bond execution service
    auto b_algo_listener= make_shared<BondAlgoExecutionListener>(b_exe_service);
    //add bond execution listener to bond execution service
    b_exe_service.AddListener(b_exe_listen.get());
    //construct market data service
    BondMarketDataService bm_ds;
    //construct bond algo execution service
    BondAlgoExecutionService b_algo_exe;
    //add algo listener to bond algo execution service
    b_algo_exe.AddListener(b_algo_listener.get());
    //construct bond market data listener and link with bond algo execution service
    auto b_mkt_listener= make_shared<BondMarketDataListeners>(b_algo_exe) ;
    //add bond market data listener to market data service
    bm_ds.AddListener(b_mkt_listener.get());

    //construct bond market data connector
    BondMarketDataConnector bm_connect;
    //flow market data to bond market data service
    for(int i=0;i<numofmarket;++i){
        bm_connect.Subscribe(bm_ds,m_bond);
    }
    //construct inquiry connector for publish
    BondInquiryPublishConnector b_publish;
    //construct inquiry connector for historical data
    BondIqHistoricalConnector b_iq_hist_connect;
    //construct bond inquiry historical data service and link with connector
    BondIqHistoricalData b_iq_data(b_iq_hist_connect);
    //construct bond inquiry historical listener and link with bond inquiry historical data service
    auto b_iq_hist_listen= make_shared<BondIqHistoricalListener>(b_iq_data);
    //construct bond inquiry service and link with connector
    BondInquiryService b_inquire(b_publish);
    //construct bond inquiry service listener and link with bond inquiry service
    auto b_iq_listen= make_shared<BondInquiryListener>(b_inquire);
    //add listeners to bond inquiry service
    b_inquire.AddListener(b_iq_hist_listen.get());
    b_inquire.AddListener(b_iq_listen.get());
    //construct bond inquiry connector
    BondInquiryConnector b_iq_connect;
    //flow data into bond inquiry service, no more than 60
    for(int i=0;i<numofiq;++i){
        b_iq_connect.Subscribe(b_inquire,m_bond);
    }
    return 0;
}