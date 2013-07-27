# libfr

Library for face recognition using Waldboost algorithm

## Original Sources

* http://medusa.fit.vutbr.cz/libabr/
* http://medusa.fit.vutbr.cz/public/software/waldboost/
* 
## Prerequisites (debian packages)

* build-essential 
* module-assistant
* git-core
* cmake
* libopencv-dev
* libxml2-dev
* libargtable2-dev

## Build

* Install required packages

```
apt-get install build-essential module-assistant git-core cmake libopencv-dev libxml2-dev libargtable2-dev
```

* Clone repository

```
git clone https://github.com/korczis/libfr.git
```

* Build sources

```
cd libfr/build
./build.sh
```
