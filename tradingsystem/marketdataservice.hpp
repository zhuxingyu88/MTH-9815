/**
 * marketdataservice.hpp
 * Defines the data types and Service for order book market data.
 *
 * @author Breman Thuraisingham
 */
#ifndef MARKET_DATA_SERVICE_HPP
#define MARKET_DATA_SERVICE_HPP

#include <string>
#include <vector>
#include "soa.hpp"
#include "products.hpp"
#include <map>
#include <fstream>
#include <sstream>

using namespace std;

// Side for market data
enum PricingSide { BID, OFFER };

/**
 * A market data order with price, quantity, and side.
 */
class Order
{

public:

  // ctor for an order
  Order(double _price, long _quantity, PricingSide _side);

  // Get the price on the order
  double GetPrice() const;

  // Get the quantity on the order
  long GetQuantity() const;

  // Get the side on the order
  PricingSide GetSide() const;

private:
  double price;
  long quantity;
  PricingSide side;

};

/**
 * Class representing a bid and offer order
 */
class BidOffer
{

public:

  // ctor for bid/offer
  BidOffer(const Order &_bidOrder, const Order &_offerOrder);

  // Get the bid order
  const Order& GetBidOrder() const;

  // Get the offer order
  const Order& GetOfferOrder() const;

private:
  Order bidOrder;
  Order offerOrder;

};

/**
 * Order book with a bid and offer stack.
 * Type T is the product type.
 */
template<typename T>
class OrderBook
{

public:

  // ctor for the order book
  OrderBook(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack);

  // Get the product
  const T& GetProduct() const;

  // Get the bid stack
  const vector<Order>& GetBidStack() const;

  // Get the offer stack
  const vector<Order>& GetOfferStack() const;

private:
  T product;
  vector<Order> bidStack;
  vector<Order> offerStack;

};

/**
 * Market Data Service which distributes market data
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class MarketDataService : public Service<string,OrderBook <T> >
{

public:

  // Get the best bid/offer order
  virtual BidOffer GetBestBidOffer(const string &productId) = 0;

  // Aggregate the order book
  virtual const OrderBook<T>& AggregateDepth(const string &productId) = 0;

};

/**
 * Bond Market Data Service which distributes bond market data
 */
class BondMarketDataService: public MarketDataService<Bond> {
private:
    multimap<string, OrderBook<Bond> > bondOrderBooks;
    vector< ServiceListener<OrderBook<Bond> >* > bondListeners;
public:
    BidOffer GetBestBidOffer(const string &productId) override {
        const OrderBook<Bond>& orderBook = AggregateDepth(productId);
        const vector<Order>& bids = orderBook.GetBidStack();
        const vector<Order>& offers = orderBook.GetOfferStack();
        double bid1 = bids[0].GetPrice();
        double offer1 = offers[0].GetPrice();
        int bid_i = 0, offer_i = 0;
        for (int i = 1; i < bids.size(); ++i){
            double tmp = bids[i].GetPrice();
            if (tmp > bid1){
                bid1 = tmp;
                bid_i = i;
            }
        }
        for (int i = 1; i < offers.size(); ++i){
            double tmp = offers[i].GetPrice();
            if (tmp < offer1){
                offer1 = tmp;
                offer_i = i;
            }
        }
        return {bids[bid_i],offers[offer_i]};
    }


    const OrderBook<Bond>& AggregateDepth(const string &productId) override {
        pair<multimap<string, OrderBook<Bond> >::iterator, multimap<string, OrderBook<Bond> >::iterator> range = bondOrderBooks.equal_range(productId);
        const Bond& product = range.first->second.GetProduct();
        map<double, long> bid_result;
        map<double, long> offer_result;
        for(auto it = range.first; it != range.second; ++it){
            vector<Order> bids = it->second.GetBidStack();
            vector<Order> offers = it->second.GetOfferStack();
            for(const auto & j : bids){
                bid_result[j.GetPrice()] += j.GetQuantity();
            }
            for(const auto & j : offers){
                offer_result[j.GetPrice()] += j.GetQuantity();
            }
        }
        vector<Order> bids(bid_result.size(), Order(0., 0, BID));
        vector<Order> offers(offer_result.size(), Order(0., 0, OFFER));
        int index = 0;
        for (auto & i: bid_result) {
            bids[index++] = Order(i.first, i.second, BID);
        }
        index = 0;
        for (auto & i: offer_result) {
            bids[index++] = Order(i.first, i.second, OFFER);
        }
        OrderBook<Bond> result(product, bids, offers);
        bondOrderBooks.erase(range.first, range.second);
        bondOrderBooks.insert(make_pair(productId, result));
        return bondOrderBooks.find(productId)->second;
    }

