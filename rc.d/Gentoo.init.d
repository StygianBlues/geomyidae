#!/sbin/openrc-run
# Copyright 1999-2010 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

start(){
    ebegin "Starting geomyidae"
    [ -n "$GEOMYIDAE_ARGS" ] && GEOMYIDAE_ARGS="-- $GEOMYIDAE_ARGS"
    start-stop-daemon -Sbm -p /run/geomyidae.pid -x /usr/sbin/geomyidae $GEOMYIDAE_ARGS
    eend $? "Failed to start geomyidae"
}

stop(){
    ebegin "Stopping geomyidae"
    start-stop-daemon -K -p /var/run/geomyidae.pid
    eend $? "Failed to stop geomyidae"
}
