#!/sbin/openrc-run
# Copyright 1999-2019 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

name="${RC_SVCNAME}"
command="/usr/bin/geomyidae"
pidfile="/run/${RC_SVCNAME}.pid"
command_args="${GEOMYIDAE_ARGS}"
command_background="yes"