    OrderBook<Bond>& GetData(string key) override{
        AggregateDepth(key);
        return bondOrderBooks.find(key)->second;
    }

    void OnMessage(OrderBook<Bond> &data) override {
        bondOrderBooks.insert(make_pair(data.GetProduct().GetProductId(),data));
        auto& tmp = GetData(data.GetProduct().GetProductId());
        for (auto& i: bondListeners) {
            i->ProcessUpdate(tmp);
        }
    }

    void AddListener(ServiceListener<OrderBook<Bond> > *listener) override{
        bondListeners.push_back(listener);
    }

    const vector< ServiceListener<OrderBook<Bond> >* >& GetListeners() const override{
        return bondListeners;
    }

};

class BondMarketDataConnector: public Connector<OrderBook<Bond> >
{
private:
    int counter;
public:
    BondMarketDataConnector():counter(0) {}

    virtual void Publish(OrderBook<Bond> &data){}

    virtual void Subscribe(BondMarketDataService& bondMarketDataService, map<string, Bond> bondMap) {
        ifstream iFile;
        iFile.open("./Input/marketdata.txt");
        vector<Order> bidStack;
        vector<Order> offerStack;
        string line;
        try{
            for (int i = 0; i < counter; ++i)
                getline(iFile,line);
        } catch(...){
            return;
        }
        if (getline(iFile, line)) {
            ++counter;
            stringstream sStream(line);
            string tmp;
            vector<string> data;
            while (getline(sStream, tmp, ',')) {
                data.push_back(tmp);
            }
            string bondId=data[0];
            int index = 0;
            for (; index < data[1].size(); ++index) {
                if (data[1][index] == '-')
                    break;
            }
            double bid1 = stod(data[1].substr(0, index));
            string bid_string = data[1].substr(index + 1);
            bid1 += (bid_string[2]=='+')?0.:(stoi(bid_string.substr(2)) / 256.);
            bid1 += stod(bid_string.substr(0, 2)) / 32.;
            index = 0;
            for (; index < data[2].size(); ++index) {
                if (data[2][index] == '-')
                    break;
            }
            double offer1 = stod(data[2].substr(0, index));
            string offer_string = data[2].substr(index + 1);
            offer1 += (offer_string[2]=='+')?0.:(stoi(offer_string.substr(2)) / 256.);
            offer1 += stod(offer_string.substr(0, 2)) / 32.;
            long volume = 10000000;
            for(int i=0;i<5;++i){
                double bid = bid1 - i * 1.0 / 256.0;
                double offer = offer1 + i * 1.0 / 256.0;
                bidStack.emplace_back(bid, volume, BID);
                offerStack.emplace_back(offer, volume, OFFER);
            }
            Bond product = bondMap[bondId];
            OrderBook<Bond> result(product, bidStack, offerStack);
            bondMarketDataService.OnMessage(result);
        }
        iFile.close();
    }
};

Order::Order(double _price, long _quantity, PricingSide _side)
{
  price = _price;
  quantity = _quantity;
  side = _side;
}

double Order::GetPrice() const
{
  return price;
}
 
long Order::GetQuantity() const
{
  return quantity;
}
 
PricingSide Order::GetSide() const
{
  return side;
}

BidOffer::BidOffer(const Order &_bidOrder, const Order &_offerOrder) :
  bidOrder(_bidOrder), offerOrder(_offerOrder)
{
}

const Order& BidOffer::GetBidOrder() const
{
  return bidOrder;
}

const Order& BidOffer::GetOfferOrder() const
{
  return offerOrder;
}

template<typename T>
OrderBook<T>::OrderBook(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack) :
  product(_product), bidStack(_bidStack), offerStack(_offerStack)
{
}

template<typename T>
const T& OrderBook<T>::GetProduct() const
{
  return product;
}

template<typename T>
const vector<Order>& OrderBook<T>::GetBidStack() const
{
  return bidStack;
}

template<typename T>
const vector<Order>& OrderBook<T>::GetOfferStack() const
{
  return offerStack;
}

#endif
