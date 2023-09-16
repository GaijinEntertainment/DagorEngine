/**
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

#include <FileWatcher/FileWatcher.h>
#include <FileWatcher/FileWatcherImpl.h>

#if FILEWATCHER_PLATFORM == FILEWATCHER_PLATFORM_WIN32
#	include <FileWatcher/FileWatcherWin32.h>
#	define FILEWATCHER_IMPL FileWatcherWin32
#elif FILEWATCHER_PLATFORM == FILEWATCHER_PLATFORM_KQUEUE
#	include <FileWatcher/FileWatcherOSX.h>
#	define FILEWATCHER_IMPL FileWatcherOSX
#elif FILEWATCHER_PLATFORM == FILEWATCHER_PLATFORM_LINUX
#	include <FileWatcher/FileWatcherLinux.h>
#	define FILEWATCHER_IMPL FileWatcherLinux
#endif

namespace FW
{

	//--------
	FileWatcher::FileWatcher()
	{
		mImpl = new FILEWATCHER_IMPL();
	}

	//--------
	FileWatcher::~FileWatcher()
	{
		delete mImpl;
		mImpl = 0;
	}

	//--------
	WatchID FileWatcher::addWatch(const String& directory, FileWatchListener* watcher)
	{
		return mImpl->addWatch(directory, watcher, false);
	}

	//--------
	WatchID FileWatcher::addWatch(const String& directory, FileWatchListener* watcher, bool recursive)
	{
		return mImpl->addWatch(directory, watcher, recursive);
	}

	//--------
	void FileWatcher::removeWatch(const String& directory)
	{
		mImpl->removeWatch(directory);
	}

	//--------
	void FileWatcher::removeWatch(WatchID watchid)
	{
		mImpl->removeWatch(watchid);
	}

	//--------
	void FileWatcher::update()
	{
		mImpl->update();
	}

};//namespace FW
