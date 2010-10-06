/**
 * \file shuffler.cpp
 * \brief Shuffler class implementation.
 */

#include <algorithm>
#include <cmath>
#include <sstream>

#include "curl.h"
#include "wa_ipc.h"
#include "nde/NDE.h"

#include "gen_museek.h"
#include "logger.h"
#include "shuffler.h"
#include "track.h"
#include "utils.h"

using namespace std;


Shuffler* Shuffler::instance = NULL;

// Extern variables
extern winampGeneralPurposePlugin   plugin;
extern DWORD                        WA_MENUITEM_SHUFFLE_ON_LIBRARY;
extern DWORD                        WA_MENUITEM_SHUFFLE_ON_PLAYLIST;
extern DWORD                        WA_MENUITEM_RESCAN_LIBRARY;


/// \brief Default constructor.
Shuffler::Shuffler() :
        configDirectory(),
        playingTrack(NULL),
        localNextTrack(NULL),
        remoteNextTrack(NULL),
        remoteScale(pow(5, 1./16.)),
        remoteConstant(0.3),
        remoteBound(sqrt(32.)/2.),
        remoteRadius(0),
        paused(false),
        playlistLength(0),
        playlistPosition(0),
        winampVersion(0),
        mode(OFF),
        menu_item_position_file(0),
        menu_item_position_option(0),
		startTime(0),
		endTime(0),
		pauseStartTime(0),
		pauseEndTime(0),
        libraryScan(this),
        mapLoad(this),
        nextTrackPreparation(this) {
    		map = Map::getInstance();
    		map->setParent(this); // KLUDGE

    		// Initialize curl
    		curl_global_init(CURL_GLOBAL_WIN32);
}


/// \brief Destructor.
Shuffler::~Shuffler() {
	
    curl_global_cleanup();
}


/// \return Unique instance of Shuffler class.
Shuffler* Shuffler::getInstance() {
    if (instance == NULL)
        instance = new Shuffler;

    return instance;
}


/// \brief Delete the unique instance of Shuffler class.
void Shuffler::kill() {
    if (instance != NULL) {
        delete instance;
        instance = NULL;
    }
}


/**
 * \brief Compute squared distance between 2 tracks of the map.
 *
 * \param track1 First track.
 * \param track2 Second track.
 * \return Squared distance between given tracks.
 */
ANNdist Shuffler::distanceBetween(const Track* track1, const Track* track2) {
    Map* map(Map::getInstance());

    ANNpoint
        point1(map->getPoint(track1->getId())),
        point2(map->getPoint(track2->getId()));

    return sqrt(annDist(map->getDimensions(), point1, point2));
}


/// \return The directory where the plugin is allowed to write config files.
string Shuffler::getConfigDirectory() {
    if (configDirectory.empty()) {
        if (!checkWinampVersion(0x2900))    exit(1);    // TODO: exit with an error
        
        //  Find path to config directory
        string path((char*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETINIDIRECTORY));

        path = path + "\\Plugins\\";
        configDirectory = path;
    }

    return configDirectory;
}


/// \return 
/*WNDPROC Shuffler::getLpWndProcOld() const {
    return lpWndProcOld;
}*/


///
int Shuffler::getListLength() {
    if (!checkWinampVersion(0x2000))    exit(1); // TODO: exit with an error
    return SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
}


///
int Shuffler::getListPosition() {
    if (!checkWinampVersion(0x2050))    exit(1); // TODO: exit with an error
    return SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);
}


///
const Track* Shuffler::getLocalNextTrack() const {
    return localNextTrack;
}


///
const Track* Shuffler::getRemoteNextTrack() const {
    return remoteNextTrack;
}


///
Track* Shuffler::getPlayingTrack() const {
    Map* map(Map::getInstance());
    return map->getTrack(playingTrack->getId());
}


///
bool Shuffler::databaseAvailable() {
    // Winamp is in "offline" mode
    if (checkWinampVersion(0x2050) && !SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_INETAVAILABLE)) 
        return false;

    // TODO: test connection to database
    return true;
}


/// \return True if a track is being played (even if paused), false otherwise.
bool Shuffler::isPlaying() const {
    if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING))
        return true;
    
    return false;
}


