/**
 * historicaldataservice.hpp
 * historicaldataservice.hpp
 *
 * @author Breman Thuraisingham
 * Defines the data types and Service for historical data.
 *
 * @author Breman Thuraisingham
 */
#ifndef HISTORICAL_DATA_SERVICE_HPP
#define HISTORICAL_DATA_SERVICE_HPP

#include <string>
#include "soa.hpp"
#include "products.hpp"
#include "positionservice.hpp"
#include "executionservice.hpp"
#include "inquiryservice.hpp"
#include "marketdataservice.hpp"
#include "pricingservice.hpp"
#include "riskservice.hpp"
#include "streamingservice.hpp"
#include "tradebookingservice.hpp"

/**
 * Service for processing and persisting historical data to a persistent store.
 * Keyed on some persistent key.
 * Type T is the data type to persist.
 */
template<typename T>
class HistoricalDataService : Service<string,T>
{

public:

  // Persist data to a store
  virtual void PersistData(string persistKey, const T& data) = 0;

};

class BondPositionHistoricalConnector: public Connector<pair<string, Position<Bond> > > {
public:
    void Publish(pair<string, Position<Bond> > &data) override;
};

class BondPositionHistoricalData: public HistoricalDataService<Position<Bond> > {
private:
    int counter;
    map<string, Position<Bond> > bondHistoricalPositionCache;
    vector<ServiceListener<Position<Bond> >* > bondPositionListeners;
    BondPositionHistoricalConnector& b_pos_historical;
public:
    BondPositionHistoricalData(BondPositionHistoricalConnector& src):b_pos_historical(src){counter=1;}

    Position<Bond>& GetData(string key) override{return bondHistoricalPositionCache.find(key)->second;}

    void OnMessage(Position<Bond> &data) override {}

    void AddListener(ServiceListener<Position<Bond> > *listener) override {bondPositionListeners.push_back(listener);}

    const vector< ServiceListener<Position<Bond> >* >& GetListeners() const override {return bondPositionListeners;}

    void PersistData(string persistKey, const Position<Bond>& data) override{
        pair<string, Position<Bond> > dataPair=make_pair(persistKey,data);
        b_pos_historical.Publish(dataPair);
    }

    void SetPersistKey(Position<Bond>& data){
        string k=to_string(counter);
        ++counter;
        PersistData(k,data);
    }
};

class BondPositionHistoricalListener: public ServiceListener<Position<Bond> > {
private:
    BondPositionHistoricalData& b_historical_data;
public:
    BondPositionHistoricalListener(BondPositionHistoricalData& src): b_historical_data(src){}

    void ProcessAdd(Position<Bond> &data) override{}

    void ProcessRemove(Position<Bond> &data) override{}

    void ProcessUpdate(Position<Bond> &data) override {b_historical_data.SetPersistKey(data);}
};

void BondPositionHistoricalConnector::Publish(pair<string, Position<Bond> > &data){
    ofstream oFile;
    oFile.open("./Output/Historical/position.txt", ios_base::app);//open the file to append
    string k=data.first;
    oFile << data.first << ",";
    oFile << data.second.GetProduct().GetProductId() << ",";
    oFile << to_string(data.second.GetAggregatePosition()) << ",";
    string book="TRSY1";
    oFile << to_string(data.second.GetPosition(book)) << ",";
    book="TRSY2";
    oFile << to_string(data.second.GetPosition(book)) << ",";
    book="TRSY3";
    oFile << to_string(data.second.GetPosition(book)) << "\n";
    oFile.close();
}

class BondRiskRecord {
public:
    string persistKey;
    PV01<Bond> b_pv01;//the bond's pv01 to be updated or added
    PV01<BucketedSector<Bond> > front_end, belly, long_end;
    BondRiskRecord(PV01<Bond>& src1, PV01<BucketedSector<Bond> >& f1, PV01<BucketedSector<Bond> >& b1, PV01<BucketedSector<Bond> >& l1):persistKey("123"), b_pv01(src1),front_end(f1),belly(b1),long_end(l1){}
};

class BondRiskHistoricalConnector: public Connector<BondRiskRecord> {
public:
    void Publish(BondRiskRecord &data) override {
        ofstream oFile;
        oFile.open("./Output/Historical/risk.txt", ios_base::app);//open the file to append
        oFile << data.persistKey << ",";
        //get pv01
        PV01<Bond> thepv01=data.b_pv01;
        oFile << data.b_pv01.GetProduct().GetProductId() << ",";
        //get pv01
        double pv01_d=thepv01.GetPV01();
        oFile << to_string(data.b_pv01.GetQuantity()) << ",";
        PV01<BucketedSector<Bond> > sector_pv01 = data.front_end;
        oFile << to_string(sector_pv01.GetPV01()) << ",";
        oFile << to_string(data.belly.GetPV01()) << ",";
        oFile << to_string(data.long_end.GetPV01()) << "\n";
        oFile.close();
    }
};

