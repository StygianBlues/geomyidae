#!/bin/sh
#
# $OpenBSD: geomyidae,v 1.7 2017/06/30 22:06:09 anonymous Exp $

daemon="/usr/local/sbin/geomyidae"
daemon_flags="-l /var/log/geomyidae.log -u _geomyidae -g _geomyidae"

. /etc/rc.d/rc.subr

rc_reload=NO

rc_cmd $1
