/**
 * \file	gen_museek.cpp
 * \brief	Plugin main functions.
 */

#include <algorithm>
#include <iostream>
#include <sstream>

#include "wa_ipc.h"

#include "constants.h"
#include "logger.h"
#include "gen_museek.h"
#include "shuffler.h"
#include "track.h"
#include "utils.h"

using namespace std;


/// Global Variables
HWND        configwnd = 0;
WNDPROC     lpWndProcOld = 0;
DWORD
    WA_MENUITEM_SHUFFLE_ON_LIBRARY  = 0,
    WA_MENUITEM_SHUFFLE_ON_PLAYLIST = 0,
    WA_MENUITEM_RESCAN_LIBRARY      = 0;


// Plugin information
winampGeneralPurposePlugin plugin = {
	GPPHDR_VER,  
	PLUGIN_NAME,
	init,
	config,
	quit,
	0,           // handle to Winamp main window, loaded by winamp when this dll is loaded
	0            // hinstance to this dll, loaded by winamp when this dll is loaded
};


//! Callback Functions
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    Shuffler* shuffler = Shuffler::getInstance();
    
    // Useful:
    // IPC_PLAYLIST_MODIFIED => TODO: rescan library
    // IPC_GETAUDIOTRACK
    // IPC_SETAUDIOTRACK
    // IPC_GET_PLAYING_FILENAME
    // IPC_GET_PLAYING_TITLE
    // IPC_ISPLAYING
    // IPC_STARTPLAY
    // IPC_PLAYFILE
    // IPC_GETLISTLENGTH
    // IPC_ISFULLSTOP
    // IPC_GET_NEXT_PLITEM
    // IPC_GET_PREVIOUS_PLITEM
    // IPC_METADATA_CHANGED

    if (message == WM_WA_IPC) {
        // A new track is playing
	    if (lParam == IPC_PLAYING_FILEW)
            shuffler->onStartPlaying();

        // Stop playing
	    else if (lParam == IPC_STOPPLAYING)
            shuffler->onStopPlaying();

        // Status changed (Play/Pause)
        else if (lParam == IPC_CB_MISC && wParam == IPC_CB_MISC_TITLE) {
            int playing = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING);
            
            // Resume
            if (playing == 1)
                shuffler->onResumePlaying();

            // Pause
            else if (playing == 3)
                shuffler->onPausePlaying();

        }
    } else if (message == WM_COMMAND) {
        // Previous button pressed
        if (LOWORD(wParam) == WINAMP_BUTTON_PREV) {
			shuffler->onStopPlaying();
            shuffler->onPreviousTrack();
		}

        // Next button pressed
        else if (LOWORD(wParam) == WINAMP_BUTTON_NEXT) {
			shuffler->onStopPlaying();
            shuffler->onNextTrack();
		}

        // Toggle smart shuffle for all media library
	    else if (wParam == (WPARAM)(WA_MENUITEM_SHUFFLE_ON_LIBRARY))
            shuffler->toggleMode(ON_LIBRARY);

        // Toggle smart shuffle for current playlist
	    else if (wParam == (WPARAM)(WA_MENUITEM_SHUFFLE_ON_PLAYLIST))
            shuffler->toggleMode(ON_PLAYLIST);

        // Rescan library
        else if (wParam == (WPARAM)(WA_MENUITEM_RESCAN_LIBRARY))
            shuffler->onRescanLibrary();
    }

	return CallWindowProc(lpWndProcOld, hwnd, message, wParam, lParam);
}


/**
 * \brief Callback function called by Winamp at starting.
 *
 * \return 0 if everything is OK, 1 otherwise.
 */
int init() {
    // We need the address of the Winamp Routine
	if (IsWindowUnicode(plugin.hwndParent))
        lpWndProcOld = (WNDPROC)SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG)WndProc);
	else
		lpWndProcOld = (WNDPROC)SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG)WndProc);

    // Load map, config and add menu entries
    Shuffler* shuffler = Shuffler::getInstance();
    shuffler->loadConfig();
    shuffler->loadMap();
    shuffler->createMenuEntries();

    return 0;
}


/**
 * \brief Callback function called by Winamp when the user requests for configuring the plugin. Do *not* change its signature !
 *
 * \return Nothing.
 */
void config() {
}


/**
 * \brief Callback function called by Winamp at exiting. Do *not* change its signature !
 *
 * \return Nothing.
 */
void quit() {
}
 

/**
 * \brief Export function called by Winamp. Do *not* change it at all !
 *
 * \return The plugin info.
 */
extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin() {
    // We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file.
    return &plugin;
}
