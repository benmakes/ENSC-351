//============================================================================
//
//% Student Name 1: Benjamin Martin
//% Student 1 #: 301347720
//% Student 1 userid (email): bpmartin@sfu.ca (stu1@sfu.ca)
//
//% Student Name 2: Yogesh Mundhra
//% Student 2 #: 301346798
//% Student 2 userid (email): ymundhra@sfu.ca (stu2@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
//          Spencer Pauls
//          Zavier Aguilas
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P3_<userid1>_<userid2>" (eg. P3_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : myIO.cpp
// Version     : September, 2019
// Description : Wrapper I/O functions for ENSC-351
// Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <unistd.h>			// for read/write/close
#include <fcntl.h>			// for open/creat
#include <sys/socket.h> 		// for socketpair
#include "SocketReadcond.h"
#include <condition_variable>
#include <vector>
#include <mutex>
std::mutex vectorMutex;

//a class for our vector of object containing the socket number, the socketpair's number, a condition variable for
//wait in tcdrain called cond, a condition variable called readCond for the wait call in myreadCond, a mutex mut for locking
//the class object, and count and tempCount ssize_t data for keeping track of the number of bytes written and read
class counter
{
    public:
    counter():descriptor(0),count(0), socketPairNum(0), tempCount(0), closeFlag(false){
        cond = std::make_shared<std::condition_variable>();
        readCond = std::make_shared<std::condition_variable>();
        mut = std::make_shared<std::mutex>();};
    counter(int desc,int socketPair){descriptor = desc; count = 0; socketPairNum = socketPair; tempCount = 0; closeFlag = false;}
    ssize_t count;
    ssize_t tempCount;
    std::shared_ptr<std::condition_variable> cond;
    std::shared_ptr<std::condition_variable> readCond ;
    std::shared_ptr<std::mutex> mut ;
    //how to initialize these shared pointers?
    int descriptor;
    int socketPairNum;
    bool closeFlag;
    void set_descriptor(int desc){descriptor = desc;}
    void set_socketPairNum(int socketPair){ socketPairNum = socketPair;}
    int get_socketPairNum(){ return socketPairNum;}
};

std::vector<counter> countVector;

//resizes the vector to the max size of the des array, sets the appropriate socket and socketpair information,
//and also locks the mutex of the object and it's socketpair object while changing them
int mySocketpair( int domain, int type, int protocol, int des[2] )
{
    std::lock_guard<std::mutex> vectorLock(vectorMutex);
    int returnVal = socketpair(domain, type, protocol, des);
    int max = des[0];
    if (des[1]>des[0])
    {
        max = des[1];
    }
    if(max >= countVector.size())
    {
        countVector.resize(max+1);
    }
    std::lock_guard<std::mutex> dataLk(*(countVector[des[0]].mut));
    std::lock_guard<std::mutex> dataLk2(*(countVector[des[1]].mut));
    countVector[des[0]].set_descriptor(des[0]);
    countVector[des[0]].set_socketPairNum(des[1]);
    countVector[des[1]].set_descriptor(des[1]);
    countVector[des[1]].set_socketPairNum(des[0]);
	return returnVal;
}

//for files. Unchanged
int myOpen(const char *pathname, int flags, mode_t mode)
{
	return open(pathname, flags, mode);
}

//for files. Unchanged
int myCreat(const char *pathname, mode_t mode)
{
	return creat(pathname, mode);
}

