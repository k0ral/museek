/**
 * \file map.cpp
 * \brief Map class implementation.
 */

#include <fstream>
#include <sstream>
//#include <windows.h>  // Needed for progress bar
//#include <commctrl.h> // Needed for progress bar

#include "curl.h"
#include "wa_ipc.h"
#include "nde/NDE.h"

#include "constants.h"
#include "logger.h"
#include "gen_museek.h"
#include "map.h"
#include "shuffler.h"
#include "track.h"
#include "utils.h"

using namespace std;


Map* Map::instance = NULL;
HINSTANCE g_inst;

extern winampGeneralPurposePlugin   plugin;


/// \brief Callback function used by curl.
static size_t writer(char* data, size_t size, size_t nmemb, string* buffer) {
    int result = 0;

    if (buffer != NULL)  {
        buffer->append(data, size * nmemb);  

        result = size * nmemb;
    }
  
    return result;  
}


/// \brief Default constructor.
Map::Map() :
        allocatedPoints(0),
        dimensions(32),
        tracksPerQuery(25),
        points(NULL),
        kDimensionalTree(NULL),
        errorBound(0),
        resultsID(NULL),
        distances(NULL) {
}


/**
 * \brief Destructor.
 *
 * Deallocate memory for coordinates, and clean up curl.
 */
Map::~Map() {
    delete kDimensionalTree;

    if (resultsID && distances) {
        delete[] resultsID;
        delete[] distances;
    }

    annDeallocPts(points);
}


/// \return Unique instance of Map class.
Map* Map::getInstance() {
    if (instance == NULL)
        instance = new Map;

    return instance;
}


/// \brief Delete the unique instance of Map class.
void Map::kill() {
    if (instance != NULL) {
        delete instance;
        instance = NULL;
    }
}


/// \return Number of dimensions of the map space.
unsigned short Map::getDimensions() const {
    return dimensions;
}


/**
 * \param k Index of the point.
 * \return The point at the given index.
 */
ANNpoint Map::getPoint(ANNidx k) {
    if (!(tracks[k].hasCoordinates()))
        return NULL;

    return points[k];
}


/// \return Number of tracks in current map.
unsigned int Map::getSize() const {
    return tracks.size();
}


/**
 * \param k Index of the track.
 * \return The track at the given index.
 */
Track* Map::getTrack(ANNidx k) {
    return &(tracks[k]);
}


/**
 * \brief Add a single track to the map.
 *
 * This should only be used for debugging purposes, unless one finds a better way to implement it;
 * indeed, the current implementation has a linear time cost.
 */
void Map::addTrack(string artist, string title, string path) {
    ANNpointArray  newPoints = annAllocPts(tracks.size() + 1, dimensions);
    unsigned int   i(0), j(0);

    while (i < tracks.size()) {
        while (j < dimensions) {
            newPoints[i][j] = points[i][j];
            j++;
        }

        j = 0;
        i++;
    }
    
    annDeallocPts(points);
    points = newPoints;


    Track newTrack(i);
        newTrack.setArtist(artist);
        newTrack.setTitle(title);
        newTrack.setPath(path);

    missingCoordinates.push_back(tracks.size());
    tracks.push_back(newTrack);
}


/**
 * \brief Download coordinates for a single track.
 *
 * \param index Index of the track in the map.
 *
 * This method is provided only for convenience.
 */
bool Map::downloadCoordinates(ANNidx index) {
    list<ANNidx> indices;
    indices.push_back(index);

    return downloadCoordinates(indices);
}


/**
 * \brief Download coordinates for several tracks.
 *
 * \param indices Indices of tracks in the map.
 */
