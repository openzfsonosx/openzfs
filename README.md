![img](https://github.com/zfsonfreebsd/ZoF/raw/master/zof-logo.png)

ZFS on Linux is an advanced file system and volume manager which was originally
developed for Solaris and is now maintained by the OpenZFS community. ZoF is
the work to bring FreeBSD support into the ZoL repo.

[![codecov](https://codecov.io/gh/zfsonlinux/zfs/branch/master/graph/badge.svg)](https://codecov.io/gh/zfsonlinux/zfs)
[![coverity](https://scan.coverity.com/projects/1973/badge.svg)](https://scan.coverity.com/projects/zfsonlinux-zfs)

# Official Resources

  * [ZoF GitHub Site](https://zfsonfreebsd.github.io/ZoF/)
  * [ZoL Site](http://zfsonlinux.org)
  * [ZoL Wiki](https://github.com/zfsonlinux/zfs/wiki)
  * [ZoL Mailing lists](https://github.com/zfsonlinux/zfs/wiki/Mailing-Lists)
  * [OpenZFS site](http://open-zfs.org/)

# Installation

ZoF is available in the FreeBSD ports tree as sysutils/openzfs and
sysutils/openzfs-kmod. It can be installed on FreeBSD stable/12 or later.

# Branches

  * projects/zfsbsd - stable branch used by the port
  * projects/pr-rebase-* - development branch, frequently rebased on ZoL master
  * projects/pr-rebase - squashed development branch for the ZoL PR

We frequently rebase the development branch on ZoL master to keep up with
upstream changes, creating a new branch to preserve the commit history in case
of mismerges and to avoid force pushes.

# Development

The following dependencies are required to build ZoF from source:
  * FreeBSD sources (in /usr/src or elsewhere specified by passing
    `--with-freebsd=$path` to `./configure`
  * Packages for build:
    ```
    autoconf
    automake
    autotools
    bash
    git
    gmake
    ```
  * Optional packages for build:
    ```
    python3 # or your preferred Python version
    ```
  * Optional packages for test:
    ```
    base64
    fio
    hs-ShellCheck
    ksh93
    py36-flake8 # or your preferred Python version
    shuf
    sudo
    ```
    The user for running tests must have NOPASSWD sudo permission.

To build and install:
```
# as user
git clone https://github.com/zfsonfreebsd/ZoF
cd ZoF
./autogen.sh
./configure
gmake
# as root
gmake install
```
The ZFS utilities will be installed in /usr/local/sbin/, so make sure your PATH
gets adjusted accordingly. Though not required, `WITHOUT_ZFS` is a useful build
option to avoid installing legacy zfs tools and kmod - see `src.conf(5)`.

Beware that the FreeBSD boot loader does not allow booting from root pools with
encryption active (even if it is not in use), so do not try encryption on a
pool you boot from.

# Contributing

Submit changes to the common code via a ZoL PR. Submit changes to FreeBSD
platform code by way of a PR to ZoF against the latest development branch.

# Issues

Issues can be reported via GitHub's [Issue Tracker](https://github.com/zfsonfreebsd/ZoF).

