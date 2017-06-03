#!/bin/sh
#
# $OpenBSD: rc.template,v 1.6 2014/08/14 19:54:10 ajacoutot Exp $

daemon="${TRUEPREFIX}/bin/geomyidae"
daemon_flags="-l /var/log/geomyidae.log -u _geomyidae -g _geomyidae"

. /etc/rc.d/rc.subr

rc_reload=NO

rc_cmd $1