void Shuffler::setListPosition(unsigned int newPosition) {
    if (!checkWinampVersion(0x2000))    exit(1);
    SendMessage(plugin.hwndParent, WM_WA_IPC, newPosition, IPC_SETPLAYLISTPOS);
}


///
void Shuffler::appendToPlaylist(const Track* track) {
    enqueueFileWithMetaStruct nextItem = {0};
    Map* map(Map::getInstance());

    // Don't know why, but using path.c_str() doesn't work...
    string path = track->getPath();
    char*  path_ = new char[path.size() + 1];
    strcpy(path_, path.c_str());

    nextItem.filename  = path_;
    nextItem.length    = 0;     // 0 = we don't know (Winamp doesn't care)
    /*if (!map->getTrack(i)->getTitle().empty())
        nextItem.title = map->getTrack(i)->getTitle().c_str();*/
    nextItem.title = "Random"; // TODO: improve this (although this might never appear)
            
    if (!checkWinampVersion(0x2900))    exit(1);
    SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&nextItem, IPC_ENQUEUEFILE);
}


///
bool Shuffler::checkWinampVersion(int version) {
    if (!winampVersion)
	    winampVersion = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVERSION);

    if (version > winampVersion)    return false;
    return true;
}



///
void Shuffler::createMenuEntries() {
    // TODO: add an entry to trigger a library scan
    if (!checkWinampVersion(0x2900))    return;     // TODO: exit with an error

    windowsMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)2, IPC_GET_HMENU);
	altMenu     = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)0, IPC_GET_HMENU);
	
    if (windowsMenu) {
        WA_MENUITEM_SHUFFLE_ON_LIBRARY  = SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&"museek_database", IPC_REGISTER_WINAMP_IPCMESSAGE);
		WA_MENUITEM_SHUFFLE_ON_PLAYLIST = SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&"museek_playlist", IPC_REGISTER_WINAMP_IPCMESSAGE);
        WA_MENUITEM_RESCAN_LIBRARY      = SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&"museek_rescan", IPC_REGISTER_WINAMP_IPCMESSAGE);

		int c = 0;
		while (c < GetMenuItemCount(windowsMenu)) {
			if (GetMenuItemID(windowsMenu, c) == 40023) // TODO: use a constant here !
				menu_item_position_file = c + 1;

			c++;
		}
		

		menu_item_position_option = 1;
		
        // Entry "Smart shuffle on whole library"
		MENUITEMINFO i = {sizeof(i), MIIM_ID | MIIM_STATE | MIIM_TYPE, MFT_STRING, 0, WA_MENUITEM_SHUFFLE_ON_LIBRARY};
		i.dwTypeData = L"Smart shuffle"; // TODO: use gettext
		InsertMenuItem(windowsMenu, menu_item_position_file, TRUE, &i);
		InsertMenuItem(altMenu, menu_item_position_option, TRUE, &i);
		
        // TODO: first implement smart shuffle on current playlist
        // Entry "Smart shuffle on current playlist"
		/*MENUITEMINFO j = {sizeof(j), MIIM_ID | MIIM_STATE | MIIM_TYPE, MFT_STRING, 0, WA_MENUITEM_SHUFFLE_ON_PLAYLIST};
		j.dwTypeData = L"Smart shuffle on current playlist"; 
		InsertMenuItem(windowsMenu, (menu_item_position_file + 1), TRUE, &j);
		InsertMenuItem(altMenu, (menu_item_position_option + 1), TRUE, &j);*/

        // Entry "Rescan library"
        MENUITEMINFO k = {sizeof(i), MIIM_ID | MIIM_STATE | MIIM_TYPE, MFT_STRING, 0, WA_MENUITEM_RESCAN_LIBRARY};
		k.dwTypeData = L"Rescan library for smart shuffle";
		InsertMenuItem(windowsMenu, menu_item_position_file + 1, TRUE, &k);
		InsertMenuItem(altMenu, menu_item_position_option + 1, TRUE, &k);

        // Disable menu items until map has been loaded
        disable();
	}
}


/**
 * \brief Stop smart shuffler and disable menu entries.
 * \return Nothing.
 */
