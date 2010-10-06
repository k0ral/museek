#ifndef LOGGER_H
    #define LOGGER_H

    /**
     * \file	logger.h
     * \brief	Logger class headers.
     */

	#include <fstream>
	#include <string>

    #include "constants.h"

    /**
     * \brief Feedback management.
     *
     * As there is no standard way to get outputs from a dynamic library,
     * use this class to log information. This class is a singleton.
     */
	class Logger {
		static Logger*  instance;
		std::string	    feedback;

		Logger();
        Logger(const Logger&);
        ~Logger();

		void operator=(const Logger&);

		public:
		static Logger*	getInstance();
        static void	    kill();

		void            log(std::string);
        void            log(std::wstring);
        void            log(double);
	};
#endif