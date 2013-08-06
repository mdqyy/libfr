# libfr

Library for face recognition using Waldboost algorithm

## Original Sources

* http://medusa.fit.vutbr.cz/libabr/
* http://medusa.fit.vutbr.cz/public/software/waldboost/

## Prerequisites (debian packages)

* autoconf 
* automake
* build-essential 
* module-assistant
* git-core
* cmake
* boost
* libprotobuf-dev
* libopencv-dev
* libxml2-dev
* libargtable2-dev
* pkg-config

## Build

* Install required packages

```
apt-get install autoconf automake build-essential module-assistant git-core cmake \
  libopencv-dev libxml2-dev libargtable2-dev pkg-config libboost-all-dev libprotobuf-dev
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

## References

* Herout Adam, Juránek Roman, Zemčík Pavel: Implementing the Local Binary
Patterns with SIMD Instructions of CPU, In: Proceedings of WSCG 2010, Plzeň,
CZ, ZČU v Plzni, 2010, p. 39-42, ISBN 978-80-86943-86-2
* Herout Adam, Zemčík Pavel, Hradiš Michal, Juránek Roman, Havel Jiří, Jošth
Radovan, Žádník Martin: Pattern Recognition, Recent Advances, Pattern
Recognition, Recent Advances, Vienna, AT, IN-TECH, 2010, p. 111-136, ISBN
978-953-7619-90-9
* Herout Adam, Zemčík Pavel, Juránek Roman, Hradiš Michal: Implementation of the
"Local Rank Differences" Image Feature Using SIMD Instructions of CPU, In:
Proceedings of Sixth Indian Conference on Computer Vision, Graphics and Image
Processing, Bhubaneswar, IN, IEEE CS, 2008, p. 9, ISBN 978-0-7695-3476-3
* Hradiš Michal, Herout Adam, Zemčík Pavel: Local Rank Patterns - Novel Features
for Rapid Object Detection, In: Proceedings of International Conference on
Computer Vision and Graphics 2008, Heidelberg, DE, Springer, 2008, p. 1-12,
ISSN 0302-9743