void Shuffler::disable() {
    mode = OFF;

    EnableMenuItem(windowsMenu, WA_MENUITEM_SHUFFLE_ON_LIBRARY, MF_GRAYED);
    EnableMenuItem(altMenu,     WA_MENUITEM_SHUFFLE_ON_LIBRARY, MF_GRAYED);
    EnableMenuItem(windowsMenu, WA_MENUITEM_RESCAN_LIBRARY,     MF_GRAYED);
    EnableMenuItem(altMenu,     WA_MENUITEM_RESCAN_LIBRARY,     MF_GRAYED);
}


/**
 * \brief Enable menu entries.
 * \return Nothing.
 */
void Shuffler::enable() {
    EnableMenuItem(windowsMenu, WA_MENUITEM_SHUFFLE_ON_LIBRARY, MF_ENABLED);
    EnableMenuItem(altMenu,     WA_MENUITEM_SHUFFLE_ON_LIBRARY, MF_ENABLED);
    EnableMenuItem(windowsMenu, WA_MENUITEM_RESCAN_LIBRARY,     MF_ENABLED);
    EnableMenuItem(altMenu,     WA_MENUITEM_RESCAN_LIBRARY,     MF_ENABLED);
}


///
bool Shuffler::loadConfig(string filename) {
    Logger* logger(Logger::getInstance());

    // Open config file
    string      path(getConfigDirectory() + filename);
    ifstream    file(path.c_str(), ios::in);

    // Unable to open file
    if (!file) {
        logger->log("[WARNING] Unable to open config file (" + path + ").\n\n");
        return false;
    }

    // Read line by line
    string          buffer;
    string          parameter;
    unsigned int    intBuffer;
    double          doubleBuffer;
    string          stringBuffer;

    while (!file.eof()) {
        getline(file, buffer);
        stringstream line(buffer);
        line >> parameter;

        // Extract dimensions
        if (parameter == "DIMENSIONS") {
            line >> intBuffer;
            map->setDimensions(intBuffer);

            logger->log("[CONFIG] Dimensions set to ");
            logger->log(intBuffer);
            logger->log("\n");
        }

        // Extract number of tracks per HTTP query
        else if (parameter == "TRACKS_PER_QUERY") {
            line >> intBuffer;
            map->setTracksPerQuery(intBuffer);

            logger->log("[CONFIG] Tracks per query set to ");
            logger->log(intBuffer);
            logger->log("\n");
        }

        // Extract nearest neighbor error bound
        else if (parameter == "ERROR_BOUND") {
            line >> doubleBuffer;
            map->setNearestNeighborErrorBound(doubleBuffer);

            logger->log("[CONFIG] Nearest neighbor error bound set to ");
            logger->log(doubleBuffer);
            logger->log("\n");
        }

        // Extract remote scale factor
        else if (parameter == "REMOTE_SCALE") {
            line >> doubleBuffer;
            remoteScale = doubleBuffer;

            logger->log("[CONFIG] Remote scale factor set to ");
            logger->log(doubleBuffer);
            logger->log("\n");
        }

        // Extract remote constant factor
        else if (parameter == "REMOTE_CONSTANT") {
            line >> doubleBuffer;
            remoteConstant = doubleBuffer;

            logger->log("[CONFIG] Remote constant factor set to ");
            logger->log(doubleBuffer);
            logger->log("\n");
        }

        // Extract remote bound
        else if (parameter == "REMOTE_BOUND") {
            line >> doubleBuffer;
            remoteBound = doubleBuffer;

            logger->log("[CONFIG] Remote bound set to ");
            logger->log(doubleBuffer);
            logger->log("\n");
        }

        // Extract database host
        else if (parameter == "DATABASE_HOST") {
            line >> stringBuffer;
            // TODO
            
            logger->log("[CONFIG] Database host set to " + stringBuffer + "\n");
        }

        // Extract database script path
        else if (parameter == "DATABASE_SCRIPT_PATH") {
            line >> stringBuffer;
            // TODO
            
            logger->log("[CONFIG] Database script path set to " + stringBuffer + "\n");
        }
    }

    file.close();
    
    return true;
}


///
void Shuffler::loadMap(string filename) {
    mapLoad.start();
}


///
void Shuffler::prepareNextTrack() {
    nextTrackPreparation.start();
}


