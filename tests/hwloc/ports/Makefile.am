# Copyright © 2009-2024 Inria.  All rights reserved.
# Copyright © 2009, 2011-2012, 2020 Université Bordeaux
# Copyright © 2009-2014 Cisco Systems, Inc.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

SRC = $(HWLOC_top_srcdir)/hwloc

if HWLOC_HAVE_LINUX
check_LTLIBRARIES = \
        libhwloc-port-aix.la \
        libhwloc-port-darwin.la \
        libhwloc-port-freebsd.la \
        libhwloc-port-hpux.la \
        libhwloc-port-netbsd.la \
        libhwloc-port-solaris.la \
        libhwloc-port-windows.la \
        libhwloc-port-opencl.la \
        libhwloc-port-cuda.la \
        libhwloc-port-nvml.la \
        libhwloc-port-rsmi.la \
        libhwloc-port-levelzero.la \
        libhwloc-port-gl.la
check_LIBRARIES = \
	liblstopo-port-windows.a \
        liblstopo-port-android.a
endif HWLOC_HAVE_LINUX

# Note that AC_CONFIG_LINKS sets up the sym links for the files in
# this directory (back to the $top_srcdir/hwloc directory).  So if you
# need more sym-linked files in here, go edit configure.ac.  Note that
# we have to use sym links in here rather than just directly
# referencing the files via $HWLOC_top_srcdir/hwloc/foo.c because of
# dependencies issues when using the Automake option "subdir-objects".
# We nodist these because they're created by configure.

