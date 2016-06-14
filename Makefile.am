AM_CFLAGS = -Wall -Wextra -Wno-unused-parameter -DIS_VAPOURSYNTH

AM_CPPFLAGS = $(VapourSynth_CFLAGS) -DIS_VAPOURSYNTH

SUBDIRS = QRCodeSource/LibQREncode

lib_LTLIBRARIES = libqrcodesource.la

libqrcodesource_la_SOURCES = \
	QRCodeSource/QRCodeSource.cpp 

libqrcodesource_la_LDFLAGS = -no-undefined -avoid-version \
	QRCodeSource/LibQREncode/bitstream.lo \
	QRCodeSource/LibQREncode/mask.lo \
	QRCodeSource/LibQREncode/mmask.lo \
	QRCodeSource/LibQREncode/mqrspec.lo \
	QRCodeSource/LibQREncode/qrencode.lo \
	QRCodeSource/LibQREncode/qrinput.lo \
	QRCodeSource/LibQREncode/qrspec.lo \
	QRCodeSource/LibQREncode/rsecc.lo \
	QRCodeSource/LibQREncode/split.lo


