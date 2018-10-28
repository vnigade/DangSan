#!/bin/bash

set -e

# to obtain the repository: git clone -b erik ssh://git@git.cs.vu.nl:1337/i.haller/MetaAlloc.git

: ${PATHROOT:="$PWD"}
if [ ! -f "$PATHROOT/autosetup.sh" ]; then
	echo "Please execute from the root of the repository or set PATHROOT" >&2
	exit 1
fi

source "$PATHROOT/autosetup-benchmarks.inc"

: ${CONFIG_FIXEDCOMPRESSION:=false}
: ${CONFIG_METADATABYTES:=16}
: ${CONFIG_DEEPMETADATA:=false}
: ${CONFIG_DEEPMETADATABYTES:=128}

: ${INSTANCES="default baselinesafestack baseline metaalloc"}
#: ${INSTANCES="metaalloc"}
: ${JOBS=16}
: ${LLVMREV:=251286}
: ${REPLACEGLIBC:=0}

: ${PATHAUTOSETUP="$PATHROOT/autosetup.dir"}
: ${PATHAUTOLLVMAPPS="$PATHAUTOSETUP/llvm-apps"}
: ${PATHAUTOPREFIX="$PATHAUTOSETUP/install"}
: ${PATHAUTOSRC="$PATHAUTOSETUP/src"}
: ${PATHAUTOSTATE="$PATHAUTOSETUP/state"}
: ${PATHAUTOPREFIXBASE="$PATHAUTOSETUP/install-baseline"}
: ${PATHAUTOPREFIXMETA="$PATHAUTOSETUP/install-metaalloc"}
: ${PATHLOG="$PATHROOT/autosetup-log.txt"}

: ${VERSIONBASH=bash-4.3}
: ${VERSIONBINUTILS=binutils-2.25}
: ${VERSIONCMAKE=cmake-3.4.1}
: ${VERSIONCMAKEURL=v3.4}
: ${VERSIONCOREUTILS=coreutils-8.22}
: ${VERSIONLIBUNWIND=libunwind-1.2-rc1}
: ${VERSIONPERL=perl-5.8.8}
: ${VERSIONPERLURL=5.0}
: ${VERSIONLIBUUID=libuuid-1.0.3}
#: ${VERSIONLIBXML2=libxml2-2.9.2}
: ${VERSIONLIBXML2=libxml2-2.7.8}
: ${VERSIONLIBGEOIP=geoip_1.6.2.orig}

: ${DANG_DEBUG=0}

PATHBINUTILS="$PATHAUTOSRC/$VERSIONBINUTILS"

export PATH="$PATHAUTOPREFIX/bin:$PATH"

exec 5> "$PATHLOG"

run()
{
	echo -------------------------------------------------------------------------------- >&5
	echo "command:          $*"               >&5
	echo "\$PATH:            $PATH"            >&5
	echo "\$LD_LIBRARY_PATH: $LD_LIBRARY_PATH" >&5
	echo "working dir:      $PWD"             >&5
	echo -------------------------------------------------------------------------------- >&5
	if "$@" >&5 2>&5; then
		echo "[done]" >&5
	else
		echo "Command '$*' failed in directory $PWD with exit code $?, please check $PATHLOG for details" >&2
		exit 1
	fi
}

echo "Creating directories"
run mkdir -p "$PATHAUTOSRC"
run mkdir -p "$PATHAUTOSTATE"

export CFLAGS="-I$PATHAUTOPREFIX/include"
export CPPFLAGS="-I$PATHAUTOPREFIX/include"
export LDFLAGS="-L$PATHAUTOPREFIX/lib"

#function comment()
#{
# build bash to override the system's default shell
echo "Building bash"
cd "$PATHAUTOSRC"
[ -f "$VERSIONBASH.tar.gz" ] || run wget "http://ftp.gnu.org/gnu/bash/$VERSIONBASH.tar.gz"
[ -d "$VERSIONBASH" ] || run tar xf "$VERSIONBASH.tar.gz"
cd "$VERSIONBASH"
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install
[ -f "$PATHAUTOPREFIX/bin/sh" ] || ln -s "$PATHAUTOPREFIX/bin/bash" "$PATHAUTOPREFIX/bin/sh"

