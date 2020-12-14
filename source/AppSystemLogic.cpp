/* Copyright (C) 2005-2020, UNIGINE. All rights reserved.
 *
 * This file is a part of the UNIGINE 2.11.0.1 SDK.
 *
 * Your use and / or redistribution of this software in source and / or
 * binary form, with or without modification, is subject to: (i) your
 * ongoing acceptance of and compliance with the terms and conditions of
 * the UNIGINE License Agreement; and (ii) your inclusion of this notice
 * in any version of this software that you use or redistribute.
 * A copy of the UNIGINE License Agreement is available by contacting
 * UNIGINE. at http://unigine.com/
 * 
 * Modified by Axel Polin, 2020, univ_apolin@protonmail.com
 */

#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <string>
#include <fstream>
#include <random>
#include <chrono>

#include <UnigineApp.h>
#include <UnigineConsole.h>
#include <UnigineEngine.h>
#include <UnigineEditor.h>
#include <UnigineGame.h>
#include <UnigineLights.h>
#include <UnigineLogic.h>
#include <UnigineWorld.h>
#include <UnigineGui.h>
#include <UnigineWidgets.h>
#include <UnigineEngine.h>
#include <UnigineStreams.h>
#include <UnigineXml.h>

#include "V2E/U_randomGenerator.hpp"
#include "V2E/C_animal.hpp"
#include "V2E/C_plant.hpp"
#include "V2E/C_environment.hpp"

#include "ComponentSystem/ComponentSystem.h"

#include "GraphicalClass/GC_animal.hpp"
#include "GraphicalClass/GC_plant.hpp"

#include "AppSystemLogic.h"
#include "AppWorldLogic.h"

using namespace std;
using namespace Unigine;
using namespace Math;

// System logic, it exists during the application life cycle.
// These methods are called right after corresponding system script's (UnigineScript) methods.

AppSystemLogic::AppSystemLogic() {}

AppSystemLogic::~AppSystemLogic() {}

int AppSystemLogic::write_data(string file, string content){

    ofstream fichier(file); //j'ouvre un fichier 
    cout << "write" << endl;
    if(fichier){
        fichier << content;
        fichier.close();
    } else {
         cerr << "Impossible d'ouvrir le fichier !" << endl;
         return 1;
    }
    return 0;
}

int AppSystemLogic::init()
{
        // Write here code to be called on engine initialization.

        cout << "Init system" << endl;

        ComponentSystem::get()->initialize();

        const unsigned int mapLength = environment.getMapLength();

        Animal * newAnimal;
        Plant * newPlant;

        for ( int specie = 0 ; specie < environment.numberOfSpeciesAnimal; ++specie ) {

                for ( int animal = 0; animal < environment.MaxNumberAgentAnimal * environment.MaxNumberAgentByTypeAnimal[specie]; ++animal ) {

                        switch ( specie ) {
                        case 0 :
                                newAnimal = new Leucorrhinia();
                                break;
                        case 1 :
                                newAnimal = new Hyla();
                                break ;
                        case 2 :
                                newAnimal = new Phengaris();
                                break ;
                        case 3 :
                                newAnimal = new Zootoca();
                                break ;
                        case 4 :
                                newAnimal = new Vipera();
                                break ;
                        }

                        for ( int i = 0; i < 2; ++i )
                                location[i] = runRNG ( 0, mapLength-1 );

                        newAnimal->setLocation ( location );
                        newAnimal->setOldLocation ( location );

                        environment.getCell ( location[0],location[1] )->addAnimal ( newAnimal->getID(), newAnimal );

                        animals.push_back ( newAnimal );
                }

        }

        for ( int specie = 0 ; specie < environment.numberOfSpeciesPlant; ++specie ) {

                for ( int plant = 0; plant <= environment.MaxNumberAgentPlant * environment.MaxNumberAgentByTypePlant[specie]; ++plant ) {

                        switch ( specie ) {
                        case 0 :
                                newPlant = new Gentiania();
                                break;
                        case 1 :
                                newPlant = new Juncus();
                                break ;
                        case 2 :
                                newPlant = new Glyceria();
                                break ;
                        case 3 :
                                newPlant = new Carex(3);
                                break ;
                        case 4 :
                                newPlant = new Iris();
                                break ;
                        }

                        for ( int i = 0; i < 2; ++i )
                                location[i] = runRNG ( 0, mapLength-1 );

                        newPlant->setLocation ( location );
                        environment.getCell ( location[0],location[1] )->addPlant ( newPlant->getID(), newPlant );

                        plants.push_back ( newPlant );
                }

        }

        runShuffle ( &animals );

        agentAnimal = animals.begin();

        runShuffle ( &plants );

        agentPlant = plants.begin();

        environment.setEnvironmentParameters ( 25,0.5,0.7 );

        return 1;
}

////////////////////////////////////////////////////////////////////////////////
// start of the main loop
////////////////////////////////////////////////////////////////////////////////