///
bool Shuffler::retrievePlayingTrack() {
    Map* map(Map::getInstance());

    // Check if Winamp is playing
    if (!isPlaying())   return false;

    wstring filename((wchar_t*)SendMessage(
        plugin.hwndParent,
        WM_WA_IPC,
        0,
        IPC_GET_PLAYING_FILENAME
    ));
    wstring title((wchar_t*)SendMessage(
        plugin.hwndParent,
        WM_WA_IPC,
        0,
        IPC_GET_PLAYING_TITLE
    ));
    int trackLength = SendMessage(
        plugin.hwndParent,
        WM_WA_IPC,
        1,
        IPC_GETOUTPUTTIME
    );

    /*if (playingTrack) {
        if (playingTrack->getPath().compare(narrow(filename)))// */
            playingTrack = map->findTrack(title, filename);
    //}

    playlistPosition = getListPosition();
    playlistLength   = getListLength();

    return true;
}


///
void Shuffler::scanLibrary() {
    libraryScan.start();
}


///
// TODO: improve this function.
void Shuffler::setMenuItem(ShuffleMode newMode, bool newState) {
    // Invalid input
    if (newMode == OFF)     return;

    // Pre-processing
    long    state(0);
    DWORD   menuItem(WA_MENUITEM_SHUFFLE_ON_LIBRARY);
    wstring text(L"Smart shuffle");
    int     positionFile(menu_item_position_file);
    int     positionOption(menu_item_position_option);

    if (newState)
        state = MFS_CHECKED;
    if (newMode == ON_PLAYLIST) {
        menuItem    = WA_MENUITEM_SHUFFLE_ON_PLAYLIST;
        text        = L"Smart shuffle on current playlist";
        positionFile++;
        positionOption++;
    }

    // Update menu item
    MENUITEMINFO k = {sizeof(k), MIIM_ID | MIIM_STATE | MIIM_TYPE, MFT_STRING, state, menuItem};
    k.dwTypeData = (LPWSTR)text.c_str();
	SetMenuItemInfo(windowsMenu, (positionFile), TRUE, &k);
	SetMenuItemInfo(altMenu, (positionOption), TRUE, &k);
}


void Shuffler::toggleMode(ShuffleMode toggledMode) {
    // Invalid input
    if (toggledMode == OFF) {
        mode = toggledMode;
        return;
    }

    // Pre-processing
    ShuffleMode otherMode(ON_LIBRARY);
    if (toggledMode == ON_LIBRARY)  otherMode = ON_PLAYLIST;

    // Update menu items accordingly
    if (mode == toggledMode) {
        setMenuItem(toggledMode, false);
        mode = OFF;

        #ifdef DEBUG
        Logger* logger = Logger::getInstance();
        logger->log("[MODE] Smart shuffle off.\n\n");
        #endif
    } else if (mode == otherMode) {
        // TODO: wait for library scan to end
        setMenuItem(toggledMode, true);
        setMenuItem(otherMode, false);
        mode = toggledMode;

        #ifdef DEBUG
        Logger* logger = Logger::getInstance();
        if      (mode == ON_LIBRARY)    logger->log("[MODE] Smart shuffle on library.\n\n");
        else                            logger->log("[MODE] Smart shuffle on playlist.\n\n");
        #endif
    } else {
        setMenuItem(toggledMode, true);
        mode = toggledMode;

        onStart();
    }
}


///
void Shuffler::toggleMode() {
    if (mode == ON_LIBRARY) mode = ON_PLAYLIST;
    else                    mode = ON_LIBRARY;
}


void Shuffler::onEndTrack() {
    // Check for mode
    if (mode == OFF)    return;

    #ifdef DEBUG
    Logger* logger = Logger::getInstance();
    logger->log("[END] Total listening time: ");
    logger->log((endTime - startTime) - (pauseEndTime - pauseStartTime));
    logger->log("s\n\n");
    #endif

	// Last track of playlist => switch to next local track
    if (playlistPosition == playlistLength - 1) {    // playlistPosition starts from 0
        appendToPlaylist(localNextTrack);

        if (playingTrack->hasCoordinates() && localNextTrack->hasCoordinates())
            remoteRadius = distanceBetween(playingTrack, localNextTrack);
        
        setListPosition(playlistPosition + 1);
        SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(WINAMP_BUTTON2, 0), 0);
    }

}


