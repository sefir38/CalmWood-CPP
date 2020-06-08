#ifndef __C_ANIMALS_H_INCLUDED__
#define __C_ANIMALS_H_INCLUDED__

#include "U_randomGenerator.hpp"
#include "C_environment.hpp"

#include <string>
#include <vector>
#include <iostream>

class Environment;

class Animals
{
public :
    Animals ( int newId = 99,
              std::vector<unsigned int> newlifeCycle = {1,1,1},
              std::vector<int> newprobabilities = {0,0,0,0,0},
              int newDetectionRadius = 2,
              int newActionRadius = 1
            )
    {
        id = newId;
        lifeCycle = newlifeCycle;
        moveProbability = newprobabilities[0];
        eatProbability = newprobabilities[1];
        growthProbability = newprobabilities[2];
        spawnProbability = newprobabilities[3];
        deadProbability = newprobabilities[4];

        actionRadius = newActionRadius;
        detectionRadius = newDetectionRadius;

        length = runRNG ( 0,10 );
        sex = runRNG ( 0,1 );
        growthState = runRNG ( 0,2 );

    };
    ~Animals ( void ) {};

    int run ( Environment * environment );

    int getID();

    std::vector<int> getProbabilities();

    int setLocation ( std::vector<float> newLocation );
    std::vector <float> getLocation();

    int triggerAgent ( Environment * environment, int alterationType, int associatedProbability );

    bool isDead();

protected :

    int id = 0;

    std::vector <float> location{0,0,0};

    int sex = 0; // 0 = male 1 = female

    int length = 0;
    int actionRadius = 1;
    int detectionRadius = 2;
    int growthState = 0;

    int satietyIndex = 100; // MAX 100 MIN 0

    int moveProbability;
    int eatProbability;
    int growthProbability;
    int spawnProbability;
    int deadProbability;

    unsigned int timeLifeCycle = 0;
    std::vector<unsigned int> lifeCycle;

    bool hidden = false;
    bool death = false;

    // Detection step
    int detection ( Environment * environment );

    // Action step
    virtual int decision ( Environment * environment, std::unordered_map<int,int> * VisibleAnimals, std::unordered_map<int,int> * VisiblePlants, std::vector<int> * CellSpecs ) = 0;
//    virtual int reproduction ( Environment * environment ) = 0;
//    virtual int attack ( Environment * environment ) = 0;

    int move ( Environment * environment );
    int moveTowards ( Environment * environment, int X, int Y );
    int eat ( Environment * environment );
    int setHiddenState ( bool state );
    int spawn ( Environment * environment );

    int growth ( Environment * environment );
    int dead();

};

class Leucorrhinia: public Animals
{
public :
    Leucorrhinia ( int id = 0,
                   std::vector<unsigned int> lifeCycle = {1,24,1},
                   std::vector<int> probabilities = {100,0,100,0,0},
                   int detectionRadius = 2,
                   int actionRadius = 1 ) :Animals ( id, lifeCycle, probabilities, detectionRadius, actionRadius ) {}
protected :
    int decision ( Environment * environment, std::unordered_map<int,int> * VisibleAnimals, std::unordered_map<int,int> * VisiblePlants, std::vector<int> * CellSpecs );
//     int reproduction ( Environment * environment );
//     int attack ( Environment * environment );
};

class Hyla: public Animals
{
public :
    Hyla ( int id = 1 ) :Animals ( id ) {}
protected :
    int decision ( Environment * environment, std::unordered_map<int,int> * VisibleAnimals, std::unordered_map<int,int> * VisiblePlants, std::vector<int> * CellSpecs );
//     int reproduction ( Environment * environment );
//     int attack ( Environment * environment );
};

class Phengaris: public Animals
{
public :
    Phengaris ( int id = 2 ) :Animals ( id ) {}
protected :
    int decision ( Environment * environment, std::unordered_map<int,int> * VisibleAnimals, std::unordered_map<int,int> * VisiblePlants, std::vector<int> * CellSpecs );
//     int reproduction ( Environment * environment );
//     int attack ( Environment * environment );
};

class Zootoca: public Animals
{
public :
    Zootoca ( int id = 3 ) :Animals ( id ) {}
protected :
    int decision ( Environment * environment, std::unordered_map<int,int> * VisibleAnimals, std::unordered_map<int,int> * VisiblePlants, std::vector<int> * CellSpecs );
//     int reproduction ( Environment * environment );
//     int attack ( Environment * environment );
};

class Vipera: public Animals
{
public :
    Vipera ( int id = 4 ) :Animals ( id ) {}
protected :
    int decision ( Environment * environment, std::unordered_map<int,int> * VisibleAnimals, std::unordered_map<int,int> * VisiblePlants, std::vector<int> * CellSpecs );
//     int reproduction ( Environment * environment );
//     int attack ( Environment * environment );
};

#endif // __C_ANIMALS_H_INCLUDED__




