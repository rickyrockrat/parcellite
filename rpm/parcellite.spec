Name:           parcellite
Version:        1.2.3
Release:        1%{?prerelease:.%{?prerelease}}%{?dist}
Summary:        A lightweight GTK+ clipboard manager

Group:          User Interface/Desktops
License:        GPLv3+
URL:            http://parcellite.sourceforge.net/
Source0:        http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
# Submitted upstream via
# https://sourceforge.net/p/parcellite/patches/29/
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  gtk2-devel >= 2.10.0 
BuildRequires:  desktop-file-utils, intltool >= 0.23

%description
Parcellite is a stripped down, basic-features-only clipboard manager with a 
small memory footprint for those who like simplicity.

In GNOME and Xfce the clipboard manager will be started automatically. For 
other desktops or window managers you should also install a panel with a 
system tray or notification area if you want to use this package.


%prep
%setup -q

%build
make -f Makefile.simple %{?_smp_mflags}


%install
rm -rf %{buildroot}
make -f Makefile.simple install DESTDIR=%{buildroot} INSTALL='install -p'
%find_lang %{name}

desktop-file-install --vendor="fedora"                     \
  --delete-original                                        \
  --remove-category=Application                            \
  --dir=%{buildroot}%{_datadir}/applications          \
  %{buildroot}%{_datadir}/applications/%{name}.desktop

desktop-file-install                                       \
  --delete-original                                        \
  --add-category=TrayIcon                                  \
  --dir=%{buildroot}%{_sysconfdir}/xdg/autostart/     \
  %{buildroot}%{_sysconfdir}/xdg/autostart/%{name}-startup.desktop


%clean
rm -rf %{buildroot}


%files -f %{name}.lang
%defattr(-,root,root,-)
%doc AUTHORS ChangeLog COPYING README NEWS
%config(noreplace) %{_sysconfdir}/xdg/autostart/%{name}-startup.desktop
%{_bindir}/%{name}
%{_datadir}/applications/fedora-%{name}.desktop
%{_datadir}/pixmaps/%{name}.*
%{_mandir}/man1/%{name}.1.*


%changelog
* Sat Dec 28 2023 Doug Springer <gpib@rickyrockrat.net> - 1.2.3-1
- Update to 1.2.3
- Remove de-po.patch, should be included.

* Sun Jan 27 2013 Christoph Wickert <cwickert@fedoraproject.org> - 1.1.4-1
- Update to 1.1.4
- Update de-po.patch

* Fri Jul 20 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.2-0.3.rc5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Sat Mar 10 2012 Christoph Wickert <cwickert@fedoraproject.org> - 1.0.2-0.2.rc5
- Don't ship prebuilt binaries (#800644)
- Fix build error with glib2 >= 2.30

* Sat Mar 03 2012 Christoph Wickert <cwickert@fedoraproject.org> - 1.0.2-0.1.rc5
- Update to 1.0.2 RC5 (#730240)

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.9.2-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Wed Feb 17 2010 Christoph Wickert <cwickert@fedoraproject.org> - 0.9.2-2
- Add patch to fix DSO linking (#565054)

* Fri Jan 01 2010 Christoph Wickert <cwickert@fedoraproject.org> - 0.9.2-1
- Update to 0.9.2

* Sat Jul 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.9.1-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Wed Mar 18 2009 Christoph Wickert <cwickert@fedoraproject.org> - 0.9.1-1
- Update to 0.9.1
- Remove both patches as all fixes got upstreamed

* Thu Feb 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.9-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Sun Nov 23 2008 Christoph Wickert <cwickert@fedoraproject.org> - 0.9-1
- Update to 0.9
- Fix Control+Click behaviour
- Small corrections to German translation

* Sat Apr 19 2008 Christoph Wickert <cwickert@fedoraproject.org> - 0.8-1
- Update to 0.8

* Sat Apr 19 2008 Christoph Wickert <cwickert@fedoraproject.org> - 0.7-2
- No longer require lxpanel
- Preserve timestamps during install
- Include NEWS in doc

* Sat Apr 12 2008 Christoph Wickert <cwickert@fedoraproject.org> - 0.7-1
- Initial Fedora RPM
