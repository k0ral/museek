#ifndef CONSTANTS_H
    #define CONSTANTS_H

    /**
     * \file	constants.h
     * \brief	Constants definition.
     */

    #include <windows.h>

    #define DEBUG               // Comment out this line to ignore all debug instructions
    
    // Plugin constants
    #define GPPHDR_VER              0x10    // Plugin version (don't touch this !)
	#define PLUGIN_NAME			    "Museek"
    
    // Paths and URLs
    #define SERVER_URL          "http://www.musicexplorer.org"
    #define SCRIPT_PATH         "/services_museek/getCoordinatesInPackagesNoXML.php"
    #define SCRIPT_URL          "http://www.musicexplorer.org/services_museek/getCoordinatesInPackagesNoXML.php"
    #define CONFIG_FILE         "museek.conf"
    #define LOG_FILE            "museek.log"
    #define MAP_FILE            "map.txt"

    /* Codes for Winamp buttons
     *  Usage:
	 *  if (message == WM_COMMAND && wParam == WINAMP_BUTTON1)
	 *      cout << "PREVIOUS" << "\n";
     */
    #define WINAMP_BUTTON1          40044
    #define WINAMP_BUTTON2          40045
    #define WINAMP_BUTTON3          40046
    #define WINAMP_BUTTON4          40047
    #define WINAMP_BUTTON5          40048

    #define WINAMP_BUTTON_PREV      WINAMP_BUTTON1
    #define WINAMP_BUTTON_PLAY      WINAMP_BUTTON2
    #define WINAMP_BUTTON_PAUSE     WINAMP_BUTTON3
    #define WINAMP_BUTTON_STOP      WINAMP_BUTTON4
    #define WINAMP_BUTTON_NEXT      WINAMP_BUTTON5

    #define IPC_STOPPLAYING         3043


    typedef struct {
	    int last_time;
	    int g_fullstop;
    } stopPlayingInfoStruct;

    union timeunion {
	    FILETIME        fileTime;
	    ULARGE_INTEGER  ul;
    };


    /**
     * \brief Return code used by museek database when searching a song.
     *
     * See Coordinate_Server_Format_Description.txt.
     */
    enum MuseekCode {
        UNTESTED = -4,              ///< Database not queried yet
        NOTHING_FOUND,              ///< No matching at all
        TITLE_NOT_FOUND,            ///< Artist found, title not found
        ARTIST_NOT_FOUND,           ///< Artist not found, title found
        ALL_FOUND,                  ///< Exact matching
        ARTIST_APPROXIMATE,         ///< Artist approximate, exact title
        TITLE_APPROXIMATE,          ///< Exact artist, title approximate
        ARTIST_TITLE_APPROXIMATE    ///< Artist and title approximate
    };


    /**
     * \brief Segments of the HTTP response.
     *
     * See Coordinate_Server_Format_Description.txt.
     */
    enum ResponseType {
        CODE,			///< Response code (see MuseekCode enum)
        ARTIST,			///< Artist name
        TITLE,			///< Title of the track
        ARTIST_ID,		///< Artist ID in Museek database
        TITLE_ID,		///< Title ID in Museek database
        COORDINATE		///< Coordinate
    };


    // Other constants
    #define PI 3.141592
#endif