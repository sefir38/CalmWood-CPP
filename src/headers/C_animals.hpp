#ifndef __C_ANIMALS_H_INCLUDED__
#define __C_ANIMALS_H_INCLUDED__

#include "U_randomGenerator.hpp"
#include "C_environment.hpp"

#include <string>
#include <vector>
#include <iostream>

class Environment;

class Animals {
public :
    Animals(int newId = 0){id = newId;};
    ~Animals(void){};
    
    //virtual int run() = 0;
    int run(Environment * environment);
    
    int getID();

    int setName(std::string newName);
    std::string getName();
    
    int setParameters(int newLength,
             int newActionRadius,
             int newDetectionRadius,
             int newGrowthState,
             int sex );
    
    int setProbabilities(int newAriseProbability, 
                         int newMoveProbability, 
                         int newEatProbability, 
                         int newGrowthProbability, 
                         int newDeadProbability);
    
    std::vector<int> getProbabilities();
    
    int setLocation(std::vector<float> newLocation);
    std::vector <float> getLocation();
    
    int triggerAgent(Environment * environment, int alterationType, int associatedProbability);

protected : 
    
    int id = 0;
    
    std::vector <float> location{0,0,0};

    std::string name;
    std::string form;
    
    int sex = 0; // 0 = male 1 = female

    int length = 1;
    int actionRadius = 1;
    int detectionRadius = 1;
    int growthState = 0;
    
    int satietyIndex = 100; // MAX 100 MIN 0
    
    int ariseProbability = 0;
    int moveProbability = 50;
    int eatProbability = 0;
    int growthProbability = 0;
    int deadProbability = 0;
    
    bool hidden = false;
    
    // Detection step
    int detection(Environment * environment);
    
    // Action step
    int arise();
    int move(Environment * environment);
    int eat(Environment * environment);
    int reproduction(Environment * environment);
    int attack(Environment * environment);
    int toggleHide();
    int spawn(Environment * environment);
    
    int growth();
    int dead();
        
};

class Leucorrhinia: public Animals {
public :
    Leucorrhinia(int id = 0):Animals(id) {}
};
    
class Hyla: public Animals {
public :
    Hyla(int id = 1):Animals(id) {}
};
    
class Phengaris: public Animals {
public :
    Phengaris(int id = 2):Animals(id) {}
};
    
class Zootoca: public Animals {
public :
    Zootoca(int id = 3):Animals(id) {}
};
    
class Vipera: public Animals {
public :
    Vipera(int id = 4):Animals(id) {}
};

#endif // __C_ANIMALS_H_INCLUDED__
