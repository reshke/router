#!/bin/bash

set -ex

export DEBIAN_FRONTEND=noninteractive
export TZ=Europe/Moskow
sudo bash -c "echo $TZ > /etc/timezone"

/root/router/docker/dpkg/tzdata.sh

#compile pg
#======================================================================================================================================
cd /pdbuild/tmp

mk-build-deps  --build-dep --install --tool='apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends --yes' debian/control
./configure --prefix=/pgbin && make -j12 && make install -j12

#======================================================================================================================================

cd /root/router
export DEB_DH_SHLIBDEPS_ARGS_ALL=--dpkg-shlibdeps-params=--ignore-missing-info

#compile router (postgresql statement transactional multishard router)
make USE_PGXS=true PG_CONFIG=/pgbin/bin/pg_config
make USE_PGXS=true PG_CONFIG=/pgbin/bin/pg_config install


#======================================================================================================================================

#mk-build-deps  --build-dep --install --tool='apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends --yes' debian/control

dpkg-buildpackage -us -uc

#======================================================================================================================================
# here we should get some files at /root
