#import <Foundation/NSFileHandle.h>
#import <Foundation/NSString.h>

#include "stdout.h"

void writeMessage(NSString* _Nonnull message, NSFileHandle* _Nonnull output)
{
  @autoreleasepool {
    [output writeData: [[message stringByAppendingString: @"\n"] dataUsingEncoding: NSUTF8StringEncoding]];
  }
}