# build a sane version of coreutils
echo "Building coreutils"
cd "$PATHAUTOSRC"
[ -f "$VERSIONCOREUTILS.tar.xz" ] || run wget "http://ftp.gnu.org/gnu/coreutils/$VERSIONCOREUTILS.tar.xz"
[ -d "$VERSIONCOREUTILS" ] || run tar xf "$VERSIONCOREUTILS.tar.xz"
cd "$VERSIONCOREUTILS"
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install

# build binutils to ensure we have gld
echo "Building binutils"
cd "$PATHAUTOSRC"
[ -f "$VERSIONBINUTILS.tar.bz2" ] || run wget "http://ftp.gnu.org/gnu/binutils/$VERSIONBINUTILS.tar.bz2"
[ -d "$VERSIONBINUTILS" ] || run tar xf "$VERSIONBINUTILS.tar.bz2"
cd "$PATHBINUTILS"
confopts="--enable-gold --enable-plugins --disable-werror"
[ -n "`gcc -print-sysroot`" ] && confopts="$confopts --with-sysroot" # match system setting to avoid 'this linker was not configured to use sysroots' error or failure to find libpthread.so
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX" $confopts
run make -j"$JOBS"
run make -j"$JOBS" all-gold
run make install
run rm "$PATHAUTOPREFIX/bin/ld"
run cp "$PATHAUTOPREFIX/bin/ld.gold" "$PATHAUTOPREFIX/bin/ld" # replace ld with gold

# build cmake, needed to build LLVM
echo "Building cmake"
cd "$PATHAUTOSRC"
[ -f "$VERSIONCMAKE.tar.gz" ] || run wget "https://cmake.org/files/$VERSIONCMAKEURL/$VERSIONCMAKE.tar.gz"
[ -d "$VERSIONCMAKE" ] || run tar xf "$VERSIONCMAKE.tar.gz"
cd "$VERSIONCMAKE"
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install

if [ "$REPLACEGLIBC" -ne 0 ]; then
# We need a patched glibc compatible with the system version
if [ "$VERSIONGLIBC" = "" ]; then
	echo "Detecting libc version"
	run mkdir -p "$PATHAUTOSRC/libcver"
	cd "$PATHAUTOSRC/libcver"
	cat > libcver.c <<EOF
#include <stdio.h>
#include <gnu/libc-version.h>
int main(void) {
	puts(gnu_get_libc_version());
	return 0;
}
EOF
	run make libcver
	VERSIONGLIBC="glibc-`./libcver`"
	case "$VERSIONGLIBC" in
	glibc-2.[2-9][0-9])
		VERSIONGLIBC="glibc-2.19" # patch does not apply on newer versions
		;;
	esac
fi

echo "Building $VERSIONGLIBC"
mkdir -p "$PATHAUTOSRC/glibc"
cd "$PATHAUTOSRC/glibc"
[ -f "$VERSIONGLIBC.tar.gz" ] || run wget "http://ftp.gnu.org/gnu/glibc/$VERSIONGLIBC.tar.gz"
[ -d "$VERSIONGLIBC" ] || tar xf "$VERSIONGLIBC.tar.gz"
cd "$VERSIONGLIBC"
run mkdir -p obj
cd obj
[ -f Makefile ] || run ../configure --prefix="$PATHAUTOPREFIX" --with-binutils="$PATHBINUTILS" CFLAGS="-O2 -pipe -U_FORTIFY_SOURCE -fno-stack-protector"
run make -j"$JOBS"
run make install
fi

# gperftools requires libunwind
echo "Building libunwind"
cd "$PATHAUTOSRC"
[ -f "$VERSIONLIBUNWIND.tar.gz" ] || run wget "http://download.savannah.gnu.org/releases/libunwind/$VERSIONLIBUNWIND.tar.gz"
[ -d "$VERSIONLIBUNWIND" ] || run tar xf "$VERSIONLIBUNWIND.tar.gz"
cd "$VERSIONLIBUNWIND"
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install