///
void Shuffler::onNextTrack() {
    // Check for mode
    if (mode == OFF)    return;

	#ifdef DEBUG
    Logger* logger = Logger::getInstance();
	logger->log(endTime - startTime);
	logger->log("s\n");
	logger->log(pauseEndTime - pauseStartTime);
	logger->log("s\n");
	#endif

	if ((pauseEndTime - pauseStartTime) > (endTime - startTime))
		pauseEndTime = pauseStartTime = 0;
	
	#ifdef DEBUG
    logger->log("[NEXT] Total Listening Time: ");
	logger->log((endTime - startTime) - (pauseEndTime - pauseStartTime));
	logger->log("s\n\n");
    #endif

    nextTrackPreparation.wait();

	// Calculating Song Played Ratio
	float timePlayed = (endTime - startTime) - (pauseEndTime - pauseStartTime);
	float playedRatio = timePlayed / (float)playingTrack->getLength();

    // Last track of playlist => switch to next remote track
    if (playlistPosition == playlistLength - 1) {    // playlistPosition starts from 0
		if (playedRatio < 0.5)
	        appendToPlaylist(remoteNextTrack);
		else
			appendToPlaylist(localNextTrack);
        setListPosition(playlistPosition + 1);
        SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(WINAMP_BUTTON2, 0), 0);
    }

	// Testing playedRatio
	#ifdef DEBUG
	logger->log("Played Ratio: ");
	logger->log(playedRatio);
	logger->log("s\n\n");
	#endif

}


///
void Shuffler::onPausePlaying() {
    // Check for mode
    if (mode == OFF)    return;

	paused = true;

	// Update chronometers
	GetSystemTime(&psTime);
	SystemTimeToFileTime(&psTime,&pstTime);

	uli.LowPart  = pstTime.dwLowDateTime;
	uli.HighPart = pstTime.dwHighDateTime;

	pauseStartTime = (uli.QuadPart/10000000);


	#ifdef DEBUG
	Logger* logger = Logger::getInstance();
	logger->log("[PAUSE]\n\n");
	#endif
}


void Shuffler::onPreviousTrack() {
    // Check for mode
    if (mode == OFF)    return;

    #ifdef DEBUG
    Logger* logger = Logger::getInstance();
    logger->log("[PREVIOUS]\n\n");
    #endif

	startTime = endTime = pauseStartTime = pauseEndTime = 0;

}


void Shuffler::onRescanLibrary() {
    #ifdef DEBUG
    Logger* logger(Logger::getInstance());
    logger->log("Rescanning library... ");
    #endif
    
    // Database unreachable
    if (!databaseAvailable()) {
        string feedback;
        feedback = "Scanning library needs access to internet.\n\n";
        feedback += "Please ensure that Winamp is not in offline mode, ";
        feedback += "and that your internet connection is working.";

        MessageBoxA(plugin.hwndParent, feedback.c_str(), "Need internet connection", MB_OK | MB_ICONWARNING);
        return;
    }

    string question;
        question += "Scanning may take several minutes for big collections; ";
        question += "it is done in background, so that you can keep using ";
        question += "Winamp during this operation. ";
        question += "You will be warned when the scan is complete.\n\n";
        question += "Would you like to do it now ?\n\n";

    int answer(MessageBoxA(
        plugin.hwndParent,
        question.c_str(),
        "Are you sure ?",
        MB_YESNO | MB_ICONQUESTION
    ));

    if (answer == IDYES)
        scanLibrary();
}


void Shuffler::onResumePlaying() {
    // Check for mode
    if (mode == OFF)    return;

    if (paused) {
        paused = false;

        // Update chronometers
        GetSystemTime(&peTime);
        SystemTimeToFileTime(&peTime,&penTime);

        uli.LowPart  = penTime.dwLowDateTime;
        uli.HighPart = penTime.dwHighDateTime;

        pauseEndTime = (uli.QuadPart/10000000);

        // Debug logging
        #ifdef DEBUG
        Logger* logger = Logger::getInstance();
        logger->log("[RESUME]\n\n");
        #endif
    }
}


///
void Shuffler::onStart() {
    srand(time(NULL));
    
    #ifdef DEBUG
    Logger* logger = Logger::getInstance();
    if      (mode == ON_LIBRARY)    logger->log("[MODE] Smart shuffle on library.\n\n");
    else                            logger->log("[MODE] Smart shuffle on playlist.\n\n");
    #endif

    // Get playing track
    mapLoad.wait();
    if (!retrievePlayingTrack()) {
        playingTrack = map->getTrack(rand() % map->getSize());
    }

    // Flush playlist
    SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_DELETE);
    appendToPlaylist(playingTrack);
    SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_STARTPLAY);
}