bool Map::downloadCoordinates(list<ANNidx> indices) {
    if (indices.empty())    return true;

    // TODO: make this work
    /* Display a progress bar
    InitCommonControls();

    progressBarHandle = CreateWindowEx(0, PROGRESS_CLASS,
        (LPCWSTR)"Downloading coordinates...", WS_CHILD | WS_VISIBLE,
        0, 0, 200, 200, 
        plugin.hwndParent, (HMENU)0, g_inst, NULL);
    
    SendMessage(progressBarHandle, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
    SendMessage(progressBarHandle, PBM_SETSTEP, (WPARAM) 1, 0);
    SendMessage(progressBarHandle, PBM_STEPIT, 0, 0);
    
    DestroyWindow(progressBarHandle);*/

    // Initialization
    Logger*                 logger(Logger::getInstance());
    string                  URL(SCRIPT_URL), // TODO: turn SCRIPT_URL into a parameter in the config file
                            response,
                            error;
    list<ANNidx>            processedIndices;
    //list<ANNidx>::iterator  i = indices.begin();
    ANNidx                  i;
    unsigned int            j(0);

    // Split queries into N tracks each
    while (!indices.empty()) {
        while (!indices.empty() && j < tracksPerQuery) {
            i = indices.front();
            indices.pop_front();

            // Check if index if valid
            if (i >= tracks.size()) {
                logger->log("[WARNING] Invalid index of track (");
                logger->log(i);
                logger->log(" > ");
                logger->log(tracks.size());
                logger->log(")\n\n");

                continue;
            }

            // Build URL
            ostringstream stream;
            stream << j;

            if (!j)     URL += "?";
            else        URL += "&";

            URL += "artist[";
            URL += stream.str();
            URL += "]=";
            URL += URLEncode(tracks[i].getArtist());
            URL += "&title[";
            URL += stream.str();
            URL += "]=";
            URL += URLEncode(tracks[i].getTitle());

            // Remember current indices
            processedIndices.push_back(i);

            j++;
        }

        // Setup the HTTP connection
        CURL* curlSession = curl_easy_init();
            curl_easy_setopt(curlSession, CURLOPT_URL, URL.c_str());
            curl_easy_setopt(curlSession, CURLOPT_NOPROGRESS, 1L);          // Turn off progress meter
            curl_easy_setopt(curlSession, CURLOPT_WRITEFUNCTION, writer);
            curl_easy_setopt(curlSession, CURLOPT_WRITEDATA, &response);
            //curl_easy_setopt(curlSession, CURLOPT_ERRORBUFFER, error);

        curl_easy_perform(curlSession);
        curl_easy_cleanup(curlSession);

        // TODO: test if response is valid
        //MessageBoxA(plugin.hwndParent, response.c_str(), "RESPONSE", MB_OK);

        /*char effectiveURL[500];
        curl_easy_getinfo(curlSession, CURLINFO_EFFECTIVE_URL, effectiveURL);
        MessageBoxA(plugin.hwndParent, effectiveURL, "EFFECTIVE URL", MB_OK);

        /*long responseCode;
        curl_easy_getinfo(curlSession, CURLINFO_RESPONSE_CODE, responseCode);
        MessageBoxA(plugin.hwndParent, responseCode, "responseCode", MB_OK);//*/

        /*double sizedownload;
        curl_easy_getinfo(curlSession, CURLINFO_SIZE_DOWNLOAD , sizedownload);
        MessageBoxA(plugin.hwndParent, sizedownload, "sizedownload", MB_OK);//*/

        /*char primaryIP[500];
        curl_easy_getinfo(curlSession, CURLINFO_PRIMARY_IP , primaryIP);
        MessageBoxA(plugin.hwndParent, primaryIP, "primaryIP", MB_OK);//*/

        // Log response
        #ifdef DEBUG_HTTP_QUERY
        logger->log("[HTTP response]\n" + response + "\n\n");
        #endif

        parseResponse(response, processedIndices);

        // Post-processing
        processedIndices.clear();
        response.clear();
        error.clear();

        URL = SCRIPT_URL;
        j   = 0;
    }

    // Update k-dimensional tree
    if (kDimensionalTree)   delete kDimensionalTree;

    kDimensionalTree = new ANNkd_tree(points, tracks.size(), dimensions);

    return true;
}


/**
 * \brief Download coordinates for all tracks that still don't have ones.
 *
 * This method is provided only for convenience.
 */
bool Map::downloadMissingCoordinates() {
    return downloadCoordinates(missingCoordinates);
}


/// \brief Inserts a track without computing its coordinates (lazy behavior).
void Map::insert(Track newTrack) {
    unsigned long n(tracks.size());

    if (newTrack.getId() != n)
        newTrack.setId(n);

    missingCoordinates.push_back(n);
    fileIndex[newTrack.getPath()] = n;
    tracks.push_back(newTrack);
}


/**
 * \brief Process HTTP response from musicexplorer database. (See Coordinate_Server_Format_Description.txt)
 * 
 * \param  response The HTTP response from the server.
 * \param  indices  The list of indices of tracks concerned by this HTTP query.
 *
 * \return Nothing.
 */
