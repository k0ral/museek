#ifndef GEN_MUSEEK_H
	#define GEN_MUSEEK_H

    /**
     * \file gen_museek.h
     * \brief Plugin main headers
     */

	#include <windows.h>

	/**
     * \brief Plugin information structure.
     *
     * Used by Winamp, do *not* change it !
     */
	typedef struct {
		int version;                    ///< Version of the winampGeneralPurposePlugin (GPP) structure
		char *description;              ///< Name of the plugin
        int (*init)();                  ///< Executed on init event
		void (*config)();               ///< Executed on config event
		void (*quit)();                 ///< Executed on quit event
		HWND hwndParent;                ///< hwnd of the Winamp client main window (stored by Winamp when dll is loaded). */
		HINSTANCE hDllInstance;         ///< hinstance of this plugin DLL (stored by Winamp when dll is loaded). */
	} winampGeneralPurposePlugin;


	// Prototypes
    LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

	int  init();
	void config();
	void quit();
#endif