///
void Shuffler::onStartPlaying() {
    // Check for mode
    if (mode == OFF)    return;

    // Retrieve information about playing track
    retrievePlayingTrack();
    
    // Manage history
    lastPlayedTracks.push_back(playingTrack);
    playingTrack->setAlreadyPlayed(true);

    if (lastPlayedTracks.size() > log((double)map->getSize()) + 1) {
        lastPlayedTracks.front()->setAlreadyPlayed(false);
        lastPlayedTracks.pop_front();
    }

    // Prepare next track
    prepareNextTrack();

    int trackLength = SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);

    // Update chronometers
	GetSystemTime(&sTime);
	SystemTimeToFileTime(&sTime,&stTime);

	uli.LowPart  = stTime.dwLowDateTime;
	uli.HighPart = stTime.dwHighDateTime;

	startTime = (uli.QuadPart/10000000);

    // Debug logging
    #ifdef DEBUG
    Logger* logger = Logger::getInstance();
    logger->log("[PLAYING] [");
    logger->log(playingTrack->getId());
    logger->log("] (");
    logger->log(playingTrack->getLength());
    logger->log("s)\nSize of history: ");
    logger->log(lastPlayedTracks.size());
    logger->log("\nCurrent position = ");
    logger->log(playlistPosition);
    logger->log("\nPlaylist length = ");
    logger->log(playlistLength);
    logger->log("\n\n");
	#endif
}


///
void Shuffler::onStopPlaying() {
    // Check for mode
    if (mode == OFF)    return;

    GetSystemTime(&eTime);
	SystemTimeToFileTime(&eTime, &enTime);

	uli.LowPart  = enTime.dwLowDateTime;
	uli.HighPart = enTime.dwHighDateTime;

	endTime = (uli.QuadPart/10000000);

    /* 
     * End of the track reached => play nearest neighbor
     * Contrary to what is suggested in the documentation
     * this will NOT be triggered when the user presses 
     * "next track" button.
     */
    if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISFULLSTOP)) {
        onEndTrack();
    } else {
        #ifdef DEBUG
        Logger* logger = Logger::getInstance();
        logger->log("[FULL STOP]\n\n");
        #endif
    }
}



///
Shuffler::LibraryScan::LibraryScan(Shuffler* newParent) : parent(newParent) {
}


///
Shuffler::LibraryScan::~LibraryScan() {
    wait();
}

/**
 * \brief Scans the whole media library and download coordinates for all tracks.
 */
