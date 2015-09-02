How to use Smart Play for Winamp

# Introduction #

This page describes how you can use Smart Play for the Winamp Media Player. The Smart Play plugin builds the music map from the Winamp media library so ensure that you have a music library in Winamp if you want to use the plugin. Also, the plugin works best when your library is well tagged (as the coordinates of the song are calculated using tags like artist,album and title).

Use the installer provided to install the plugin. The dependencies for the plugin are:
  * Visual C++ 2008 Redistributable (9.0.30729.17) - this package is included in the installer. If it is not present on your computer, the installer will prompt you to install it.
  * .NET framework (>= 2.0 SP2). If .NET framework is not present on your computer, the installation will abort, prompting you to install .NET. You can do so from the official Microsoft website.

Once the plugin is installed, you can start Winamp!

## Known Issue (at Startup) ##
On startup, if the map of music is not present Winamp will throw a dialogue asking you to build the map. It is highly likely that this dialogue will be hidden behind the winamp window.  Try to resize the winamp window, move it to the edge of the screen, and you will see the dialogue box. This is a **known bug** and **will be fixed** in the next version.

# Using Smart Play #
Once the map is generated, you will be presented with a dialogue box which tells you that the map has been generated. (There is currently no progress bar which indicates the progress of the map building progress. It will be implemented in future versions).

Now you have two ways of using smart shuffle:
  * Select a seed, that is, select a song which you want to be the starting point for the smart shuffling algorithm. All subsequent similar (and dissimilar) songs will be calculated with this song as the reference point. When the song starts playing, enable smart shuffle using Play -> Smart Play. Note that this will start the seed song again from the beginning. Please note that we need the plugin to behave like this.
  * Just select Play -> Smart Play while no song is playing. A random song will be selected as seed for you.

# Updating the map #
We currently do not track whether the map is outdated with respect to the user's media library. We are planning to implement this in future versions. For now, if you want to regenerate the map, you can do it via Play -> Rescan library for smart shuffle.