#
# spec file for configuration of package apache
#
# Copyright  (c)  2010  Ericsson LM
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# please send bugfixes or comments to paolo.palmieri@ericsson.com
#

%define packer %(finger -lp `echo "$USER"` | head -n 1 | cut -d: -f 3)
%define _topdir /var/tmp/%(echo "$USER")/rpms/
%define _tmppath %_topdir/tmp
%define cpf_cxc_path %{_cxcdir}
Name:      %{_name} 
Summary:   Installation package for CPF. 
Version:   %{_prNr} 
Release:   %{_rel} 
License:   Ericsson Proprietary 
Vendor:    Ericsson LM 
Packager:  %packer 
Group:     Library 
BuildRoot: %_tmppath 
Requires:  APOS_OSCONFBIN

%define _CPF_LIB_NAME libfms_cpf.so.10.1.3 
%define _CPF_LIB_LINKNAME libfms_cpf.so.10
%define _CPF_LIB_SONAME libfms_cpf.so
%define __os_install_post /usr/lib/rpm/brp-compress \
						  %{nil}
%description
Installation package for CPF Service application

%install
echo " This is the CPF package install section"

mkdir -p $RPM_BUILD_ROOT%FMSdir
mkdir -p $RPM_BUILD_ROOT%FMSLIB64dir
mkdir -p $RPM_BUILD_ROOT%FMSBINdir
mkdir -p $RPM_BUILD_ROOT%FMSCONFdir

cp %cpf_cxc_path/conf/c_AxeCpFileSystem_imm_classes.xml  $RPM_BUILD_ROOT%FMSCONFdir/c_AxeCpFileSystem_imm_classes.xml
cp %cpf_cxc_path/conf/o_AxeCpFileSystemInstances_imm_objects.xml  $RPM_BUILD_ROOT%FMSCONFdir/o_AxeCpFileSystemInstances_imm_objects.xml
cp %cpf_cxc_path/conf/ha_fms_cpf_2N_objects.xml  $RPM_BUILD_ROOT%FMSCONFdir/ha_fms_cpf_2N_objects.xml

cp %cpf_cxc_path/bin/fms_cpf_clc $RPM_BUILD_ROOT%FMSBINdir/fms_cpf_clc
cp %cpf_cxc_path/bin/fms_cpfd $RPM_BUILD_ROOT%FMSBINdir/fms_cpfd
cp %cpf_cxc_path/bin/cpfmkvol $RPM_BUILD_ROOT%FMSBINdir/cpfmkvol
cp %cpf_cxc_path/bin/cpfmkfile $RPM_BUILD_ROOT%FMSBINdir/cpfmkfile
cp %cpf_cxc_path/bin/cpfls $RPM_BUILD_ROOT%FMSBINdir/cpfls
cp %cpf_cxc_path/bin/cpfrm $RPM_BUILD_ROOT%FMSBINdir/cpfrm
cp %cpf_cxc_path/bin/cpfrename $RPM_BUILD_ROOT%FMSBINdir/cpfrename
cp %cpf_cxc_path/bin/cpfport $RPM_BUILD_ROOT%FMSBINdir/cpfport
cp %cpf_cxc_path/bin/cpfcp $RPM_BUILD_ROOT%FMSBINdir/cpfcp
cp %cpf_cxc_path/bin/cpfcp.sh $RPM_BUILD_ROOT%FMSBINdir/cpfcp.sh
cp %cpf_cxc_path/bin/cpfls.sh $RPM_BUILD_ROOT%FMSBINdir/cpfls.sh
cp %cpf_cxc_path/bin/cpfmkfile.sh $RPM_BUILD_ROOT%FMSBINdir/cpfmkfile.sh
cp %cpf_cxc_path/bin/cpfport.sh $RPM_BUILD_ROOT%FMSBINdir/cpfport.sh
cp %cpf_cxc_path/bin/cpfrename.sh $RPM_BUILD_ROOT%FMSBINdir/cpfrename.sh
cp %cpf_cxc_path/bin/cpfrm.sh $RPM_BUILD_ROOT%FMSBINdir/cpfrm.sh

cp %cpf_cxc_path/bin/lib_ext/%_CPF_LIB_NAME $RPM_BUILD_ROOT%FMSLIB64dir

