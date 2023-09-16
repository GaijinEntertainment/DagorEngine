/**
	Implementation header file for Linux based on inotify.

	@author James Wynn
	@date 4/15/2009

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
#ifndef _FW_FILEWATCHERLINUX_H_
#define _FW_FILEWATCHERLINUX_H_
#pragma once

#include "FileWatcherImpl.h"

#if FILEWATCHER_PLATFORM == FILEWATCHER_PLATFORM_LINUX

#include <map>
#include <sys/types.h>

namespace FW
{
	/// Implementation for Linux based on inotify.
	/// @class FileWatcherLinux
	class FileWatcherLinux : public FileWatcherImpl
	{
	public:
		/// type for a map from WatchID to WatchStruct pointer
		typedef std::map<WatchID, WatchStruct*> WatchMap;

	public:
		///
		///
		FileWatcherLinux();

		///
		///
		virtual ~FileWatcherLinux();

		/// Add a directory watch
		/// @exception FileNotFoundException Thrown when the requested directory does not exist
		WatchID addWatch(const String& directory, FileWatchListener* watcher, bool recursive);

		/// Remove a directory watch. This is a brute force lazy search O(nlogn).
		void removeWatch(const String& directory);

		/// Remove a directory watch. This is a map lookup O(logn).
		void removeWatch(WatchID watchid);

		/// Updates the watcher. Must be called often.
		void update();

		/// Handles the action
		void handleAction(WatchStruct* watch, const String& filename, unsigned long action);

	private:
		/// Map of WatchID to WatchStruct pointers
		WatchMap mWatches;
		/// The last watchid
		WatchID mLastWatchID;
		/// inotify file descriptor
		int mFD;
		/// time out data
		struct timeval mTimeOut;
		/// File descriptor set
		fd_set mDescriptorSet;

	};//end FileWatcherLinux

};//namespace FW

#endif//FILEWATCHER_PLATFORM_LINUX

#endif//_FW_FILEWATCHERLINUX_H_
