#!/bin/bash

PPWD="$(pwd)"
cd "$(dirname "$0")" || exit 1

IN="$(cat version)"
VIN=(${IN//./ })
VER="${VIN[0]}.${VIN[1]}.$(("${VIN[2]}" + 1))"
echo $VER > version

printf '#define VERSION "%s"' $VER > ../inc/version.h

cd "$PPWD" || exit 1
