/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXClipboardTests.h"

#include "platform/OSXClipboard.h"
#include "platform/OSXClipboardBMPConverter.h"
#include "platform/OSXClipboardUTF8Converter.h"

#include <QtEndian>

void OSXClipboardTests::open()
{
  OSXClipboard clipboard;
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  clipboard.close();
}

void OSXClipboardTests::singleFormat()
{
  using enum IClipboard::Format;

  OSXClipboard clipboard;
  QVERIFY(clipboard.empty());
  clipboard.add(Text, m_testString);
  QVERIFY(clipboard.has(Text));
  QCOMPARE(clipboard.get(Text), m_testString);
}

void OSXClipboardTests::formatConvert_UTF8()
{
  OSXClipboardUTF8Converter converter;
  QCOMPARE(IClipboard::Format::Text, converter.getFormat());
  QCOMPARE(converter.getOSXFormat(), CFSTR("public.utf8-plain-text"));
  QCOMPARE(converter.fromIClipboard("test data\n"), "test data\r");
  QCOMPARE(converter.toIClipboard("test data\r"), "test data\n");
}

void OSXClipboardTests::formatConvert_BMPMalformedV5Header()
{
  constexpr qsizetype fileHeaderSize = 14;
  constexpr qsizetype dibHeaderSize = 40;
  constexpr qsizetype pixelOffset = fileHeaderSize + dibHeaderSize;
  std::string bmp(pixelOffset + 4, '\0');
  auto *raw = reinterpret_cast<quint8 *>(&bmp[0]);
  raw[0] = 'B';
  raw[1] = 'M';
  qToLittleEndian<quint32>(static_cast<quint32>(bmp.size()), raw + 2);
  qToLittleEndian<quint32>(pixelOffset, raw + 10);
  qToLittleEndian<quint32>(124, raw + fileHeaderSize); // advertised V5 header
  qToLittleEndian<quint32>(1, raw + fileHeaderSize + 4);
  qToLittleEndian<quint32>(1, raw + fileHeaderSize + 8);
  qToLittleEndian<quint16>(1, raw + fileHeaderSize + 12);
  qToLittleEndian<quint16>(32, raw + fileHeaderSize + 14);
  qToLittleEndian<quint32>(3, raw + fileHeaderSize + 16); // BI_BITFIELDS, but no masks follow
  raw[pixelOffset] = 0x00;
  raw[pixelOffset + 1] = 0x00;
  raw[pixelOffset + 2] = 0x00;
  raw[pixelOffset + 3] = 0xff;

  OSXClipboardBMPConverter converter;
  const auto dib = converter.toIClipboard(bmp);

  QCOMPARE(dib.size(), std::string::size_type(dibHeaderSize + 4));
  const auto *dibRaw = reinterpret_cast<const quint8 *>(dib.data());
  QCOMPARE(qFromLittleEndian<quint32>(dibRaw), quint32(dibHeaderSize));
  QCOMPARE(qFromLittleEndian<quint32>(dibRaw + 16), quint32(0)); // BI_RGB
  QCOMPARE(dibRaw[dibHeaderSize], quint8(0x00));
  QCOMPARE(dibRaw[dibHeaderSize + 3], quint8(0xff));
}

QTEST_MAIN(OSXClipboardTests)
