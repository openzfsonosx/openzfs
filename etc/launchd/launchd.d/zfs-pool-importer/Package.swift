// swift-tools-version:5.1
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
  name: "zfs-pool-importer",
  platforms: [.macOS(.v10_15)],
  dependencies: [],
  targets: [
    .target(
      name: "zfs-pool-importer",
      dependencies: []),
  ]
)
