#ifndef __C_ANIMALS_H_INCLUDED__
#define __C_ANIMALS_H_INCLUDED__

#include "U_randomGenerator.hpp"

#include <string>
#include <vector>
#include <iostream>

class Animals {
public :
    Animals(int id = 0){idSaved = id;};
    ~Animals(void){};
    
    //virtual int run() = 0;
    int run();
    
    int getID(){return idSaved;}

    int setName(std::string newName);
    std::string getName();
    
    int setParameters(int newLength,
             int newActionRadius,
             int newDetectionRadius,
             int newGrowthState);
    
    int setProbabilities(int newAriseProbability, 
                         int newMoveProbability, 
                         int newEatProbability, 
                         int newGrowthProbability, 
                         int newDeadProbability);
    
    std::vector<int> getProbabilities();
    
    int triggerAgent(int alterationType, int associatedProbability);

protected : 
    
    int idSaved = 0;
    
    std::vector <float> location{0,0,0};

    std::string name;
    std::string form;

    int length = 0;
    int actionRadius = 0;
    int detectionRadius = 0;
    int growthState = 0;
    
    // Detection step
    int detection();
    
    // Action step
    int arise();
    int move();
    int eat();
    int growth();
    int dead();
    
    int satietyIndex = 100; // MAX 100 MIN 0

    // Utilities
    int setLocation(std::vector<float> newLocation);
    std::vector <float> getLocation();
    
    int ariseProbability = 0;
    int moveProbability = 0;
    int eatProbability = 0;
    int growthProbability = 0;
    int deadProbability = 0;
    
};

class Leucorrhinia: public Animals {
public :
    Leucorrhinia(int id = 0):Animals(id) {}
    int run();
};
    
class Hyla: public Animals {
public :
    Hyla(int id = 1):Animals(id) {}
    int run();
};
    
class Phengaris: public Animals {
public :
    Phengaris(int id = 2):Animals(id) {}
    int run();
};
    
class Zootoca: public Animals {
public :
    Zootoca(int id = 3):Animals(id) {}
    
    int run();
};
    
class Vipera: public Animals {
public :
    Vipera(int id = 4):Animals(id) {}
    
    int run();
};

#endif // __C_ANIMALS_H_INCLUDED__
