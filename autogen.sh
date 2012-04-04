#!/bin/sh

GDBUS_CODEGEN=`which gdbus-codegen`
test -x ${GDBUS_CODEGEN} || { echo "You need gdbus-codegen"; exit 1; }

echo "Generating D-Bus interfaces"
${GDBUS_CODEGEN} --generate-c-code dbus/korva-dbus-interface \
                 --c-namespace Korva \
                 --interface-prefix org.jensge.Korva \
                 data/Korva.xml

autoreconf -if && ./configure --enable-debug $*
