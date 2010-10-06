#ifndef CTHREAD_H
    #define CTHREAD_H
 
    /**
     * \file	cthread.h
     * \brief	CThread class headers.
     */

    #include "pthread.h"
 
    /**
     * \brief Oriented-object wrapping for pthread functions.
     *
     * Pthread functions aren't designed to work with class functions;
     * instead, transform the class function you want to thread into an
     * instance of this class, and implement it in the run() function.
     */
    class CThread {
        bool        started;
        pthread_t   thread;

        CThread(const CThread&);
        CThread& operator=(const CThread&);

	    public:
		CThread();
        virtual ~CThread() {};
 
		virtual void    run() = 0;
		bool            start();
		void            stop();
        void            wait();

        static void*    thread_func(void*);
    };
#endif