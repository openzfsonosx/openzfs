#import <Foundation/NSString.h>
#import <Foundation/NSFileHandle.h>
#import <Foundation/NSFileManager.h>
#import <Foundation/NSThread.h>

#import <Foundation/NSDate.h>
#import <Foundation/NSLocale.h>
#import <Foundation/NSDateFormatter.h>

#include <assert.h>
#include <sysexits.h>

#include "launch.h"
#include "stdout.h"

NSString* zpoolImportCookie = @"/var/run/org.openzfsonosx.zpool-import-all.didRun";
NSString* invariantDisksCookie = @"/var/run/disk/invariant.idle";

NSTimeInterval disksTimeout = 60;

int main(int argc, const char* argv[])
{
  @autoreleasepool {
    if (argc != 2)
    {
      writeMessage(@"This command expects 1 argument", [NSFileHandle fileHandleWithStandardError]);
      return EX_USAGE;
    }

    NSString* command = [[NSString stringWithCString: argv[0] encoding: [NSString defaultCStringEncoding]] lastPathComponent];
    NSString* sbinPath = [NSString stringWithCString: argv[1] encoding: [NSString defaultCStringEncoding]];
    NSString* zpool = nil;
    if ([sbinPath hasPrefix: @"--sbindir"])
    {
      NSString* rawPath = [[sbinPath componentsSeparatedByString: @"="] lastObject];
      BOOL dir = false;
      if ([[NSFileManager defaultManager] fileExistsAtPath: rawPath isDirectory: &dir] == false)
      {
        if (dir == true)
        {
          zpool = [[[NSURL fileURLWithPath: rawPath isDirectory: true] path] stringByAppendingString: @"/zpool"];
        }
      }
    }
    if (zpool == nil)
    {
      zpool = @"/usr/local/zfs/bin/zpool";
    }
    assert(zpool != nil);

    NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
    formatter.locale = [[NSLocale alloc] initWithLocaleIdentifier: @"en_US"];
    formatter.dateStyle = NSDateFormatterMediumStyle;
    formatter.timeStyle = NSDateFormatterLongStyle;

    if ([[NSFileManager defaultManager] isExecutableFileAtPath: zpool] == false)
    {
      NSString* message = [command stringByAppendingString: @" requires the OpenZFS tool \"zpool\""];
      writeMessage(message, [NSFileHandle fileHandleWithStandardError]);
      return EX_CONFIG;
    }

    writeMessage([@"+" stringByAppendingString: command], [NSFileHandle fileHandleWithStandardOutput]);
    writeMessage([formatter stringFromDate: [NSDate date]], [NSFileHandle fileHandleWithStandardOutput]);

    formatter.dateStyle = NSDateFormatterNoStyle;
    formatter.timeStyle = NSDateFormatterMediumStyle;

    writeMessage(@"Running system_profiler to ensure the device tree is populated...", [NSFileHandle fileHandleWithStandardOutput]);
    launch(@"/usr/sbin/system_profiler",
           [NSArray arrayWithObjects: @"SPParallelATADataType", @"SPCardReaderDataType", @"SPFibreChannelDataType",
                                      @"SPFireWireDataType", @"SPHardwareRAIDDataType", @"SPNetworkDataType",
                                      @"SPPCIDataType", @"SPParallelSCSIDataType", @"SPSASDataType",
                                      @"SPSerialATADataType", @"SPStorageDataType", @"SPThunderboltDataType",
                                      @"SPUSBDataType", @"SPNetworkVolumeDataType", nil],
           [NSFileHandle fileHandleWithNullDevice], [NSFileHandle fileHandleWithNullDevice]);
    launch(@"/bin/sync", [NSArray array], [NSFileHandle fileHandleWithNullDevice], [NSFileHandle fileHandleWithNullDevice]);

    writeMessage([[[[formatter stringFromDate: [NSDate date]]
                    stringByAppendingString: @": waiting until file "]
                   stringByAppendingString: invariantDisksCookie]
                  stringByAppendingString: @" is found..."],
                 [NSFileHandle fileHandleWithStandardOutput]);

    NSDate* beganWaiting = [NSDate date];
    NSTimeInterval waited = 0;
    NSFileManager* fm = [NSFileManager defaultManager];
    BOOL found = [fm fileExistsAtPath: invariantDisksCookie];
    while (!found && waited < disksTimeout)
    {
      [NSThread sleepForTimeInterval: 0.1];
      waited = [[NSDate date] timeIntervalSinceDate: beganWaiting];
      found = [fm fileExistsAtPath: invariantDisksCookie];
    }
    if (found)
    {
      writeMessage([[invariantDisksCookie stringByAppendingString: @" was found in "]
                    stringByAppendingString: [NSString stringWithFormat: @"%.2fs", waited]],
                   [NSFileHandle fileHandleWithStandardOutput]);
    }
    else
    {
      writeMessage([[invariantDisksCookie stringByAppendingString: @" was not found in "]
                    stringByAppendingString: [NSString stringWithFormat: @"%.2fs", waited]],
                   [NSFileHandle fileHandleWithStandardOutput]);
    }

    [NSThread sleepForTimeInterval: 10.0];
    writeMessage([[formatter stringFromDate: [NSDate date]] stringByAppendingString: @": running zpool import -a"],
                 [NSFileHandle fileHandleWithStandardOutput]);

    int code = launch(zpool,
                      [NSArray arrayWithObjects: @"import", @"-a", @"-d", @"/var/run/disk/by-id", nil],
                      [NSFileHandle fileHandleWithStandardOutput], [NSFileHandle fileHandleWithStandardOutput]);
    writeMessage([[[formatter stringFromDate: [NSDate date]]
                   stringByAppendingString: @": zpool import returned with exit code "]
                  stringByAppendingString: [NSString stringWithFormat: @"%d", code]],
                 [NSFileHandle fileHandleWithStandardOutput]);

    NSError* err = nil;
    NSDictionary* attr = [[NSDictionary alloc] initWithObjectsAndKeys: NSFileModificationDate, [NSDate date], nil];
    [fm setAttributes: attr ofItemAtPath: zpoolImportCookie error: &err];
    if (err == nil)
    {
      writeMessage([@"Updated modification time of " stringByAppendingString: zpoolImportCookie],
                   [NSFileHandle fileHandleWithStandardOutput]);
    }
    else
    {
      if ([fm createFileAtPath: zpoolImportCookie contents: nil attributes: attr])
      {
        writeMessage([@"Create file " stringByAppendingString: zpoolImportCookie],
                     [NSFileHandle fileHandleWithStandardOutput]);
      }
      else
      {
        writeMessage([@"Failed to create " stringByAppendingString: zpoolImportCookie],
                     [NSFileHandle fileHandleWithStandardOutput]);
      }
    }

    formatter.dateStyle = NSDateFormatterMediumStyle;
    formatter.timeStyle = NSDateFormatterLongStyle;
    writeMessage([formatter stringFromDate: [NSDate date]], [NSFileHandle fileHandleWithStandardOutput]);
    writeMessage([@"-" stringByAppendingString: command], [NSFileHandle fileHandleWithStandardOutput]);

    return code;
  }
}
