#!/bin/bash

BUILDDIR=$(dirname "$0")
pushd "$BUILDDIR" >/dev/null
BUILDDIR=$(pwd)
popd >/dev/null

cd "$BUILDDIR"

prompt() {
  echo "$1"
  if [ "$FORCE_INSTALL" != "1" ]; then
    read -p "Enter [Y]es to continue: " v
    if [ "$v" != "Y" ] && [ "$v" != "y" ]; then
      exit 1
    fi
  fi
}

updaterepo() {
  if [ ! -d "$2" ]; then
    git clone "$1" -b "$3" --depth=1 "$2" || exit 1
  fi
  pushd "$2" >/dev/null
  git pull
  if [ "$2" != "UDK" ]; then
    sym=$(find . -not -type d -exec file "{}" ";" | grep CRLF)
    if [ "${sym}" != "" ]; then
      echo "Repository $1 named $2 contains CRLF line endings"
      exit 1
    fi
  fi
  popd >/dev/null
}

package() {
  if [ ! -d "$1" ]; then
    echo "Missing package directory"
    exit 1
  fi

  local ver=$(cat Include/AppleSupportPkgVersion.h | grep APPLE_SUPPORT_VERSION | cut -f4 -d' ' | cut -f2 -d'"' | grep -E '^[0-9.]+$')
  if [ "$ver" = "" ]; then
    echo "Invalid version $ver"
  fi

  pushd "$1" || exit 1
  rm -rf tmp || exit 1
  mkdir -p tmp/Drivers || exit 1
  mkdir -p tmp/Tools   || exit 1
  cp ApfsDriverLoader.efi tmp/Drivers/  || exit 1
  cp FwRuntimeServices.efi tmp/Drivers/ || exit 1
  cp UsbKbDxe.efi tmp/Drivers/          || exit 1
  cp VBoxHfs.efi tmp/Drivers/           || exit 1
  cp VerifyMsrE2.efi tmp/Tools/         || exit 1
  pushd tmp || exit 1
  zip -qry -FS ../"AppleSupport-${ver}-${2}.zip" * || exit 1
  popd || exit 1
  rm -rf tmp || exit 1
  popd || exit 1
}

if [ "$BUILDDIR" != "$(printf "%s\n" $BUILDDIR)" ]; then
  echo "EDK2 build system may still fail to support directories with spaces!"
  exit 1
fi

if [ "$(which clang)" = "" ] || [ "$(which git)" = "" ] || [ "$(clang -v 2>&1 | grep "no developer")" != "" ] || [ "$(git -v 2>&1 | grep "no developer")" != "" ]; then
  echo "Missing Xcode tools, please install them!"
  exit 1
fi

if [ "$(nasm -v)" = "" ] || [ "$(nasm -v | grep Apple)" != "" ]; then
  echo "Missing or incompatible nasm!"
  echo "Download the latest nasm from http://www.nasm.us/pub/nasm/releasebuilds/"
  prompt "Install last tested version automatically?"
  pushd /tmp >/dev/null
  rm -rf nasm-mac64.zip
  curl -OL "https://github.com/acidanthera/ocbuild/raw/master/external/nasm-mac64.zip" || exit 1
  nasmzip=$(cat nasm-mac64.zip)
  rm -rf nasm-*
  curl -OL "https://github.com/acidanthera/ocbuild/raw/master/external/${nasmzip}" || exit 1
  unzip -q "${nasmzip}" nasm*/nasm nasm*/ndisasm || exit 1
  sudo mkdir -p /usr/local/bin || exit 1
  sudo mv nasm*/nasm /usr/local/bin/ || exit 1
  sudo mv nasm*/ndisasm /usr/local/bin/ || exit 1
  rm -rf "${nasmzip}" nasm-*
  popd >/dev/null
fi

