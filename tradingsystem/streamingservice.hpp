/**
 * streamingservice.hpp
 * Defines the data types and Service for price streams.
 *
 * @author Breman Thuraisingham
 */
#ifndef STREAMING_SERVICE_HPP
#define STREAMING_SERVICE_HPP

#include "soa.hpp"
#include "marketdataservice.hpp"
#include "pricingservice.hpp"

/**
 * A price stream order with price and quantity (visible and hidden)
 */
class PriceStreamOrder
{

public:

  // ctor for an order
  PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side);

  // The side on this order
  PricingSide GetSide() const {return side;}

  // Get the price on this order
  double GetPrice() const;

  // Get the visible quantity on this order
  long GetVisibleQuantity() const;

  // Get the hidden quantity on this order
  long GetHiddenQuantity() const;

private:
  double price;
  long visibleQuantity;
  long hiddenQuantity;
  PricingSide side;

};

/**
 * Price Stream with a two-way market.
 * Type T is the product type.
 */
template<typename T>
class PriceStream
{

public:

  // ctor
  PriceStream(const T &_product, const PriceStreamOrder &_bidOrder, const PriceStreamOrder &_offerOrder);

  // Get the product
  const T& GetProduct() const;

  // Get the bid order
  const PriceStreamOrder& GetBidOrder() const;

  // Get the offer order
  const PriceStreamOrder& GetOfferOrder() const;

private:
  T product;
  PriceStreamOrder bidOrder;
  PriceStreamOrder offerOrder;

};

