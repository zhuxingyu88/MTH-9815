/**
 * riskservice.hpp
 * Defines the data types and Service for fixed income risk.
 *
 * @author Breman Thuraisingham
 */
#ifndef RISK_SERVICE_HPP
#define RISK_SERVICE_HPP

#include "soa.hpp"
#include "positionservice.hpp"

/**
 * PV01 risk.
 * Type T is the product type.
 */
template<typename T>
class PV01
{

public:

  // ctor for a PV01 value
  PV01(const T &_product, double _pv01, long _quantity);

  // Get the product on this PV01 value
  const T& GetProduct() const {return product;}

  // Get the PV01 value
  double GetPV01() const {return pv01;}

  // Get the quantity that this risk value is associated with
  long GetQuantity() const {return quantity;}

    void AddQuantity(long q){quantity+=q;}

private:
  T product;
  double pv01;
  long quantity;

};

/**
 * A bucket sector to bucket a group of securities.
 * We can then aggregate bucketed risk to this bucket.
 * Type T is the product type.
 */
template<typename T>
class BucketedSector
{

public:

  // ctor for a bucket sector
  BucketedSector(const vector<T> &_products, string _name);

  // Get the products associated with this bucket
  const vector<T>& GetProducts() const;

  // Get the name of the bucket
  const string& GetName() const;

private:
  vector<T> products;
  string name;

};

/**
 * Risk Service to vend out risk for a particular security and across a risk bucketed sector.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class RiskService : public Service<string,PV01 <T> >
{

public:

  // Add a position that the service will risk
  virtual void AddPosition(Position<T> &position) = 0;

  // Get the bucketed risk for the bucket sector
  virtual const PV01< BucketedSector<T> > GetBucketedRisk(const BucketedSector<T> &sector) const = 0;

};

template<typename T>
PV01<T>::PV01(const T &_product, double _pv01, long _quantity) :
  product(_product)
{
  pv01 = _pv01;
  quantity = _quantity;
}

template<typename T>
BucketedSector<T>::BucketedSector(const vector<T>& _products, string _name) :
  products(_products)
{
  name = _name;
}

template<typename T>
const vector<T>& BucketedSector<T>::GetProducts() const
{
  return products;
}

template<typename T>
const string& BucketedSector<T>::GetName() const
{
  return name;
}

using SectorsRisk=tuple<PV01<BucketedSector<Bond> >, PV01<BucketedSector<Bond> >, PV01<BucketedSector<Bond> > >;

class BondRiskService: public RiskService<Bond> {
private:
    map<string, PV01<Bond> > bondRiskCache;
    vector<ServiceListener<PV01<Bond> >* > bondRiskListeners;
    vector<ServiceListener<SectorsRisk>* > bondSectorRiskListeners;
    map<string, double> bondPV01;
public:
    BondRiskService(map<string,double>& bondPV01_, map<string,Bond> m_bond):bondPV01(bondPV01_){
        for(auto & it : m_bond){
            Bond bnd = it.second;//get second
            string bondid = it.first;//get first
            double pv = bondPV01.find(bondid)->second;//get pv
            PV01<Bond> thepv01(bnd,pv,0);//construct one
            bondRiskCache.insert(make_pair(bondid,thepv01));
        }
    }
    void UpdateBondPV01(string bondid, double newpv01) {}

    PV01<Bond>& GetData(string key) override{return bondRiskCache.find(key)->second;}

    void OnMessage(PV01<Bond> &data) override {}//do nothing as no need for connector

    void AddListener(ServiceListener<PV01<Bond> > *listener) override {bondRiskListeners.push_back(listener);}

    virtual void AddListener(ServiceListener<SectorsRisk>* listener){bondSectorRiskListeners.push_back(listener);}

    const vector< ServiceListener<PV01<Bond> >* >& GetListeners() const override {return bondRiskListeners;}

    void AddPosition(Position<Bond> &position) override {}

    // Get the bucketed risk for the bucket sector
    const PV01<BucketedSector<Bond> > GetBucketedRisk(const BucketedSector<Bond> &sector) const override {
        vector<Bond> bonds=sector.GetProducts();
        double risk_bucket=0;
        long sum_quantity=0;
        for(int i=0;i<bonds.size();++i){
            //iterate bonds
            Bond bnd=bonds[i];//get bond
            string bid=bnd.GetProductId();//get bond id
            long q;
            PV01<Bond>  thepv01(bondRiskCache.find(bid)->second);//get the pv01 of this bond
            q=thepv01.GetQuantity();//get quantity of the associated pv01
            q=abs(q);//always set q to be positive in calculation of pv01
            risk_bucket+=double(q)*thepv01.GetPV01();//get accumulate risk of the bucket
            sum_quantity+=q;//get the sum of associated products
        }
        double bucket_pv01;
        if(sum_quantity>0){
            bucket_pv01=risk_bucket/double(sum_quantity);//compute pv01 for bucket
        }
        else{
            bucket_pv01=0;
        }
        PV01<BucketedSector<Bond> > sector_pv01(sector, bucket_pv01, sum_quantity);//get pv01 of bucket
        return sector_pv01;
    }
};

class BondPositionServiceListener: public ServiceListener<Position<Bond> >
{
private:
    BondRiskService& bnd_risk_service;
public:
    BondPositionServiceListener(BondRiskService& bnd_risk): bnd_risk_service(bnd_risk){}
    // Listener callback to process an add event to the Service
    virtual void ProcessAdd(Position<Bond> &data){
        bnd_risk_service.AddPosition(data);
    }

    // Listener callback to process a remove event to the Service
    virtual void ProcessRemove(Position<Bond> &data){}

    // Listener callback to process an update event to the Service
    virtual void ProcessUpdate(Position<Bond> &data){}

};


#endif
