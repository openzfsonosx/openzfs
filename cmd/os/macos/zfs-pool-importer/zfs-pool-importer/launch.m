#import <Foundation/NSFileManager.h>
#import <Foundation/NSFileHandle.h>
#import <Foundation/NSTask.h>
#import <Foundation/NSString.h>
#import <Foundation/NSArray.h>
#import <Foundation/NSData.h>

#include <sysexits.h>

#include "launch.h"
#include "stdout.h"

int launch(NSString* _Nonnull command, NSArray<NSString*>* _Nonnull arguments,
           NSFileHandle* _Nonnull output, NSFileHandle* _Nonnull error)
{
  @autoreleasepool {
    NSFileManager *fm = [NSFileManager defaultManager];
    BOOL dir;
    if ([fm fileExistsAtPath: command isDirectory: &dir] == false)
    {
      NSString* message = [[@"Path " stringByAppendingString: command]
                           stringByAppendingString: @" does not exist"];
      writeMessage(message, error);
      return EX_UNAVAILABLE;
    }

    if (dir == true)
    {
      NSString* message = [[@"Path " stringByAppendingString: command]
                           stringByAppendingString: @" represents a directory"];
      writeMessage(message, error);
      return EX_USAGE;
    }

    if ([fm isExecutableFileAtPath: command] == false)
    {
      NSString* message = [[@"File " stringByAppendingString: command]
                           stringByAppendingString: @" is not executable"];
      writeMessage(message, error);
      return EX_NOPERM;
    }

    NSTask* task = [[NSTask alloc] init];
    task.launchPath = command;
    task.arguments = arguments;
    task.standardOutput = output;
    task.standardError = error;

    NSError* error = nil;
    BOOL launched = [task launchAndReturnError: &error];
    if (launched == false || error != nil)
    {
      return EX_SOFTWARE;
    }

    [task waitUntilExit];
    return task.terminationStatus;
  }
}