/**
 * Streaming service to publish two-way prices.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class StreamingService : public Service<string,PriceStream <T> >
{

public:

  // Publish two-way prices
  virtual void PublishPrice(const PriceStream<T>& priceStream) = 0;

};

PriceStreamOrder::PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side)
{
  price = _price;
  visibleQuantity = _visibleQuantity;
  hiddenQuantity = _hiddenQuantity;
  side = _side;
}

double PriceStreamOrder::GetPrice() const
{
  return price;
}

long PriceStreamOrder::GetVisibleQuantity() const
{
  return visibleQuantity;
}

long PriceStreamOrder::GetHiddenQuantity() const
{
  return hiddenQuantity;
}

template<typename T>
PriceStream<T>::PriceStream(const T &_product, const PriceStreamOrder &_bidOrder, const PriceStreamOrder &_offerOrder) :
  product(_product), bidOrder(_bidOrder), offerOrder(_offerOrder)
{
}

template<typename T>
const T& PriceStream<T>::GetProduct() const
{
  return product;
}

template<typename T>
const PriceStreamOrder& PriceStream<T>::GetBidOrder() const
{
  return bidOrder;
}

template<typename T>
const PriceStreamOrder& PriceStream<T>::GetOfferOrder() const
{
  return offerOrder;
}

template<typename T>
class AlgoStreamingService: public Service<string, PriceStream<T> > {
public:
    virtual void ExecuteAlgoStream(PriceStream<T>& data) = 0;
};

class BondAlgoStreamingService: public AlgoStreamingService<Bond>
{
private:
    map<string, PriceStream<Bond> > bondAlgoStreamingServices;
    vector< ServiceListener<PriceStream<Bond> >* > algoStreamListeners;
public:
    PriceStream<Bond>& GetData(string key) override {
        return bondAlgoStreamingServices.find(key)->second;
    }

    void OnMessage(PriceStream<Bond> &data) override {}

    void AddListener(ServiceListener<PriceStream<Bond> > *listener) override {algoStreamListeners.push_back(listener);}

    const vector< ServiceListener<PriceStream<Bond> >* >& GetListeners() const override {return algoStreamListeners;}

    void ExecuteAlgoStream(PriceStream<Bond>& data) override {
        string productId = data.GetProduct().GetProductId();
        if (bondAlgoStreamingServices.find(productId) != bondAlgoStreamingServices.end())
            bondAlgoStreamingServices.erase(productId);
        bondAlgoStreamingServices.insert(make_pair(productId, data));
        for(auto & algoStreamListener : algoStreamListeners){
            algoStreamListener->ProcessAdd(data);//invoke listeners for new data addition
        }
    }
};

class BondPriceListener: public ServiceListener<Price<Bond> > {
private:
    BondAlgoStreamingService& bondAlgoStreamingService;
public:
    BondPriceListener(BondAlgoStreamingService& src): bondAlgoStreamingService(src){}

    void ProcessUpdate(Price<Bond> &data) override{}

    void ProcessRemove(Price<Bond> &data) override{}

    void ProcessAdd(Price<Bond> &data) override{
        Bond product = data.GetProduct();
        string productId = data.GetProduct().GetProductId();
        double mid = data.GetMid();
        double spread = data.GetBidOfferSpread();
        double bidPrice = mid - 0.5 * spread;
        double offerPrice = mid + 0.5 * spread;
        long visible=(rand()%10+1)*10000;
        long hidden=(rand()%20+1)*15000;
        PriceStreamOrder bid_order(bidPrice, visible, hidden, BID);
        visible=(rand()%10+1)*10000;
        hidden=(rand()%20+1)*15000;
        PriceStreamOrder offer_order(offerPrice, visible, hidden, OFFER);
        PriceStream<Bond> priceStream(product, bid_order, offer_order);
        bondAlgoStreamingService.ExecuteAlgoStream(priceStream);
    }
};

string PriceProcess(double p){
    int part1 = int(p);
    double p2 = p-double(part1);
    int part2 = int(p2*32);
    double p3 = p2-double(part2)/32;
    int part3 = round(p3*256);
    string p_str;
    if(part2 >= 10){
        p_str = to_string(part1)+"-"+to_string(part2)+to_string(part3);
    }
    else{
        p_str = to_string(part1)+"-"+"0"+to_string(part2)+to_string(part3);
    }
    return p_str;
}

class BondStreamingConnector: public Connector<PriceStream<Bond> > {
public:
    void Publish(PriceStream<Bond> &data) override {
        ofstream oFile;
        oFile.open("./Output/PriceStreams.txt", ios_base::app);
        oFile << data.GetProduct().GetProductId() << ",";
        PriceStreamOrder bid_order = data.GetBidOrder();
        oFile << PriceProcess(data.GetBidOrder().GetPrice()) << ",";
        oFile << to_string(bid_order.GetVisibleQuantity()) << ",";
        oFile << to_string(bid_order.GetHiddenQuantity()) << ",";
        PriceStreamOrder offer_order=data.GetOfferOrder();
        oFile << PriceProcess(offer_order.GetPrice()) << ",";
        oFile << to_string(offer_order.GetVisibleQuantity()) << ",";
        oFile << to_string(offer_order.GetHiddenQuantity()) << "\n";
        oFile.close();
    }
};

class BondStreamingService: public StreamingService<Bond>
{
private:
    map<string, PriceStream<Bond> > bondPriceStreams;
    vector<ServiceListener<PriceStream<Bond> >* > priceStreamListeners;
    BondStreamingConnector bondStreamingConnector;
public:
    PriceStream<Bond>& GetData(string key) override{
        return bondPriceStreams.find(key)->second;
    }

    void OnMessage(PriceStream<Bond> &data) override {}

    void AddListener(ServiceListener<PriceStream<Bond> > *listener) override {priceStreamListeners.push_back(listener);}

    const vector< ServiceListener<PriceStream<Bond> >* >& GetListeners() const override {return priceStreamListeners;}

    void PublishPrice(const PriceStream<Bond>& priceStream) override{
        Bond product = priceStream.GetProduct();
        string productId = product.GetProductId();
        PriceStream<Bond> copy = priceStream;
        auto it = bondPriceStreams.find(productId);
        if (it == bondPriceStreams.end()){
            bondPriceStreams.insert(make_pair(productId, copy));
        } else {
            bondPriceStreams.erase(productId);
            bondPriceStreams.insert(make_pair(productId, copy));
        }
        for(auto & priceStreamListener : priceStreamListeners){
            priceStreamListener->ProcessAdd(copy);
        }
        bondStreamingConnector.Publish(copy);
    }
};

class BondAlgoStreamListener: public ServiceListener<PriceStream<Bond> >
{
private:
    BondStreamingService& b_stream_service;
public:
    BondAlgoStreamListener(BondStreamingService& src): b_stream_service(src){}//constructor

    void ProcessUpdate(PriceStream<Bond> &data) override{}

    void ProcessRemove(PriceStream<Bond> &data) override{}

    void ProcessAdd(PriceStream<Bond> &data) override{
        b_stream_service.PublishPrice(data);
    }
};

#endif
