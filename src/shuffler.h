#ifndef SHUFFLER_H
    #define SHUFFLER_H

    /**
     * \file shuffler.h
     * \brief Shuffler class headers.
     */

    #include <deque>
    #include <string>

    #include "ANN.h"

    #include "constants.h"
    #include "map.h"
    #include "track.h"


    ///
    enum ShuffleMode {
        OFF,
        ON_LIBRARY,
        ON_PLAYLIST
    };


    /**
     * \brief Class to handle shuffle mode algorithm.
     */
    class Shuffler {
        static Shuffler*            instance;
        std::string                 configDirectory;
        Map*                        map;
        std::deque<Track*>          lastPlayedTracks;
        ShuffleMode                 mode;
        ULARGE_INTEGER              uli;
        Track*                      playingTrack;
        Track*                      localNextTrack;
        Track*                      remoteNextTrack;
        ANNdist
            remoteScale,
            remoteConstant,
            remoteBound,
            remoteRadius;
        bool
            paused;    
        int
            playlistLength,
            playlistPosition,
            winampVersion,
            menu_item_position_file,    ///< position for the menu item(s) in the Winamp Play Menu
            menu_item_position_option;  ///< position for the menu item(s) in the Main Options Menu
        HMENU
            windowsMenu,                ///< menu handle for Winamp Play Menu, available only in modern skins
            altMenu;                    ///< menu handle for Main Options Menu, available in all skins
        ULONGLONG
            startTime,
            endTime,
            pauseStartTime,
            pauseEndTime;
        SYSTEMTIME
            sTime,
            eTime,
            psTime,
            peTime;
        FILETIME
            stTime,
            enTime,
            pstTime,
            penTime;


        // Threads
        class LibraryScan : public CThread {
            Shuffler* parent;

            public:
            LibraryScan(Shuffler*);
            ~LibraryScan();

            void run();
        } libraryScan;

        class MapLoad : public CThread {
            Shuffler*   parent;
            std::string filename;

            public:
            MapLoad(Shuffler*, std::string newFilename = std::string(MAP_FILE));
            ~MapLoad();

            void run();
        } mapLoad;

        class NextTrackPreparation : public CThread {
            Shuffler*   parent;

            public:
            NextTrackPreparation(Shuffler*);
            ~NextTrackPreparation();

            void run();
        } nextTrackPreparation;


        // Private methods
        Shuffler();
        Shuffler(const Shuffler&);
        ~Shuffler();

        void operator=(const Shuffler &);

        public:
        static Shuffler*    getInstance();
        static void         kill();

		std::string			getConfigDirectory();
        int                 getListLength();
        int                 getListPosition();
        Track*              getPlayingTrack()           const;
        const Track*              getLocalNextTrack()         const;
        const Track*              getRemoteNextTrack()        const;
        bool                isPlaying()                 const;
        bool                databaseAvailable();

        void                setListPosition(unsigned int);
        
        void                appendToPlaylist(const Track*);
        bool                checkWinampVersion(int);
        void                createMenuEntries();
        void                disable();
        void                enable();
        bool                loadConfig(std::string filename = std::string(CONFIG_FILE));
        void                loadMap(std::string filename = std::string(MAP_FILE));
        void                prepareNextTrack();
        bool                retrievePlayingTrack();
        void                scanLibrary();
        void                toggleMode(ShuffleMode);
        void                toggleMode();

        void                onEndTrack();
        void                onNextTrack();
        void                onPausePlaying();
        void                onPreviousTrack();
        void                onRescanLibrary();
        void                onResumePlaying();
        void                onStart();
        void                onStartPlaying();
        void                onStopPlaying();

        private:
        ANNdist             distanceBetween(const Track*, const Track*);
        void                setMenuItem(ShuffleMode, bool);
    };
#endif
