%define name     gfxindex
%define ver      2.0pre2
%define rel      1
%define prefix   /usr


Summary: GFXIndex - Web thumbnails generator
Name: %{name}
Version: %{ver}
Release: %{rel}
License: GPL
Packager: Fredrik Rambris <fredrik@rambris.com>
Group: Applications/Multimedia
Source: http://fredrik.rambris.com/files/%{name}-%{PACKAGE_VERSION}.tar.bz2
BuildRoot: /var/tmp/%{name}-build-root
URL: http://fredrik.rambris.com/%{name}/
Provides: gfxindex
Requires: glib popt libjpeg libpng libexif expat
BuildRequires: glib-devel popt libjpeg-devel libpng-devel libexif-devel expat-devel

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
make DESTDIR=$RPM_BUILD_ROOT install-strip

%clean
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files
%doc AUTHORS COPYING ChangeLog docs/*

%{prefix}/bin/gfxindex

