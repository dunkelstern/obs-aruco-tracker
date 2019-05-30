# Maintainer: Dunkelstern <hallo@dunkelstern.de>
pkgname=obs-aruco-tracker
pkgver=0.1.0
pkgrel=1
pkgdesc="Track ArUco markers and output movement commands to re-center the marker"
arch=('i686' 'x86_64')
url="https://github.com/dunkelstern/obs-aruco-tracker"
license=("GPL2")
depends=("obs-studio" "opencv" "libjpeg-turbo")
makedepends=("cmake")
source=("https://github.com/dunkelstern/${pkgname}/archive/v${pkgver}.tar.gz")
sha256sums=("")

build() {
  cd ${srcdir}/${pkgname}-${pkgver}
  cmake . -DSYSTEM_INSTALL=1
  make
}

package() {
  cd ${srcdir}/${pkgname}-${pkgver}
  make DESTDIR=${pkgdir}/ install
}
 
