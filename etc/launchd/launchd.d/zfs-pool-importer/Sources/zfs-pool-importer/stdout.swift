//
//  stdout.swift
//  

import Foundation

extension FileHandle
{
  func print(_ string: String)
  {
    write(Data(string.appending("\n").utf8))
  }
}
