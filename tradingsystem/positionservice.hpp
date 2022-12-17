/**
 * positionservice.hpp
 * Defines the data types and Service for positions.
 *
 * @author Breman Thuraisingham
 */
#ifndef POSITION_SERVICE_HPP
#define POSITION_SERVICE_HPP

#include <string>
#include <map>
#include "soa.hpp"
#include "tradebookingservice.hpp"

using namespace std;

/**
 * Position class in a particular book.
 * Type T is the product type.
 */
template<typename T>
class Position
{

public:

  // ctor for a position
  Position(const T &_product);

  // Get the product
  const T& GetProduct() const;

  // Get the position quantity
  long GetPosition(string &book);

  // Get the aggregate position
  long GetAggregatePosition();

  void ChangePosition(long quantity, string& book);

private:
  T product;
  map<string,long> positions;

};

/**
 * Position Service to manage positions across multiple books and secruties.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PositionService : public Service<string,Position <T> >
{

public:

  // Add a trade to the service
  virtual void AddTrade(const Trade<T> &trade) = 0;

};

template<typename T>
Position<T>::Position(const T &_product) :
  product(_product)
{
}

template<typename T>
const T& Position<T>::GetProduct() const
{
  return product;
}

template<typename T>
long Position<T>::GetPosition(string &book)
{
  return positions[book];
}

template<typename T>
long Position<T>::GetAggregatePosition()
{
  long ans = 0;
  for(auto & position : positions)
      ans += position.second;
  return ans;
}

template<typename T>
void Position<T>::ChangePosition(long quantity, string &book) {
    positions[book] += quantity;
}

class BondPositionService: public PositionService<Bond> {
private:
    map<string, Position<Bond> > bondPositions; // key: BookID, value: Position
    vector<ServiceListener<Position<Bond> >* > bondPositionListeners;
public:
    Position<Bond>& GetData(string key) override {
        return bondPositions.find(key)->second;
    }

    void OnMessage(Position<Bond> &data) override {}

    void AddListener(ServiceListener<Position<Bond> > *listener) override {
        bondPositionListeners.push_back(listener);
    }

    const vector< ServiceListener<Position<Bond> >* >& GetListeners() const override {
        return bondPositionListeners;
    }

    void AddTrade(const Trade<Bond> &trade) override {
        string bondID = trade.GetProduct().GetProductId();
        string bookID = trade.GetBook();
        long quantity = trade.GetQuantity();
        Side side = trade.GetSide();
        if(side==SELL)
            quantity = -quantity;
        auto tmp = bondPositions.find(bondID);
        if (tmp == bondPositions.end()) {
            Position<Bond> pos(trade.GetProduct());
            pos.ChangePosition(quantity, bookID);
            bondPositions.insert(make_pair(bondID, pos));
            for(auto & bondPositionListener : bondPositionListeners)
                bondPositionListener->ProcessAdd(pos);
        } else {
            tmp->second.ChangePosition(quantity, bookID);
            for(auto & bondPositionListener : bondPositionListeners)
                bondPositionListener->ProcessUpdate(tmp->second);
        }
    }
};

class BondTradeListener: public ServiceListener<Trade<Bond> > {
private:
    BondPositionService& bondPositionService;
public:

    explicit BondTradeListener(BondPositionService& service): bondPositionService(service){}

    virtual ~BondTradeListener() = default;

    void ProcessAdd(Trade<Bond> &data) override{
        bondPositionService.AddTrade(data);
    }

    void ProcessRemove(Trade<Bond> &data) override {
        Side side = data.GetSide();
        if(side==BUY)
            side = SELL;
        else
            side = BUY;
        Trade<Bond> reverseTrade(data.GetProduct(),data.GetTradeId(),data.GetPrice(), data.GetBook(), data.GetQuantity(), side);
        bondPositionService.AddTrade(reverseTrade);
    }

    void ProcessUpdate(Trade<Bond> &data) override{
    }
};

#endif
