#ifndef OBSERVER_H
#define OBSERVER_H

#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <map>

#include "esp_log.h"

#include "ConstType.h"

class Observer
{

public:
  virtual void registerObserver(Observer *observer, CentralServices s)
  {
    observers[s] = observer;
  }

  virtual void registerTwoWayObserver(Observer *observer, CentralServices s)
  {
    std::cout << "registerTwoWayObserver: " << this->_service << std::endl;
    this->observers[s] = observer;
    observer->registerObserver(this, this->_service);
  }

  Observer *getObserver(CentralServices s)
  {
    return observers[s];
  }

  virtual void onReceive(CentralServices s, void *data) = 0;

protected:
  void notify(CentralServices from, CentralServices to, void *data)
  {
    if (observers[to])
    {
      observers[to]->onReceive(from, data);
    }
  }

  std::map<CentralServices, Observer *> observers;
  CentralServices _service = CentralServices::NONE;
};

#endif