class BondRiskHistoricalData: public HistoricalDataService<BondRiskRecord> {
private:
    int counter;
    map<string, BondRiskRecord> bondRecordRiskCache;
    vector<ServiceListener<BondRiskRecord>* > bondRiskListeners;
    BondRiskHistoricalConnector& b_risk_historical;
public:
    explicit BondRiskHistoricalData(BondRiskHistoricalConnector& src):b_risk_historical(src) {counter=1;}

    BondRiskRecord& GetData(string key) override{return bondRecordRiskCache.find(key)->second;}

    void OnMessage(BondRiskRecord &data) override{}

    void AddListener(ServiceListener<BondRiskRecord> *listener) override {
        bondRiskListeners.push_back(listener);
    }

    const vector< ServiceListener<BondRiskRecord>* >& GetListeners() const override {
        return bondRiskListeners;
    }

    void PersistData(string persistKey, const BondRiskRecord& data) override {
        BondRiskRecord datacopy=data;
        b_risk_historical.Publish(datacopy);
    }

    void SetPersistKey(BondRiskRecord& data){
        string k = to_string(counter);
        ++counter;
        data.persistKey = k;
        BondRiskRecord data_copy = data;
        PersistData(k,data_copy);
    }
};

class BondRiskRecordListener: public ServiceListener<BondRiskRecord> {
private:
    BondRiskHistoricalData& b_historical_data;//to flow into
public:
    explicit BondRiskRecordListener(BondRiskHistoricalData& src): b_historical_data(src){}

    void ProcessAdd(BondRiskRecord &data) override{}

    void ProcessRemove(BondRiskRecord &data) override{}

    void ProcessUpdate(BondRiskRecord &data) override{
        b_historical_data.SetPersistKey(data);
    }

    void SetUpdate(PV01<Bond>& data1, SectorsRisk& data2) {
        BondRiskRecord bnd_risk_record(data1,std::get<0>(data2),std::get<1>(data2),std::get<2>(data2));
        ProcessUpdate(bnd_risk_record);
    }
};

class BondPV01HistoricalListener: public ServiceListener<PV01<Bond> > {
private:
    PV01<Bond> theData;
    bool needProcessed;
public:
    explicit BondPV01HistoricalListener(PV01<Bond>& data_):theData(data_),needProcessed(false){}

    void ProcessAdd(PV01<Bond> &data) override{
        theData=data;needProcessed=true;
    }

    void ProcessRemove(PV01<Bond> &data) override{}

    void ProcessUpdate(PV01<Bond> &data) override{}

    PV01<Bond> GetData(){return theData;}

    void SetProcessed(bool s){needProcessed=s;}

    bool GetProcessed() const{return needProcessed;}
};

class BondSectorsRiskListener: public ServiceListener<SectorsRisk> {
private:
    BondPV01HistoricalListener& b_pv01_listener;
    BondRiskRecordListener& b_risk_record_listener;
public:
    BondSectorsRiskListener(BondPV01HistoricalListener& src1, BondRiskRecordListener& src2): b_pv01_listener(src1),b_risk_record_listener(src2) {}

    void ProcessAdd(SectorsRisk &data) override{}

    void ProcessRemove(SectorsRisk &data) override{}

    void ProcessUpdate(SectorsRisk &data) override {
        bool status=b_pv01_listener.GetProcessed();//get status
        if(status){
            b_pv01_listener.SetProcessed(false);//update the status
            PV01<Bond> bnd_pv01=b_pv01_listener.GetData(); //get the data
            b_risk_record_listener.SetUpdate(bnd_pv01,data);
        }
    }
};

class BondExecutionHistoricalConnector: public Connector<pair<string,ExecutionOrder<Bond> > > {
public:
    void Publish(pair<string, ExecutionOrder<Bond> > &data) override;
};

class BondExecutionHistoricalData: public HistoricalDataService<ExecutionOrder<Bond> >{
private:
    int counter;//count record to determine the key
    map<string, ExecutionOrder<Bond> > bondHistoricalCache;
    vector<ServiceListener<ExecutionOrder<Bond> >* > bondListeners;//listeners
    BondExecutionHistoricalConnector& b_historical;//connector to output file
public:
    explicit BondExecutionHistoricalData(BondExecutionHistoricalConnector& src):b_historical(src){counter=1;}

    ExecutionOrder<Bond>& GetData(string key) override {return bondHistoricalCache.find(key)->second;}

    void OnMessage(ExecutionOrder<Bond> &data) override{}//do nothing

    void AddListener(ServiceListener<ExecutionOrder<Bond> > *listener) override{bondListeners.push_back(listener);}