//lock the vectorMutex and the object mutex
//check the socketpair's amount of data. If there is less than the minimum, read would stall, so instead
//store the socketpair count, and set the socketpair count to zero. Unlock the vectorMutex, notify tcDrain?
//and wait for count greater than or equal to n. This will automatically unlock the dataMutex
//If socketPair count has enough data, read from it, and decrement the amount read from socketPair's count
//notify tcDrain that we are done?
int myReadcond(int des, void * buf, int n, int min, int time, int timeout)
{
    std::unique_lock<std::mutex> vectorLk(vectorMutex);
    std::unique_lock<std::mutex> dataLk(*(countVector[des].mut));
    int SPfildes = countVector[des].get_socketPairNum();
    int pairCount = (int) countVector[SPfildes].count;
    if (pairCount < min)
    {
        countVector[SPfildes].tempCount += countVector[SPfildes].count;
        countVector[SPfildes].count = 0;
        vectorLk.unlock();
        countVector[SPfildes].cond->notify_all();
        countVector[des].readCond->wait(dataLk,[SPfildes, min]{return countVector[SPfildes].count >= min || countVector[SPfildes].closeFlag;});
        //notify tcdrain?
    }
    int tempRC = (wcsReadcond(des, buf, n, min, time, timeout ));
    countVector[SPfildes].count -= tempRC;
    countVector[SPfildes].cond->notify_all();
    return int(tempRC);
}
//Zavier explains when to lock with what mutex: vector mutex locked always, only unlocked in myReadCond, and possibly tcDrain
//socket mutex: only when changing the count

//locks the vectorMutex. Checks if fildes is a file by checking: fildes >= size of vector or if the socket and socketPair are both zero
//If file then use read function
//If socket use myReadcond
ssize_t myRead( int fildes, void* buf, size_t nbyte )
{
    std::unique_lock<std::mutex> vectorLk(vectorMutex);
    //determine if we are reading a file
    if (fildes >= countVector.size() || (countVector[fildes].descriptor == 0 && countVector[fildes].socketPairNum == 0))
    {
        return read(fildes, buf, nbyte);
    }
    vectorLk.unlock();
    return myReadcond(fildes, buf, int(nbyte), 1, 0, 0);
}

//lock the vectorMutex and the dataMutex. Include the tempCount that has been stored when the myReadcond didn't have
//enough data to be read into the count. Notify the readCond that we wrote some stuff
ssize_t myWrite( int fildes, const void* buf, size_t nbyte )
{
    if (fildes >= countVector.size() || (countVector[fildes].descriptor == 0 && countVector[fildes].socketPairNum == 0))
    {
            return write(fildes, buf, nbyte);
    }
    std::lock_guard<std::mutex> vectorLk(vectorMutex);
    std::lock_guard<std::mutex> dataLk(*(countVector[fildes].mut));
    ssize_t tempWC = write(fildes, buf, nbyte );
    countVector[fildes].count += tempWC;
    countVector[fildes].count += countVector[fildes].tempCount;
    countVector[fildes].tempCount = 0;
    int SPfildes = countVector[fildes].get_socketPairNum();
    countVector[SPfildes].readCond->notify_all();
	return tempWC;
}

int myClose( int fd )
{
    std::lock_guard<std::mutex> vectorLk(vectorMutex);
    if (fd >= countVector.size() || (countVector[fd].descriptor == 0 && countVector[fd].socketPairNum == 0))
    {
        return close(fd);
    }
    std::lock_guard<std::mutex> dataLk(*(countVector[fd].mut));
    int spfd = countVector[fd].socketPairNum;
    if (countVector[fd].count > 0)
    {
        countVector[fd].closeFlag = true;
        countVector[spfd].readCond->notify_all();
        countVector[spfd].cond->notify_all();
    }
	return close(fd);
}

//do we have to lock the vector?
//lock the data
//wait for the data to be set to zero
int myTcdrain(int des)
{ //is also included for purposes of the course.

    std::unique_lock<std::mutex> objLk(*(countVector[des].mut));
    //std::unique_lock<std::mutex> vectorLk(vectorMutex);
    //counter* descriptorP = countVector[des];
    int spfd = countVector[des].socketPairNum;

    countVector[des].cond->wait(objLk, [spfd, des]{return (countVector[des].count == 0)||(countVector[spfd].closeFlag == true);});
	return 0;
}

/* See:
 *  https://developer.blackberry.com/native/reference/core/com.qnx.doc.neutrino.lib_ref/topic/r/readcond.html
 *
 *  */