# httpd requires libuuid
echo "Building libuuid"
cd "$PATHAUTOSRC"
[ -f "$VERSIONLIBUUID.tar.gz" ] || run wget "https://sourceforge.net/projects/libuuid/files/latest/download/$VERSIONLIBUUID.tar.gz"
[ -d "$VERSIONLIBUUID" ] || run tar xf "$VERSIONLIBUUID.tar.gz"
cd "$VERSIONLIBUUID"
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install

#Libxml2 required to build Php
echo "Building libxml2"
cd "$PATHAUTOSRC"
[ -f "$VERSIONLIBXML2.tar.gz" ] || run wget "ftp://xmlsoft.org/libxml2/$VERSIONLIBXML2.tar.gz"
[ -d "$VERSIONLIBXML2" ] || run tar xf "$VERSIONLIBXML2.tar.gz"
cd "$VERSIONLIBXML2"
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install

#openlite requires libgeoip-dev
echo "Building libgeoip-dev"
cd "$PATHAUTOSRC"
[ -f "$VERSIONLIBGEOIP.tar.gz" ] || run wget "https://launchpad.net/ubuntu/+archive/primary/+files/geoip_1.6.2.orig.tar.gz"
[ -d "$VERSIONLIBGEOIP" ] || run tar xv "$VERSIONLIBGEOIP.tar.gz"
cd "$VERSIONLIBGEOIP"
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install

# We need a patched LLVM
echo "Building LLVM"
cd "$PATHAUTOSRC"
[ -d llvm-svn/.svn ] || run svn co -r"$LLVMREV" http://llvm.org/svn/llvm-project/llvm/trunk llvm-svn
[ -d llvm-svn/tools/clang/.svn ] || run svn co -r"$LLVMREV" http://llvm.org/svn/llvm-project/cfe/trunk llvm-svn/tools/clang
[ -d llvm-svn/projects/compiler-rt/.svn ] || run svn co -r"$LLVMREV" http://llvm.org/svn/llvm-project/compiler-rt/trunk llvm-svn/projects/compiler-rt
cd "$PATHAUTOSRC/llvm-svn/projects/compiler-rt"
if [ ! -f .autosetup.patched-COMPILERRT-safestack ]; then
	run patch -p0 < "$PATHROOT/patches/COMPILERRT-safestack.diff"
	touch .autosetup.patched-COMPILERRT-safestack
fi
cd "$PATHAUTOSRC/llvm-svn"
if [ ! -f .autosetup.patched-LLVM-gold-plugins ]; then
	run patch -p0 < "$PATHROOT/patches/LLVM-gold-plugins.diff"
	touch .autosetup.patched-LLVM-gold-plugins
fi
if [ ! -f .autosetup.patched-LLVM-safestack ]; then
	run patch -p0 < "$PATHROOT/patches/LLVM-safestack.diff"
	touch .autosetup.patched-LLVM-safestack
fi
run mkdir -p "$PATHAUTOSRC/llvm-svn/obj"
cd "$PATHAUTOSRC/llvm-svn/obj"
[ -f Makefile ] || run cmake -DCMAKE_INSTALL_PREFIX="$PATHAUTOPREFIX" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_BINUTILS_INCDIR="$PATHBINUTILS/include" "$PATHAUTOSRC/llvm-svn"
run make -j"$JOBS"
run make install

# Build baseline version of gperftools
echo "Building gperftools"
cd "$PATHAUTOSRC"
if [ ! -d gperftools/.git ]; then
	run git clone https://github.com/gperftools/gperftools.git
	cd gperftools
	run git checkout c46eb1f3d2f7a2bdc54a52ff7cf5e7392f5aa668
fi
cd "$PATHAUTOSRC/gperftools"
if [ ! -f .autosetup.patched-gperftools-speedup ]; then
	run patch -p1 < "$PATHROOT/patches/GPERFTOOLS_SPEEDUP.patch"
	touch .autosetup.patched-gperftools-speedup
