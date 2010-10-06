/**
 * \file	cthread.cpp
 * \brief	CThread class implementation.
 */

#include <iostream>

#include "cthread.h"

using namespace std;


/// \brief Static threaded function.
void* CThread::thread_func(void *instance) {
    ((CThread*)instance)->run();

    return NULL;
}


/// \brief Default constructor.
CThread::CThread() : started(false) {
}


/// \brief Start thread.
bool CThread::start() {
    return (pthread_create(&thread, NULL, CThread::thread_func, (void*)this) == 0);
}


/// \brief Stop thread, even if undone.
void CThread::stop() {
    pthread_exit(NULL);
}


/// \brief Wait for thread to be done.
void CThread::wait() {
    pthread_join(thread, NULL);
}