if [ "$(which mtoc.NEW)" == "" ] || [ "$(which mtoc)" == "" ]; then
  echo "Missing mtoc or mtoc.NEW!"
  echo "To build mtoc follow: https://github.com/tianocore/tianocore.github.io/wiki/Xcode#mac-os-x-xcode"
  prompt "Install prebuilt mtoc and mtoc.NEW automatically?"
  pushd /tmp >/dev/null
  rm -f mtoc mtoc-mac64.zip
  curl -OL "https://github.com/acidanthera/ocbuild/raw/master/external/mtoc-mac64.zip" || exit 1
  unzip -q mtoc-mac64.zip mtoc || exit 1
  sudo mkdir -p /usr/local/bin || exit 1
  sudo cp mtoc /usr/local/bin/mtoc || exit 1
  sudo mv mtoc /usr/local/bin/mtoc.NEW || exit 1
  popd >/dev/null
fi

if [ ! -d "Binaries" ]; then
  mkdir Binaries || exit 1
  cd Binaries || exit 1
  ln -s ../UDK/Build/AppleSupportPkg/RELEASE_XCODE5/X64 RELEASE || exit 1
  ln -s ../UDK/Build/AppleSupportPkg/DEBUG_XCODE5/X64 DEBUG || exit 1
  ln -s ../UDK/Build/AppleSupportPkg/NOOPT_XCODE5/X64 NOOPT || exit 1
  cd .. || exit 1
fi

while true; do
  if [ "$1" == "--skip-tests" ]; then
    SKIP_TESTS=1
    shift
  elif [ "$1" == "--skip-build" ]; then
    SKIP_BUILD=1
    shift
  elif [ "$1" == "--skip-package" ]; then
    SKIP_PACKAGE=1
    shift
  else
    break
  fi
done

if [ "$1" != "" ]; then
  MODE="$1"
  shift
fi

if [ ! -f UDK/UDK.ready ]; then
  rm -rf UDK

  sym=$(find . -not -type d -exec file "{}" ";" | grep CRLF)
  if [ "${sym}" != "" ]; then
    echo "Repository CRLF line endings"
    exit 1
  fi
fi

updaterepo "https://github.com/acidanthera/audk" UDK master || exit 1
cd UDK
updaterepo "https://github.com/acidanthera/EfiPkg" EfiPkg master || exit 1
updaterepo "https://github.com/acidanthera/OcSupportPkg" OcSupportPkg master || exit 1

if [ ! -d AppleSupportPkg ]; then
  ln -s .. AppleSupportPkg || exit 1
fi

source edksetup.sh || exit 1

if [ "$SKIP_TESTS" != "1" ]; then
  make -C BaseTools || exit 1
  touch UDK.ready
fi

if [ "$SKIP_BUILD" != "1" ]; then
  if [ "$MODE" = "" ] || [ "$MODE" = "DEBUG" ]; then
    build -a X64 -b DEBUG -t XCODE5 -p AppleSupportPkg/AppleSupportPkg.dsc || exit 1
  fi

  if [ "$MODE" = "" ] || [ "$MODE" = "DEBUG" ]; then
    build -a X64 -b NOOPT -t XCODE5 -p AppleSupportPkg/AppleSupportPkg.dsc || exit 1
  fi

  if [ "$MODE" = "" ] || [ "$MODE" = "RELEASE" ]; then
    build -a X64 -b RELEASE -t XCODE5 -p AppleSupportPkg/AppleSupportPkg.dsc || exit 1
  fi
fi

cd .. || exit 1

if [ "$SKIP_PACKAGE" != "1" ]; then
  if [ "$PACKAGE" = "" ] || [ "$PACKAGE" = "DEBUG" ]; then
    package "Binaries/DEBUG" "DEBUG" || exit 1
  fi

  if [ "$PACKAGE" = "" ] || [ "$PACKAGE" = "RELEASE" ]; then
    package "Binaries/RELEASE" "RELEASE" || exit 1
  fi
fi
