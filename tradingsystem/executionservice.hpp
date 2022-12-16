/**
 * executionservice.hpp
 * Defines the data types and Service for executions.
 *
 * @author Breman Thuraisingham
 */
#ifndef EXECUTION_SERVICE_HPP
#define EXECUTION_SERVICE_HPP

#include <string>
#include <fstream>
#include "soa.hpp"
#include "marketdataservice.hpp"

enum OrderType { FOK, IOC, MARKET, LIMIT, STOP };

enum Market { BROKERTEC, ESPEED, CME };

/**
 * An execution order that can be placed on an exchange.
 * Type T is the product type.
 */
template<typename T>
class ExecutionOrder
{

public:

  // ctor for an order
  ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, double _visibleQuantity, double _hiddenQuantity, string _parentOrderId, bool _isChildOrder);

  // Get the product
  const T& GetProduct() const;

  const PricingSide& GetSide() const {return side;}

  // Get the order ID
  const string& GetOrderId() const;

  // Get the order type on this order
  OrderType GetOrderType() const;

  // Get the price on this order
  double GetPrice() const;

  // Get the visible quantity on this order
  long GetVisibleQuantity() const;

  // Get the hidden quantity
  long GetHiddenQuantity() const;

  // Get the parent order ID
  const string& GetParentOrderId() const;

  // Is child order?
  bool IsChildOrder() const;

private:
  T product;
  PricingSide side;
  string orderId;
  OrderType orderType;
  double price;
  double visibleQuantity;
  double hiddenQuantity;
  string parentOrderId;
  bool isChildOrder;

};

