all: build

SWIFT = /usr/bin/swift
ALLSOURCEFILES = $(wildcard Sources/*/*.swift)
BUILDDIR := $(shell swift build --show-bin-path)

launchdexecdir = ${libexecdir}/zfs/launchd.d

.PHONY: clean build install

clean:
	$(SWIFT) package reset; $(RM) zfs-pool-importer

build: $(BUILDDIR)/zfs-pool-importer

$(BUILDDIR)/zfs-pool-importer: $(ALLSOURCEFILES)
	$(SWIFT) build

install: build
	/bin/cp $(BUILDDIR)/zfs-pool-importer $(launchdexecdir)/zfs-pool-importer
