#include "adapt_array.h"

#ifndef ADAPT_DATATYPE_H
#define ADAPT_DATATYPE_H

struct US3D{
    
    ParArray<double>* xcn;
    
    ParArray<int>* ien;
    ParArray<int>* ief;
    ParArray<int>* iee;
    
    Array<int>* ifn;
    Array<int>* ife;
    
    ParArray<double>* interior;
    Array<double>* ghost;
    
};

struct Vec3D
{
    double c0;
    double c1;
    double c2;
};
#endif