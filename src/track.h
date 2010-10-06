#ifndef TRACK_H
    #define TRACK_H

    /**
     * \file track.h
     * \brief Track class headers
     */

    #include <string>

    #include "ANN.h"

    #include "constants.h"
    #include "map.h"

    /**
     * \brief Class to manipulate tracks and their coordinates.
     */
    class Track {
        unsigned long   artistID,
                        length,
                        titleID,
                        year;
        std::string     album,
                        artist,
                        genre,
                        path,
                        title;
        ANNidx          id;
        MuseekCode      code;
        bool            alreadyPlayed;

        public:
        Track();
        Track(ANNidx);
        Track(std::string, std::string, std::string path = std::string(""));
        ~Track();

        std::string     getAlbum()          const;
        std::string     getArtist()         const;
        unsigned int    getArtistID()       const;
        std::string     getGenre()          const;
        ANNidx          getId()             const;
        unsigned long   getLength()         const;
        std::string     getPath()           const;
        std::string     getTitle()          const;
        unsigned int    getTitleID()        const;
        unsigned int    getYear()           const;
        MuseekCode      getCode()           const;
        ANNcoord        getCoordinate(unsigned short);
        ANNpoint        getCoordinates();
        bool            hasCoordinates();
        bool            isAlreadyPlayed()   const;
        
        void            setAlreadyPlayed(bool);
        void            setAlbum(std::string);
        void            setArtist(std::string);
        void            setArtistID(unsigned int);
        void            setCode(MuseekCode);
        void            setGenre(std::string);
        void            setId(ANNidx);
        void            setLength(unsigned long);
        void            setPath(std::string);
        void            setTitle(std::string);
        void            setTitleID(unsigned int);
        void            setYear(unsigned int);

        void            downloadCoordinates();

        private:
        void            setCoordinate(unsigned short, ANNcoord);
    };
#endif