void Shuffler::LibraryScan::run() {
    Logger* logger(Logger::getInstance());
    logger->log("Scanning library...\n");

    // Disable shuffler
    parent->disable();

    //  Build path to Media Library files
    string directory = parent->getConfigDirectory();
    char*  pathToDat = new char[directory.size() + 12];
    char*  pathToIdx = new char[directory.size() + 12];

    strcpy(pathToDat, directory.c_str());
    strcpy(pathToIdx, directory.c_str());
    strcat(pathToDat, "ml\\main.dat");      // SENSITIVE: may depend on Winamp version !
    strcat(pathToIdx, "ml\\main.idx");      // SENSITIVE: may depend on Winamp version !


    //  Pre-processing
    Database        db;
	Table*          table(db.OpenTable(pathToDat, pathToIdx, false, false)); // Do not create table either index
    Map*            map(Map::getInstance());
    Scanner         *scanner = table->NewScanner(0);
    unsigned long   i(0);
    
    map->clear();
    map->setSize(table->GetRecordsCount()); // ANN cannot allocate dynamically


	//  A scanner lets us iterate through the records
	for (scanner->First(); !scanner->Eof(); scanner->Next()) {
		//time_t time; // time_t -> char * conversion routines require a pointer, so we'll allocate on the stack

		/*
		Scanner::GetFieldByName(char *) returns a "Field *"
		we have to cast it to the appropriate subclass of Field * and hope for the best
		using dynamic_cast<> and making sure it doesn't cast to 0 would be better
		*/
		FilenameField*  fileName    = (FilenameField*)scanner->GetFieldByName("filename");
		StringField*    title       = (StringField*)scanner->GetFieldByName("title");
		StringField*    artist      = (StringField*)scanner->GetFieldByName("artist");
		StringField*    album       = (StringField*)scanner->GetFieldByName("album");
		IntegerField*   year        = (IntegerField*)scanner->GetFieldByName("year");
		StringField*    genre       = (StringField*)scanner->GetFieldByName("genre");
        IntegerField*   length      = (IntegerField*)scanner->GetFieldByName("length");
		IntegerField*   lastPlay    = (IntegerField*)scanner->GetFieldByName("lastplay"); // time_t
		IntegerField*   rating      = (IntegerField*)scanner->GetFieldByName("rating");
		IntegerField*   playCount   = (IntegerField*)scanner->GetFieldByName("playCount");

		/*
		sometimes Scanner::GetFieldByName() returns 0 (usually if the field isn't present for this record)
		so we need to check.
		*/

        Track newTrack(i);
		if (fileName && fileName->GetString())
			newTrack.setPath(fileName->GetString());
        if (title && title->GetString())
			newTrack.setTitle(title->GetString());
        if (artist && artist->GetString())
			newTrack.setArtist(artist->GetString());
		if (album && album->GetString())
			newTrack.setAlbum(album->GetString());
		if (year)
            newTrack.setYear(year->GetValue());
        if (genre && genre->GetString())
            newTrack.setGenre(genre->GetString());
        if (length)
            newTrack.setLength(length->GetValue());

        // TODO: implement the following
		/*if (lastPlay) {
			newTrack.setLastPlay(lastPlay->GetValue());
			cout << "lastPlay="<<  asctime(localtime(&lastPlay->GetValue()));
		}
		if (rating)			
			cout << "rating="<< rating->GetValue() << endl;
		if (playCount)
			cout << "playCount="<< playCount->GetValue() << endl; // */
        
        map->insert(newTrack);
	}

    // Download coordinates and save them
    map->downloadMissingCoordinates();
    map->save();

	//  Cleanup
	table->DeleteScanner(scanner);
	db.CloseTable(table);
    parent->enable();

    // Notify user
    MessageBoxA(
        plugin.hwndParent,
        "The library has been scanned successfully, you can now use smart shuffle.",
        "Scan complete",
        MB_OK
    );

    logger->log("Library successfully scanned.\n\n");
}


///
Shuffler::MapLoad::MapLoad(Shuffler* newParent, string newFilename) :
        parent(newParent),
        filename(newFilename) {
}


///
Shuffler::MapLoad::~MapLoad() {
    wait();
}


/**
 * \brief Load map from file.
 */
void Shuffler::MapLoad::run() {
    // Disable shuffler first
    parent->disable();

    Map* map(Map::getInstance());
    if (map->load(filename)) {
        parent->enable();
        return;
    }

    // Unable to load map from file
    //  => propose to scan library, if database is reachable
    if (!parent->databaseAvailable())     return;

    string question;
        question += "In order to use smart shuffle, ";
        question += "you must first scan the media library.";

        question += "Scanning may take several minutes for big collections; ";
        question += "it is done in background, so that you can keep using ";
        question += "Winamp during this operation. ";
        question += "You will be warned when the scan is complete.\n\n";

        question += "Would you like to do it now ?\n\n";

        question += "If you don't, you can do it later via the ";
        question += "\"Play\" menu entry.";

    int answer(MessageBoxA(
        plugin.hwndParent,
        question.c_str(),
        "Scan library now ?",
        MB_YESNO | MB_ICONQUESTION
    ));

    if (answer == IDYES)
        parent->scanLibrary();
}


///
Shuffler::NextTrackPreparation::NextTrackPreparation(Shuffler* newParent) :
        parent(newParent) {
}


///
Shuffler::NextTrackPreparation::~NextTrackPreparation() {
    wait();
}


