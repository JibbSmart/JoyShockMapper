#/bin/bash

printerror() { cat <<< "$@" 1>&2; }

USAGE=$(printf "\nUsage:\n$0 -v|--version 1.6.0 -r|--release 2\n\n")
ARGS=$(getopt -o :v:r: --long version:,release: -- "$@")

if [ $? != 0 ] ; then
    printerror "$USAGE"
    exit 1
fi

eval set -- "$ARGS"
while true ; do
    case $1 in
        -v|--version)
            VERSION=$2 ; shift 2 ;;
        -r|--release)
            RELEASE=$2 ; shift 2 ;;
        --)
            shift ; break ;;
    esac
done

if [ -z "$VERSION" ]; then
    printerror "-v|--version is a mandatory parameter"
    printerror "$USAGE"
    exit 1
fi

if [ -z "$RELEASE" ]; then
    printerror "-r|--release is a mandatory parameter"
    printerror "$USAGE"
    exit 1
fi

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
    DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"

PROJECT_ROOT=$DIR/../

pushd &>/dev/null

cd "$DIR"/../dist/linux

rm -rf release

mkdir release && cd release

cp -f ../PKGBUILD.in PKGBUILD
cp -f ../joyshockmapper.dsc.in joyshockmapper.dsc

sed -i 's/@VERSION@/'$VERSION'/g' PKGBUILD
sed -i 's/@VERSION@/'$VERSION'/g' joyshockmapper.dsc

sed -i 's/@RELEASE@/'$RELEASE'/g' PKGBUILD
sed -i 's/@RELEASE@/'$RELEASE'/g' joyshockmapper.dsc

mkdir build && cd build
cmake "$PROJECT_ROOT"

cd ..

mkdir src && cd src

cp -arf "$PROJECT_ROOT"/cmake .
mkdir -p dist/linux
cp -arf "$PROJECT_ROOT"/dist/linux/*.rules ./dist/linux/
cp -arf "$PROJECT_ROOT"/dist/linux/*.desktop ./dist/linux/
cp -arf "$PROJECT_ROOT"/dist/linux/*.svg ./dist/linux/
cp -arf "$PROJECT_ROOT"/dist/GyroConfigs ./dist/
cp -arf "$PROJECT_ROOT"/JoyShockMapper .
cp -arf "$PROJECT_ROOT"/CMakeLists.txt .
cp -arf "$PROJECT_ROOT"/*.md .
cp -arf ../build/_deps/joyshocklibrary-src JoyShockLibrary

find . -type d -name ".git" -exec rm -rf {} \;

TARBALL=joyshockmapper_$VERSION-$RELEASE.tar.gz

tar zcfv $TARBALL *

MD5=$(md5sum $TARBALL | cut -f1 -d' ')
SHA1=$(sha1sum $TARBALL | cut -f1 -d' ')
SHA256=$(sha256sum $TARBALL | cut -f1 -d' ')
SIZE=$(du --bytes $TARBALL | cut -f1)

mv joyshockmapper_$VERSION-$RELEASE.tar.gz ../
cd ../

rm -rf src
rm -rf build

sed -i 's/@MD5@/'$MD5'/g' PKGBUILD
sed -i 's/@MD5@/'$MD5'/g' joyshockmapper.dsc

sed -i 's/@SHA1@/'$SHA1'/g' PKGBUILD
sed -i 's/@SHA1@/'$SHA1'/g' joyshockmapper.dsc

sed -i 's/@SHA256@/'$SHA256'/g' PKGBUILD
sed -i 's/@SHA256@/'$SHA256'/g' joyshockmapper.dsc

sed -i 's/@TARBALL@/'$TARBALL'/g' PKGBUILD
sed -i 's/@TARBALL@/'$TARBALL'/g' joyshockmapper.dsc

sed -i 's/@SIZE@/'$SIZE'/g' PKGBUILD
sed -i 's/@SIZE@/'$SIZE'/g' joyshockmapper.dsc

popd &>/dev/null

exit 0