void Map::parseResponse(string response, list<ANNidx>& indices) {
    Logger*                     logger(Logger::getInstance());
    unsigned long               k(0), buffer(0);
    ResponseType                next(CODE);
    MuseekCode                  code(UNTESTED);
    short                       newCode;
    ANNcoord                    coordinate;
    string                      line;
    stringstream                stream(response);
    ANNidx                      i(indices.front());

    // Parse lines one by one
    while (!stream.eof()) {
        // Response code
        if (next == CODE) {
            getline(stream, line);
            stringstream ss(line);
            ss >> newCode;

            if (newCode == 4) newCode = 3; // Codes 3 and 4 are equivalent
                code = (MuseekCode)newCode;

            tracks[i].setCode(code);

            //  Next line
            if (code == ALL_FOUND)
                next = ARTIST_ID;
            else if (code == ARTIST_APPROXIMATE || code == ARTIST_TITLE_APPROXIMATE || code == TITLE_NOT_FOUND)
                next = ARTIST;
            else if (code == TITLE_APPROXIMATE)
                next = TITLE;
            else if (code == ARTIST_NOT_FOUND || code == NOTHING_FOUND) {
                // Switch to next track
                indices.pop_front();
                if (!indices.size())    break;
                
                i       = indices.front();
                next    = CODE;
            }
        }


        //  Artist
        else if (next == ARTIST) {
            getline(stream, line);

            if (stream.bad())
                logger->log("[WARNING] Unable to read artist name from HTTP response.");
            else
                tracks[i].setArtist(line);

            //  Next line
            if (code == ARTIST_APPROXIMATE || code == TITLE_NOT_FOUND)
                next = ARTIST_ID;
            else if (code == ARTIST_TITLE_APPROXIMATE)
                next = TITLE;
            //else
            //  TODO: throw exception
        }


        //  Title
        else if (next == TITLE) {
            getline(stream, line);

            if (stream.bad())
                logger->log("[WARNING] Unable to read title from HTTP response.");
            else
                tracks[i].setTitle(line);

            next = ARTIST_ID;
        }


        //  Artist ID
        else if (next == ARTIST_ID) {
            getline(stream, line);
            stringstream ss(line);
            ss >> buffer;

            tracks[i].setArtistID(buffer);

            //  Next line
            if (code == ALL_FOUND || code == TITLE_APPROXIMATE || code == ARTIST_APPROXIMATE || code == ARTIST_TITLE_APPROXIMATE)
                next = TITLE_ID;
            else if (code == TITLE_NOT_FOUND)
                next = COORDINATE;
            //else
            //  TODO: throw exception
        }


        // Title ID
        else if (next == TITLE_ID) {
            getline(stream, line);
            stringstream ss(line);
            ss >> buffer;

            tracks[i].setTitleID(buffer);
            next = COORDINATE;
        }


        // Coordinates
        else if (next == COORDINATE && k < dimensions) {
            getline(stream, line);
            stringstream ss(line);
            ss >> coordinate;
            
            points[i][k] = coordinate;
            
            k++;

            if (k == dimensions) {
                missingCoordinates.remove(i);
                
                // Switch to next track
                indices.pop_front();
                if (!indices.size())    break;
                
                i       = indices.front();
                next    = CODE;
                k       = 0;
            }
        }
    }
}


///
void Map::setNearestNeighborErrorBound(double newBound) {
    errorBound = newBound;
}


///
void Map::setDimensions(unsigned short newDimensions) {
    dimensions = newDimensions;
}


///
void Map::setCoordinate(ANNidx i, unsigned short k, ANNcoord coordinate) {
    (points[i])[k] = coordinate;
}


///
void Map::setParent(Shuffler* newParent) {
    parent = newParent;
}


/**
 * \brief Allocates memory for coordinates.
 *
 * ANN cannot allocate memory dynamically; therefore, this function has to
 * be used each time the size of the map changes.
 * 
 * \param n New size for the map.
 */
void Map::setSize(unsigned long n) {
    if (points)     annDeallocPts(points);

    points = annAllocPts(n, dimensions);
    allocatedPoints = n;

    // TODO: extend vectors<> if necessary ?
    // tracks.reserve()
}


///
void Map::setTracksPerQuery(unsigned short n) {
    tracksPerQuery = n;
}


void Map::clear() {
    if (points)     annDeallocPts(points);
    fileIndex.clear();
    missingCoordinates.clear();
    tracks.clear();
    
    allocatedPoints = 0;
}


