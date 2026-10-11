#include "daScript/misc/platform.h"

#include "dasStdDlgTray.h"

#import <Cocoa/Cocoa.h>
#include <ApplicationServices/ApplicationServices.h>

namespace das {
    class OsxTray;
}

@interface DasTrayTarget : NSObject
@property (nonatomic, assign) das::OsxTray * tray;
- (void)statusItemClicked:(id)sender;
- (void)menuItemClicked:(id)sender;
@end

namespace das {

    static bool isMouseEvent(NSEvent * ev) {
        if ( !ev ) return false;
        switch ( ev.type ) {
            case NSEventTypeLeftMouseDown:
            case NSEventTypeLeftMouseUp:
            case NSEventTypeRightMouseDown:
            case NSEventTypeRightMouseUp:
            case NSEventTypeOtherMouseDown:
            case NSEventTypeOtherMouseUp:
                return true;
            default:
                return false;
        }
    }

    class OsxTray final : public TrayBackend {
    public:
        NSStatusItem * item = nil;
        NSMenu * menu = nil;
        DasTrayTarget * target = nil;
        vector<TrayEvent> events;

        virtual ~OsxTray() override {
            @autoreleasepool {
                if ( item ) {
                    item.button.target = nil;
                    item.button.action = nil;
                    [[NSStatusBar systemStatusBar] removeStatusItem:item];
                    [item release];
                }
                if ( menu ) {
                    for ( NSMenuItem * mi in menu.itemArray ) mi.target = nil;
                    [menu release];
                }
                target.tray = nullptr;
                [target release];
            }
        }

        virtual bool create(const char * tooltip) override {
            @autoreleasepool {
                [NSApplication sharedApplication];
                if ( NSApp.activationPolicy == NSApplicationActivationPolicyProhibited ) {
                    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
                    [NSApp finishLaunching];
                }
                item = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
                if ( !item || !item.button ) return false;
                target = [[DasTrayTarget alloc] init];
                target.tray = this;
                item.button.toolTip = [NSString stringWithUTF8String:tooltip];
                item.button.target = target;
                item.button.action = @selector(statusItemClicked:);
                [item.button sendActionOn:(NSEventMaskLeftMouseUp | NSEventMaskRightMouseUp)];
                menu = [[NSMenu alloc] initWithTitle:@""];
                menu.autoenablesItems = NO;
                return true;
            }
        }

        virtual void setIcon(const uint8_t * rgba8, int32_t width, int32_t height) override {
            @autoreleasepool {
                NSBitmapImageRep * rep = [[[NSBitmapImageRep alloc]
                    initWithBitmapDataPlanes:NULL pixelsWide:width pixelsHigh:height
                    bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO
                    colorSpaceName:NSCalibratedRGBColorSpace
                    bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
                    bytesPerRow:width * 4 bitsPerPixel:32] autorelease];
                if ( !rep || !rep.bitmapData ) return;
                memcpy(rep.bitmapData, rgba8, size_t(width) * size_t(height) * 4);
                NSBitmapImageRep * tagged = [rep bitmapImageRepByRetaggingWithColorSpace:[NSColorSpace sRGBColorSpace]];
                if ( tagged ) rep = tagged;
                rep.size = NSMakeSize(18, 18);
                NSImage * image = [[[NSImage alloc] initWithSize:NSMakeSize(18, 18)] autorelease];
                [image addRepresentation:rep];
                item.button.image = image;
            }
        }

        virtual void setTooltip(const char * text) override {
            @autoreleasepool {
                item.button.toolTip = [NSString stringWithUTF8String:text];
            }
        }

