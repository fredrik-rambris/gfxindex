%define ver      1.2
%define rel      1
%define prefix   /usr


Summary: GFXIndex - Web thumbnails generator
Name: gfxindex
Version: %{ver}
Release: %{rel}
License: GPL
Packager: Fredrik Rambris <fredrik@rambris.com>
Group: Applications/Multimedia
Source: http://fredrik.rambris.com/gfxindex/gfxindex-%{PACKAGE_VERSION}.tar.gz
BuildRoot: /tmp/gfxindex_root/
URL: http://fredrik.rambris.com/gfxindex/
Provides: gfxindex

%description
GFXIndex helps you organize your pictures by creating thumbnails and indexing
them in HTML-indexes. This allows you to put the entire thing directly on the
web, on a CD or whereever you want as long as it can be reached with a
web browser.

%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{prefix}
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files
%doc AUTHORS COPYING ChangeLog docs/README.html docs/bevel.png docs/pad.jpg docs/thumbs.gif extras/gfxindex.php3 extras/sample.css

%{prefix}/bin/gfxindex