/**
 * \brief   Find a track by its title and filename.
 * 
 * \param   title       Title of the track.
 * \param   filename    Full path to the track file.
 * \return  Pointer to the track in map.
 */
Track* Map::findTrack(string title, string filename) {
    // TODO: we should perform some check using title
    // For now, title remains unused
    return getTrack(fileIndex[filename]);
}


/**
 * \brief   Find a track by its title and filename.
 *
 * This function is only provided for convenience.
 *
 * \param   title       Title of the track.
 * \param   filename    Full path to the track file.
 * \return  Pointer to the track in map.
 */
Track* Map::findTrack(wstring title, wstring filename) {
    return findTrack(narrow(title), narrow(filename));
}


/**
 * \brief Find nearest neighbors of a given point among the map.
 * 
 * \param   point   Reference point from which nearest neighbor has to be found.
 * \param   k       Number of nearest neighbors to search.
 * \return          Indices of ordered nearest neighbors.
 */
const ANNidxArray Map::findNearestNeighbors(ANNpoint point, unsigned short k) {
    // Pre-processing
    unsigned short n(max(k,2));

    if (resultsID && distances) {
        delete[] resultsID;
        delete[] distances;
    }

    //  Perform the search
    resultsID   = new ANNidx[n];
    distances   = new ANNdist[n];

    kDimensionalTree->annkSearch(point, n, resultsID, distances, errorBound);

    return resultsID;
}


/**
 * \brief Find nearest neighbors of a track among the map.
 *
 * Only provided for convenience.
 *
 * \param i Index of the track in the map.
 * \param k Number of nearest neighbors to search.
 * \return  Indices of ordered nearest neighbors.
 */
const ANNidxArray Map::findNearestNeighbors(ANNidx i, unsigned short k) {
    return findNearestNeighbors(points[i], k);
}


/**
 * \brief Find nearest neighbors of a track among the map.
 *
 * Only provided for convenience.
 *
 * \param track Pointer to a track in the map.
 * \param k     Number of nearest neighbors to search.
 * \return      Indices of ordered nearest neighbors.
 */
const ANNidxArray Map::findNearestNeighbors(const Track* track, unsigned short k) {
    return findNearestNeighbors(points[track->getId()], k);
}


ANNidx Map::findNearestNeighbor(ANNidx i) {
    const ANNidxArray indices = findNearestNeighbors(points[i]);

    return indices[1];
}


Track Map::findNearestNeighbor(Track track) {
    const ANNidxArray indices = findNearestNeighbors(track.getCoordinates());

    return tracks[indices[1]];
}


/**
 * \brief Load map from a text file.
 *
 * The file should be in the user's winamp directory;
 * the absolute path to this directory is automatically generated and should not be provided in the argument.
 *
 * \param filename Name of the file.
 * \return True if map was successfully loaded, false otherwise.
 */