common_CPPFLAGS = \
        $(HWLOC_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/include \
        -DHWLOC_COMPILE_PORTS \
        -DHWLOC_INSIDE_LIBHWLOC

nodist_libhwloc_port_aix_la_SOURCES = topology-aix.c
libhwloc_port_aix_la_SOURCES = \
        include/aix/procinfo.h \
        include/aix/sys/processor.h \
        include/aix/sys/rset.h \
        include/aix/sys/systemcfg.h \
        include/aix/sys/thread.h
libhwloc_port_aix_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/aix \
        -DHWLOC_AIX_SYS \
        -DHWLOC_HAVE_PTHREAD_GETTHRDS_NP

nodist_libhwloc_port_darwin_la_SOURCES = topology-darwin.c
libhwloc_port_darwin_la_SOURCES = \
        include/darwin/sys/sysctl.h \
        include/darwin/IOKit/IOKitLib.h \
        include/darwin/CoreFoundation/CoreFoundation.h \
        include/darwin/Availability.h
libhwloc_port_darwin_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/darwin \
        -DHWLOC_DARWIN_SYS \
	-DHWLOC_HAVE_DARWIN_FOUNDATION -DHWLOC_HAVE_DARWIN_IOKIT

nodist_libhwloc_port_freebsd_la_SOURCES = topology-freebsd.c
libhwloc_port_freebsd_la_SOURCES = \
        include/freebsd/pthread.h \
        include/freebsd/pthread_np.h \
        include/freebsd/sys/cpuset.h \
        include/freebsd/sys/sysctl.h \
        include/freebsd/sys/thr.h \
        include/freebsd/sys/user.h \
        include/freebsd/sys/param.h \
        include/freebsd/sys/domainset.h
libhwloc_port_freebsd_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/freebsd \
        -DHWLOC_FREEBSD_SYS \
        -DHAVE_PTHREAD_NP_H \
        -DHAVE_SYS_CPUSET_H \
        -DHAVE_SYS_SYSCTL_H \
        -DHAVE_SYSCTL \
        -DHAVE_CPUSET_SETAFFINITY \
        -Dhwloc_thread_t=pthread_t \
        -DHAVE_DECL_PTHREAD_SETAFFINITY_NP=1 \
        -DHAVE_DECL_PTHREAD_GETAFFINITY_NP=1 \
        -DHAVE_CPUSET_SETID \
        -DHAVE_SYS_DOMAINSET_H \
        -DHAVE_SYS_THR_H

nodist_libhwloc_port_hpux_la_SOURCES = topology-hpux.c
libhwloc_port_hpux_la_SOURCES = \
        include/hpux/sys/mpctl.h
libhwloc_port_hpux_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/hpux \
        -DHWLOC_HPUX_SYS

nodist_libhwloc_port_netbsd_la_SOURCES = topology-netbsd.c
libhwloc_port_netbsd_la_SOURCES = \
        include/netbsd/pthread.h \
        include/netbsd/sched.h \
        include/netbsd/sys/sysctl.h
libhwloc_port_netbsd_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/netbsd \
        -DHWLOC_NETBSD_SYS \
        -DHAVE_SYS_SYSCTL_H \
        -DHAVE_SYSCTL

nodist_libhwloc_port_solaris_la_SOURCES = topology-solaris.c topology-solaris-chiptype.c
libhwloc_port_solaris_la_SOURCES = \
        include/solaris/kstat.h \
        include/solaris/picl.h \
        include/solaris/sys/lgrp_user.h \
        include/solaris/sys/processor.h \
        include/solaris/sys/procset.h \
        include/solaris/sys/systeminfo.h
libhwloc_port_solaris_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/solaris \
        -DHWLOC_SOLARIS_SYS \
        -DHAVE_LIBLGRP \
        -DHAVE_DECL_LGRP_LATENCY_COOKIE=1 \
        -DHAVE_LIBKSTAT \
        -DHAVE_PICL_H

nodist_libhwloc_port_windows_la_SOURCES = topology-windows.c
libhwloc_port_windows_la_SOURCES = \
        include/windows/windows.h
libhwloc_port_windows_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/windows \
        -DHWLOC_WIN_SYS

nodist_liblstopo_port_windows_a_SOURCES = lstopo-windows.c
liblstopo_port_windows_a_SOURCES = \
        include/windows/windowsx.h
liblstopo_port_windows_a_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/windows \
        -I$(HWLOC_top_srcdir)/utils/lstopo \
        -I$(HWLOC_top_srcdir)/utils/hwloc \
        -DHWLOC_WIN_SYS

nodist_liblstopo_port_android_a_SOURCES = lstopo-android.c
liblstopo_port_android_a_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/utils/lstopo \
        -I$(HWLOC_top_srcdir)/utils/hwloc

nodist_libhwloc_port_opencl_la_SOURCES = topology-opencl.c
libhwloc_port_opencl_la_SOURCES = \
        include/opencl/CL/cl.h
libhwloc_port_opencl_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/opencl

nodist_libhwloc_port_cuda_la_SOURCES = topology-cuda.c
libhwloc_port_cuda_la_SOURCES = \
        include/cuda/cuda.h \
        include/cuda/cuda_runtime_api.h
libhwloc_port_cuda_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/cuda

nodist_libhwloc_port_nvml_la_SOURCES = topology-nvml.c
libhwloc_port_nvml_la_SOURCES = \
        include/nvml/nvml.h
libhwloc_port_nvml_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/nvml \
        -DHAVE_DECL_NVMLDEVICEGETMAXPCIELINKGENERATION=1

nodist_libhwloc_port_rsmi_la_SOURCES = topology-rsmi.c
libhwloc_port_rsmi_la_SOURCES = \
        include/rsmi/rocm_smi/rocm_smi.h
libhwloc_port_rsmi_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/rsmi \
	-DHAVE_DECL_RSMI_DEV_PARTITION_ID_GET=1

nodist_libhwloc_port_levelzero_la_SOURCES = topology-levelzero.c
libhwloc_port_levelzero_la_SOURCES = \
        include/levelzero/level_zero/ze_api.h \
        include/levelzero/level_zero/zes_api.h
libhwloc_port_levelzero_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/levelzero \
        -DHWLOC_HAVE_ZEDEVICEPCIGETPROPERTIESEXT=1

nodist_libhwloc_port_gl_la_SOURCES = topology-gl.c
libhwloc_port_gl_la_SOURCES = \
        include/gl/X11/Xlib.h \
        include/gl/NVCtrl/NVCtrl.h \
        include/gl/NVCtrl/NVCtrlLib.h
libhwloc_port_gl_la_CPPFLAGS = $(common_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/tests/hwloc/ports/include/gl
