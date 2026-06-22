#include "daScript/misc/platform.h"

#include "dasStdDlg.h"

#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

namespace das {

    vector<string> split ( const char * str, const char * delim );

	void StdDlgInit() {
	}

	bool GetOkCancelFromUser(const char * _caption, const char * _body) {
        NSString* body = [NSString stringWithCString:_body encoding:NSISOLatin1StringEncoding];
        NSString* caption = [NSString stringWithCString:_caption encoding:NSISOLatin1StringEncoding];
        NSAlert* alert = [[[NSAlert alloc] init] autorelease];
        alert.messageText = caption;
        alert.informativeText = [NSString stringWithFormat:@"%@", body];
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Cancel"];
        int idx = [alert runModal];
        return idx == NSAlertFirstButtonReturn;
	}

	bool GetOkFromUser(const char * caption, const char * body) {
        NSAlert* alert = [[NSAlert alloc]init];
        [alert setMessageText:[NSString stringWithCString:caption encoding:NSISOLatin1StringEncoding]];
        [alert setInformativeText:[NSString stringWithCString:body encoding:NSISOLatin1StringEncoding]];
        [alert runModal];
        [[NSRunLoop mainRunLoop] runMode:NSModalPanelRunLoopMode beforeDate:[NSDate distantFuture]];
        [alert release];
	    return true;
	}

	string DLG_INITIAL_PATH;
    static NSMutableArray* BuildFilter( const char* filter ) {
        NSMutableArray* filt = [[NSMutableArray alloc] init];
        vector<string> filters;
		if ( filter ) {
		    filters = split(filter, "|");
        }
        if ( filters.empty() ) {
            filters.push_back("*");
        }
        for ( unsigned int i = 0; i < filters.size(); ++i ) {
            string filter = filters[i];
            NSString* str = [NSString stringWithCString:filter.c_str() encoding:NSISOLatin1StringEncoding];
            [filt addObject:[str retain]];
            [str release];
        }
        return filt;
    }

	string GetSaveFileFromUser ( const char * initialFileName, const char * initialPath, const char * filter ) {
	    DLG_INITIAL_PATH = initialPath ? initialPath : "";
        NSMutableArray* filter_obj = BuildFilter( filter );
        NSSavePanel* openDlg = [NSSavePanel savePanel];
        [openDlg setAllowedFileTypes:filter_obj];
        [openDlg setShowsHiddenFiles:FALSE];
        if  ( initialPath ) {
            [[NSFileManager defaultManager] changeCurrentDirectoryPath:[NSString stringWithCString:initialPath encoding:NSISOLatin1StringEncoding]];
        }
        if ( initialFileName ) {
            [openDlg setNameFieldStringValue:[NSString stringWithCString:initialFileName encoding:NSISOLatin1StringEncoding]];
        }
		__block int res = NSModalResponseCancel;
		res = [openDlg runModal];
        // dispatch_sync(dispatch_get_main_queue(), ^{
        //    res = [openDlg runModal];
        // });
        char c_name[1024];
        c_name[0]=0;
        if ( res == NSModalResponseOK ) {
            NSString* fileName = [[openDlg URL] path];
            [fileName getCString:c_name maxLength:1024 encoding:NSASCIIStringEncoding ];
        }
        [filter_obj release];
        return c_name;
	}

	string GetOpenFileFromUser ( const char * initialPath, const char * filter ) {
	    DLG_INITIAL_PATH = initialPath ? initialPath : "";
        NSMutableArray* filter_obj = BuildFilter( filter );
        NSOpenPanel* openDlg = [NSOpenPanel openPanel];
        [openDlg setCanChooseFiles:YES];
        [openDlg setCanChooseDirectories:NO];
        [openDlg setAllowedFileTypes: filter_obj];
        [openDlg setShowsHiddenFiles:FALSE];
        //[openDlg makeKeyAndOrderFront:openDlg];
        [openDlg setCanHide:NO];
        if  ( initialPath ) {
            [[NSFileManager defaultManager] changeCurrentDirectoryPath:[NSString stringWithCString:initialPath encoding:NSISOLatin1StringEncoding]];
        }
        int res = [openDlg runModal];
        char c_name[1024];
        c_name[0]=0;
        if ( res == NSModalResponseOK ) {
            NSString* fileName = [[openDlg URL] path];
            [fileName getCString:c_name maxLength:1024 encoding:NSASCIIStringEncoding ];
        }
        [filter_obj release];
        return c_name;
	}
}