/**
 * Service for executing orders on an exchange.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class ExecutionService : public Service<string,ExecutionOrder <T> >
{

public:

  // Execute an order on a market
  virtual void ExecuteOrder(const ExecutionOrder<T>& order, Market market) = 0;

};

template<typename T>
ExecutionOrder<T>::ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, double _visibleQuantity, double _hiddenQuantity, string _parentOrderId, bool _isChildOrder) :
  product(_product)
{
  side = _side;
  orderId = _orderId;
  orderType = _orderType;
  price = _price;
  visibleQuantity = _visibleQuantity;
  hiddenQuantity = _hiddenQuantity;
  parentOrderId = _parentOrderId;
  isChildOrder = _isChildOrder;
}

template<typename T>
const T& ExecutionOrder<T>::GetProduct() const
{
  return product;
}

template<typename T>
const string& ExecutionOrder<T>::GetOrderId() const
{
  return orderId;
}

template<typename T>
OrderType ExecutionOrder<T>::GetOrderType() const
{
  return orderType;
}

template<typename T>
double ExecutionOrder<T>::GetPrice() const
{
  return price;
}

template<typename T>
long ExecutionOrder<T>::GetVisibleQuantity() const
{
  return visibleQuantity;
}

template<typename T>
long ExecutionOrder<T>::GetHiddenQuantity() const
{
  return hiddenQuantity;
}

template<typename T>
const string& ExecutionOrder<T>::GetParentOrderId() const
{
  return parentOrderId;
}

template<typename T>
bool ExecutionOrder<T>::IsChildOrder() const
{
  return isChildOrder;
}
//
//template<typename T>
//class AlgoExecution
//{
//private:
//    ExecutionOrder<T>& exe_orders; //each algo execution is associated with a vector of references
//public:
//    //constructor
//    AlgoExecution(ExecutionOrder<T>& m_exe_order): exe_orders(m_exe_order){}
//    //get execution order
//    ExecutionOrder<T>& GetExecutionOrder(){return exe_orders;}
//    //set execution order
//    void SetExecutionOrder(const ExecutionOrder<T>& src){exe_orders=src;}
//};

template<typename T>
class AlgoExecutionService: public Service<string, ExecutionOrder<T> > {
public:
    virtual void Execute(OrderBook<T>& orderBook) = 0;
};

class BondAlgoExecutionService: public AlgoExecutionService<Bond> {
private:
    map<string, ExecutionOrder<Bond> > bondExecutionOrders;
    vector< ServiceListener<ExecutionOrder<Bond> >* > BondExecutionListeners;
    map<string, bool> bidOffer;
    int orderNum;//it will be converted to order id
public:
    BondAlgoExecutionService(){orderNum=1;}

    ExecutionOrder<Bond>& GetData(string key) override{
        return bondExecutionOrders.find(key)->second;
    }

    void OnMessage(ExecutionOrder<Bond> &data) override {}

    void AddListener(ServiceListener<ExecutionOrder<Bond> > *listener) override {BondExecutionListeners.push_back(listener);}

    const vector< ServiceListener<ExecutionOrder<Bond> >* >& GetListeners() const override {return BondExecutionListeners;}

    void Execute(OrderBook<Bond>& orderBook) override {
        const Bond& product = orderBook.GetProduct();
        string bondID = product.GetProductId();
        // First execute set to buy
        if (bidOffer.find(bondID) == bidOffer.end()) {
            bidOffer.insert(make_pair(bondID, true));
        } else {
            bidOffer[bondID] =! bidOffer[bondID];
        }
        if (bidOffer[bondID]) {
            vector<Order> offers = orderBook.GetOfferStack();
            auto index = offers.begin();
            double price = offers[0].GetPrice();
            for (auto it = offers.begin()+1; it != offers.end(); ++it){
                if (it->GetPrice() < price) {
                    price = it->GetPrice();
                    index = it;
                }
            }
            long quantity = index->GetQuantity();
            long visible= quantity * 0.3;
            long invisible= quantity - visible;
            offers.erase(index);
            orderBook.SetOfferStack(offers);
            ExecutionOrder<Bond> executionOrder(product, BID, to_string(orderNum), MARKET, price, visible, invisible, to_string(orderNum), false);
            orderNum++;
            auto m_algo_exe=bondExecutionOrders.find(bondID);
            if(m_algo_exe==bondExecutionOrders.end()){
                bondExecutionOrders.insert(make_pair(bondID, executionOrder));
                for (auto & BondExecutionListener : BondExecutionListeners){
                    BondExecutionListener->ProcessAdd(executionOrder);
                }
            }
            else{
                bondExecutionOrders.erase(bondID);
                bondExecutionOrders.insert(make_pair(bondID, executionOrder));
                for (auto & BondExecutionListener : BondExecutionListeners){
                    BondExecutionListener->ProcessAdd(executionOrder);
                }
            }
        }
        else{
            vector<Order> bids = orderBook.GetBidStack();
            auto index=bids.begin();
            double p=bids[0].GetPrice();
            for(auto it = bids.begin() + 1; it < bids.end(); ++it){
                if(it->GetPrice() > p){
                    p=it->GetPrice();
                    index=it;
                }
            }
            long q=index->GetQuantity();
            long visible = q*0.3;
            long invisible=q-visible;
            bids.erase(index);
            orderBook.SetBidStack(bids);
            ExecutionOrder<Bond> executionOrder(product, OFFER, to_string(orderNum), MARKET, p, visible, invisible, to_string(orderNum), false);
            orderNum++;
            auto m_algo_exe= bondExecutionOrders.find(bondID);
            if(m_algo_exe==bondExecutionOrders.end()){
                bondExecutionOrders.insert(make_pair(bondID, executionOrder));
                for (auto & BondExecutionListener : BondExecutionListeners){
                    BondExecutionListener->ProcessAdd(executionOrder);
                }
            }
            else{
                bondExecutionOrders.erase(bondID);
                bondExecutionOrders.insert(make_pair(bondID, executionOrder));
                for (auto & BondExecutionListener : BondExecutionListeners){
                    BondExecutionListener->ProcessAdd(executionOrder);
                }
            }
        }
    }
};

class BondMarketDataListeners: public ServiceListener<OrderBook<Bond> > {
private:
    BondAlgoExecutionService& bondAlgoExecutionService;
public:
    explicit BondMarketDataListeners(BondAlgoExecutionService& src): bondAlgoExecutionService(src){}

    void ProcessUpdate(OrderBook<Bond> &data) override{bondAlgoExecutionService.Execute(data);}

    void ProcessRemove(OrderBook<Bond> &data) override{}

    void ProcessAdd(OrderBook<Bond> &data) override{}
};

class BondExecutionConnector: public Connector<pair<Market, ExecutionOrder<Bond> > > {
public:
    void Publish(pair<Market, ExecutionOrder<Bond> > &data) override {
        ofstream oFile;
        oFile.open("./Output/ExecutionOrders.txt", ios_base::app);
        Market market = data.first;
        ExecutionOrder<Bond> executionOrder = data.second;
        string orderId = executionOrder.GetOrderId();
        oFile << orderId << ",";
        Bond product = executionOrder.GetProduct();
        string bondID = product.GetProductId();
        oFile << bondID << ",";
        PricingSide side = executionOrder.GetSide();
        if (side == BID) {
            oFile << "BID,";
        } else {
            oFile << "OFFER,";
        }
        OrderType orderType = executionOrder.GetOrderType();
        switch(orderType){
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
        long visible = executionOrder.GetVisibleQuantity();
        oFile << to_string(visible) << ",";
        long hidden = executionOrder.GetHiddenQuantity();
        oFile << to_string(hidden) << ",";
        switch(market){
            case BROKERTEC: oFile << "BROKERTEC,";
                break;
            case ESPEED: oFile << "ESPEED,";
                break;
            case CME: oFile << "CME,";
                break;
        }
        double price = executionOrder.GetPrice();
        int part1 = int(price);
        double p2 = price - double(part1);
        int part2 = int(p2*32);
        double p3 = p2 - double(part2)/32;
        int part3 = round(p3*256);
        string p_str;
        if(part2>=10){
            p_str=to_string(part1)+"-"+to_string(part2)+to_string(part3);
        } else {
            p_str=to_string(part1)+"-"+"0"+to_string(part2)+to_string(part3);
        }
        oFile << p_str << "\n";
        oFile.close();
    }
};

class BondExecutionService: public ExecutionService<Bond> {
private:
    map<string, ExecutionOrder<Bond> > bondExecutionOrders;
    vector< ServiceListener<ExecutionOrder<Bond> >* > orderListeners;
    BondExecutionConnector bondExecutionConnector;
public:
    ExecutionOrder<Bond>& GetData(string key) override{
        return bondExecutionOrders.find(key)->second;
    }

    void OnMessage(ExecutionOrder<Bond> &data) override{}

    void AddListener(ServiceListener<ExecutionOrder<Bond> > *listener) override {orderListeners.push_back(listener);}

    const vector< ServiceListener<ExecutionOrder<Bond> >* >& GetListeners() const override {return orderListeners;}

    void ExecuteOrder(const ExecutionOrder<Bond>& order, Market market) override {
        string productId = order.GetProduct().GetProductId();
        auto it=bondExecutionOrders.find(productId);
        ExecutionOrder<Bond> copy = order;
        if(it == bondExecutionOrders.end()){
            bondExecutionOrders.insert(make_pair(productId, order));
            for(auto & exeOrderListener : orderListeners){
                exeOrderListener->ProcessAdd(copy);
            }
        } else {
            bondExecutionOrders.erase(productId);
            bondExecutionOrders.insert(make_pair(productId, order));
            for (auto & exeOrderListener : orderListeners) {
                exeOrderListener->ProcessAdd(copy);
            }
        }
        pair<Market, ExecutionOrder<Bond> > currentOrder(make_pair(market,copy));
        bondExecutionConnector.Publish(currentOrder);
    }
};

class BondAlgoExecutionListener: public ServiceListener<ExecutionOrder<Bond> > {
private:
    BondExecutionService& bondExecutionService;
public:
    explicit BondAlgoExecutionListener(BondExecutionService& src): bondExecutionService(src){}

    void ProcessUpdate(ExecutionOrder<Bond> &data) override {}

    void ProcessRemove(ExecutionOrder<Bond> &data) override {}

    void ProcessAdd(ExecutionOrder<Bond> &data) override {
        int i = rand() % 3;
        Market market;
        switch(i) {
            case 0: market=BROKERTEC;
                break;
            case 1: market=ESPEED;
                break;
            case 2: market=CME;
                break;
        }
        bondExecutionService.ExecuteOrder(data, market);
    }
};

#endif
