import Foundation

let zpoolImportCookie="/var/run/org.openzfsonosx.zpool-import-all.didRun"
let invariantDisksCookie="/var/run/disk/invariant.idle"
let timeout = TimeInterval(60)

let formatter = DateFormatter()
formatter.locale = Locale(identifier: "en_US")
formatter.dateStyle = .medium
formatter.timeStyle = .long

let stdout = FileHandle.standardOutput

let command = ProcessInfo.processInfo.arguments.first?.split(separator: "/").last ?? "zfs_pool_importer"
let arguments = ProcessInfo.processInfo.arguments.dropFirst()
let sbin = arguments.first.flatMap {
  arg in // elementary (and inflexible) parsing of a single possible command line option
  var dir = ObjCBool(false)
  guard arg.hasPrefix("--sbindir"),
        let rawPath = arg.split(separator: "=").last.map(String.init),
        FileManager.default.fileExists(atPath: rawPath, isDirectory: &dir),
        dir.boolValue == true
    else { return nil }
  return URL(fileURLWithPath: rawPath, isDirectory: true).path.appending("/")
} ?? "/usr/local/zfs/bin/"

let zpool = sbin + "zpool"

stdout.print("+\(command)")
stdout.print(formatter.string(from: Date()))

formatter.dateStyle = .none
formatter.timeStyle = .medium
stdout.print("Running system_profiler to ensure the device tree is populated...")
launch(command: "/usr/sbin/system_profiler",
       arguments: ["SPParallelATADataType", "SPCardReaderDataType", "SPFibreChannelDataType", "SPFireWireDataType",
                   "SPHardwareRAIDDataType", "SPNetworkDataType", "SPPCIDataType", "SPParallelSCSIDataType",
                   "SPSASDataType", "SPSerialATADataType", "SPStorageDataType", "SPThunderboltDataType",
                   "SPUSBDataType", "SPNetworkVolumeDataType"],
       output: .nullDevice, error: .nullDevice)
launch(command: "/bin/sync")

stdout.print("\(formatter.string(from: Date())): waiting until file \(invariantDisksCookie) is found...")

let fm = FileManager.default
let beganWaiting = Date()
var waited = TimeInterval.zero
var found = fm.fileExists(atPath: invariantDisksCookie)
while !found && waited < timeout
{
  Thread.sleep(forTimeInterval: 0.1)
  waited = Date().timeIntervalSince(beganWaiting)
  found = fm.fileExists(atPath: invariantDisksCookie)
}
stdout.print("\(invariantDisksCookie) was\(found ? "" : "n't") found in \(String(format: "%.2f", waited)) seconds")

Thread.sleep(forTimeInterval: 10)
stdout.print("\(formatter.string(from: Date())): running zpool import -a")

let code = launch(command: zpool, arguments: ["import", "-a", "-d", "/var/run/disk/by-id"])

stdout.print("\(formatter.string(from: Date())): zpool import returned with exit code \(code)")

let mod = [FileAttributeKey.modificationDate: NSDate()]
do {
  try fm.setAttributes(mod, ofItemAtPath: zpoolImportCookie)
  stdout.print("Updated modification time at path \(zpoolImportCookie)")
}
catch {
  if fm.createFile(atPath: zpoolImportCookie, contents: nil, attributes: mod)
  { stdout.print("Created \(zpoolImportCookie)") }
  else
  { FileHandle.standardError.print("Failed to create \(zpoolImportCookie)") }
}

formatter.dateStyle = .medium
formatter.timeStyle = .long
stdout.print(formatter.string(from: Date()))
stdout.print("-\(command)")

exit(code)
