/*
 * topology-linux.cpp
 *
 * Created on: 14/11/2014
 *
 * =========================================================================
 *  Copyright (C) 2014-, Daniele De Sensi (d.desensi.software@gmail.com)
 *
 *  This file is part of mammut.
 *
 *  mammut is free software: you can redistribute it and/or
 *  modify it under the terms of the Lesser GNU General Public
 *  License as published by the Free Software Foundation, either
 *  version 3 of the License, or (at your option) any later version.

 *  mammut is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  Lesser GNU General Public License for more details.
 *
 *  You should have received a copy of the Lesser GNU General Public
 *  License along with mammut.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 * =========================================================================
 */

#include <mammut/utils.hpp>
#include <mammut/topology/topology-linux.hpp>

#include <cmath>
#include <fstream>
#include <stdexcept>

#include "../task/task.hpp"

namespace mammut{
namespace topology{

TopologyLinux::TopologyLinux():Topology(){
    ;
}

void TopologyLinux::maximizeUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->maximizeUtilization();
    }
}

void TopologyLinux::resetUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->resetUtilization();
    }
}

std::string getTopologyPathFromVirtualCoreId(VirtualCoreId id){
    return "/sys/devices/system/cpu/cpu" + utils::intToString(id) + "/topology/";
}

CpuLinux::CpuLinux(CpuId cpuId, std::vector<PhysicalCore*> physicalCores):
    Cpu(cpuId, physicalCores){
    ;
}

std::string CpuLinux::getCpuInfo(const std::string& infoName) const{
    std::ifstream infile("/proc/cpuinfo");
    if(!infile.is_open()){
        throw std::runtime_error("Impossble to open /proc/cpuinfo.");
    }

    //TODO: Non una linea qualsiasi ma quella di questa cpu
    std::string line;
    while(std::getline(infile, line)){
        if(!line.compare(0, infoName.length(), infoName)){
            return utils::ltrim(utils::split(line, ':').at(1));
        }
    }
    return "";
}

std::string CpuLinux::getVendorId() const{
    return getCpuInfo("vendor_id");
}

std::string CpuLinux::getFamily() const{
    return getCpuInfo("cpu family");
}

std::string CpuLinux::getModel() const{
    return getCpuInfo("model");
}

void CpuLinux::maximizeUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->maximizeUtilization();
    }
}

void CpuLinux::resetUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->resetUtilization();
    }
}

PhysicalCoreLinux::PhysicalCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, std::vector<VirtualCore*> virtualCores):
    PhysicalCore(cpuId, physicalCoreId, virtualCores){
    ;
}

void PhysicalCoreLinux::maximizeUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->maximizeUtilization();
    }
}

void PhysicalCoreLinux::resetUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->resetUtilization();
    }
}

VirtualCoreIdleLevelLinux::VirtualCoreIdleLevelLinux(const VirtualCoreLinux& virtualCore, uint levelId):
    VirtualCoreIdleLevel(virtualCore.getVirtualCoreId(), levelId),
    _virtualCore(virtualCore),
    _path("/sys/devices/system/cpu/cpu" + utils::intToString(virtualCore.getVirtualCoreId()) +
          "/cpuidle/state" + utils::intToString(levelId) + "/"){
    resetTime();
    resetCount();
}

std::string VirtualCoreIdleLevelLinux::getName() const{
    return utils::readFirstLineFromFile(_path + "name");
}

std::string VirtualCoreIdleLevelLinux::getDesc() const{
    return utils::readFirstLineFromFile(_path + "desc");
}

bool VirtualCoreIdleLevelLinux::isEnableable() const{
    return utils::existsFile(_path + "disable");
}

bool VirtualCoreIdleLevelLinux::isEnabled() const{
    if(isEnableable()){
        return (utils::readFirstLineFromFile(_path + "disable").compare("0") == 0);
    }else{
        return true;
    }
}

void VirtualCoreIdleLevelLinux::enable() const{
    if(utils::executeCommand("echo 0 | tee " + _path + "disable")){
        throw std::runtime_error("Impossible to enable idle level.");
    }
}

void VirtualCoreIdleLevelLinux::disable() const{
    if(utils::executeCommand("echo 1 | tee " + _path + "disable")){
        throw std::runtime_error("Impossible to disable idle level.");
    }
}

uint VirtualCoreIdleLevelLinux::getExitLatency() const{
    return utils::stringToInt(utils::readFirstLineFromFile(_path + "latency"));
}

uint VirtualCoreIdleLevelLinux::getConsumedPower() const{
    return utils::stringToInt(utils::readFirstLineFromFile(_path + "power"));
}

/* TODO
static uint64_t tmp(VirtualCoreId id){
    utils::Msr msr(id);
    uint64_t result;
    assert(msr.read(MSR_CORE_C7_RESIDENCY, result));
    return result;
}
*/

uint VirtualCoreIdleLevelLinux::getAbsoluteTime() const{
    return utils::stringToInt(utils::readFirstLineFromFile(_path + "time"));
}

uint VirtualCoreIdleLevelLinux::getTime() const{
    //printf("%llu %llu\n", tmp(_virtualCore.getVirtualCoreId()) - _tmp, _virtualCore.getAbsoluteTicks() - _ticks);
    //printf("%f\n", 100.0 * (((double)tmp(_virtualCore.getVirtualCoreId()) - (double)_tmp)/((double)_virtualCore.getAbsoluteTicks() - (double)_ticks)));
    return getAbsoluteTime() - _lastAbsTime;
}

