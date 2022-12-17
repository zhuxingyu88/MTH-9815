//
// Created by Xingyu Zhu on 12/16/22.
//

#ifndef MTH9815_PROJECT_GUISERVICE_HPP
#define MTH9815_PROJECT_GUISERVICE_HPP

#include "soa.hpp"
#include "pricingservice.hpp"
#include <chrono>

/**
* Pre-declearations to avoid errors.
*/
template<typename T>
class GUIConnector;
template<typename T>
class GUIToPricingListener;

template<typename T>
class GUIService : Service<string, Price<T>>
{

private:
    map<string, Price<T>> guis;
    vector<ServiceListener<Price<T>>*> listeners;
    GUIConnector<T>* connector;
    ServiceListener<Price<T>>* listener;
    int throttle;
    long millisec;

public:
    GUIService(){
        guis = map<string, Price<T>>();
        listeners = vector<ServiceListener<Price<T>>*>();
        connector = new GUIConnector<T>(this);
        listener = new GUIToPricingListener<T>(this);
        throttle = 300;
        millisec = 0;
    }

    Price<T>& GetData(string _key){
        return guis[_key];
    }

    void OnMessage(Price<T>& _data){
        guis[_data.GetProduct().GetProductId()] = _data;
        connector->Publish(_data);
    }

    void AddListener(ServiceListener<Price<T>>* _listener){
        listeners.push_back(_listener);
    }

    const vector<ServiceListener<Price<T>>*>& GetListeners() const {
        return listeners;
    }

    GUIConnector<T>* GetConnector(){
        return connector;
    }

    ServiceListener<Price<T>>* GetListener(){
        return listener;
    }

    int GetThrottle() const {
        return throttle;
    }

    long GetMillisec() {
        return millisec;
    }

    void SetMillisec(long _millisec){
        millisec = _millisec;
    }

};

long GetMillisecond()
{
    auto _timePoint = std::chrono::system_clock::now();
    auto _sec = std::chrono::time_point_cast<chrono::seconds>(_timePoint);
    auto _millisec = std::chrono::duration_cast<chrono::milliseconds>(_timePoint - _sec);
    long _millisecCount = _millisec.count();
    return _millisecCount;
}

string TimeStamp(){
    auto _timePoint = std::chrono::system_clock::now();
    auto _sec = std::chrono::time_point_cast<chrono::seconds>(_timePoint);
    auto _millisec = std::chrono::duration_cast<chrono::milliseconds>(_timePoint - _sec);

    auto _millisecCount = _millisec.count();
    string _milliString = to_string(_millisecCount);
    if (_millisecCount < 10) _milliString = "00" + _milliString;
    else if (_millisecCount < 100) _milliString = "0" + _milliString;

    time_t _timeT = std::chrono::system_clock::to_time_t(_timePoint);
    char _timeChar[24];
    strftime(_timeChar, 24, "%F %T", localtime(&_timeT));
    string _timeString = string(_timeChar) + "." + _milliString + " ";

    return _timeString;
}

template<typename T>
class GUIConnector : public Connector<Price<T>> {
private:
    GUIService<T>& service;

public:
    GUIConnector(GUIService<T>& _service){
        service = _service;
    }

    ~GUIConnector() {}

    // Publish data to the Connector
    void Publish(Price<T>& _data) {
        int _throttle = service->GetThrottle();
        long _millisec = service->GetMillisec();
        long _millisecNow = GetMillisecond();
        while (_millisecNow < _millisec) _millisecNow += 1000;
        if (_millisecNow - _millisec >= _throttle)
        {
            service->SetMillisec(_millisecNow);
            ofstream _file;
            _file.open("gui.txt", ios::app);

            _file << TimeStamp() << ",";
            vector<string> _strings = _data.ToStrings();
            for (auto& s : _strings)
            {
                _file << s << ",";
            }
            _file << endl;
        }
    }

    void Subscribe(ifstream& _data) {}

};

/**
* GUI Service Listener subscribing data to GUI Data.
* Type T is the product type.
*/
template<typename T>
class GUIToPricingListener : public ServiceListener<Price<T>>
{

private:
    GUIService<T>* service;

public:

    GUIToPricingListener(GUIService<T>* _service);
    ~GUIToPricingListener();

    // Listener callback to process an add event to the Service
    void ProcessAdd(Price<T>& _data);

    // Listener callback to process a remove event to the Service
    void ProcessRemove(Price<T>& _data);

    // Listener callback to process an update event to the Service
    void ProcessUpdate(Price<T>& _data);

};

template<typename T>
GUIToPricingListener<T>::GUIToPricingListener(GUIService<T>* _service)
{
    service = _service;
}

template<typename T>
GUIToPricingListener<T>::~GUIToPricingListener() {}

template<typename T>
void GUIToPricingListener<T>::ProcessAdd(Price<T>& _data)
{
    service->OnMessage(_data);
}

template<typename T>
void GUIToPricingListener<T>::ProcessRemove(Price<T>& _data) {}

template<typename T>
void GUIToPricingListener<T>::ProcessUpdate(Price<T>& _data) {}

#endif //MTH9815_PROJECT_GUISERVICE_HPP
