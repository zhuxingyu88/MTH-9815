/**
 * pricingservice.hpp
 * Defines the data types and Service for internal prices.
 *
 * @author Breman Thuraisingham
 */
#ifndef PRICING_SERVICE_HPP
#define PRICING_SERVICE_HPP

#include <string>
#include <fstream>
#include "soa.hpp"
#include "products.hpp"

/**
 * A price object consisting of mid and bid/offer spread.
 * Type T is the product type.
 */
template<typename T>
class Price
{

public:

  // ctor for a price
  Price(const T &_product, double _mid, double _bidOfferSpread);

  // Get the product
  const T& GetProduct() const;

  // Get the mid price
  double GetMid() const;

  // Get the bid/offer spread around the mid
  double GetBidOfferSpread() const;

private:
  const T& product;
  double mid;
  double bidOfferSpread;

};

/**
 * Pricing Service managing mid prices and bid/offers.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PricingService : public Service<string,Price <T> >
{
};

template<typename T>
Price<T>::Price(const T &_product, double _mid, double _bidOfferSpread) :
  product(_product)
{
  mid = _mid;
  bidOfferSpread = _bidOfferSpread;
}

template<typename T>
const T& Price<T>::GetProduct() const
{
  return product;
}

template<typename T>
double Price<T>::GetMid() const
{
  return mid;
}

template<typename T>
double Price<T>::GetBidOfferSpread() const
{
  return bidOfferSpread;
}

class BondPriceService: public PricingService<Bond> {
private:
    map<string, Price<Bond> > bondPrices;
    vector<ServiceListener<Price<Bond> >* > bondPriceListeners;

public:
    Price<Bond>& GetData(string key) override{
        return bondPrices.find(key)->second;
    }

    void OnMessage(Price<Bond> &data) override {
        string productId = data.GetProduct().GetProductId();
        if(bondPrices.find(productId) != bondPrices.end())
            bondPrices.erase(productId);
        bondPrices.insert(make_pair(productId, data));
        for (auto & listener : bondPriceListeners)
            listener->ProcessAdd(bondPrices.find(productId)->second);
    }

    void AddListener(ServiceListener<Price<Bond> > *listener) override {
        bondPriceListeners.push_back(listener);
    }

    const vector< ServiceListener<Price<Bond> >* >& GetListeners() const override {
        return bondPriceListeners;
    }
};

class BondPriceConnector: public Connector<Price<Bond> > {
private:
    int counter;
public:
    BondPriceConnector():counter(0){}

    virtual void Publish(Price<Bond> &data){}

    virtual void Subscribe(BondPriceService& bprice_service, map<string, Bond> m_bond) {
        ifstream iFile;
        iFile.open("./Input/prices.txt");
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
            string bondId = data[0];
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
            double spread = stod(data[3]) / 256.;
            double mid = (bid1 + offer1) / 2.;
            Bond product = m_bond[bondId];
            Price<Bond> bondPrice(product , mid, spread);
            bprice_service.OnMessage(bondPrice);
        }
        iFile.close();
    }
};


#endif