    const vector< ServiceListener<ExecutionOrder<Bond> >* >& GetListeners() const override{return bondListeners;}

    void PersistData(string persistKey, const ExecutionOrder<Bond>& data) override{
        pair<string, ExecutionOrder<Bond> > dataPair=make_pair(persistKey,data);//construct data pair
        b_historical.Publish(dataPair);//publish data to file
    }

    void SetPersistKey(ExecutionOrder<Bond>& data){
        string k=to_string(counter); ++counter;
        PersistData(k,data);
    }
};

class BondExecutionHistoricalListener: public ServiceListener<ExecutionOrder<Bond> >{
private:
    BondExecutionHistoricalData& b_historical_data;//to flow into
public:
    explicit BondExecutionHistoricalListener(BondExecutionHistoricalData& src): b_historical_data(src){}

    void ProcessUpdate(ExecutionOrder<Bond> &data) override{}

    void ProcessRemove(ExecutionOrder<Bond> &data) override{}

    void ProcessAdd(ExecutionOrder<Bond> &data) override{b_historical_data.SetPersistKey(data);}
};

void BondExecutionHistoricalConnector::Publish(pair<string, ExecutionOrder<Bond> > &data){
    ofstream oFile;
    oFile.open("./Output/Historical/executions.txt", ios_base::app);//open the file to append
    oFile << data.first << ",";
//    ExecutionOrder<Bond> exe_order=data.second;//get the execution order
    oFile << data.second.GetOrderId() << ",";//write orderid to file
    oFile << data.second.GetProduct().GetProductId() << ",";//write cusip to file
    PricingSide side=data.second.GetSide();//Get side
    if(side==BID){
        oFile << "BID,";//write side to file
    } else{
        oFile << "OFFER,";//write side to file
    }
    OrderType o_type = data.second.GetOrderType();//get order type
    switch(o_type){
        case FOK: oFile << "FOK,";
            break;
        case IOC: oFile << "IOC,";
            break;
        case MARKET: oFile << "MARKET,";
            break;
        case LIMIT: oFile << "LIMIT,";
            break;
        case STOP: oFile << "STOP,";
            break;
    }
    oFile << to_string(data.second.GetVisibleQuantity()) << ",";//write to file
    oFile << to_string(data.second.GetHiddenQuantity()) << ",";//write to file
    double p=data.second.GetPrice();//get price
    int part1=int(p);//get the integer part of price
    double p2=p-double(part1);//get the decimal part of price
    int part2=int(p2*32);//get part2 of price
    double p3=p2-double(part2)/32;//get last part of price
    int part3=round(p3*256);//get part3 of price
    string p_str;//store the string of price
    if(part2>=10){
        p_str=to_string(part1)+"-"+to_string(part2)+to_string(part3);//construct price string
    }
    else{
        p_str=to_string(part1)+"-"+"0"+to_string(part2)+to_string(part3);
    }
    oFile << p_str << "\n";//write to file
    oFile.close();
}

class BondIqHistoricalConnector: public Connector<pair<string,Inquiry<Bond> > >
{
public:
    // Publish data to the Connector
    virtual void Publish(pair<string, Inquiry<Bond> > &data);
};

class BondIqHistoricalData: public HistoricalDataService<Inquiry<Bond> >
{
private:
    int counter;//count record to determine the key
    map<string, Inquiry<Bond> > bondHistoricalCache;
    vector<ServiceListener<Inquiry<Bond> >* > bondListeners;//listeners
    BondIqHistoricalConnector& b_historical;//connector to output file
public:
    BondIqHistoricalData(BondIqHistoricalConnector& src):b_historical(src){counter=1;}//constructor

    Inquiry<Bond>& GetData(string key) override{return bondHistoricalCache.find(key)->second;}

    void OnMessage(Inquiry<Bond> &data) override{}//do nothing

    void AddListener(ServiceListener<Inquiry<Bond> > *listener) override{bondListeners.push_back(listener);}

    const vector< ServiceListener<Inquiry<Bond> >* >& GetListeners() const override{return bondListeners;}

    void PersistData(string persistKey, const Inquiry<Bond>& data) override{
        pair<string, Inquiry<Bond> > dataPair=make_pair(persistKey,data);//construct data pair
        b_historical.Publish(dataPair);//publish data to file
    }

    void SetPersistKey(Inquiry<Bond>& data){
        //get key from counter and increment counter
        string k=to_string(counter); ++counter;
        Inquiry<Bond> data_copy=data;
        PersistData(k,data_copy);
    }
};

class BondIqHistoricalListener: public ServiceListener<Inquiry<Bond> > {
private:
    BondIqHistoricalData& b_historical_data;
public:
    BondIqHistoricalListener(BondIqHistoricalData& src): b_historical_data(src){}

