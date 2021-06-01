#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "warehousesim.h"

// stations
struct station stations[4];
package *pileHead = NULL; // pile of pending packages

// tracking packages completed
int packagesCompleted = 0;
int bluePackages = 0;
int redPackages = 0;
int greenPackages = 0;
int yellowPackages = 0;

// locks/mutexes
pthread_mutex_t grabMtx = PTHREAD_MUTEX_INITIALIZER;    // mutex for grabbing packages
pthread_mutex_t stationMtx = PTHREAD_MUTEX_INITIALIZER; // mutex for getting on a station
pthread_mutex_t doneMtx = PTHREAD_MUTEX_INITIALIZER;

// condition variables for conveyor and robot queue
pthread_cond_t readyCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t teamConds[4] = {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};

void incrementPackages(int teamNum)
{
    // keeps track of the number of packages each team has completed globally
    packagesCompleted++;

    switch (teamNum)
    {
    case 0:
        bluePackages++;
        break;
    case 1:
        redPackages++;
        break;
    case 2:
        greenPackages++;
        break;
    case 3:
        yellowPackages++;
        break;
    }

    return;
}

// main thread function, handles robot robots grabbing packages, doing work, moving packages, and joining the queue
void *slaveAway(void *arg)
{
    robotNode *robot = ((robotNode *)arg); // casting input to type robot
    package *robotPackage = NULL;          // just to prevent ptr->package->whatever
    station *currStation = NULL;           // pointer to station robot is working at or wants to move to

    int robotId = robot->robotId;                     // makes accessing the robot's id easier
    int robotTeam = robot->team;                      // makes accessing the robot's team number easier
    char *teamName = robot->teamName;                 // makes accessing the robot's team name easier
    pthread_cond_t *busyCond = &teamConds[robotTeam]; // setting the robot queue condition variable based on the robot's team

    while (1)
    {
        while (1)
        {

            if (pileHead == NULL)
            {
                return NULL; // terminate thread when there are no more packages
            }

            pthread_mutex_lock(&grabMtx);
            // grabbing a package should be protected
            if (robot->isFree)
            {
                break; // enter package grabbing logic if robot is free and package is available, grabMtx remains locked to make grabbing package atomic
            }
            else
            {
                printf("BUSY:  Currently not robot %s #%d's turn\n",
                       teamName, robotId);
                pthread_cond_wait(busyCond, &grabMtx); // if it is not robot's turn to grab a package, relinquish the CPU until a robot on the same team completes a package
                pthread_mutex_unlock(&grabMtx);
            }
        }

        robot->isFree = 0;             // once a robot grabs a package they cannot grab another
        robot->package = pileHead;     // store package into robot to work on
        robotPackage = robot->package; // store reference to package in robotPackage
        printf("GRAB:  Robot %s #%d grabbed package %d\n",
               teamName, robotId, robotPackage->packageNum); // prints info about robot id and package id when a robot grabs a package
        printf("INST:  Package #%d has a total of %d instructions\n",
               robotPackage->packageNum, robotPackage->instructionCount); // prints the number of instructions the given package has
        printf("INFO:  Package #%d has the following instructions: ", robotPackage->packageNum);
        printf("[");
        for (int i = 0; i < robotPackage->instructionCount; i++)
        // prints each of the instructions of the package in order
        {
            if (i)
                printf(", ");
            int temp;
            temp = robotPackage->custInstructions[i] - 1;
            printf("%s", temp == WEIGHT ? "Weight" : temp == BARCODE ? "Barcode"
                                                 : temp == XRAY      ? "X-ray"
                                                                     : "Jostle");
        }
        printf("]\n");
        pileHead = pileHead->nextPackage; // pile goes to next package for another robot to grab

        pthread_mutex_unlock(&grabMtx);

        printf("STRT:  Robot %s #%d is starting to work on package %d\n",
               teamName, robotId, robotPackage->packageNum); // prints that the robot is about to begin moving and working on packages

        for (int i = 0; i < robotPackage->instructionCount; i++) // cycles through each instruction for the given robotPackage
        {
            currStation = stations + (robotPackage->custInstructions[i] - 1);
            printf("MOVE:  Robot %s #%d is moving Package #%d to Station %s\n",
                   teamName, robotId, robotPackage->packageNum, currStation->stationName); // prints that the robot is about to attempt to move a package, either from the PPP to a station or along the conveyor system

            /* below is the conveyor system */
            int stationFreeFlag = 1;
            while (1)
            {
                pthread_mutex_lock(&stationMtx); // checking whether or not a station is free must be atomic so no two robots and working at the same place at the same time
                stationFreeFlag = currStation->isFree;
                if (stationFreeFlag) // checks if the incoming station is free
                {
                    usleep(rand() % (10000 - 1000 + 1) + 1000); // Conveyor belt - takes random time
                    currStation->isFree = 0;                    // if a robot can move to a station, that station is no longer free
                    pthread_mutex_unlock(&stationMtx);
                    break; // enter working-on-package logic
                }
                else
                {
                    printf("WAIT:  Station %s is currently busy. Robot %s #%d waiting with Package #%d\n",
                           currStation->stationName, teamName, robotId, robotPackage->packageNum); // prints that the robot cannot move to its desired station because the station is busy
                    pthread_cond_wait(&readyCond, &stationMtx);                                    // waits for a robot to finish working on a station before checking station availability again
                    pthread_mutex_unlock(&stationMtx);
                }
            }

            printf("WORK:  Robot %s #%d working on Package #%d at Station %s\n",
                   teamName, robotId, robotPackage->packageNum, currStation->stationName); // prints out that a working is doing work at a station
            if (robotPackage->fragile == 1, strcmp(currStation->stationName, "Jostle") == 0)
            {
                printf("VLNT:  Package #%d is fragile! Robot %s #%d is shaking the sh*t out of it\n",
                       robotPackage->packageNum, teamName, robotId); // shakes the package if necessary
            }

            usleep(rand() % (10000 - 1000 + 1) + 1000); // Work on package - random time

            printf("DONE:  Robot %s #%d is finished working on Package #%d at Station %s\n",
                   teamName, robotId, robotPackage->packageNum, currStation->stationName); // prints that a Robot finished working on a package at their station
            printf("FREE:  Station %s is now free\n", currStation->stationName);           // prints that the station they were previously working on is now free
            currStation->isFree = 1;                                                       // sets the current station to free, simulating the working picking up their package from the station
            pthread_cond_broadcast(&readyCond);                                            // broadcasts the condition variable to every Robot currently waiting on a station, allowing them to recheck if they can move where they want to go
        }
        robot->nextRobot->isFree = 1;   // lets the next Robot in the queue know that it can now grab a package
        pthread_mutex_lock(&doneMtx);   // incrementing packages must be atomic
        incrementPackages(robotTeam);   // calls package incrementing helper function
        pthread_mutex_unlock(&doneMtx); // grabbing a package should be protected
        printf("CMLT:  Robot %s #%d is finished working on Package #%d \n",
               teamName, robotId, robotPackage->packageNum); // prints that a Robot is finished working on their package
        pthread_cond_broadcast(busyCond);                    // lets the Robots in the Robot's team's queue know that they might be next in line
    }
}