bool Map::load(string filename) {
    Shuffler*   shuffler(Shuffler::getInstance());
    Logger*     logger(Logger::getInstance());
    
    // Open map file
    string path = shuffler->getConfigDirectory() + filename;

    ifstream file(path.c_str(), ios::in);

    // Unable to open file
    if (!file) {
        logger->log("[WARNING] Unable to open map file (" + path + ").\n\n");
        return false;
    }
    
    // Pre-processing
    string          line;
    unsigned long   total(0);

    clear();
    getline(file, line);
    {stringstream stream(line);
    stream >> total;}


    // Check if library needs to be rescanned
    string directory = shuffler->getConfigDirectory();
    char*  pathToDat = new char[directory.size() + 12];
    char*  pathToIdx = new char[directory.size() + 12];

    strcpy(pathToDat, directory.c_str());
    strcpy(pathToIdx, directory.c_str());
    strcat(pathToDat, "ml\\main.dat");      // SENSITIVE: may depend on Winamp version !
    strcat(pathToIdx, "ml\\main.idx");      // SENSITIVE: may depend on Winamp version !

    Database        db;
	Table*          table(db.OpenTable(pathToDat, pathToIdx, false, false)); // Do not create table either index
    Scanner         *scanner = table->NewScanner(0);
    unsigned long   mapRecordsCount(table->GetRecordsCount());

    table->DeleteScanner(scanner);
	db.CloseTable(table);
    
    if (mapRecordsCount != total) {
        string question;
        question += "Your media library has changed since last time.\n";
        question += "Do you want to rescan it ?";

        int answer(MessageBoxA(
            plugin.hwndParent,
            question.c_str(),
            "Rescan library ?",
            MB_YESNO | MB_ICONQUESTION
        ));

        if (answer == IDYES) {
            shuffler->scanLibrary();
            return true;
        }
    }
    

    // No rescanning required => load from file
    points = annAllocPts(total, dimensions);

    ANNidx          i(0);
    unsigned short  j(0);
    unsigned int    ID(0);
    unsigned long   length(0);
    ANNcoord        coordinate;
    short           code;
    string          newPath;

    // Read line by line
    while (!file.eof() && i < total) {
        Track newTrack(i);
        getline(file, line);
        stringstream stream(line);

        // Extract MuseekCode
        stream >> code;
        newTrack.setCode((MuseekCode)code);

        // Extract artist ID
        if (code != ARTIST_NOT_FOUND && code != NOTHING_FOUND) {
            stream >> ID;
            newTrack.setArtistID(ID);
        }

        // Extract title ID
        if (code != ARTIST_NOT_FOUND && code != TITLE_NOT_FOUND && code != NOTHING_FOUND) {
            stream >> ID;
            newTrack.setTitleID(ID);
        }

        // Extract length
        stream >> length;
        newTrack.setLength(length);

        // Extract coordinates
        if (newTrack.hasCoordinates()) {
            while (j < dimensions) {
                stream >> coordinate;
                points[i][j] = coordinate;

                j++;
            }

            j = 0;
        }

        // Extract path
        stream >> newPath;
        newTrack.setPath(findAndReplace(newPath, "|", " "));

        tracks.push_back(newTrack);
        fileIndex[newTrack.getPath()] = tracks.size() - 1;

        /*#ifdef DEBUG
        Logger* logger = Logger::getInstance();
        logger->log(newTrack.getPath());
        logger->log(" => ");
        logger->log(tracks.size() - 1);
        logger->log("\n");
        #endif*/

        i++;
    }

    file.close();

    // Update k-dimensional tree
    if (kDimensionalTree)   delete kDimensionalTree;

    kDimensionalTree = new ANNkd_tree(points, tracks.size(), dimensions);
    
    return true;
}


/**
 * \brief Store current map in a text file.
 *
 * \param filename Name of the file.
 * 
 * The file will be created, if necessary, in the user's Winamp directory;
 * the absolute path to this directory is automatically generated and should not be provided in the argument.
 * Each track is stored in a different line with the following pattern:
 *      MuseekCode artistID titleID length coordinate1 coordinate2 ... coordinateN path
 */
bool Map::save(string filename) {
    Shuffler*   shuffler(Shuffler::getInstance());
    Logger*     logger(Logger::getInstance());

    #ifdef DEBUG
    logger->log("Saving map...\n");
    #endif

    // Create/overwrite file
    string path = shuffler->getConfigDirectory() + filename;
    ofstream file(path.c_str(), ios::out | ios::trunc);
 
    // Unable to create/overwrite map file
    if (!file) {
        logger->log("[ERROR] Unable to create/overwrite map file (" + path + ").\n");
        return false;
    }
    
    // Download missing coordinates
    downloadMissingCoordinates();
    

    // Write total number of tracks
    file << tracks.size() << endl;

    vector<Track>::iterator i(tracks.begin());
    unsigned short          j(0);

    while (i != tracks.end()) {
        Track       track   = *i;
        MuseekCode  code    = track.getCode();

        file << code;

        // Write artist ID
        if (code == ALL_FOUND
        ||  code == ARTIST_APPROXIMATE
        ||  code == TITLE_APPROXIMATE
        ||  code == ARTIST_TITLE_APPROXIMATE
        ||  code == TITLE_NOT_FOUND)
            file << " " << track.getArtistID();

        // Write title ID
        if (code == ALL_FOUND
        ||  code == ARTIST_APPROXIMATE 
        ||  code == TITLE_APPROXIMATE
        ||  code == ARTIST_TITLE_APPROXIMATE)
            file << " " << track.getTitleID();

        // Write length
        file << " " << track.getLength();

        // Write coordinates
        if (track.hasCoordinates()) {
            while (j < dimensions) {
                file << " " << track.getCoordinate(j);
                j++;
            }
        
            j = 0;
        }

        // Write path
        file << " " << findAndReplace(track.getPath(), " ", "|");
        
        file << endl;
        ++i;
    }

    file.close();

    #ifdef DEBUG
    logger->log("Map successfully saved\n\n");
    #endif

    return true;
}