    void ProcessUpdate(Inquiry<Bond> &data) override{
        b_historical_data.SetPersistKey(data);
        InquiryState s=data.GetState();//get state
        if(s==QUOTED){
            data.SetState(DONE);//update to done
            b_historical_data.SetPersistKey(data);//flow updated data
        }
    }

    void ProcessRemove(Inquiry<Bond> &data) override{}

    void ProcessAdd(Inquiry<Bond> &data) override{b_historical_data.SetPersistKey(data);}
};

void BondIqHistoricalConnector::Publish(pair<string, Inquiry<Bond> > &data1){
    ofstream oFile;
    oFile.open("./Output/Historical/allinquiries.txt", ios_base::app);//open the file to append
    oFile << data1.first << ",";
//    Inquiry<Bond> data=data1.second;//get price stream
    oFile << data1.second.GetInquiryId() << ",";
    oFile << data1.second.GetProduct().GetProductId() << ",";//write cusip to file
    Side side=data1.second.GetSide();//get side
    if(side == BUY){
        oFile << "BUY,";
    } else{
        oFile << "SELL,";
    }
    oFile << to_string(data1.second.GetQuantity()) << ",";
    oFile << PriceProcess(data1.second.GetPrice()) << ",";
    InquiryState s=data1.second.GetState();//get inquiry state
    switch(s){
        case RECEIVED:
            oFile << "RECEIVED\n"; break;
        case QUOTED:
            oFile << "QUOTED\n";break;
        case REJECTED:
            oFile << "REJECTED\n";break;
        case CUSTOMER_REJECTED:
            oFile << "CUSTOMER_REJECTED\n";break;
        case DONE:
            oFile << "DONE\n";break;
    }
    oFile.close();
}

class BondStreamHistoricalConnector: public Connector<pair<string,PriceStream<Bond> > > {
public:
    void Publish(pair<string, PriceStream<Bond> > &data1) override {
        ofstream oFile;
        oFile.open("./Output/Historical/streaming.txt", ios_base::app);//open the file to append
        oFile << data1.first << ",";
        oFile << data1.second.GetProduct().GetProductId() << ",";
        PriceStreamOrder bid_order=data1.second.GetBidOrder();//get bid order
        oFile << PriceProcess(data1.second.GetBidOrder().GetPrice()) << ",";//write to file
        oFile << to_string(bid_order.GetVisibleQuantity()) << ",";//write to file
        oFile << to_string(bid_order.GetHiddenQuantity()) << ",";//write to file
        PriceStreamOrder offer_order=data1.second.GetOfferOrder();//get offer order
        oFile << PriceProcess(offer_order.GetPrice()) << ",";//write to file
        oFile << to_string(offer_order.GetVisibleQuantity()) << ",";
        oFile << to_string(offer_order.GetHiddenQuantity()) << "\n";
        oFile.close();
    }
};

class BondStreamHistoricalData: public HistoricalDataService<PriceStream<Bond> >
{
private:
    int counter;
    map<string, PriceStream<Bond> > bondHistoricalCache;
    vector<ServiceListener<PriceStream<Bond> >* > bondListeners;
    BondStreamHistoricalConnector& b_historical;
public:
    explicit BondStreamHistoricalData(BondStreamHistoricalConnector& src):b_historical(src){counter=1;}

    PriceStream<Bond>& GetData(string key) override{return bondHistoricalCache.find(key)->second;}

    void OnMessage(PriceStream<Bond> &data) override{}//do nothing

    void AddListener(ServiceListener<PriceStream<Bond> > *listener) override{bondListeners.push_back(listener);}

    const vector< ServiceListener<PriceStream<Bond> >* >& GetListeners() const override{return bondListeners;}

    void PersistData(string persistKey, const PriceStream<Bond>& data) override{
        pair<string, PriceStream<Bond> > dataPair=make_pair(persistKey,data);//construct data pair
        b_historical.Publish(dataPair);//publish data to file
    }

    void SetPersistKey(PriceStream<Bond>& data){
        //get key from counter and increment counter
        string k=to_string(counter);
        ++counter;
        PersistData(k,data);
    }
};

class BondStreamHistoricalListener: public ServiceListener<PriceStream<Bond> >
{
private:
    BondStreamHistoricalData& b_historical_data;//to flow into
public:
    explicit BondStreamHistoricalListener(BondStreamHistoricalData& src): b_historical_data(src){}
    // Listener callback to process an update event to the Service
    void ProcessUpdate(PriceStream<Bond> &data) override{}

    // Listener callback to process a remove event to the Service
    void ProcessRemove(PriceStream<Bond> &data) override{}

    // Listener callback to process an add event to the Service
    void ProcessAdd(PriceStream<Bond> &data) override{b_historical_data.SetPersistKey(data);}
};

#endif