fi
[ -f configure ] || run autoreconf -fi
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIXBASE"
run make
run make install

echo "Building metapagetable"
cd "$PATHROOT/metapagetable"
export METALLOC_OPTIONS="-DFIXEDCOMPRESSION=$CONFIG_FIXEDCOMPRESSION -DMETADATABYTES=$CONFIG_METADATABYTES -DDEEPMETADATA=$CONFIG_DEEPMETADATA -DALLOC_SIZE_HOOK=dang_alloc_size_hook"
[ "true" = "$CONFIG_DEEPMETADATA" ] && METALLOC_OPTIONS="$METALLOC_OPTIONS -DDEEPMETADATABYTES=$CONFIG_DEEPMETADATABYTES"
rm -f metapagetable.h
run make config
run make -j"$JOBS"

# Build patched gperftools for new allocator
echo "Building gperftools-metalloc"
cd "$PATHROOT/gperftools-metalloc"
[ -f configure ] || run ./autogen.sh
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIXMETA"
run make -j"$JOBS"
run make install

echo "Building static libraries"
cd "$PATHROOT/staticlib"
run make -j"$JOBS" TC_ALLOC=1

echo "Building llvm-plugins"
cd "$PATHROOT/llvm-plugins"
run make -j"$JOBS" GOLDINSTDIR="$PATHAUTOPREFIX"

echo "Building demo"
cd "$PATHROOT/demo"
LD_LIBRARY_PATH="$PATHAUTOPREFIX/lib:$LD_LIBRARY_PATH" PATH="$PATHAUTOPREFIXMETA/bin:$PATH" #run make

echo "Testing demo"
cd "$PATHROOT/demo"
#run ./demo-none
LD_LIBRARY_PATH="$PATHAUTOPREFIXMETA/lib" #run ./demo
#}  #COMMENT END

EXTRA_LDFLAGS_BASELINE="-fsanitize=safe-stack -Wl,-plugin-opt=-load=$PATHROOT/llvm-plugins/libplugins.so -Wl,-plugin-opt=-byvalhandler -L$PATHAUTOSRC/gperftools/.libs -ltcmalloc"

if [ "$DANG_DEBUG" -ne 1 ]; then
    EXTRA_LDFLAGS_METAALLOC="-fsanitize=safe-stack -Wl,-plugin-opt=-largestack=false -Wl,-plugin-opt=-load=$PATHROOT/llvm-plugins/libplugins.so -Wl,-plugin-opt=-stats -Wl,-plugin-opt=-mergedstack=false -Wl,-plugin-opt=-byvalhandler -Wl,-plugin-opt=-stacktracker -Wl,-plugin-opt=-globaltracker -Wl,-plugin-opt=-pointertracker -Wl,-plugin-opt=-FreeSentryLoop -Wl,-plugin-opt=-custominline -umetaset_8 -umetaget_8 -uinitialize_global_metadata -L$PATHROOT/staticlib -Wl,-whole-archive -l:libmetadata.a -Wl,-no-whole-archive -L$PATHROOT/gperftools-metalloc/.libs -ltcmalloc @$PATHROOT/metapagetable/linker-options -L$PATHROOT/staticlib/Dangling/lib -ldang-malloc -Wl -Wl,-rpath $PATHROOT/staticlib/Dangling/lib"
