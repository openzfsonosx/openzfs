//
//  launch.swift
//

import Foundation

@discardableResult
func launch(command: String, arguments: [String] = [], environment: [String: String] = [:],
            output: FileHandle = .standardOutput, error: FileHandle = .standardError) -> Int32
{
  var dir = ObjCBool(false)
  guard FileManager.default.fileExists(atPath: command, isDirectory: &dir)
  else {
    FileHandle.standardError.print("Path \"\(command)\" does not exist")
    return EX_UNAVAILABLE
  }

  if dir.boolValue == true
  {
    FileHandle.standardError.print("Path \"\(command)\" represents a directory")
    return EX_USAGE
  }

  guard FileManager.default.isExecutableFile(atPath: command)
  else {
    FileHandle.standardError.print("File \"\(command)\" is not executable")
    return EX_NOPERM
  }

  let task = Process()
  task.launchPath = command
  task.arguments = arguments
  task.standardOutput = output
  task.standardError = error

  if !environment.isEmpty
  {
    let env = ProcessInfo.processInfo.environment
    task.environment = env.merging(environment, uniquingKeysWith: { $1 })
  }

  task.launch()
  task.waitUntilExit()

  return task.terminationStatus
}
