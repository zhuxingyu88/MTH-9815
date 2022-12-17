/**
 * inquiryservice.hpp
 * Defines the data types and Service for customer inquiries.
 *
 * @author Breman Thuraisingham
 */
#ifndef INQUIRY_SERVICE_HPP
#define INQUIRY_SERVICE_HPP

#include "soa.hpp"
#include "tradebookingservice.hpp"

// Various inqyury states
enum InquiryState { RECEIVED, QUOTED, DONE, REJECTED, CUSTOMER_REJECTED };

/**
 * Inquiry object modeling a customer inquiry from a client.
 * Type T is the product type.
 */
template<typename T>
class Inquiry
{

public:

  // ctor for an inquiry
  Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state);

  // Get the inquiry ID
  const string& GetInquiryId() const;

  // Get the product
  const T& GetProduct() const;

  // Get the side on the inquiry
  Side GetSide() const;

  // Get the quantity that the client is inquiring for
  long GetQuantity() const;

  // Get the price that we have responded back with
  double GetPrice() const;

  // Get the current state on the inquiry
  InquiryState GetState() const;

  void SetState(InquiryState _state) {state = _state;}

  void SetPrice(double _price) {price = _price;}

private:
  string inquiryId;
  T product;
  Side side;
  long quantity;
  double price;
  InquiryState state;

};

/**
 * Service for customer inquirry objects.
 * Keyed on inquiry identifier (NOTE: this is NOT a product identifier since each inquiry must be unique).
 * Type T is the product type.
 */
template<typename T>
class InquiryService : public Service<string,Inquiry <T> >
{

public:

  // Send a quote back to the client
  virtual void SendQuote(const string &inquiryId, double price) = 0;

  // Reject an inquiry from the client
  virtual void RejectInquiry(const string &inquiryId) = 0;

};

template<typename T>
Inquiry<T>::Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state) :
  product(_product)
{
  inquiryId = _inquiryId;
  side = _side;
  quantity = _quantity;
  price = _price;
  state = _state;
}

template<typename T>
const string& Inquiry<T>::GetInquiryId() const
{
  return inquiryId;
}

template<typename T>
const T& Inquiry<T>::GetProduct() const
{
  return product;
}

template<typename T>
Side Inquiry<T>::GetSide() const
{
  return side;
}

template<typename T>
long Inquiry<T>::GetQuantity() const
{
  return quantity;
}

template<typename T>
double Inquiry<T>::GetPrice() const
{
  return price;
}

template<typename T>
InquiryState Inquiry<T>::GetState() const
{
  return state;
}

class BondInquiryService;

class BondInquiryPublishConnector: public Connector<Inquiry<Bond> > {
public:
    virtual void Publish(Inquiry<Bond> &data){
        //transit to Quoted state
        data.SetState(QUOTED);
    }

    virtual void SetPublish(Inquiry<Bond> &data, BondInquiryService& b_iqure);
};

class BondInquiryService: public InquiryService<Bond> {
private:
    map<string, Inquiry<Bond> > bondInquiryCache;
    vector< ServiceListener<Inquiry<Bond> >* > bondInquiryListeners;
    BondInquiryPublishConnector b_publish;
public:
    BondInquiryService(BondInquiryPublishConnector& src):b_publish(src){}

    Inquiry<Bond>& GetData(string key) override{return bondInquiryCache.find(key)->second;}

    void OnMessage(Inquiry<Bond> &data) override {
        InquiryState state1=data.GetState();
        string iqId=data.GetInquiryId();//get inquiry id
        auto it=bondInquiryCache.find(iqId);
        if(state1==RECEIVED){
            if(it==bondInquiryCache.end()){
                bondInquiryCache.insert(make_pair(iqId,data));//insert data
            }
            else{
                bondInquiryCache.erase(iqId);//erase old record
                bondInquiryCache.insert(make_pair(iqId,data));//insert data
            }
            for(auto & bondInquiryListener : bondInquiryListeners){
                bondInquiryListener->ProcessAdd(data);
            }
        } else if(state1==QUOTED){
            for(auto & bondInquiryListener : bondInquiryListeners){
                bondInquiryListener->ProcessUpdate(data);
            }
            if(it==bondInquiryCache.end()){
                bondInquiryCache.insert(make_pair(iqId,data));//insert data
            }
            else{
                bondInquiryCache.erase(iqId);//erase old record
                bondInquiryCache.insert(make_pair(iqId,data));//insert data
            }
        }
    }

    void AddListener(ServiceListener<Inquiry<Bond> > *listener) override {
        bondInquiryListeners.push_back(listener);
    }

    const vector< ServiceListener<Inquiry<Bond> >* >& GetListeners() const override {
        return bondInquiryListeners;
    }

    void SendQuote(const string& inquiryId, double price) override{
        auto it=bondInquiryCache.find(inquiryId);
        if(it==bondInquiryCache.end()){
            return;
        }
        it->second.SetPrice(price);
        b_publish.SetPublish(it->second,*this);
    }

    void RejectInquiry(const string& inquiryId) override {}

};

void BondInquiryPublishConnector::SetPublish(Inquiry<Bond> &data, BondInquiryService &b_iqure) {
    Publish(data);
    b_iqure.OnMessage(data);
}

//subscribe only connector
class BondInquiryConnector: public Connector<Inquiry<Bond> >
{
private:
    int counter;
public:
    BondInquiryConnector(){counter=0;}

    virtual void Publish(Inquiry<Bond> &data){}

    virtual void Subscribe(BondInquiryService& b_inquire, map<string, Bond> m_bond) {
        ifstream iFile;
        iFile.open("./Input/inquiries.txt");
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
            string inquireId = data[0];
            string bondId = data[1];
            Side side = data[2]=="SELL"?SELL:BUY;
            long qty=stol(data[3]);//get quantity
            int index = 0;
            for (; index < data[4].size(); ++index) {
                if (data[4][index] == '-')
                    break;
            }
            double bid1 = stod(data[4].substr(0, index));
            string bid_string = data[4].substr(index + 1);
            bid1 += (bid_string[2]=='+')?0.:(stoi(bid_string.substr(2)) / 256.);
            bid1 += stod(bid_string.substr(0, 2)) / 32.;
            Bond bnd=m_bond[bondId];
            Inquiry<Bond> iq_bnd(inquireId,bnd,side,qty,bid1,RECEIVED);
            b_inquire.OnMessage(iq_bnd);
        }
        iFile.close();
    }
};

class BondInquiryListener: public ServiceListener<Inquiry<Bond> >
{
private:
    BondInquiryService& b_inquire;
public:
    BondInquiryListener(BondInquiryService& src):b_inquire(src){}

    virtual ~BondInquiryListener() = default;

    virtual void ProcessAdd(Inquiry<Bond> &data) {
        const string& inquiryId = data.GetInquiryId();
        double price = 100;
        b_inquire.SendQuote(inquiryId, price);
    }

    virtual void ProcessRemove(Inquiry<Bond> &data){}

    virtual void ProcessUpdate(Inquiry<Bond> &data){data.SetState(DONE);}
};

#endif