# FreeSentry approach
#    EXTRA_LDFLAGS_METAALLOC="-fsanitize=safe-stack -Wl,-plugin-opt=-largestack=false -Wl,-plugin-opt=-load=$PATHROOT/llvm-plugins/libplugins.so -Wl,-plugin-opt=-stats -Wl,-plugin-opt=-mergedstack=false -Wl,-plugin-opt=-byvalhandler -Wl,-plugin-opt=-stacktracker -Wl,-plugin-opt=-globaltracker -Wl,-plugin-opt=-FSGraph -Wl,-plugin-opt=-FreeSentry -Wl,-plugin-opt=-FreeSentryLoop -Wl,-plugin-opt=-custominline -umetaset_8 -umetaget_8 -uinitialize_global_metadata -L$PATHROOT/staticlib -Wl,-whole-archive -l:libmetadata.a -Wl,-no-whole-archive -L$PATHROOT/gperftools-metalloc/.libs -ltcmalloc @$PATHROOT/metapagetable/linker-options -L$PATHROOT/staticlib/Dangling/lib -ldang-malloc -Wl -Wl,-rpath $PATHROOT/staticlib/Dangling/lib"
else
    EXTRA_LDFLAGS_METAALLOC="-fsanitize=safe-stack -Wl,-plugin-opt=-largestack=false -Wl,-plugin-opt=-load=$PATHROOT/llvm-plugins/libplugins.so -Wl,-plugin-opt=-stats -Wl,-plugin-opt=-mergedstack=false -Wl,-plugin-opt=-byvalhandler -Wl,-plugin-opt=-stacktracker -Wl,-plugin-opt=-globaltracker -Wl,-plugin-opt=-pointertracker -Wl,-plugin-opt=-FreeSentryLoop -umetaset_8 -umetaget_8 -uinitialize_global_metadata -L$PATHROOT/staticlib -Wl,-whole-archive -l:libmetadata.a -Wl,-no-whole-archive -L$PATHROOT/gperftools-metalloc/.libs -ltcmalloc @$PATHROOT/metapagetable/linker-options -L$PATHROOT/staticlib/Dangling/lib -ldang-malloc -Wl -Wl,-rpath $PATHROOT/staticlib/Dangling/lib"

# FreeSentry Approach
    #EXTRA_LDFLAGS_METAALLOC="-fsanitize=safe-stack -Wl,-plugin-opt=-largestack=false -Wl,-plugin-opt=-load=$PATHROOT/llvm-plugins/libplugins.so -Wl,-plugin-opt=-stats -Wl,-plugin-opt=-mergedstack=false -Wl,-plugin-opt=-byvalhandler -Wl,-plugin-opt=-stacktracker -Wl,-plugin-opt=-globaltracker -Wl,-plugin-opt=-FSGraph -Wl,-plugin-opt=-FreeSentry -Wl,-plugin-opt=-FreeSentryLoop -umetaset_8 -umetaget_8 -uinitialize_global_metadata -L$PATHROOT/staticlib -Wl,-whole-archive -l:libmetadata.a -Wl,-no-whole-archive -L$PATHROOT/gperftools-metalloc/.libs -ltcmalloc @$PATHROOT/metapagetable/linker-options -L$PATHROOT/staticlib/Dangling/lib -ldang-malloc -Wl -Wl,-rpath $PATHROOT/staticlib/Dangling/lib"
fi

#EXTRA_LDFLAGS_METAALLOC="-fsanitize=safe-stack -Wl,-plugin-opt=-largestack=false -Wl,-plugin-opt=-load=$PATHROOT/llvm-plugins/libplugins.so -Wl,-plugin-opt=-mergedstack=false -Wl,-plugin-opt=-byvalhandler -Wl,-plugin-opt=-stacktracker -Wl,-plugin-opt=-globaltracker -Wl,-plugin-opt=-custominline -umetaset_8 -umetaget_8 -uinitialize_global_metadata -L$PATHROOT/staticlib -Wl,-whole-archive -l:libmetadata.a -Wl,-no-whole-archive -L$PATHROOT/gperftools-metalloc/.libs -ltcmalloc @$PATHROOT/metapagetable/linker-options"

