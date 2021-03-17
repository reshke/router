#!/bin/bash
export LC_ALL=C
cat > debian/changelog<<EOH
router ($(git rev-list HEAD --count)-$(git rev-parse --short HEAD)) stable; urgency=low

  * Yandex autobuild

 -- ${USER} <${USER}@$(hostname)>  $(date +'%a, %d %b %Y %H:%M:%S %z')
EOH
echo "VERSION=$(git rev-list HEAD --count)-$(git rev-parse --short HEAD)" > version.properties
