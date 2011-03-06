#!/bin/sh
#

# REQUIRE: local
# PROVIDE: geomyidae

$_rc_subr_loaded . /etc/rc.subr

name="geomyidae"
rcvar=$name
command="/usr/pkg/sbin/${name}"

#####################################################
# Geomyidae Options Section - "?" => geomyidae(8)   #
#  Uncomment & define options (defaults are shown)  #
#####################################################
#
#LOGFILE="-l /var/log/gopherd.log"
#LOGLEVEL="-v 15"
#HTDOCS="-b /var/gopher"
#PORT="-p 70"
#SPORT="-o 70"
#USR="-u $USER"
#GRP="-g $GROUP"
#HOST="-h localhost"
#IP="-i 127.0.0.1"

######################################################
# Now remove any UNDEFINED options from line below:  #
######################################################
#
command_args="$LOGFILE $LOGLEVEL $HTDOCS $PORT $SPORT $USR $GRP $HOST $IP"


######################################################
#  Uncomment this section if a PID file is desired   #
######################################################

#pidfile="/var/run/${name}.pid"
#start_cmd="geomyidae_start"
#
#geomyidae_start()
#{
#        echo "Starting $name"
#        $command $command_args
#        pgrep -x $name > $pidfile
#}

######################################################
#  Lastly, add the following to /etc/rc.conf:        #
#  "geomyidae=YES"  (without the quotes)             #
######################################################

load_rc_config $name
run_rc_command "$1"