int AppSystemLogic::update()
{
        // Write here code to be called before updating each render frame.

        ifps = Game::getIFps();

        if ( simulationEnd == false ) {

                if ( runTime < 0.0f ) {

                        if ( agentAnimal == animals.end() ) {
                                runShuffle ( &animals );
                                agentAnimal = animals.begin();
                        }

                        if ( agentPlant == plants.end() ) {
                                runShuffle ( &plants );
                                agentPlant = plants.begin();
                        }


                        for ( int agent = 0; agent < environment.MaxAgentRun; ++agent ) {

                                ( *agentAnimal )->run ( &environment );
                                //std::cout <<  ( *agentAnimal )->isDead()  << std::endl;
                                if ( ( *agentAnimal )->isGrowing() && ( *agentAnimal )->getGrowthState() == 2 ) {
                                        worldlogic_ptr->createAnimal ( ( *agentAnimal ) );
                                        ( *agentAnimal )->growthFinished();
                                }
                                
                                
                                if ( ( *agentAnimal )->isDead() ) {
                                        setDeadTypeCountAn();
                                        setDeadCountAn();
                                        deadCount += 1;
                                        
                                        agentAnimal = animals.erase ( agentAnimal );
                                        

                                        if ( animals.empty() ) {
                                                simulationEnd = true;
                                                Log::message ( "STOPING SIMULATION...\n" );
                                                return 1;
                                        }

                                        --agentAnimal;
                                }
                                if ( ( *agentAnimal )->isReproduction() ) { //>isSpawn()
                                        if (( *agentAnimal )->getID()==0)
                                                spawnCountL += ( *agentAnimal )->getSpawnNumber();
                                        else if (( *agentAnimal )->getID()==1)
                                                spawnCountH += ( *agentAnimal )->getSpawnNumber();
                                        else if (( *agentAnimal )->getID()==4)
                                                spawnCountV += ( *agentAnimal )->getSpawnNumber();

                                        spawn ( ( *agentAnimal )->getFecondedAnimal()  );
                                        ( *agentAnimal )->setReproductionState( false );
                                        cout << "update spawn " << std::endl;
                                        spawnCount+= ( *agentAnimal )->getSpawnNumber();
                                } 

                                spawnCount += ( *agentAnimal )->getSpawnNumber();

                                if ( ( *agentPlant )->isDead() ) {
                                        
                                        deadCountCarex += 1;
                                        agentPlant = plants.erase ( agentPlant );

                                        --agentPlant;
                                }
                                
                                runTime = environment.RunDuration;

                                ++agentAnimal;
                                
                        }
                }

                if ( simulationTime < 0.0f ) {
                        Log::message ( "SIMULATION ENDING...\n" );
                        simulationEnd = true;
                }

                if ( timeDuration < 0.0f ) {

                        if ( environment.getMonth() + 1 > 12 ) {
                                environment.setYear ( environment.getYear() + 1 );
                                environment.setMonth ( 1 );
                        } else {
                                environment.setMonth ( environment.getMonth() + 1 );
                        }

                        timeDuration = environment.MaxRunPerRunTime;

                }

                runTime -= ifps;
                simulationTime -= ifps;
                timeDuration -= ifps;
        }
        float nbr_agent_total=environment.MaxNumberAgentByTypeAnimal[( *agentAnimal )->getID()]*float(environment.MaxNumberAgentAnimal);
        
        deadLeucoVector.push_back(nbr_agent_total-deadLeucoCount+spawnCountL);
        
        deadHylaVector.push_back(nbr_agent_total-deadHylaCount+spawnCountH);  
        deadViperaVector.push_back(nbr_agent_total-deadViperaCount+spawnCountV);
        deadCountVectorCarex.push_back(deadCountCarex);
        return 1;
}

int AppSystemLogic::postUpdate()
{
        // Write here code to be called after updating each render frame.
        return 1;
}

////////////////////////////////////////////////////////////////////////////////
// end of the main loop
////////////////////////////////////////////////////////////////////////////////