        virtual void setMenu(const vector<TrayMenuEntry> & entries) override {
            @autoreleasepool {
                for ( NSMenuItem * mi in menu.itemArray ) mi.target = nil;
                [menu removeAllItems];
                for ( const auto & e : entries ) {
                    if ( e.separator ) {
                        [menu addItem:[NSMenuItem separatorItem]];
                        continue;
                    }
                    NSMenuItem * mi = [[[NSMenuItem alloc]
                        initWithTitle:[NSString stringWithUTF8String:e.label.c_str()]
                        action:@selector(menuItemClicked:) keyEquivalent:@""] autorelease];
                    mi.target = target;
                    mi.tag = e.id;
                    mi.enabled = e.enabled;
                    mi.state = e.checked ? NSControlStateValueOn : NSControlStateValueOff;
                    [menu addItem:mi];
                }
            }
        }

        virtual void poll(vector<TrayEvent> & out) override {
            @autoreleasepool {
                for ( ;; ) {
                    NSEvent * ev = [NSApp nextEventMatchingMask:NSEventMaskAny
                        untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
                    if ( !ev ) break;
                    [NSApp sendEvent:ev];
                }
                [NSApp updateWindows];
            }
            out.swap(events);
            events.clear();
        }

        virtual bool notify(const char * title, const char * body) override {
            @autoreleasepool {
                NSTask * task = [[[NSTask alloc] init] autorelease];
                task.launchPath = @"/usr/bin/osascript";
                NSString * script = [NSString stringWithFormat:@"display notification \"%@\" with title \"%@\"",
                    escapeForAppleScript(body), escapeForAppleScript(title)];
                task.arguments = @[@"-e", script];
                task.standardOutput = [NSFileHandle fileHandleWithNullDevice];
                task.standardError = [NSFileHandle fileHandleWithNullDevice];
                @try {
                    [task launch];
                } @catch ( NSException * ) {
                    return false;
                }
                [task waitUntilExit];
                return task.terminationStatus == 0;
            }
        }

        static NSString * escapeForAppleScript(const char * text) {
            NSString * s = [NSString stringWithUTF8String:text];
            if ( !s ) s = @"";
            s = [s stringByReplacingOccurrencesOfString:@"\\" withString:@"\\\\"];
            return [s stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
        }

        void onStatusItemClicked() {
            NSEvent * ev = NSApp.currentEvent;
            bool mouse = isMouseEvent(ev);
            NSPoint p = NSMakePoint(0, 0);
            if ( mouse ) {
                p = [NSEvent mouseLocation];
                NSArray<NSScreen *> * screens = [NSScreen screens];
                if ( screens.count > 0 ) p.y = screens[0].frame.size.height - p.y;
            }
            bool rightClick = mouse && (ev.type == NSEventTypeRightMouseUp || ev.type == NSEventTypeRightMouseDown
                || (ev.modifierFlags & NSEventModifierFlagControl));
            if ( rightClick ) {
                events.push_back(TrayEvent{TrayEventKind::right_click, 0, int32_t(p.x), int32_t(p.y)});
                if ( menu.numberOfItems > 0 ) {
                    item.menu = menu;
                    [item.button performClick:nil];
                    item.menu = nil;
                }
            } else {
                TrayEventKind kind = (mouse && ev.clickCount >= 2) ? TrayEventKind::double_click : TrayEventKind::click;
                events.push_back(TrayEvent{kind, 0, int32_t(p.x), int32_t(p.y)});
            }
        }

        void onMenuItemClicked(int32_t id) {
            events.push_back(TrayEvent{TrayEventKind::menu, id, 0, 0});
        }
    };

    bool TrayPlatformAvailable() {
        if ( ![NSThread isMainThread] ) return false;
        CFDictionaryRef session = CGSessionCopyCurrentDictionary();
        if ( !session ) return false;
        CFRelease(session);
        return true;
    }

    TrayBackend * TrayPlatformCreate() {
        if ( !TrayPlatformAvailable() ) return nullptr;
        return new OsxTray();
    }
}

@implementation DasTrayTarget
- (void)statusItemClicked:(id)sender {
    (void) sender;
    if ( self.tray ) self.tray->onStatusItemClicked();
}
- (void)menuItemClicked:(id)sender {
    if ( self.tray ) self.tray->onMenuItemClicked(int32_t([(NSMenuItem *) sender tag]));
}
@end