%clean
rm -rf $RPM_BUILD_ROOT/*

%pre
if [ $1 == 1 ]
then
        echo "This is the %{_name} package %{_rel} preinstall script during installation phase"
        echo "... Commands to be executed only during installation phase in pre section..."
fi

if [ $1 == 2 ]
then
        echo "This is the %{_name} package %{_rel} preinstall script during upgrade phase"
fi

%post
echo " This is the CPF package post install section"

if [ "$1" == "2" ];then
        # Upgrade happens, delete all links which are created in Old version, & recreate them again
        # If no changes, leave this section blank
		echo " Removing CPF links in %_lib64dir"
		
		rm -f %_lib64dir/%_CPF_LIB_SONAME*

		echo " Removing CPF links in %_bindir"
		rm -f %_bindir/fms_cpfd
		rm -f %_bindir/cpfmkvol
		rm -f %_bindir/cpfmkfile
		rm -f %_bindir/cpfls
		rm -f %_bindir/cpfrm
		rm -f %_bindir/cpfrename
		rm -f %_bindir/cpfport
		rm -f %_bindir/cpfcp

fi

echo " Creating CPF links in %_lib64dir"
ln -sf %FMSLIB64dir/%_CPF_LIB_NAME %_lib64dir/%_CPF_LIB_LINKNAME
ln -sf %_lib64dir/%_CPF_LIB_LINKNAME %_lib64dir/%_CPF_LIB_SONAME

echo " Creating CPF links in %_bindir"
ln -sf %FMSBINdir/fms_cpfd %_bindir/fms_cpfd
ln -sf %FMSBINdir/cpfmkvol %_bindir/cpfmkvol
ln -sf %FMSBINdir/cpfmkfile.sh %_bindir/cpfmkfile
ln -sf %FMSBINdir/cpfls.sh %_bindir/cpfls
ln -sf %FMSBINdir/cpfrm.sh %_bindir/cpfrm
ln -sf %FMSBINdir/cpfrename.sh %_bindir/cpfrename
ln -sf %FMSBINdir/cpfport.sh %_bindir/cpfport
ln -sf %FMSBINdir/cpfcp.sh %_bindir/cpfcp

%preun
echo " This is the CPF package pre uninstall section"

%postun
echo " This is the %{_name} package post uninstall section"

if [ "$1" == "0" ]; then
	# * Uninstallation.. remove all the files here! 
	
	echo " Removing CPF links in %_lib64dir"
	
	rm -f %_lib64dir/%_CPF_LIB_SONAME*

	echo " Removing CPF links in %_bindir"
	rm -f %_bindir/fms_cpfd
	rm -f %_bindir/cpfmkvol
	rm -f %_bindir/cpfmkfile
	rm -f %_bindir/cpfls
	rm -f %_bindir/cpfrm
	rm -f %_bindir/cpfrename
	rm -f %_bindir/cpfport
    rm -f %_bindir/cpfcp
fi

if [ "$1" == "1" ]; then #true for upgrade
        echo "upgrade postun section calls for %{_rel} "
        # Delete any other files which are not necessary in Old revision
        # * Do not delete binaries here *;
fi

%files
%defattr(-,root,root,-)
%FMSLIB64dir/%_CPF_LIB_NAME
%FMSBINdir/fms_cpfd
%FMSBINdir/fms_cpf_clc
%FMSCONFdir/c_AxeCpFileSystem_imm_classes.xml
%FMSCONFdir/o_AxeCpFileSystemInstances_imm_objects.xml
%FMSCONFdir/ha_fms_cpf_2N_objects.xml
%FMSBINdir/cpfmkvol
%FMSBINdir/cpfmkfile
%FMSBINdir/cpfls
%FMSBINdir/cpfrm
%FMSBINdir/cpfrename
%FMSBINdir/cpfport
%FMSBINdir/cpfcp
%FMSBINdir/cpfcp.sh
%FMSBINdir/cpfport.sh
%FMSBINdir/cpfrename.sh
%FMSBINdir/cpfrm.sh
%FMSBINdir/cpfls.sh
%FMSBINdir/cpfmkfile.sh

%changelog
* Wed Jun 29 2011  <vincenzo.xx.conforti@ericsson.com> 
- cpf rpm
- model namespace update

