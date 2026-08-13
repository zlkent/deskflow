/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsClipboardTests.h"

#include "platform/MSWindowsClipboard.h"
#include "platform/MSWindowsClipboardBitmapConverter.h"

#include <QtEndian>

void MSWindowsClipboardTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);

  MSWindowsClipboard clipboard(NULL);

  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
}

void MSWindowsClipboardTests::cleanupTestCase()
{
  initTestCase();
}

void MSWindowsClipboardTests::emptyUnusedClipboard()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.emptyUnowned());
}

void MSWindowsClipboardTests::emptyOpenCalled()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
}

void MSWindowsClipboardTests::emptySingleFormat()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));

  clipboard.add(IClipboard::Format::Text, m_testString);
  QVERIFY(clipboard.empty());
  QVERIFY(!clipboard.has(IClipboard::Format::Text));
}

void MSWindowsClipboardTests::addValue()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));

  clipboard.add(IClipboard::Format::Text, m_testString);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);
}

void MSWindowsClipboardTests::replaceValue()
{
  using enum IClipboard::Format;

  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));

  clipboard.add(Text, m_testString);
  clipboard.add(Text, m_testString2);

  QCOMPARE(clipboard.get(Text), m_testString2);
}

void MSWindowsClipboardTests::openTimeIsOne()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
}

void MSWindowsClipboardTests::closeIsOpen()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
  clipboard.close();
}

void MSWindowsClipboardTests::getTimeOpenWithNoEmpty()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
  // this behavior is different to that of Clipboard which only
  // returns the value passed into open(t) after empty() is called.
  QCOMPARE(clipboard.getTime(), 1);
}

void MSWindowsClipboardTests::getTimeOpenAndEmpty()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
  QVERIFY(clipboard.empty());
  QCOMPARE(clipboard.getTime(), 1);
}

void MSWindowsClipboardTests::has_withFormatAdded()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  clipboard.add(IClipboard::Format::Text, m_testString);
  QVERIFY(clipboard.has(IClipboard::Format::Text));
}

void MSWindowsClipboardTests::has_withNoFormatAdded()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  QCOMPARE(clipboard.get(IClipboard::Format::Text), "");
}

void MSWindowsClipboardTests::getNonEmptyText()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  clipboard.add(IClipboard::Format::Text, m_testString);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);
}

void MSWindowsClipboardTests::isOwnedByDeskflow()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.isOwnedByDeskflow());
}

void MSWindowsClipboardTests::normalisesBitfieldsBitmap()
{
  constexpr qsizetype headerSize = sizeof(BITMAPINFOHEADER);
  constexpr qsizetype maskSize = 3 * sizeof(DWORD);
  std::string dib(headerSize + maskSize + 4, '\0');
  auto *raw = reinterpret_cast<quint8 *>(&dib[0]);
  qToLittleEndian<quint32>(headerSize, raw);
  qToLittleEndian<quint32>(1, raw + 4);
  qToLittleEndian<quint32>(1, raw + 8);
  qToLittleEndian<quint16>(1, raw + 12);
  qToLittleEndian<quint16>(32, raw + 14);
  qToLittleEndian<quint32>(BI_BITFIELDS, raw + 16);
  qToLittleEndian<quint32>(0x00ff0000, raw + headerSize);
  qToLittleEndian<quint32>(0x0000ff00, raw + headerSize + 4);
  qToLittleEndian<quint32>(0xff000000, raw + headerSize + 8); // malformed blue mask from macOS screenshot
  raw[headerSize + maskSize + 3] = 0xff;

  MSWindowsClipboardBitmapConverter converter;
  const auto handle = converter.fromIClipboard(dib);
  QVERIFY(handle != nullptr);
  QCOMPARE(GlobalSize(handle), SIZE_T(headerSize + 4));
  const auto *result = static_cast<const quint8 *>(GlobalLock(handle));
  QVERIFY(result != nullptr);
  QCOMPARE(qFromLittleEndian<quint32>(result), quint32(headerSize));
  QCOMPARE(qFromLittleEndian<quint32>(result + 16), quint32(BI_RGB));
  QCOMPARE(result[headerSize], quint8(0x00));
  QCOMPARE(result[headerSize + 3], quint8(0xff));
  GlobalUnlock(handle);
  GlobalFree(handle);
}

