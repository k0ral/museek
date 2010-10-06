#ifndef MAP_H
    #define MAP_H

    /**
     * \file map.h
     * \brief Map class headers.
     */

    #include <list>
    #include <map>
    #include <string>
    #include <vector>

    #include "pthread.h"
	#include "ANN.h"

    #include "constants.h"
    #include "cthread.h"
    #include "gen_museek.h"

    class Track;
    class Shuffler;

    
    /**
     * \brief Class to manage sets of tracks, including the shuffle mode.
     * 
     * This class is a singleton.
     */
    class Map {
        static Map*                     instance;
        Shuffler*                       parent;

        unsigned long                   allocatedPoints;
        unsigned short
            dimensions,
            tracksPerQuery;

        ANNpointArray                   points;
        std::vector<Track>              tracks;
        std::list<ANNidx>               missingCoordinates;
        std::map<std::string, ANNidx>   fileIndex;
        ANNkd_tree*                     kDimensionalTree;
        
        double                          errorBound;
        ANNidxArray                     resultsID;
        ANNdistArray                    distances;

        //HWND                            progressBarHandle;

        // Private methods
        Map();
        Map(const Map&);
        ~Map();

        void                operator=(const Map &);
        void                parseResponse(std::string, std::list<ANNidx>&);

        public:
        static Map*         getInstance();
        static void         kill();

        unsigned short      getDimensions()         const;
        ANNpoint            getPoint(ANNidx);
        unsigned int        getSize()			    const;
        Track*              getTrack(ANNidx);

        void                setCoordinate(ANNidx, unsigned short, ANNcoord);
        void                setDimensions(unsigned short);
        void                setNearestNeighborErrorBound(double);
        void                setParent(Shuffler*);
        void                setSize(unsigned long);
        void                setTracksPerQuery(unsigned short);

        void                addTrack(std::string, std::string, std::string path = std::string(""));
        bool                downloadCoordinates(ANNidx);
        bool                downloadCoordinates(std::list<ANNidx>);
        bool                downloadMissingCoordinates();
        void                insert(Track);

        void                clear();
        bool                load(std::string filename = std::string(MAP_FILE));
        bool                save(std::string filename = std::string(MAP_FILE));

        Track*              findTrack(std::string, std::string);
        Track*              findTrack(std::wstring, std::wstring);
        const ANNidxArray   findNearestNeighbors(ANNpoint, unsigned short k = 2);
        const ANNidxArray   findNearestNeighbors(ANNidx, unsigned short k = 2);
        const ANNidxArray   findNearestNeighbors(const Track*, unsigned short k = 2);
        Track               findNearestNeighbor(Track);
        ANNidx              findNearestNeighbor(ANNidx);
    };
#endif
