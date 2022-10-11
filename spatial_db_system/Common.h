#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include "QuadTree.h"
#include "RTree.h"

#define Default    0
#define RANGEPOINT 1
#define RANGELINE  2
#define NNPOINT    3
#define NNLINE     4
#define QUADTREE   5
#define RTREE	   6
#define DELETEP    7
#define DELETER    8
#define INSERTP    9
#define INSERTR   10
#define SJOIN     11

#define TEST1 1
#define TEST2 2
#define TEST3 3
#define TEST4 4
#define TEST5 5
#define TEST6 6
#define TEST7 7
#define TEST8 8

extern hw6::QuadTree qtree;
extern vector<hw6::Feature> features;

#endif