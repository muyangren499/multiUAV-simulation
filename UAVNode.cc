//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2015 OpenSim Ltd.
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#ifdef WITH_OSG
#include "UAVNode.h"
#include "OsgEarthScene.h"
#include "ChannelController.h"
#include <fstream>
#include <iostream>

using namespace omnetpp;

Define_Module(UAVNode);

UAVNode::UAVNode()
{
}

UAVNode::~UAVNode()
{
}

void UAVNode::initialize(int stage)
{
    MobileNode::initialize(stage);
    switch (stage) {
    case 0:
        // fill the track
        for (int _ = 0; _ < 100; ++_) {
            //Dirty little hack for enough waypoints
            readWaypointsFromFile(par("trackFile"));
        }
        // initial position
        x = par("startX");
        y = par("startY");
        z = 2;
        speed = par("speed");
        break;
    }
}

void UAVNode::readWaypointsFromFile(const char *fileName)
{
    std::ifstream inputFile(fileName);
    while (true) {
        std::string commandName;
        double param1, param2, param3;
        inputFile >> commandName >> param1 >> param2 >> param3;
        if (inputFile.fail()) {
            break;
        }
        if (commandName == "WAYPOINT") {
            commands.push_back(new WaypointCommand(OsgEarthScene::getInstance()->toX(param2), OsgEarthScene::getInstance()->toY(param1), param3));
        } else if (commandName == "TAKEOFF") {
            commands.push_back(new TakeoffCommand(param3));
        } else if (commandName == "HOLDPOSITION") {
            commands.push_back(new TakeoffCommand(param3));
        } else {
            break;
        }
    }
}

void UAVNode::loadNextCommand() {
    commands.pop_front();
    if (commands.size() == 0) {
        throw cRuntimeError("updateCommand(): UAV has no commands left.");
    }
    //TODO allow other command types
    WaypointCommand *wpcommand;
    Command *tempCmd = commands.front();
    wpcommand = dynamic_cast<WaypointCommand*>(tempCmd);
    if (wpcommand == nullptr) throw cRuntimeError("updateCommand(): invalid cast.");
    delete commandExecEngine;
    commandExecEngine = new WaypointCEE(*this, *wpcommand);
}

void UAVNode::initializeState() {
    if (commandExecEngine == nullptr) {
        throw cRuntimeError("initializeState(): Command Engine missing.");
    }
    commandExecEngine->initializeState();
}

void UAVNode::move()
{
    //distance to move, based on simulation time passed since last update
    double stepSize = (simTime() - lastUpdate).dbl();
    updateState(stepSize);
}

void UAVNode::updateState(double stepSize) {
    commandExecEngine->updateState(stepSize);
}

bool UAVNode::commandCompleted() {
    return commandExecEngine->commandCompleted();
}

double UAVNode::getNextStepSize() {
    Command *command = commands.front();
    double dx = command->getX() - x;
    double dy = command->getY() - y;
    double dz = command->getZ() - z;
    double remainingTime = sqrt(dx*dx + dy*dy + dz*dz) / speed;
    return (timeStep == 0 || remainingTime < timeStep) ? remainingTime : timeStep;
}

//obsolete
int UAVNode::normalizeAngle(int angle)
{
    int newAngle = angle;
    while (newAngle <= -180) newAngle += 360;
    while (newAngle > 180) newAngle -= 360;
    return newAngle;
}

#endif // WITH_OSG
