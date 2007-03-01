CXXFLAGS = -DP_USE_PRAGMA -g -D_REENTRANT -O3 -Wall -fPIC -DPIC -DPTRACING
CFLAGS = -DP_USE_PRAGMA -g -D_REENTRANT -O3 -Wall -fPIC -DPIC -DPTRACING

SRC = golay.c \
	crc8.cpp \
	crc16.cpp \
	Timer.cpp \
	H223Demuxer.cpp \
	H223Flag.cpp \
	H223Header.cpp \
	H223Muxer.cpp \
	H223MuxSDU.cpp \
	H223MuxTable.cpp \
	H223Session.cpp \
	H235ALReceiver.cpp \
	H245Capabilities.cpp \
	H245Channel.cpp \
	H245LogicalChannels.cpp \
	H245ChannelsFactory.cpp \
	H245MaintenanceLoop.cpp \
	H245MasterSlave.cpp \
	H245MuxTable.cpp \
	H245Negotiator.cpp \
	H245RoundTrip.cpp \
	H245TerminalCapability.cpp \
	H324CCSRLayer.cpp \
	H324MAL1.cpp \
	H324MAL2.cpp \
	H324MAL3.cpp \
	H324MControlChannel.cpp \
	H324MMediaChannel.cpp \
	H324MSession.cpp \
	H245_1.cpp \
	H245_2.cpp \
	H245_3.cpp \
	H324pdu.cpp \
	FileLogger.cpp \
	Media.cpp

	

OBJS1 = $(SRC:.cpp=.o)
OBJS = $(OBJS1:.c=.o)

all : $(OBJS)
	ar -r libh324m.a $(OBJS)

clean:
	rm -f $(OBJS) libh324m.a
