/**
 * \file	logger.cpp
 * \brief	Logger class implementation.
 */

#include <iostream>
#include <windows.h>

#include "gen_museek.h"
#include "logger.h"
#include "map.h"
#include "shuffler.h"
#include "utils.h"

using namespace std;


Logger* Logger::instance = NULL;

extern winampGeneralPurposePlugin plugin;


/// \brief Default constructor.
Logger::Logger() : feedback() {
}

/// \brief Destructor.
Logger::~Logger() {
}


/// \return Unique instance of Logger class.
Logger* Logger::getInstance() {
    if (instance == NULL)
        instance = new Logger;

    return instance;
}


/// \brief Delete the unique instance of Logger class.
void Logger::kill() {
    if (instance != NULL) {
        delete instance;
        instance = NULL;
    }
}


/**
 * \brief Write given feedback to log file.
 *
 * \param newFeedback Text to be written.
 */
void Logger::log(string newFeedback) {
    Shuffler* shuffler = Shuffler::getInstance();

    feedback += newFeedback;
	string	path(shuffler->getConfigDirectory() + LOG_FILE);

	ofstream file(path.c_str(), ios::app);
	if (file) {
		file << newFeedback;
		file.close();
	} else
        MessageBoxA(plugin.hwndParent, newFeedback.c_str(), "Feedback", MB_OK);
}


/**
 * \brief Write given feedback to log file.
 *
 * Only provided for convenience.
 *
 * \param newFeedback Text to be written.
 */
void Logger::log(wstring newFeedback) {
    log(narrow(newFeedback));
}


/**
 * \brief Write given number to log file.
 *
 * Only provided for convenience.
 *
 * \param value Number to be written.
 */
void Logger::log(double value) {
    Shuffler* shuffler = Shuffler::getInstance();
	string	path(shuffler->getConfigDirectory() + LOG_FILE);

	ofstream file(path.c_str(), ios::app);
	if (file) {
		file << value;
		file.close();
	}
}