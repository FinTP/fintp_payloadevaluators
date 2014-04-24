ACLOCAL_AMFLAGS = -I ../fintp_utils/m4

lib_LTLIBRARIES = libAchBlkAccPayload.la	\
libIsoXmlPayload.la	\
libSepaStsPayload.la	\
libSwiftXmlPayload.la

AM_CPPFLAGS = -DUNIX -DDEBUG_ENABLED -DXALAN_1_9     \
@CPPFLAGS@      \
@XERCES_CPPFLAGS@       \
@XALAN_CPPFLAGS@        \
-I$(FINTP_HOME)/fintp_utils/src \
-I$(FINTP_HOME)/fintp_log/src   \
-I$(FINTP_HOME)/fintp_udal/src  \
-I$(FINTP_HOME)/fintp_transport/src     \
-I$(FINTP_HOME)/fintp_base/src  \
-I$(FINTP_HOME)/fintp_base/src/DB       \
-I$(FINTP_HOME)/fintp_ws/src	\
-I$(FINTP_HOME)/fintp_routingengine/src

AUTOMAKE_OPTIONS = foreign

AM_LDFLAGS = @LIBS@ @LDFLAGS@        \
@PTHREAD_LIBS@  \
XALAN_LDFLAGS@ -lxerces-c -lxalan-c   \
-L$(FINTP_HOME)/fintp_utils/.libs -lfintp_utils \
-L$(FINTP_HOME)/fintp_log/.libs -lfintp_log     \
-L$(FINTP_HOME)/fintp_base/.libs -lfintp_base   \
-L$(FINTP_HOME)/fintp_ws/.libs -lfintp_ws

libAchBlkAccPayload_la_SOURCES = AchBlkAccPayload/AchBlkAccPayload.cpp	\
AchBlkAccPayload/AchBlkAccPlugin.cpp	\
AchBlkAccPayload/AchBlkAccPayload.h

libIsoXmlPayload_la_SOURCES = IsoXmlPayload/IsoXmlPayload.cpp	\
IsoXmlPayload/IsoXmlPlugin.cpp	\
IsoXmlPayload/IsoXmlPayload.h

libSepaStsPayload_la_SOURCES = SepaStsPayload/SepaStsPayload.cpp	\
SepaStsPayload/SepaStsPlugin.cpp	\
SepaStsPayload/SepaStsPayload.h

libSwiftXmlPayload_la_SOURCES = SwiftXmlPayload/SwiftXmlPayload.cpp	\
SwiftXmlPayload/SwiftXmlPlugin.cpp	\
SwiftXmlPayload/SwiftXmlPayload.h

SUBDIRS = .
