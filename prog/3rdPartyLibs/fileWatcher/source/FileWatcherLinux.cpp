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

	James Wynn james@jameswynn.com
*/

#include <FileWatcher/FileWatcherLinux.h>

#if FILEWATCHER_PLATFORM == FILEWATCHER_PLATFORM_LINUX

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/inotify.h>

#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*1024)

namespace FW
{

	struct WatchStruct
	{
		WatchID mWatchID;
		String mDirName;
		FileWatchListener* mListener;		
	};

	//--------
	FileWatcherLinux::FileWatcherLinux()
	{
		mFD = inotify_init();
		if (mFD < 0)
			fprintf (stderr, "Error: %s\n", strerror(errno));
		
		mTimeOut.tv_sec = 0;
		mTimeOut.tv_usec = 0;
	   		
		FD_ZERO(&mDescriptorSet);
	}

	//--------
	FileWatcherLinux::~FileWatcherLinux()
	{
		WatchMap::iterator iter = mWatches.begin();
		WatchMap::iterator end = mWatches.end();
		for(; iter != end; ++iter)
		{
			delete iter->second;
		}
		mWatches.clear();
	}

	//--------
	WatchID FileWatcherLinux::addWatch(const String& directory, FileWatchListener* watcher, bool recursive)
	{
		int wd = inotify_add_watch (mFD, directory.c_str(), 
			IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_MOVED_FROM | IN_DELETE);
		if (wd < 0)
		{
			if(errno == ENOENT)
				throw FileNotFoundException(directory);
			else
				throw Exception(strerror(errno));

//			fprintf (stderr, "Error: %s\n", strerror(errno));
//			return -1;
		}
		
		WatchStruct* pWatch = new WatchStruct();
		pWatch->mListener = watcher;
		pWatch->mWatchID = wd;
		pWatch->mDirName = directory;
		
		mWatches.insert(std::make_pair(wd, pWatch));
	
		return wd;
	}

	//--------
	void FileWatcherLinux::removeWatch(const String& directory)
	{
		WatchMap::iterator iter = mWatches.begin();
		WatchMap::iterator end = mWatches.end();
		for(; iter != end; ++iter)
		{
			if(directory == iter->second->mDirName)
			{
				removeWatch(iter->first);
				return;
			}
		}
	}

	//--------
	void FileWatcherLinux::removeWatch(WatchID watchid)
	{
		WatchMap::iterator iter = mWatches.find(watchid);

		if(iter == mWatches.end())
			return;

		WatchStruct* watch = iter->second;
		mWatches.erase(iter);
	
		inotify_rm_watch(mFD, watchid);
		
		delete watch;
		watch = 0;
	}

	//--------
	void FileWatcherLinux::update()
	{
		FD_SET(mFD, &mDescriptorSet);

		int ret = select(mFD + 1, &mDescriptorSet, NULL, NULL, &mTimeOut);
		if(ret < 0)
		{
			perror("select");
		}
		else if(FD_ISSET(mFD, &mDescriptorSet))
		{
			ssize_t len, i = 0;
			char action[81+FILENAME_MAX] = {0};
			char buff[BUFF_SIZE] = {0};

			len = read (mFD, buff, BUFF_SIZE);
		   
			while (i < len)
			{
				struct inotify_event *pevent = (struct inotify_event *)&buff[i];

				WatchStruct* watch = mWatches[pevent->wd];
				handleAction(watch, pevent->name, pevent->mask);
				i += sizeof(struct inotify_event) + pevent->len;
			}
		}
	}

	//--------
	void FileWatcherLinux::handleAction(WatchStruct* watch, const String& filename, unsigned long action)
	{
		if(!watch->mListener)
			return;

		if(IN_CLOSE_WRITE & action)
		{
			watch->mListener->handleFileAction(watch->mWatchID, watch->mDirName, filename,
								Actions::Modified);
		}
		if(IN_MOVED_TO & action || IN_CREATE & action)
		{
			watch->mListener->handleFileAction(watch->mWatchID, watch->mDirName, filename,
								Actions::Add);
		}
		if(IN_MOVED_FROM & action || IN_DELETE & action)
		{
			watch->mListener->handleFileAction(watch->mWatchID, watch->mDirName, filename,
								Actions::Delete);
		}
	}

};//namespace FW

#endif//FILEWATCHER_PLATFORM_LINUX