run mkdir -p "$PATHAUTOLLVMAPPS"
[ -d "$PATHAUTOSRC/llvm-3.8" ] || ln -s "$PATHAUTOSRC/llvm-svn" "$PATHAUTOSRC/llvm-3.8"
[ -d "$PATHAUTOSRC/llvm-3.8/bin" ] || ln -s "$PATHAUTOPREFIX" "$PATHAUTOSRC/llvm-3.8/bin"
for instance in $INSTANCES; do
	echo "Setting up LLVM-apps-$instance"
	[ -d "$PATHAUTOLLVMAPPS/$instance" ] || run git clone git@github.com:cgiuffr/llvm-apps.git "$PATHAUTOLLVMAPPS/$instance"
	cd "$PATHAUTOLLVMAPPS/$instance"
	[ -f common.overrides.conf.inc ] || clean="y" install_pkgs="n" have_llvm="y" have_di="n" have_dr="n" llvm_version="3.8" llvm_basedir="$PATHAUTOSRC" have_pin="n" have_dune="n" ov_PIE="n" ov_dsa="n" ov_gdb="n" ov_segfault="n" run ./configure
	[ "$instance" = baseline ] && echo "LLVMGOLD_LDFLAGS_EXTRA=$EXTRA_LDFLAGS_BASELINE" > common.overrides.baseline.inc
	[ "$instance" = metaalloc ] && echo "LLVMGOLD_LDFLAGS_EXTRA=$EXTRA_LDFLAGS_METAALLOC" > common.overrides.metaalloc.inc
done

# build Perl, default perl does not work with perlbrew
echo "building perl"
cd "$PATHAUTOSRC"
[ -f "$VERSIONPERL.tar.gz" ] || run wget "http://www.cpan.org/src/$VERSIONPERLURL/$VERSIONPERL.tar.gz"
[ -d "$VERSIONPERL" ] || run tar xf "$VERSIONPERL.tar.gz"
cd "$VERSIONPERL"
if [ ! -f .autosetup.patched-makedepend ]; then
	run patch -p1 < "$PATHAUTOLLVMAPPS/default/conf/perl-patches/perl-makedepend.patch"
	touch .autosetup.patched-makedepend
fi
if [ ! -f .autosetup.patched-pagesize ]; then
	run patch -p1 < "$PATHAUTOLLVMAPPS/default/conf/perl-patches/perl-pagesize.patch"
	touch .autosetup.patched-pagesize
fi
if [ ! -f .autosetup.patched-Configure ]; then
	libfile="`gcc -print-file-name=libm.so`"
	libpath="`dirname "$libfile"`"
	[ "$libpath" == . ] || sed -i "s|^xlibpth='|xlibpth='$libpath |" Configure
	touch .autosetup.patched-Configure
fi
[ -f Makefile ] || run "$PATHAUTOPREFIX/bin/bash" ./Configure -des -Dprefix="$PATHAUTOPREFIX"
for m in makefile x2p/makefile; do
	grep -v "<command-line>" "$m" > "$m.tmp"
	mv "$m.tmp" "$m"
done
sed -i 's,# *include *<asm/page.h>,#define PAGE_SIZE 4096,' ext/IPC/SysV/SysV.xs
run make -j"$JOBS"
run make install

# install perlbrew (needed by SPEC2006), fixing its installer in the process
echo "building perlbrew"
export PERLBREW_ROOT="$PATHAUTOSETUP/perl-root"
export PERLBREW_HOME="$PATHAUTOSETUP/perl-home"
run mkdir -p "$PATHAUTOSRC/perlbrew"
cd "$PATHAUTOSRC/perlbrew"
[ -f perlbrew-installer ] || run wget -O perlbrew-installer http://install.perlbrew.pl
[ -f perlbrew-installer-patched ] || sed "s,/usr/bin/perl,$PATHAUTOPREFIX/bin/perl,g" perlbrew-installer > perlbrew-installer-patched
run chmod u+x perlbrew-installer-patched
run "$PATHAUTOPREFIX/bin/bash" ./perlbrew-installer-patched

source "$PERLBREW_ROOT/etc/bashrc"
export PATH="$PERLBREW_ROOT/perls/perl-5.8.8/bin:$PATH"

echo "installing perl packages"
if [ ! -f "$PATHAUTOSTATE/installed-perlbrew-perl-5.8.8" ]; then
	run perlbrew --notest install 5.8.8
	touch "$PATHAUTOSTATE/installed-perlbrew-perl-5.8.8"
