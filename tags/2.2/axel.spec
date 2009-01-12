# note: version 2.0 is in devel, version 1.1 is stable
%define build_stable_version 0
%if %{build_stable_version}
%define ver 1.1
%define build_number 1
%else
%define ver 2.0
%define build_number 0.1
%define snapshot .20080705
%define source_folder axel-svn-trunk
%endif
Name:           axel
Version:        %{ver}
Release:        %{build_number}%{?snapshot}%{?dist}
Summary:        A lightweight download accelerator by using multiple connections

Group:          Applications/Internet
License:        GPLv2
URL:            http://axel.alioth.debian.org
Source0: http://download.alioth.debian.org/axel/axel/%{ver}/axel-%{ver}%{?snapshot}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

#BuildRequires:  
#Requires:       

%description
Axel tries to accelerate HTTP/FTP downloading process by using multiple
connections for one file. It can use multiple mirrors for a download. Axel has
no dependencies and is lightweight, so it might be useful as a wget clone on
byte-critical systems. 


%prep
%setup -q %{?source_folder:-n %{source_folder}}


%build
%configure --etcdir=%{_sysconfdir} --i18n=1
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%find_lang %{name}

%clean
rm -rf $RPM_BUILD_ROOT


%files -f %{name}.lang
%defattr(-,root,root,-)
%doc API CHANGES COPYING CREDITS README
%{_bindir}/axel
%config(noreplace) %{_sysconfdir}/axelrc
%{_mandir}/man1/*



%changelog
* Sat Jul 05 2008 bbbush <bbbush.yuan@gmail.com> - 2.0-0.1.20080705
- create spec.

