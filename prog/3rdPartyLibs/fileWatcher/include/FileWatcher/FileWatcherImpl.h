/**
	Basic interface for the FileWatcher backend.

	@author James Wynn
	@date 5/11/2009

	Copyright (c) 2009 James Wynn (james@jameswynn.com)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/
#ifndef _FW_FILEWATCHERIMPL_H_
#define _FW_FILEWATCHERIMPL_H_
#pragma once

#include "FileWatcher.h"

#define FILEWATCHER_PLATFORM_WIN32 1
#define FILEWATCHER_PLATFORM_LINUX 2
#define FILEWATCHER_PLATFORM_KQUEUE 3

#if defined(_WIN32)
#	define FILEWATCHER_PLATFORM FILEWATCHER_PLATFORM_WIN32
#elif defined(__APPLE_CC__) || defined(BSD)
#	define FILEWATCHER_PLATFORM FILEWATCHER_PLATFORM_KQUEUE
#elif defined(__linux__)
#	define FILEWATCHER_PLATFORM FILEWATCHER_PLATFORM_LINUX
#endif

namespace FW
{
	struct WatchStruct;

	class FileWatcherImpl
	{
	public:
		///
		///
		FileWatcherImpl() {}

		///
		///
		virtual ~FileWatcherImpl() {}

		/// Add a directory watch
		/// @exception FileNotFoundException Thrown when the requested directory does not exist
		virtual WatchID addWatch(const String& directory, FileWatchListener* watcher, bool recursive) = 0;

		/// Remove a directory watch. This is a brute force lazy search O(nlogn).
		virtual void removeWatch(const String& directory) = 0;

		/// Remove a directory watch. This is a map lookup O(logn).
		virtual void removeWatch(WatchID watchid) = 0;

		/// Updates the watcher. Must be called often.
		virtual void update() = 0;

		/// Handles the action
		virtual void handleAction(WatchStruct* watch, const String& filename, unsigned long action) = 0;

	};//end FileWatcherImpl
};//namespace FW

#endif//_FW_FILEWATCHERIMPL_H_
