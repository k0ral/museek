/**
 * \file track.cpp
 * \brief Track class implementation.
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include "curl.h"
#include "wa_ipc.h"

#include "gen_museek.h"
#include "track.h"
#include "utils.h"

using namespace std;


extern winampGeneralPurposePlugin plugin;


/**
 * \brief Default constructor.
 *
 * Allocate storage for coordinates.
 */
Track::Track() :
        artistID(),
        titleID(),
        year(0),
        length(0), 
        path(), 
        id(),
        code(UNTESTED),
        alreadyPlayed(false) {
}


/**
 * \brief Constructor from map and ANNpoint index.
 *
 * \param i Index of the corresponding ANNpoint in the ANNpointArray of the parent map.
 */
Track::Track(ANNidx i) :
        artist(),
        artistID(),
        title(),
        titleID(),
        length(0),
        path(),
        id(i),
        code(UNTESTED),
        alreadyPlayed(false) {
};


/**
 * \brief Constructor from artist, title and path.
 *
 * \param newArtist Artist's name.
 * \param newTitle  Title of the track.
 * \param newPath   File location (facultative).
 */
Track::Track(string newArtist, string newTitle, string newPath) :
        artist(newArtist),
        artistID(),
        title(newTitle),
        titleID(),
        path(newPath),
        id(),
        code(UNTESTED),
        alreadyPlayed(false) {
};


/**
 * \brief Destructor.
 *
 * Does nothing.
 */
Track::~Track() {
};


/// \return Album of the track.
string Track::getAlbum() const {
    return album;
}


/// \return Artist's name.
string Track::getArtist() const {
    return artist;
}


/// \return Artist ID in Museek database.
unsigned int Track::getArtistID() const {
    return artistID;
}


/// \return Genre of the song.
string Track::getGenre() const {
    return genre;
}


/// \return Index of the track in the map.
ANNidx Track::getId() const {
    return id;
}


/// \return Length of the track, in seconds.
unsigned long Track::getLength() const {
    return length;
}


/// \return File location.
string Track::getPath() const {
    return path;
}


/// \return Title of the track.
string Track::getTitle() const {
    return title;
}


/// \return Title ID in Museek database.
unsigned int Track::getTitleID() const {
    return titleID;
}


/// \return Year of the track.
unsigned int Track::getYear() const {
    return year;
}


/// \return The so-called Museek code, corresponding to the response code from the server.
MuseekCode Track::getCode() const {
    return code;
}


/// \return Requested coordinate for the track; if not retrieved from the server yet, perform the adequate HTTP query.
ANNcoord Track::getCoordinate(unsigned short i) {
    if (!hasCoordinates())
        return NULL;

    Map* map = Map::getInstance();
    return (map->getPoint(id))[i];
}


/// \return Coordinates array for the track; if not retrieved from the server yet, perform the adequate HTTP query.
ANNpoint Track::getCoordinates() {
    if (!hasCoordinates())
        return NULL;

    Map* map = Map::getInstance();
    return map->getPoint(id);
}


/// \return True if coordinates were found in server, false otherwise.
bool Track::hasCoordinates() {
    // Try to download coordinates, if necessary
    if (code == UNTESTED)
        downloadCoordinates();

    // Check if coordinates were found
    if (code == NOTHING_FOUND || code == ARTIST_NOT_FOUND)
        return false;
    
    return true;
}


/// \return True if this tracks has already been played, false otherwise.
bool Track::isAlreadyPlayed() const {
    return alreadyPlayed;
}


/**
 * \brief Set whether this tracks has already been played or not.
 * 
 * \param newState 
 */
void Track::setAlreadyPlayed(bool newState) {
    alreadyPlayed = newState;
}


/**
 * \brief Set the album of the track.
 * 
 * \param newAlbum New album for the track.
 */
void Track::setAlbum(string newAlbum) {
    album = newAlbum;
}


/**
 * \brief Set the artist name.
 * 
 * \param newArtist New artist name.
 */
void Track::setArtist(string newArtist) {
    artist = newArtist;
}


/**
 * \brief Set the artist ID.
 * 
 * \param newID New artist ID.
 */
void Track::setArtistID(unsigned int newID) {
    artistID = newID;
}


/**
 * \brief Set the response code used by museek server.
 * 
 * \param newCode New code.
 */
void Track::setCode(MuseekCode newCode) {
    code = newCode;
}


/**
 * \brief Set the genre of the track.
 *
 * \param newGenre New genre.
 */
void Track::setGenre(string newGenre) {
    genre = newGenre;
}


/**
 * \brief Set the ID of the track, index of its corresponding point.
 *
 * \param n New id.
 */
void Track::setId(ANNidx n) {
    id = n;
}


/**
 * \brief Set the length of the track, in seconds.
 *
 * \param newLength New length.
 */
void Track::setLength(unsigned long newLength) {
    length = newLength;
}


/**
 * \brief Set the file location.
 * 
 * \param newPath New location.
 */
void Track::setPath(string newPath) {
    path = newPath;
    //transform(path.begin(), path.end(), path.begin(), ::tolower);
}


/**
 * \brief Set the track title.
 * 
 * \param newTitle New title.
 */
void Track::setTitle(string newTitle) {
    title = newTitle;
}


/**
 * \brief Set the title ID.
 * 
 * \param newID New title ID.
 */
void Track::setTitleID(unsigned int newID) {
    titleID = newID;
}


/**
 * \brief Set the year of the track.
 * 
 * \param newYear New year for the track.
 */
void Track::setYear(unsigned int newYear) {
    year = newYear;
}


/**
 * \brief Ask www.musicexplorer.org server for the coordinates of the track.
 * 
 * \return Nothing.
 */
void Track::downloadCoordinates() {
    Map* map = Map::getInstance();
    map->downloadCoordinates(id);
}


/**
 * \brief Set the k-th coordinate to the given value.
 * 
 * \param k         index of the coordinate.
 * \param ANNcoord  new value for the coordinate.
 * 
 * \return Nothing.
 */
void Track::setCoordinate(unsigned short k, ANNcoord coordinate) {
    Map* map = Map::getInstance();
    map->setCoordinate(id, k, coordinate);
}