void MSWindowsClipboardTests::normalisesMacV5Bitmap()
{
  constexpr qsizetype headerSize = sizeof(BITMAPV5HEADER);
  std::string dib(headerSize + 4, '\0');
  auto *raw = reinterpret_cast<quint8 *>(&dib[0]);
  qToLittleEndian<quint32>(headerSize, raw);
  qToLittleEndian<quint32>(1, raw + 4);
  qToLittleEndian<quint32>(-1, raw + 8);
  qToLittleEndian<quint16>(1, raw + 12);
  qToLittleEndian<quint16>(32, raw + 14);
  qToLittleEndian<quint32>(BI_RGB, raw + 16);
  qToLittleEndian<quint32>(0xff000000, raw + 52); // V5 alpha mask
  raw[headerSize] = 0x00;
  raw[headerSize + 1] = 0x00;
  raw[headerSize + 2] = 0x00;
  raw[headerSize + 3] = 0xff;

  MSWindowsClipboardBitmapConverter converter;
  const auto handle = converter.fromIClipboard(dib);
  QVERIFY(handle != nullptr);
  QCOMPARE(GlobalSize(handle), SIZE_T(sizeof(BITMAPINFOHEADER) + 4));
  const auto *result = static_cast<const quint8 *>(GlobalLock(handle));
  QVERIFY(result != nullptr);
  QCOMPARE(qFromLittleEndian<quint32>(result), quint32(sizeof(BITMAPINFOHEADER)));
  QCOMPARE(qFromLittleEndian<quint32>(result + 16), quint32(BI_RGB));
  QCOMPARE(result[sizeof(BITMAPINFOHEADER)], quint8(0x00));
  QCOMPARE(result[sizeof(BITMAPINFOHEADER) + 3], quint8(0xff));
  GlobalUnlock(handle);
  GlobalFree(handle);
}

void MSWindowsClipboardTests::normalisesMalformedMacV5Bitmap()
{
  constexpr qsizetype headerSize = sizeof(BITMAPINFOHEADER);
  std::string dib(headerSize + 4, '\0');
  auto *raw = reinterpret_cast<quint8 *>(&dib[0]);
  qToLittleEndian<quint32>(sizeof(BITMAPV5HEADER), raw); // claims V5 but only has an INFOHEADER
  qToLittleEndian<quint32>(1, raw + 4);
  qToLittleEndian<quint32>(-1, raw + 8);
  qToLittleEndian<quint16>(1, raw + 12);
  qToLittleEndian<quint16>(32, raw + 14);
  qToLittleEndian<quint32>(BI_BITFIELDS, raw + 16);
  raw[headerSize] = 0x00;
  raw[headerSize + 1] = 0x00;
  raw[headerSize + 2] = 0x00;
  raw[headerSize + 3] = 0xff;

  MSWindowsClipboardBitmapConverter converter;
  const auto handle = converter.fromIClipboard(dib);
  QVERIFY(handle != nullptr);
  QCOMPARE(GlobalSize(handle), SIZE_T(headerSize + 4));
  const auto *result = static_cast<const quint8 *>(GlobalLock(handle));
  QVERIFY(result != nullptr);
  QCOMPARE(qFromLittleEndian<quint32>(result), quint32(headerSize));
  QCOMPARE(qFromLittleEndian<quint32>(result + 16), quint32(BI_RGB));
  QCOMPARE(result[headerSize], quint8(0x00));
  QCOMPARE(result[headerSize + 3], quint8(0xff));
  GlobalUnlock(handle);
  GlobalFree(handle);
}

QTEST_MAIN(MSWindowsClipboardTests)
