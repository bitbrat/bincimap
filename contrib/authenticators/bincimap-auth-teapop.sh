#!/bin/bash
#
# bincimap authentication using teapop textfile configs
#
# 2003 - Ivan F. Martinez <ivanfm@users.sourceforge.net>
#
if [ "$BINC_USERID" != "" ] && [ "$BINC_PASSWORD" != "" ]
then
    DOMAIN=`echo "$BINC_USERID" | cut -f 2 -d "@"`
    USERID=`echo "$BINC_USERID" | cut -f 1 -d "@"`
    LN=`grep "^${DOMAIN}:\*:textfile" /etc/teapop.passwd`
    if [ $? -eq 0 ]
    then
        DOMDIR=`echo "$LN" | cut -f 4 -d ":"`
        USER=`echo "$LN" | cut -f 6 -d ":"`
        GROUP=`echo "$LN" | cut -f 7 -d ":"`
        PASSFILE=`echo "$LN" | cut -f 8 -d ":"`
        if [ "$PASSFILE" != "" ] && [ -f $PASSFILE ]
        then
            LN=`grep "^${USERID}:${BINC_PASSWORD}" $PASSFILE`
            if [ $? -eq 0 ]
            then   
                USERDIR=`echo "$LN" | cut -f 3 -d ":"`
                USERDIR="${DOMDIR}/${USERDIR}"
                XUID=`grep "^${USER}:" /etc/passwd | cut -f 3 -d ":"`
                # If blank probably already numeric
                if [ "$XUID" = "" ]
                then
                    XUID="${USER}"
                fi
                XGID=`grep "^${GROUP}:" /etc/group | cut -f 3 -d ":"`
                # If blank probably already numeric
                if [ "$XGID" = "" ]
                then
                    XGID="${GROUP}"
                fi
                echo "${XUID}.${XGID}"
                echo "${USERDIR}"
                exit 0
            fi
        fi
    fi
fi
exit 112
