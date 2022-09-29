// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"

#define WM_APPEND_MESSAGE		WM_USER + 1	//For appending to the Messages CListBox
#define WM_CLEAR_CLIENTS		WM_USER + 2
#define WM_ADD_CLIENT			WM_USER + 3
#define WM_REMOVE_CLIENT		WM_USER + 4
#define WM_SERVER_DISCONNECT	WM_USER + 5

#endif //PCH_H