int AppSystemLogic::shutdown()
{
        // Write here code to be called on engine shutdown.

        cout  << "Some statistics on the simulation" << endl;

        cout << "Number of death ( animals ) : " << deadCount << endl;
        cout << "Number of birth ( animals ) : " << spawnCount << endl;

        cout << "Number of Leucorrhinia at simulation start : " << environment.MaxNumberAgentAnimal * environment.MaxNumberAgentByTypeAnimal[0] << endl;
        cout << "Number of Hyla at simulation start : " << environment.MaxNumberAgentAnimal * environment.MaxNumberAgentByTypeAnimal[1] << endl;
        cout << "Number of viperia at simulation start : " << environment.MaxNumberAgentAnimal * environment.MaxNumberAgentByTypeAnimal[4] << endl;



        int LeucorrhiniaCount = 0;
        int HylaCount = 0;
        int ViperiaCount = 0;

        for ( agentAnimal = animals.begin(); agentAnimal != animals.end() ; ++agentAnimal )
                if ( ( * agentAnimal )->getID() == 0 )
                        LeucorrhiniaCount++;

        for ( agentAnimal = animals.begin(); agentAnimal != animals.end() ; ++agentAnimal )
                if ( ( * agentAnimal )->getID() == 1 )
                        HylaCount++;

        for ( agentAnimal = animals.begin(); agentAnimal != animals.end() ; ++agentAnimal )
                if ( ( * agentAnimal )->getID() == 4 )
                        ViperiaCount++;

        cout << "Number of Leucorrhinia at the end : " << LeucorrhiniaCount << endl;
        cout << "Number of Hyla at the end : " << HylaCount << endl;
        cout << "Number of Viperia at the end : " << ViperiaCount << endl;

        cout << "Temperature at the end of the simulation : " << environment.getEnvironmentParameters() [0] << endl;

        cout << "Hygrometry at the end of the simulation : " << environment.getEnvironmentParameters() [1] << endl;

        cout << "Antrhopization at the end of the simulation : " << environment.getEnvironmentParameters() [2] << endl;
        cout << "Number of Leucorrhinia mort naturellement : " << deadLeuco[0] << endl;

        string filename="data.txt";
        string final;
        string content1;
        string content2;
        string content3;
        for (int i=0; i<deadLeucoVector.size();++i){
        //content += to_string(deadLeuco.at(0))+"\t"+to_string(deadLeuco.at(1))+"\t"+to_string(deadLeuco.at(2))+"\t"+to_string(deadLeuco.at(3))+"\n"+to_string(deadHyla.at(0))+"\t"+to_string(deadHyla.at(1))+"\t"+to_string(deadHyla.at(2))+"\t"+to_string(deadHyla.at(3))+"\n"+to_string(deadVipera.at(0))+"\t"+to_string(deadVipera.at(1))+"\t"+to_string(deadVipera.at(2))+"\t"+to_string(deadVipera.at(3))+"\n";
                content1 += to_string(deadLeucoVector.at(i))+" ";
                
                content2 += to_string(deadHylaVector.at(i))+" ";
                content3 += to_string(deadViperaVector.at(i))+" ";
                
                
        
        }
        
        final=content1+"\n"+content2+"\n"+content3;
        write_data(filename, final);
        return 1;
}

int AppSystemLogic::spawn ( Animal * animal )
{
        Animal * newAnimal;
        vector<int> spawnLocation = animal->getLocation();
        int id = animal->getID();
        std::cout << "spawn appsystem" << std::endl;
        for ( int i = 0 ; i < animal->getSpawnNumber() ; ++i ) {

                switch ( id ) {

                case 0 :
                        newAnimal = new Leucorrhinia ( 0, "Leucorrhinia", {1,24,1}, {0,0,0,100,1,20}, {1,1,2}, {1,1,1}, true );
                        break;
                case 1 :
                        newAnimal = new Hyla ( 1, "Leucorrhinia", {1,24,1}, {0,0,0,100,1,20}, {1,1,2}, {1,1,1}, true);
                        break;
                case 2 :
                        newAnimal = new Phengaris();
                        break;
                case 3 :
                        newAnimal = new Zootoca();
                        break;
                case 4 :
                        newAnimal = new Vipera(4, "Vipera", {1,24,1}, {0,0,0,100,1,20}, {1,1,2}, {1,1,1}, true);
                        break;
                }

                newAnimal->setLocation ( spawnLocation );
                environment.getCell ( spawnLocation[0],spawnLocation[1] )->addAnimal ( newAnimal->getID(), newAnimal );
                animals.push_back ( newAnimal );
                std::cout << "spawn appsystem" << std::endl;
        }

        animal->setSpawnAbility ( false );

        runShuffle ( &animals );

        return 0;
}

int AppSystemLogic::getDeadCount()
{
        return deadCount;
}

int AppSystemLogic::getSpawnCount()
{
        return spawnCount;
}

int AppSystemLogic::setDeadCountAn()
{
        switch ((*agentAnimal)->getID())
        {
                case 0:
                        deadLeucoCount+=1;
                        break;
                case 1:   
                        deadHylaCount+=1;
                        break;
                case 4:
                        deadViperaCount+=1;
                        break;
                
                default:
                        break;
        }
        
        return 0;
}

int AppSystemLogic::setDeadTypeCountAn()
{
        switch ((*agentAnimal)->getID())
        {
                case 0:
                        deadLeuco[(*agentAnimal)->getDeadType()]+=1;                    
                        
                        break;
                case 1:
                        deadHyla[(*agentAnimal)->getDeadType()]+=1;                                   
                        break;
                case 4:
                        deadVipera[(*agentAnimal)->getDeadType()]+=1;

                        break;
                
                default:
                        break;
        }
        
        return 0;
}

int AppSystemLogic::setDeadTypeCountPl()
{
        deadCarex[(*agentPlant)->getDeadType()]+=1;
        deadCountCarex+=1;
        deadCountVectorCarex.push_back(deadCountCarex);
        
        return 0;
} 



