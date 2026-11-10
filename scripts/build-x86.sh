#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CPP="$ROOT/app/src/main/cpp"
GLIBC_OUT="$ROOT/app/src/main/assets/glibc-x86_64"
X86_OUT="$ROOT/app/src/main/assets/x86_64-libs"

mkdir -p "$GLIBC_OUT" "$X86_OUT"

CFLAGS="-O2 -fPIC -shared"
LDFLAGS="-ldl -lpthread"
build() {
  local src="$1" out="$2" soname="${3:-}"
  local so_opt=""
  [[ -n "$soname" ]] && so_opt="-Wl,-soname,$soname"
  echo "[shim] $src -> $(basename "$out")${soname:+  (soname $soname)}"
  gcc $CFLAGS -o "$out" "$CPP/$src" $so_opt $LDFLAGS
}

build dbus_stub.c             "$GLIBC_OUT/libdbus-1.so.3"         libdbus-1.so.3
build pulse_stub.c             "$GLIBC_OUT/libpulse.so.0"          libpulse.so.0
build pulse_simple_stub.c      "$GLIBC_OUT/libpulse-simple.so.0"   libpulse-simple.so.0
build fmod_sched.c             "$GLIBC_OUT/libfmod_sched.so"
build connect_redirect.c       "$GLIBC_OUT/libconnect_redirect.so"

build dns_resolver.c           "$X86_OUT/libdns_resolver.so"
build unity_crash_fix.c        "$X86_OUT/libunity_crash_fix.so"
build godot_audio.c            "$X86_OUT/libasound.so.2"           libasound.so.2
build ctype_fix.c              "$X86_OUT/libctype_fix.so"
build godot_ctype_fix.c        "$X86_OUT/libgodot_ctype_patch.so"
build pthread_recursive_fix.c  "$X86_OUT/libpthread_recursive_fix.so"
build machine_id.c             "$X86_OUT/libmachine_id.so"
build sem_shim.c               "$X86_OUT/libsem_shim.so"

echo "[shim] libX11_stub.c + x11_extra.c + xrm_extra.c -> libX11_stub.so  (soname libX11.so.6)"
X11NEW="$(mktemp)"
if gcc $CFLAGS -o "$X11NEW" "$CPP/libX11_stub.c" "$CPP/x11_extra.c" "$CPP/xrm_extra.c" -Wl,-soname,libX11.so.6 $LDFLAGS; then
  nm -D --defined-only "$X86_OUT/libX11_stub.so" | awk '{print $3}' | sort -u > /tmp/x11_old.txt
  nm -D --defined-only "$X11NEW" | awk '{print $3}' | sort -u > /tmp/x11_new.txt
  LOST="$(comm -23 /tmp/x11_old.txt /tmp/x11_new.txt | tr '\n' ' ')"
  if [[ -z "$LOST" ]] && grep -qx XrmGetResource /tmp/x11_new.txt; then
    cp "$X11NEW" "$X86_OUT/libX11_stub.so"
    echo "[shim] libX11_stub.so rebuilt OK ($(wc -l < /tmp/x11_new.txt) exports)"
  else
    echo "[shim] WARNING: rebuilt libX11_stub.so lost exports: $LOST - keeping committed one"
  fi
else
  echo "[shim] WARNING: libX11_stub.so rebuild failed - keeping committed one"
fi

echo "[shim] curl_stub.c -> libcurl.so.4  (soname libcurl.so.4)"
gcc $CFLAGS -o "$X86_OUT/libcurl.so.4" "$CPP/curl_stub.c" -Wl,-soname,libcurl.so.4 -Wl,--version-script,"$CPP/curl_stub.map"