fi
run perlbrew switch 5.8.8
[ -f "$PERLBREW_ROOT/bin/cpanm" ] || run perlbrew install-cpanm
run cpanm -n IO::Uncompress::Bunzip2
run cpanm -n LWP::UserAgent
run cpanm -n XML::SAX
run cpanm -n IO::Scalar
run cpanm -n Digest::MD5

for instance in $INSTANCES; do
	echo "building SPEC2006-$instance"
	#export SPEC_LLVM_ROOT="$PATHAUTOLLVMAPPS/$instance/apps/SPEC_CPU2006"
        export SPEC_LLVM_ROOT="$PATHAUTOLLVMAPPS/$instance/apps/nginx-0.8.54"
        #export SPEC_LLVM_ROOT="$PATHAUTOLLVMAPPS/$instance/apps/httpd-2.2.23"
	cd "$SPEC_LLVM_ROOT"

	# select prefix for instance
	prefix="$PATHAUTOPREFIXBASE"
	[ "$instance" = metaalloc ] && prefix="$PATHAUTOPREFIXMETA"

	# avoid running second part of configure.llvm as it changes .bashrc and starts an interactive shell
	touch init.done
	PATH="$prefix/bin:$PATH" INPUT_LIMIT=2 C="$BENCHMARKS" run "$PATHAUTOPREFIX/bin/bash" ./configure.llvm
# --with-included-apr
	PATH="$prefix/bin:$PATH" CONFIGNAME="$CONFIGNAME" LD_LIBRARY_PATH="$PATHAUTOPREFIX/lib:$LD_LIBRARY_PATH" CLEAN=1 INPUT_LIMIT=2 C="$BENCHMARKS" run "$PATHAUTOPREFIX/bin/bash" ./relink.llvm
	PATH="$prefix/bin:$PATH" CONFIGNAME="$CONFIGNAME" LD_LIBRARY_PATH="$PATHAUTOPREFIX/lib:$LD_LIBRARY_PATH" CLEAN=1 C=$1 run "$PATHAUTOPREFIX/bin/bash" ./build.llvm
        
#	(
#		echo "export PATH=\"$prefix/bin:$PATHAUTOPREFIX/bin:$PATH\""
#		echo "export SPEC_LLVM_ROOT=\"$SPEC_LLVM_ROOT\""
#		echo "export PERLBREW_HOME=\"$PERLBREW_HOME\""
#		echo "export PERLBREW_ROOT=\"$PERLBREW_ROOT\""
#		[ "$instance" = baseline ] && echo "export METALLOC_PATH=\"$PATHAUTOSRC/gperftools\""
#		[ "$instance" = metaalloc ] && echo "export METALLOC_PATH=\"$PATHROOT/gperftools-metalloc\""
#		[ "$instance" = metaalloc ] && echo "export SAFESTACK_OPTIONS=largestack=true"
#		[ "$instance" = baseline ] && echo "export LD_LIBRARY_PATH=\"$PATHAUTOPREFIXBASE/lib:\$LD_LIBRARY_PATH\""
#		[ "$instance" = metaalloc ] && echo "export LD_LIBRARY_PATH=\"$PATHAUTOPREFIXMETA/lib:\$LD_LIBRARY_PATH\""
#		echo "source \"$PERLBREW_ROOT/etc/bashrc\""
#		echo "source \"$SPEC_LLVM_ROOT/shrc\""
#	) > "$SPEC_LLVM_ROOT/autosetup.inc"
#	(
#		echo "#!$PATHAUTOPREFIX/bin/bash"
#		echo "set -e"
#		echo "cd \"$SPEC_LLVM_ROOT\""
#		echo "source $SPEC_LLVM_ROOT/autosetup.inc"
#		echo "./clientctl bench"
#	) > "$PATHAUTOLLVMAPPS/run-spec-$instance.sh"
#	chmod u+x "$PATHAUTOLLVMAPPS/run-spec-$instance.sh"
done

echo done