void VirtualCoreIdleLevelLinux::resetTime(){
    _lastAbsTime = getAbsoluteTime();
    //_tmp = tmp(_virtualCore.getVirtualCoreId());
    //_ticks = _virtualCore.getAbsoluteTicks();
}

uint VirtualCoreIdleLevelLinux::getAbsoluteCount() const{
    return utils::stringToInt(utils::readFirstLineFromFile(_path + "usage"));
}

uint VirtualCoreIdleLevelLinux::getCount() const{
    return getAbsoluteCount() - _lastAbsCount;
}

void VirtualCoreIdleLevelLinux::resetCount(){
    _lastAbsCount = getAbsoluteCount();
}

SpinnerThread::SpinnerThread():_stop(false){;}

void SpinnerThread::setStop(bool s){
    _lock.lock();
    _stop = s;
    _lock.unlock();
}

bool SpinnerThread::isStopped(){
    bool r;
    _lock.lock();
    r = _stop;
    _lock.unlock();
    return r;
}

void SpinnerThread::run(){
    double r = rand();
    while(!isStopped()){
        r = sin(r);
    };
    utils::executeCommand("echo " + utils::intToString(r) + " > /dev/null");
}

VirtualCoreLinux::VirtualCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, VirtualCoreId virtualCoreId):
            VirtualCore(cpuId, physicalCoreId, virtualCoreId),
            _hotplugFile("/sys/devices/system/cpu/cpu" + utils::intToString(virtualCoreId) + "/online"),
            _utilizationThread(new SpinnerThread()),
            _msr(virtualCoreId){
    std::vector<std::string> levelsNames =
            utils::getFilesNamesInDir("/sys/devices/system/cpu/cpu" + utils::intToString(getVirtualCoreId()) + "/cpuidle", false, true);
    for(size_t i = 0; i < levelsNames.size(); i++){
        std::string levelName = levelsNames.at(i);
        if(levelName.compare(0, 5, "state") == 0){
            uint levelId = utils::stringToInt(levelName.substr(5));
            _idleLevels.push_back(new VirtualCoreIdleLevelLinux(*this, levelId));
        }
    }
    resetIdleTime();
}

VirtualCoreLinux::~VirtualCoreLinux(){
    utils::deleteVectorElements<VirtualCoreIdleLevel*>(_idleLevels);
    resetUtilization();
    delete _utilizationThread;
}

uint64_t VirtualCoreLinux::getAbsoluteTicks() const{
    uint64_t ticks = 0;
    if(_msr.available() && _msr.read(MSR_TSC, ticks)){
        return ticks;
    }
    return 0;
}

void VirtualCoreLinux::maximizeUtilization() const{
    if(_utilizationThread->running()){
        /** Is useless for the moment. Is just a placeholder in case different utilization levels will be added in the future. **/
        resetUtilization();
    }

    _utilizationThread->setStop(false);
    _utilizationThread->start();
    mammut::task::ThreadHandler* h =_utilizationThread->getThreadHandler();
    h->move(this);
    h->setPriority(MAMMUT_PROCESS_PRIORITY_MAX);
    _utilizationThread->releaseThreadHandler(h);
}

void VirtualCoreLinux::resetUtilization() const{
    if(_utilizationThread->running()){
        _utilizationThread->setStop(true);
        _utilizationThread->join();
    }
}

double VirtualCoreLinux::getProcStatTime(ProcStatTimeType type) const{
    /** +1 because cut fields start from 1. **/
    std::string cutFieldNum = utils::intToString(type + 1);
    std::vector<std::string> output = mammut::utils::getCommandOutput("cat /proc/stat | grep cpu" + utils::intToString(getVirtualCoreId()) + " | cut -d ' ' -f " + cutFieldNum);
    if(output.size() == 0 || output.at(0).compare("") == 0){
        return -1;
    }else{
        return ((double)mammut::utils::stringToInt(output.at(0))/(double)utils::getClockTicksPerSecond()) * (double)MAMMUT_MICROSECS_IN_SEC;
    }
}

double VirtualCoreLinux::getAbsoluteIdleTime() const{
    return getProcStatTime(MAMMUT_TOPOLOGY_PROC_STAT_IDLE);
#if 0
    /** Alternative way of computing the idle time. **/
    double r = 0;
    for(size_t i = 0; i < _idleLevels.size(); i++){
        r += (_idleLevels.at(i)->getAbsoluteTime());
    }
    return r;
#endif

}

double VirtualCoreLinux::getIdleTime() const{
    return getAbsoluteIdleTime() - _lastProcIdleTime;
}

void VirtualCoreLinux::resetIdleTime(){
    _lastProcIdleTime = getAbsoluteIdleTime();
}

bool VirtualCoreLinux::isHotPluggable() const{
    return utils::existsFile(_hotplugFile);
}

bool VirtualCoreLinux::isHotPlugged() const{
    if(isHotPluggable()){
        std::string online = utils::readFirstLineFromFile(_hotplugFile);
        return (utils::stringToInt(online) > 0);
    }else{
        return true;
    }
}

void VirtualCoreLinux::hotPlug() const{
    if(utils::executeCommand("echo 1 | tee " + _hotplugFile)){
        throw std::runtime_error("Impossible to hotPlug virtual core.");
    }
}

void VirtualCoreLinux::hotUnplug() const{
    if(utils::executeCommand("echo 0 | tee " + _hotplugFile)){
        throw std::runtime_error("Impossible to hotUnplug virtual core.");
    }
}

std::vector<VirtualCoreIdleLevel*> VirtualCoreLinux::getIdleLevels() const{
    return _idleLevels;
}

}
}