///
void Shuffler::NextTrackPreparation::run() {
    Map* map(Map::getInstance());
    srand(time(NULL));

    unsigned int n(parent->lastPlayedTracks.size());

    // Current track has no coordinate => next is random
    if (!parent->playingTrack->hasCoordinates()) {
        parent->localNextTrack  = map->getTrack(rand() % map->getSize());
        parent->remoteNextTrack = parent->localNextTrack;

        // Debug logging
        #ifdef DEBUG
        Logger* logger = Logger::getInstance();
        logger->log("Local next track : [");
        logger->log(parent->localNextTrack->getId());
        logger->log("]");
        if (parent->playingTrack->hasCoordinates()) {
            logger->log(" (distance = ");
            logger->log(parent->distanceBetween(parent->playingTrack, parent->localNextTrack));
            logger->log(")");
        }
        logger->log("\nRemote next track : [");
        logger->log(parent->remoteNextTrack->getId());
        logger->log("]");
        if (parent->playingTrack->hasCoordinates() && parent->remoteNextTrack->hasCoordinates()) {
            logger->log(" (distance = ");
            logger->log(parent->distanceBetween(parent->playingTrack, parent->remoteNextTrack));
            logger->log(")");
        }
        logger->log("\n");
        #endif DEBUG

        return;
    }
    
    // Next track within local area
    const ANNidxArray   nearestTracks(map->findNearestNeighbors(parent->playingTrack, n+1));
    unsigned long       i(1);

    while (i < n+1) {
        if (!map->getTrack(nearestTracks[i])->isAlreadyPlayed()) {
            parent->localNextTrack = map->getTrack(nearestTracks[i]);
            break;
        }

        i++;
    }


    // No reference to compute distance => next remote is random
    if ((n > 1 && !parent->lastPlayedTracks[n-2]->hasCoordinates())
    ||  !parent->playingTrack->hasCoordinates()) {
        parent->remoteNextTrack = map->getTrack(rand() % map->getSize());

        // Debug logging
        #ifdef DEBUG
        Logger* logger = Logger::getInstance();
        logger->log("Local next track : [");
        logger->log(parent->localNextTrack->getId());
        logger->log("]");
        if (parent->playingTrack->hasCoordinates()) {
            logger->log(" (distance = ");
            logger->log(parent->distanceBetween(parent->playingTrack, parent->localNextTrack));
            logger->log(")");
        }
        logger->log("\nRemote next track : [");
        logger->log(parent->remoteNextTrack->getId());
        logger->log("]");
        if (parent->playingTrack->hasCoordinates() && parent->remoteNextTrack->hasCoordinates()) {
            logger->log(" (distance = ");
            logger->log(parent->distanceBetween(parent->playingTrack, parent->remoteNextTrack));
            logger->log(")");
        }
        logger->log("\n");
        #endif DEBUG

        return;
    }
    
    // Increase distance to next remote
    if (n > 1) {
        parent->remoteRadius = parent->remoteScale * parent->remoteRadius;
        parent->remoteRadius = parent->remoteConstant + parent->remoteRadius;
        parent->remoteRadius = min(parent->remoteRadius, parent->remoteBound);
    } else {
        parent->remoteRadius = parent->remoteScale * parent->remoteRadius;
        parent->remoteRadius = parent->remoteConstant + parent->remoteRadius;
        parent->remoteRadius = min(parent->remoteRadius, parent->remoteBound);
    }
       
    ANNpoint            remotePoint(randomPointOnSphere(map->getDimensions(), parent->remoteRadius));
    const ANNidxArray   results(map->findNearestNeighbors(remotePoint, 3));
    unsigned int        j(1);

    if (results[j] == parent->playingTrack->getId())    j = 2;
        
    parent->remoteNextTrack = map->getTrack(results[j]);

    // Debug logging
    #ifdef DEBUG
    Logger* logger = Logger::getInstance();
    logger->log("Local next track : [");
    logger->log(parent->localNextTrack->getId());
    logger->log("]");
    if (parent->playingTrack->hasCoordinates()) {
        logger->log(" (distance = ");
        logger->log(parent->distanceBetween(parent->playingTrack, parent->localNextTrack));
        logger->log(")");
    }
    logger->log("\nRemote next track : [");
    logger->log(parent->remoteNextTrack->getId());
    logger->log("]");
    if (parent->playingTrack->hasCoordinates()) {
        logger->log(" (distance = ");
        //logger->log(parent->distanceBetween(parent->playingTrack, parent->remoteNextTrack));
        logger->log(parent->remoteRadius);
        logger->log(")");
    }
    logger->log("\n");
    #endif DEBUG
}
