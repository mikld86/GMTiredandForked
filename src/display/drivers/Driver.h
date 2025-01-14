//
// Created by Jochen Ullrich on 14.01.25.
//

#ifndef DRIVER_H
#define DRIVER_H

class Driver {
  public:
    virtual ~Driver() = default;

    virtual bool isCompatible();
    virtual void init();
};

#endif // DRIVER_H