int main()
{
    robotNode *robotsHead[4] = {NULL, NULL, NULL, NULL}; // initializes the robot queue heads

    int randSeed = getSeed(); // calls helper function to extract seed from seed.txt
    printf("Random seed is: %d\n", randSeed);
    sleep(1);
    printf("Seeding the randomizer...\n");
    sleep(1);
    srand(randSeed); // seed the randomizer

    // number of packageCount, randomly generated (current upper and lower bounds are 80 to adhere to project specs)
    int packageCount = rand() % (UPPER - LOWER + 1) + LOWER;
    printf("Total number of packages to be processed is: %d\n", packageCount);
    // sleep(1);

    // initialize everything
    createPackages(packageCount, &pileHead); // calls helper function to create PPP
    createStations(stations);                // calls helper function to create each station

    for (int i = 0; i < NUM_TEAMS; i++)
    {
        createRobots(&robotsHead[i], i); // creates each robot queue and stores 10 robots in each
    }

    pthread_t *robotThreads = (pthread_t *)malloc(NUM_ROBOTS * NUM_TEAMS * sizeof(pthread_t)); // allocating enough memory for each pthread (number of robots per team * number of teams * size of pthread)

    // spawn threads (40 threads, 4 teams 10 robots each team)
    printf("-----------Beginning warehouse simulation-----------\n");
    sleep(1);

    int spawnIndex = 0;                 // keeps track of which thread is being spawned
    for (int i = 0; i < NUM_TEAMS; i++) // loops through each team
    {
        for (int j = 0; j < NUM_ROBOTS; j++) // loops through each robot in each team
        {
            pthread_create(&robotThreads[spawnIndex], NULL, slaveAway, robotsHead[i]);
            robotsHead[i] = robotsHead[i]->nextRobot;
            spawnIndex++;
        }
    }

    // join threads
    int joinIndex = 0;
    for (int i = 0; i < NUM_TEAMS; i++)
    {
        for (int j = 0; j < NUM_ROBOTS; j++)
        {
            pthread_join(robotThreads[joinIndex], NULL);
            joinIndex++;
        }
    }

    printf("----------------------------------------------------------------\n");
    printf("Packages requested: %d\tPackages completed: %d \n", packageCount, packagesCompleted);
    printf("Blue Processed: %d\tRed Processed: %d\tGreen Processed: %d\tYellow Processed: %d\n",
           bluePackages, redPackages, greenPackages, yellowPackages);

    printf("--------------------------Simulation Over-----------------------\n");

    free(robotThreads);

    return 0;
}
