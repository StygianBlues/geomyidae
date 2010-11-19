#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
. /etc/conf.d/geomyidae

PID=`pidof -o %PPID /usr/bin/geomyidae`
case "$1" in
  start)
    stat_busy "Starting geomyidae"
    [ -z "$PID" ] && /usr/bin/geomyidae $GEOMYIDAE_ARGS 2>&1
    if [ $? -gt 0 ]; then
      stat_fail
    else
      PID=`pidof -o %PPID /usr/bin/geomyidae`
      echo $PID >/var/run/geomyidae.pid
      add_daemon geomyidae
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping geomyidae"
    [ ! -z "$PID" ]  && kill $PID &>/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon geomyidae 
      stat_done
    fi
    ;;
  restart)
    $0 stop
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"  
esac
exit 0
