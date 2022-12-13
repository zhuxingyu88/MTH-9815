/**
 * tradebookingservice.hpp
 * Defines the data types and Service for trade booking.
 *
 * @author Breman Thuraisingham
 */
#ifndef TRADE_BOOKING_SERVICE_HPP
#define TRADE_BOOKING_SERVICE_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "soa.hpp"
#include "products.hpp"

// Trade sides
enum Side { BUY, SELL };

/**
 * Trade object with a price, side, and quantity on a particular book.
 * Type T is the product type.
 */
template<typename T>
class Trade
{

public:

  // ctor for a trade
  Trade(const T &_product, string _tradeId, double _price, string _book, long _quantity, Side _side);

  // Get the product
  const T& GetProduct() const;

  // Get the trade ID
  const string& GetTradeId() const;

  // Get the mid price
  double GetPrice() const;

  // Get the book
  const string& GetBook() const;

  // Get the quantity
  long GetQuantity() const;

  // Get the side
  Side GetSide() const;

private:
  T product;
  string tradeId;
  double price;
  string book;
  long quantity;
  Side side;

};

/**
 * Trade Booking Service to book trades to a particular book.
 * Keyed on trade id.
 * Type T is the product type.
 */
template<typename T>
class TradeBookingService : public Service<string,Trade <T> >
{

public:

  // Book the trade
  virtual void BookTrade(const Trade<T> &trade) = 0;

};

class BondTradeBookService: public TradeBookingService<Bond> {
private:
    map<string, Trade<Bond> > bondTradeBooks;
    vector<ServiceListener<Trade<Bond> >* > bondTradeListeners;
public:
    Trade<Bond>& GetData(string key) override {
        return bondTradeBooks.find(key)->second;
    }

    void OnMessage(Trade<Bond> &data) override{
        BookTrade(data);
    }

    void AddListener(ServiceListener<Trade<Bond> > *listener) override{
        bondTradeListeners.push_back(listener);
    }

    const vector<ServiceListener<Trade<Bond> >* >& GetListeners() const override {
        return bondTradeListeners;
    }

    void BookTrade(const Trade<Bond> &trade) override {
        string tradeID = trade.GetTradeId();
        auto tmp = bondTradeBooks.find(tradeID);
        if (tmp != bondTradeBooks.end()){
            bondTradeBooks.erase(tmp);
            bondTradeBooks.insert(make_pair(tradeID, trade));
            for (auto& listener: bondTradeListeners) {
                listener->ProcessUpdate(bondTradeBooks.find(tradeID)->second);
            }
        } else {
            bondTradeBooks.insert(make_pair(tradeID, trade));
            for (auto& listener: bondTradeListeners) {
                listener->ProcessAdd(bondTradeBooks.find(tradeID)->second);
            }
        }
    }

};

class BondTradeBookingConnector: public Connector<Trade<Bond> > {
private:
    int counter;
public:
    virtual void Publish(Trade<Bond> &data) {}

    BondTradeBookingConnector():counter(0) {}

    virtual void Subscribe(BondTradeBookService& bt_book_service, map<string, Bond> m_bond) {
        ifstream iFile;
        iFile.open("./Input/trades.txt");
        string line;
        try{
            for (int i = 0; i < counter; ++i)
                getline(iFile,line);
        } catch(...){
            iFile.close();
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
            string tradeID = data[0];
            Bond product = m_bond[data[1]];
            string bookID = data[2];
            long quantity = stol(data[3]);
            Side side = (data[4] == "BUY")?BUY:SELL;
            double price = stod(data[5]);
            Trade<Bond> trade(product, tradeID, price, bookID, quantity, side);
            bt_book_service.OnMessage(trade);
        }
        iFile.close();
    }
};



template<typename T>
Trade<T>::Trade(const T &_product, string _tradeId, double _price, string _book, long _quantity, Side _side) :
  product(_product)
{
  tradeId = _tradeId;
  price = _price;
  book = _book;
  quantity = _quantity;
  side = _side;
}

template<typename T>
const T& Trade<T>::GetProduct() const
{
  return product;
}

template<typename T>
const string& Trade<T>::GetTradeId() const
{
  return tradeId;
}

template<typename T>
double Trade<T>::GetPrice() const
{
  return price;
}

template<typename T>
const string& Trade<T>::GetBook() const
{
  return book;
}

template<typename T>
long Trade<T>::GetQuantity() const
{
  return quantity;
}

template<typename T>
Side Trade<T>::GetSide() const
{
  return side;
}

template<typename T>
void TradeBookingService<T>::BookTrade(const Trade<T> &trade)
{
}

#endif
