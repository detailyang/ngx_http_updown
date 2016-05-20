%define     oname   ngx_http_updown_module
Name:       yz-ngx_http_updown_module
Version:    0.2.0
Release:    6%{?dist}
Summary:   ngx_http_updown_module is a an addon for Nginx to markup graceful up or down

Group:      Development/Libraries
License:    GPL
URL:        https://github.com/detailyang/ngx_http_updown
Source0:    %{oname}-%{version}.tar.gz
BuildRoot:  %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  tengine-devel tengine
Requires:   tengine

%description
ngx_http_updown_module is a an addon for Nginx to markup graceful up or down

%prep
%setup -q -n %{oname}-%{version}

%build

%install
rm -rf %{buildroot}
mkdir -p $RPM_BUILD_ROOT/opt/tengine/modules/
mkdir -p $RPM_BUILD_ROOT/data/updown
/opt/tengine/sbin/dso_tool --add-module=%{_builddir}/%{oname}-%{version} --dst=$RPM_BUILD_ROOT/opt/tengine/modules/

%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
/opt/tengine/modules/ngx_http_updown_module.so
/data/updown
%doc



%changelog
* May 16 2016 Yang Bingwu  <detailyang@gmail.com>
    upgrade to 0.2.0
